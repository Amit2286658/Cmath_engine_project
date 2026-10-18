# Math Engine Complete Tutorial — Table of Contents & Architecture Overview

Welcome to the chapter-by-chapter guide for the **100% `.math`-Driven, Filesystem-Free C Mathematical Programming Engine**.

## 1. Core Design Philosophy

1. **Zero Hardcoded Math or Grammar in C**:
   Calling `eng_init_blank(&eng)` initializes an empty arena and symbol registry where even `"1 + 2"` or `"(1)"` is rejected. Every delimiter (`(`, `)`, `,`), lexical pattern (`$real`, `$str`), scalar/container type (`$iota`, `$vec`), struct (`$complex`, `$vec2`, `$vec3`, `$circle`, `$mat2`), control structure (`;`, `:=`, `=`, `??`, `if`), constant (`π`, `e`, `g0`, `N_A`), operator, and function is defined in external `.math` files.
2. **100% Filesystem- & OS-Independent Core (`Core/`)**:
   `Core/Headers/*.h` and `Core/Code/*.c` contain zero file paths, directory scanners, or networking calls.
   - `eng_init(&eng)` boots all 9 standard library `.math` files in O(1) `memcpy` time from the embedded byte array `k_stdlib_mathc` (`Core/Headers/stdlib_precompiled.h`).
   - `user_loader.h` exposes strictly in-memory compilation (`user_compile_ext`) and O(1) binary loading (`user_load_mathc`), leaving file storage and cache invalidation entirely to your frontend host application.
3. **Dual Value Representation (`USE_TAGGED_12B`)**:
   Switch at compile time between **8-byte IEEE-754 NaN-boxing** (`USE_TAGGED_12B=0`, default) and a **12-byte Packed Tagged Union** (`USE_TAGGED_12B=1`) without changing a single line of `.math` or lexer/parser/evaluator code.

## 2. Chapter Guide

- **Chapter 1 (`01_Getting_Started_and_Host_API.md`)**: Building the project, booting `Engine`, evaluating expressions, compiling/caching user `.math` extensions in memory, and O(1) module toggling.
- **Chapter 2 (`02_Grammar_Types_and_Operators.md`)**: How `.math` files define syntax delimiters, consuming regex types, prefix/postfix/infix operators, and the postfix `"i"` imaginary operator.
- **Chapter 3 (`03_Control_Flow_Structs_and_Vectors.md`)**: Writing multi-statement loops (`;`, `:=`, `??`), conditionals (`if`), composite structs (`$vec2`, `$vec3`, `$circle`), and generic growable vectors (`$vec` of any `T`).
- **Chapter 4 (`04_Algebra_Polynomials_and_Matrices.md`)**: Solving linear systems (`solve_2x2`), real and complex quadratic equations (`quad_roots`, `quad_croot`), 2x2 matrix algebra (`$mat2`), symbolic/numerical polynomial algebra (`poly_val`, `poly_deriv`, `poly_mul`, `poly_newton`), and extending to a Symbolic CAS.
- **Chapter 5 (`05_Units_Physics_and_Chemistry.md`)**: Using postfix physical units (`deg`, `°`, `km`, `kph`, `atm`, `kcal`, `dB`) and the complete `[physics]` and `[chemistry]` formula suites.
- **Chapter 6 (`06_Host_Extern_Callbacks_and_Networking.md`)**: Connecting `.math` functions to live internet APIs, databases, and hardware sensors via `eng_register_extern()`.
