# Chapter 7 — Unit Type Groups, Concrete Variable Locks & Generic Unit `T`

## 1. Orthogonal `(EntityType, UnitType)` Architecture
Every `Value` in the engine carries two independent tags:
- **Entity Type (`type_id`)**: `real`, `iota`, `complex`, `vec2`, `vec3`, `vec`, `str`, etc.
- **Unit Type Group (`unit_id`)**: `distance`, `angle`, `mass`, `time`, `speed`, `pressure`, `energy`, `power`, or any user-defined custom unit group (e.g., `data_size`, `bandwidth`).

## 2. Postfix Unit Disambiguation (`m`, `im`, `mi`)
Because postfix operators only trigger *after* a valid left-hand operand expression has been parsed:
- Standalone `m := 5 ; m + 10` parses `m` as a local variable identifier (`15`).
- `50m` or `(10 + 5)m` parses `50` as the left-hand expression and `"m"` (`:95`) as the postfix distance operator (`50 [distance]`).
- `50im` and `50mi` both produce `50i` with unit group `distance`.

## 3. Concrete Variable Type & Unit Locks
Variables declared with `:=` can optionally lock their Entity Type, Unit Type group, or both:
- `x := 5` — dynamic entity and dynamic unit.
- `x : real := 5` — locked to entity `real` (`x += 5i` is rejected), unit remains dynamic.
- `x : distance := 50m` — locked to unit group `distance`; entity remains dynamic (`x += 50im` promotes `x` to `50 + 50i [distance]`, whereas `x += 60°` or unitless `x += 5i` is rejected).
- `x : real, distance := 50m` — locked to both `real` and `distance`.
- `x, y : real, distance := 50m` — multi-variable concrete declaration.

## 4. Generic Unit `T` vs Unannotated Parameters
- **Same-unit operations (`+`, `-`)**: `(a:real, T) "+" (b:real, T) :<50 -> real, T = @add(a, b)` requires both operands to have the exact same unit group `T` and preserves `T` on the result.
- **Scaling / cross-unit operations (`*`, `/`)**: `(a:real) "*" (b:real) :<60 -> real = @mul(a, b)` allows scaling any unit by a dimensionless number (`50m * 2 -> 100 [distance]`), while specific overloads like `(d:real, distance) "/" (t:real, time) :<60 -> real, speed = @div(d, t)` transform dimensions.

## 5. Defining Custom User-Side Unit Type Groups
Unit group names are dynamically registered in `SymTable.units[]` (not hardcoded in C). Any user `.math` extension can create new unit groups on the fly:
```text
(x:real) "B"  :95 -> real, data_size = x
(x:real) "KB" :95 -> real, data_size = x * 1024
(x:real) "MB" :95 -> real, data_size = x * (1024 ^ 2)
(x:real) "GB" :95 -> real, data_size = x * (1024 ^ 3)

(sz:real, data_size) "/" (t:real, time) :<60 -> real, bandwidth = @div(sz, t)
(bw:real, bandwidth) "*" (t:real, time) :<60 -> real, data_size = @mul(bw, t)
```
