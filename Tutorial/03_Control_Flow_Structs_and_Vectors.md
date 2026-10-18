# Chapter 3 — Control Flow, Local Scopes, Tagged Jumps/Breaks, Logic & Dynamic Vectors

This chapter covers lexical `{ ... }` scope blocks, multi-branch `if ... or ... else` statements, `switch` blocks, standard `cond ? x : y` ternary expressions, colon-tagged scopes (`Tag : { ... }`, `jump(Tag)`, `run(Tag)`, `break(Tag)`), chained & distributed boolean logic `(x & y == 1 | 2)`, multi-variable declarations `x, y := 5`, compound assignments `+=`, composite structs, and generic vectors (`$vec`).

## 1. Local Scope Blocks `{ ... }` (Provided via `[boot_syntax]`)

Curly braces `"{" => @lbrace` and `"}" => @rbrace` are declared in `Library/00_bootstrap.math` under `[boot_syntax]` (not hardcoded in C):
- **Value-Returning Scope**: When assigned (`x = { a := 3 ; b := 4 ; a^2 + b^2 }`), the final expression (`25`) is assigned to `x`.
- **Isolated / Void Scope**: When called in isolation (`{ x += 5 ; 9999 } ; y = x`), the `;` sequence operator (`@seq`) safely discards any unused return value, pops inner lexical bindings at `}`, and continues to the next statement. Empty `{}` returns `<void>`.
- **Lexical Shadowing (`:=` vs `=`)**: Declaring `x := 99` inside `{ ... }` shadows an outer `x` locally without modifying the outer `x`, while `x = 99` or `x += 5` mutates an existing outer `x`.

## 2. Multi-Variable Declarations (`x, y := 5`) & Compound Assignments (`+=`, `-=`, `*=`, `/=`, `%=`, `^=`)

- **Multi-Variable Declarations**: `x, y := 5` (or `a, b, c := 7`) initializes all listed variables to `5`.
- **Compound Assignments**: `+=`, `-=`, `*=`, `/=`, `%=`, `^=` update variables in place (`x := 5 ; x += 1` -> `6`, and `x, y += 3` updates both `x` and `y`).
- **Dynamic Slot Promotion**: Local variable stack slots hold self-describing `Value`s, so `x := 5 ; x = x + 5i` (or `x += 5i`) dynamically promotes `x` from `$real` (`5`) to composite `$complex` (`5 + 5i`), including full `.re` / `.im` field access!

## 3. Chained Boolean Logic & Distributed Comparisons `(x & y == 1 | 2)`

- **Boolean Operators**: `&&`, `||`, `!`, `and`, `or`, `not`, `nor`, `xor`, `nand`, `^^` support chained expressions such as `(x == 1 && y == 2 || z == 3)`.
- **Distributed Comparison Shorthand (`&` and `|`)**: Because `&` (`:<35`) and `|` (`:<34`) bind tighter than relational comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=` at `:<30`), writing `(x & y == 1 | 2)` distributes at compile time into `((x == 1) | (x == 2)) & ((y == 1) | (y == 2))` ("both `x` and `y` equal either `1` or `2`"), and can be parenthesized to chain further: `((x & y == 1 | 2) && (z == 3 | 4))`.

## 4. Multi-Branch `if ... or ... else` Statements

In `if` chains, `"or"` is used for `"else if"` conditions. There can be **only one `if`** at the start, **zero or more `or (cond) { ... }`** branches, and **at most one `else { ... }`** at the end; only the single matching branch is evaluated:

```math
"classify_mach"(m:real) -> str = {
    if (m < 0.8) {
        "subsonic"
    } or (m < 1.2) {
        "transonic"
    } or (m < 5.0) {
        "supersonic"
    } else {
        "hypersonic"
    }
}
```

## 5. `switch` Blocks & Standard Ternary `cond ? x : y`

- **`switch` Blocks**: `switch (id) { case 1 : { 3.7 } case 2 : { 8.87 } else : { 0 } }` (works on both numbers and strings, including multi-value `case "Ag", "Cu" : { ... }`).
- **Ternary Operator**: Right-associative `cond ? x : y` (`(s >= 90) ? 4 : (s >= 80) ? 3 : 0`).

## 6. Tagged Scopes & Loops (`Tag : { ... }`, `jump(Tag)`, `run(Tag)`, `break`, `break(Tag)`)

- **`break` and `break(OuterTag)`**: Unlabeled `break` exits the innermost `??` loop or tagged scope; `break(OuterTag)` breaks out of a tagged outer loop or scope even from inside a deeply nested inner loop:
  ```math
  Outer : {
      i := 1 ;
      (i <= 5) ?? {
          j := 1 ;
          (j <= 5) ?? {
              if (i == 2 and j == 3) { break(Outer) } ;
              tot += 10 ; j += 1
          } ;
          i += 1
      }
  }
  ```
- **`jump(Tag)` and `run(Tag)` / `Tag()`**: Jump backward/forward to `Tag : { ... }` or reuse `Tag`'s body inline without declaring extra functions.
