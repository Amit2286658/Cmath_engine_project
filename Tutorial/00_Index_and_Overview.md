# Math Engine Complete Tutorial — Table of Contents & Architecture Overview

Welcome to the chapter-by-chapter guide for the **Extension-Driven, Filesystem-Free C Mathematical Programming Engine**.

## 1. Core Design Philosophy & Directory Architecture

1. **Three-Way Separation of Concerns**:
   - **`Core/Code/` & `Core/Headers/` (Core Engine Pipeline)**: Memory arena (`mem_arena`), symbol/overload table (`sym_table`), pattern-driven lexer (`lexer`), Pratt parser (`parser`), in-memory extension text/binary loader (`ext_loader`, `user_loader`), and AST evaluator (`evaluator`).
   - **`Core/lib/delegate/` (`delegate.h` / `delegate.c` — C `@` Delegates)**: Contains the `@<intrinsic>` registry and all C implementations of operations dispatched via `@` (`@add`, `@sub`, `@sin`, `@vnew`, `@vpush`, `@vrange`, `@vin`, `@mzero`, `@mmul`, `@mdet`, `@minv`, `@calc_diff`, `@calc_integ`, `@calc_lim`, `@calc_sigma`, `@calc_prod`, `@calc_solve`, `@calc_fmin`, `@calc_fmax`, etc.).
   - **`Core/lib/hardcode/` (`hardcode.h` / `hardcode.c` — Hardcoded Language & Ergonomic Rules)**: Isolates all hardcoded grammar and ergonomic rules: reserved keyword checks (`if`, `or`, `then`, `else`, `default`, `switch`, `case`, `loop`, `jump`, `run`, `break`), generic wildcards (`T`, `any`, `none`) and default `:real` / `-> real` type fallback, default parameter overload synthesis (`param = default`), keywordless `(i in 1...5){}` lookahead & `AST_FOR_IN` execution, implicit multiplication (`2x`, `3(x+1)`), variadic `vec(...)` / `list(...)`, and struct/vector/matrix destructuring (`AST_UNPACK_ELEM`).
   - **`Library/` (External Extension Modules)**: All operators, precedences, types, units, structs, constants, and scientific functions.
2. **100% Filesystem- & Extension-Name-Independent Core (`Core/`)**:
   `Core/` never opens files, scans directories, or inspects file extensions (`.math`, `.txt`, or anything else). The host application supplies either a raw UTF-8 text string (`user_compile_ext` / `ext_load_str`) or a compiled binary buffer (`user_load_mathc` / `ext_load_mathc_mem`).
3. **Pure 8-Byte IEEE-754 NaN-Boxed `Value` Representation (`sizeof(Value) == 8`)**:
   Every `Value` is a single 64-bit `uint64_t` register. Unitless `$real` values are stored directly as IEEE-754 `double` bits with zero pool allocation, while `$iota`, unit-tagged scalars, `$str`, structs, `$vec`, and `$mat` are stored in the quiet-NaN payload space.

## 2. Chapter Guide

- **Chapter 1 (`01_Getting_Started_and_Host_API.md`)**: Building the project, booting `Engine`, evaluating expressions, compiling/caching user extensions in memory, and O(1) module toggling.
- **Chapter 2 (`02_Grammar_Types_and_Operators.md`)**: Defining syntax delimiters, consuming regex types, prefix/postfix/infix operators, shorthand unquoted declarations (`fn(a, b) = ...`, `c = ...`), default parameter values (`round(x, digits = 0)`), implicit multiplication (`2x^2 + 3(x + 1)`), and the postfix `"i"` imaginary operator.
- **Chapter 3 (`03_Control_Flow_Structs_and_Vectors.md`)**: Local `{ ... }` scopes, keywordless `(i in 1...5){ ... }` & `(v in vec){ ... }` loops, the `loop` keyword, `if ... or ... else` statements, `switch` blocks, `cond ? x : y` ternary, `Tag : { ... }` labeled scopes (`jump`, `run`, `break(Tag)`), reserved-keyword guardrails, destructuring, UFCS dot-chaining (`v.push(10).vec_sum()`), variadic `vec(...)` / `list(...)`, range `..` / `...`, and `x in vec`.
- **Chapter 4 (`04_Algebra_Polynomials_and_Matrices.md`)**: Solving linear systems (`solve_2x2`), real and complex quadratic equations (`quad_roots`, `quad_croot`), 2x2 matrix algebra (`$mat2`), and symbolic/numerical polynomial algebra.
- **Chapter 5 (`05_Units_Physics_and_Chemistry.md`)**: Using postfix physical units (`deg`, `°`, `km`, `kph`, `atm`, `kcal`, `dB`) and the complete `[physics]` and `[chemistry]` formula suites.
- **Chapter 6 (`06_Host_Extern_Callbacks_and_Networking.md`)**: Connecting extension functions to live internet APIs, databases, and hardware sensors via `eng_register_extern()`.
- **Chapter 7 (`07_Unit_Groups_and_Concrete_Types.md`)**: Orthogonal `(EntityType, UnitType)` tags, postfix unit disambiguation, concrete variable locks (`x : real, distance := 50m`), generic unit `T`, and custom user-side unit groups.
- **Chapter 8 (`08_Conversions_Array_Indexing_and_Multidim_Matrices.md`)**: Prefix/distributed `!` boolean-condition NOT, approximate equality `~` (`<= 0.001`), standard same-unit conversions (`Library/65_conversions.math`), C-style vector/struct subscripting `v[i]`, and multidimensional matrices `$mat` (`[...]`).
- **Chapter 9 (`09_Calculus_Arith_and_Sciences.md`)**: High school calculus (`diff`, `diff2`, `diff_n`, `integrate`/`ntg`, `lim`, `lim_left`, `lim_right`, `sigma`, `prod`, general equation solver `solve`, extrema finders `fmin`/`fmax`, and `Library/55_calculus.math`), expanded commercial/general arithmetic (`Library/10_arith.math`), and high school Physics (`70_physics.math`), Chemistry (`80_chemistry.math`), and Biology (`90_biology.math`).
