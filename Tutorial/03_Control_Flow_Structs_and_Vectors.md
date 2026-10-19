# Chapter 3 — Control Flow, Keywordless `(i in 1...5){}` Loops, Destructuring, UFCS & Dynamic Vectors

This chapter covers lexical `{ ... }` scope blocks, keywordless `(i in 1...5){}` & `(v in vec){}` loops, the `loop` keyword, multi-branch `if ... or ... else` statements, `switch` blocks, `cond ? x : y` ternary expressions, colon-tagged scopes (`Tag : { ... }`, `jump(Tag)`, `run(Tag)`, `break(Tag)`), reserved-keyword guardrails, struct/vector/matrix destructuring, UFCS dot-chaining, and variadic vectors (`$vec`).

## 1. Local Scope Blocks `{ ... }` (Provided via `[boot_syntax]`)

Curly braces `"{" => @lbrace` and `"}" => @rbrace` are declared in `Library/00_bootstrap.math` under `[boot_syntax]` (not hardcoded in C):
- **Value-Returning Scope**: When assigned (`x = { a := 3 ; b := 4 ; a^2 + b^2 }`), the final expression (`25`) is assigned to `x`.
- **Isolated / Void Scope**: When called in isolation (`{ x += 5 ; 9999 } ; y = x`), the `;` sequence operator (`@seq`) safely discards any unused return value, pops inner lexical bindings at `}`, and continues to the next statement. Empty `{}` returns `<void>`.
- **Lexical Shadowing (`:=` vs `=`)**: Declaring `x := 99` inside `{ ... }` shadows an outer `x` locally without modifying the outer `x`, while `x = 99` or `x += 5` mutates an existing outer `x`.

## 2. Multi-Variable Declarations (`x, y := 5`) & Struct/Vector/Matrix Destructuring

- **Scalar Broadcast**: `x, y := 5` initializes all listed variables to `5`.
- **Struct, Vector & Matrix Destructuring**: When the right-hand side evaluates to a struct (`$vec2`, `$vec3`, `$complex`), a `$vec`, or a `$mat`, `a, b := rhs` unpacks its fields or elements in order:
  - `r1, r2 := quad_roots(1, -5, 6)` -> `r1 = 3`, `r2 = 2`
  - `a, b, c := vec(10, 20, 30)` -> `a = 10`, `b = 20`, `c = 30`
  - `p, q := [100, 200]` -> `p = 100`, `q = 200`
- **Compound Assignments**: `+=`, `-=`, `*=`, `/=`, `%=`, `^=` update variables in place (`x := 5 ; x += 1` -> `6`, and `x, y += 3` updates both `x` and `y`).

## 3. Keywordless Iteration `(i in 1...5){ ... }` & `(v in vec){ ... }`, plus `loop`

- **Keywordless Range & Collection Loops**: Any `(var in iterable)` header followed by `{ ... }` (or `?? body`) iterates over an inclusive range (`a..b` or `a...b`, supporting both ascending `1...5` and descending `5...1`) or over the elements of a `$vec` or `$mat`:
  ```math
  sum := 0 ; (i in 1...5) { sum += i } ; sum                  # => 15
  v := vec(10, 20, 30, 40) ; s := 0 ; (x in v) { s += x } ; s # => 100
  ```
- **Explicit `loop` Construct**: The `loop` keyword supports range/vector headers (`loop (i in 1...5) { ... }`), while-condition headers (`loop (k <= 4) { ... }`), and unconditional loops (`loop { ... break }`).
- **Coexistence of `"in"`**: Because `"in"` uses multiple dispatch (`(item:T) "in" (v:vec)` vs `(a:real, distance) "in" (b:real, distance)`) and `(var in expr) { ... }` only triggers when followed by `{` (or `loop`), unit conversions like `(2.5 km) in (1 m)` (`2500`) and membership checks like `(3 in (1...5))` (`1`) work without conflict.

## 4. Variadic Vectors `vec(a, b, c, ...)`, Ranges `a..b` / `a...b`, `in` & UFCS Dot-Chaining

- **Variadic Constructors**: `vec(10, 20, 30, 40)` (for 3+ items) and `list(a, ...)` construct a `$vec` directly, while `vec()` / `vec(cap)` and `vec(count, init)` preserve their 0-, 1-, and 2-argument meanings.
- **Range & Membership Operators**: `1..5` and `1...5` construct `[1, 2, 3, 4, 5]`; `30 in vec(10, 20, 30)` returns `1` (`0` if absent).
- **Uniform Function Call Syntax (UFCS)**: Any `obj.fn(arg1, ...)` call where `fn` is not a struct field on `obj` dispatches to `fn(obj, arg1, ...)`:
  - `v := vec() ; v.push(10).push(20).push(30).vec_sum() + v.len()` -> `63`
  - `M := [4, 7 ; 2, 6] ; M.transpose().det() + (3 - 4i).cabs()` -> `15`

## 5. Multi-Branch `if ... or ... else`, `switch`, Tagged Scopes & Reserved-Keyword Errors

- **Multi-Branch `if ... or ... else` & `switch`**: Uses `or (cond) { ... }` for else-if branches and `else { ... }` for the fallback branch.
- **Tagged Scopes & Breaks**: `Outer : (i in 1...5) { (j in 1...5) { if (i * j > 6) { break(Outer) } } }`, `jump(Tag)`, and `run(Tag)` / `Tag()`.
- **Reserved Keyword Guardrails**: Using any active control-flow keyword (`if`, `or`, `then`, `else`, `default`, `switch`, `case`, `loop`, `jump`, `run`, `break`) as a variable/parameter name throws `"Cannot use reserved keyword '<kw>' as a variable name"`, and using one as a scope/jump/break tag throws `"Cannot use reserved keyword '<kw>' as a scope tag"`.
