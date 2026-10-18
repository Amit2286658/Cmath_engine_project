# Chapter 3 — Control Flow, Flat C-Style Structs & Dynamic Vectors

This chapter covers imperative control flow (`;`, `:=`, `=`, `??`, `if`), composite structs (`$vec2`, `$vec3`, `$circle`), and generic growable/shrinkable vectors (`$vec` of any type `T`).

## 1. Multi-Instruction Blocks, Variables & Loops (`00_bootstrap.math`)

All control flow in the language comes from four bootstrap intrinsics in `Library/00_bootstrap.math`:

1. **Sequencing (`;`)**: `( stmt1 ; stmt2 ; final_expr )` evaluates statements left-to-right and returns the value of `final_expr`.
2. **Local Variable Assignment (`:=` and `=`)**: Allocates a local stack slot on first use and updates it in place.
3. **Lazy While Loop (`??`)**: `(cond) ?? (body)` repeatedly evaluates `body` while `cond != 0`.
4. **Lazy Conditional (`if(cond, then_expr, else_expr)`)**: Evaluates only the taken branch.

## 2. Flat C-Style Composite Structs (`40_geom.math`)

Non-consuming structs are declared with `$name(field1:type1, field2:type2, ...)` and flattened into contiguous `double` slots in `MemArena.f64_pool`:

```math
$vec2(x:real, y:real)
$vec3(x:real, y:real, z:real)
$circle(center:vec2, radius:real)
```

- **Construction**: `vec2(3, 4)`, `circle(vec2(1, 2), 5)`
- **Compile-Time Field Offset Resolution (`.`)**: `c.center.y` resolves at parse time to flat offset `1` inside `circle`.

## 3. Generic Growable/Shrinkable Vectors (`$vec` of Any `T`)

`$vec` is a homogeneous dynamic array container that can store elements of **any** type `T` (`$real`, `$iota`, `$str`, `$complex`, `$vec2`, `$vec3`, `$circle`, or `$mat2`) using `vec()`, `vec(cap)`, `vec(count, init)`, `push(v, item)`, `pop(v)`, `at(v, idx)`, `set(v, idx, val)`, `len(v)`, `cap(v)`, `resize(v, n)`, `shrink(v)`, and `clear(v)`.
