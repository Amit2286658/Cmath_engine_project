# Math Engine (MEP) — Comprehensive Language Guide

This reference documents **every construct and its every variant**, **every entity type, struct, 7D SI unit group, and constant**, and **every operator and function with its every overload** across all 19 modules of the Math Engine (MEP). Every entry is annotated with its exact parameter types, physical units, default values, fixity/precedence, and return type/unit, and is accompanied by a live-tested example verified directly against the compiled engine.

## Module Inventory & Overview

| Module ID | Module Name | Source File | Registered Overloads | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| `0` | `[default]` | (Runtime / Session Scope) | 0 | Default module for headerless user extensions and session declarations |
| `1` | `[boot_syntax]` | `Library/bootstrap.math` | 7 | Core expression grouping `( )`, scope `{ }`, matrix `[ ]`, and argument `,` delimiters |
| `2` | `[boot_types]` | `Library/bootstrap.math` | 0 | Consuming lexical types (`$real`, `$iota`, `$str`), containers (`$vec`, `$mat`, `$fn`, `$map`), and wildcards (`$T`, `$any`, `$none`) |
| `3` | `[boot_control]` | `Library/bootstrap.math` | 61 | Sequencing `;`, assignments `:=`/`=`/`+=`, lambdas `=>`, pipe `|>`, loops `??`/`?!?`, conditionals `if`/`switch`/`? :`, symbolic CAS, and introspection |
| `4` | `[arith]` | `Library/arith.math` | 134 | Core arithmetic, bitwise operators, rounding, number theory, combinatorics, percentages, finance, work/time, progressions, and mathematical constants |
| `5` | `[trig]` | `Library/trig.math` | 37 | Circular, inverse, and hyperbolic trigonometry, exponentials/logarithms, roots, error functions, Gamma/Beta, and Gaussian normal distribution |
| `6` | `[complex_ext]` | `Library/complex.math` | 74 | Postfix imaginary unit `i`, rectangular `$complex(re, im)` struct, mixed real/iota/complex algebra, polar form, and complex transcendental functions |
| `7` | `[units]` | `Library/units.math` | 169 | 32 7D SI dimensional unit declarations (`@dim`) and 160+ postfix SI/imperial unit suffixes across 32 physical unit groups |
| `8` | `[chemistry]` | `Library/chemistry.math` | 61 | Fundamental chemical constants, stoichiometry, molarity, colligative properties, gas laws, acid-base pH/buffers, kinetics, nuclear decay, and electrochemistry |
| `9` | `[conversions]` | `Library/conversions.math` | 239 | Universal infix `in` conversion operator, postfix `to_<unit>` extractors, and pairwise unit conversion functions |
| `10` | `[vec]` | `Library/vec.math` | 109 | Generic `$vec`/`list`/`stack`/`queue`, generic `...`/`>..`/`..<`/`>..<` ranges, `by` stepping, higher-order pipelines, Hadamard ops, statistics, OLS regression, and `$map` dictionaries |
| `11` | `[biology]` | `Library/biology.math` | 60 | Population genetics (Hardy-Weinberg), molecular biology/PCR, Michaelis-Menten enzyme kinetics, plant water potential, cell morphometrics, human physiology, and ecology |
| `12` | `[geom_ext]` | `Library/geom.math` | 38 | Flat C-style `$vec2(x, y)`, `$vec3(x, y, z)`, and nested `$circle(center:vec2, radius)` structs, dot/cross products, norms, interpolation, and circle/sphere geometry |
| `13` | `[mat]` | `Library/mat.math` | 91 | 1D–4D `$mat` tensors, reshaping, Hadamard ops, matrix multiplication, `det`/`inv`/`linsolve`, LU, QR, Cholesky, Eigen, SVD, Moore-Penrose `pinv`, `lstsq`, and matrix norms |
| `14` | `[algebra]` | `Library/algebra.math` | 28 | `$mat2(a, b, c, d)` struct, Cramer's 2x2 solver, real & complex quadratic root solvers, and symbolic/numerical polynomial algebra on coefficient vectors |
| `15` | `[physics]` | `Library/physics.math` | 100 | 14 CODATA constants, SUVAT kinematics, projectiles, collisions, rotational/SHM mechanics, orbital astrophysics, fluids, thermodynamics, circuits, optics, quantum, and relativity |
| `16` | `[string]` | `Library/string.math` | 57 | String concatenation, slicing, search, case conversion, trimming, padding, splitting/joining, `fmt`/`format` formatting, hex/bin formatters, and backtracking regular expressions |
| `17` | `[calculus]` | `Library/calculus.math` | 76 | Unit-preserving numerical derivatives (`diff`, `diff2`, `diff_n`), Simpson integration (`integrate`), Richardson limits (`lim`), series (`sigma`, `prod`), root/extrema solvers (`solve`, `fmin`, `fmax`), multivariable calculus (`grad2`, `grad3`, `hessian2`, `laplacian2`), and RK4 ODE solvers |
| `18` | `[user_physics]` | `Test/user_physics.math` | 44 | Aerospace engineering, Mach/planet classifiers, Collatz tagged-scope state machine, host `@extern` FX/KV/Periodic-Table bindings, custom `data_size`/`bandwidth` units, and pattern-matched `switch` |

## Part I — Complete Language Constructs & Every Variant

### 1. Numeric, Imaginary, String & Matrix Literals

MEP supports consuming lexical literals for 64-bit IEEE-754 real numbers (decimal, leading-dot, scientific notation, hexadecimal, binary, and underscore digit separators), pure imaginary numbers (`$iota`), UTF-8 strings (`$str`), and 1D–4D dense matrices/tensors (`$mat`).

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Decimal Integer & Floating-Point Literal** | `num` | `() -> real, none` | Standard decimal number. | `10 + 3.14159` | `13.14159` (`real`) |
| **Leading-Dot Fractional Literal** | `.digits` | `() -> real, none` | Fractional literal without leading zero. | `.5 + .25` | `0.75` (`real`) |
| **Scientific Notation Literal** | `mantissa e [+-]exp` | `() -> real, none` | Base-10 exponential notation; coexists with implicit multiplication `2e == 2 * e`. | `1.5e3 + 2.5e-1` | `1500.25` (`real`) |
| **Underscore Digit Separator Literal** | `d_ddd_ddd` | `() -> real, none` | Visual digit grouping inside decimal, hex, or binary literals. | `1_000_000 + 2_500.75` | `1002500.75` (`real`) |
| **Hexadecimal Integer Literal** | `0xHEX / 0XHEX` | `() -> real, none` | Base-16 integer literal (with optional `_` separators). | `0xFF + 0x10_00` | `4351` (`real`) |
| **Binary Integer Literal** | `0bBIN / 0BBIN` | `() -> real, none` | Base-2 integer literal (with optional `_` separators). | `0b1010 + 0b1111_0000` | `250` (`real`) |
| **Pure Imaginary Literal (`$iota`)** | `num i / (expr)i` | `(x:real, T) -> iota, T` | Constructs a pure imaginary number preserving any physical unit tag `T`. | `(3 + 4)i` | `7i` (`iota`) |
| **Rectangular Complex Literal (`$complex`)** | `a + bi / a - bi` | `(a:real, T, b:iota, T) -> complex, T` | Combines real and imaginary parts into a 2-slot rectangular `$complex` struct. | `3 + 4i` | `3 + 4i` (`complex`) |
| **UTF-8 String Literal (`$str`)** | `"..."` | `() -> str, none` | Interned UTF-8 string slice supporting escape sequences (`\"`, `\n`, `\t`, `\\`). | `"Hello, " + "MEP!"` | `"Hello, MEP!"` (`str`) |
| **1D Row Matrix Literal (`$mat`)** | `[e1, e2, ...]` | `(elems:real, T...) -> mat, T` | Constructs a rank-1 contiguous `$mat` vector. | `[10, 20, 30]` | `[10, 20, 30]` (`mat`) |
| **2D Semicolon-Separated Matrix Literal** | `[a, b ; c, d]` | `(rows...) -> mat, T` | Constructs a rank-2 matrix using `;` as row separator. | `[1, 2, 3 ; 4, 5, 6]` | `[[1, 2, 3], [4, 5, 6]]` (`mat`) |
| **2D Nested-Bracket Matrix Literal** | `[[a, b], [c, d]]` | `(rows...) -> mat, T` | Constructs a rank-2 matrix from nested row brackets. | `[[1, 2], [3, 4]]` | `[[1, 2], [3, 4]]` (`mat`) |
| **3D / 4D Multidimensional Tensor Literal** | `[[[...]], [[...]]]` | `(subtensors...) -> mat, T` | Constructs rank-3 or rank-4 dense tensors with compile-time rectangularity validation. | `T := [[[1, 2], [3, 4]], [[5, 6], [7, 8]]] ; mat_shape(T)` | `[2, 2, 2]` (`vec`) |

### 2. Implicit Multiplication & Postfix Unit/String/Imaginary Application

Adjacent values, parenthesized groups, variables, constants, prefix operators, and function calls multiply implicitly at precedence `60` without requiring an explicit `*` token.

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Number-Variable Implicit Multiplication** | `2x / 3x^2` | `(a:real, x:T) -> T` | Multiplies scalar coefficient by variable (`^` binds tighter than implicit `*`, so `2x^2 == 2 * (x^2)`). | `x := 4 ; 2x^2 + 3x + 1` | `45` (`real`) |
| **Number-Constant Implicit Multiplication** | `2π / 2e` | `(a:real, c:real) -> real` | Multiplies literal by named constant (`2e` is `2 * e`, whereas `2e3` is `2000`). | `round(2π, 4)` | `6.2832` (`real`) |
| **Number-Parenthesis & Group-Group Multiplication** | `3(x + 1) / (x + 1)(x - 1)` | `(a:T, b:T) -> T` | Multiplies adjacent parenthesized expressions. | `x := 5 ; 3(x + 1) + (x + 1)(x - 1)` | `42` (`real`) |
| **Number-Function & Number-Prefix Multiplication** | `2 sin(x) / 2 √16` | `(a:real, b:real) -> real` | Multiplies coefficient by function call or prefix operator result. | `2 sin(π / 2) + 2 √16` | `10` (`real`) |
| **Postfix Physical Unit Suffix** | `val unit` | `(x:real) -> real, unit_group` | Normalizes value to SI base units and attaches dimensional unit tag. | `1.5 km + 250 m` | `1750` (`real`, `distance`) |
| **Postfix Imaginary Unit Suffix on Unit Value** | `50im / (50m)i` | `(x:real) -> iota, distance` | Attaches both imaginary type (`iota`) and physical unit (`distance`). | `x := 50m + 50im ; type_of(x) + ":" + unit_of(x)` | `"complex:distance"` (`str`) |
| **Postfix Stringification (`_`)** | `expr_` | `(x:T) -> str, none` | Formats any runtime value (`real`, `complex`, `vec`, `mat`, `map`, struct) to its canonical string representation. | `vec2(3, 4)_` | `"vec2(x=3, y=4)"` (`str`) |
| **Prefix String Parsing (`_`)** | `_str_expr` | `(s:str) -> T` | Parses and evaluates a string representation back into a typed, unit-tagged runtime value. | `(_"[10, 20, 30]").sum()` | `60` (`real`) |

### 3. Variables, Type/Unit Slot Locks, Broadcast & Destructuring Assignment

Variables are lexically scoped slots on the arena frame stack. Slots are dynamically typed by default, or can be locked to a concrete entity type (`: real`), a concrete physical unit (`: distance`), or both (`: real, distance`).

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Dynamic Variable Declaration (`:=`)** | `x := expr` | `(target, val:T) -> T` | Declares or updates `x` in the current scope with dynamic type and unit. | `x := 15 ; x * 2` | `30` (`real`) |
| **Variable Mutation (`=`)** | `x = expr` | `(target, val:T) -> T` | Mutates an existing variable (or promotes its type/unit if unlocked). | `x := 5 ; x = x + 5i ; x` | `5 + 5i` (`complex`) |
| **Compound Assignment (`+=`, `-=`, `*=`, `/=`, `%=`, `^=`)** | `x op= expr` | `(target, val:T) -> T` | In-place arithmetic/power update on variable, struct field, or indexed element. | `x := 10 ; x += 5 ; x *= 2 ; x` | `30` (`real`) |
| **Concrete Entity Type Lock (`x : type := expr`)** | `x : real := 5` | `(target:type, val:type, any) -> type` | Locks variable slot to entity type `type` while allowing unit changes. | `x : real := 5 ; x = 50m ; x` | `50` (`real`, `distance`) |
| **Concrete Physical Unit Lock (`x : unit := expr`)** | `x : distance := 50m` | `(target:any, unit, val:any, unit) -> any, unit` | Locks variable slot to physical unit `unit` while allowing entity promotion (e.g. `real` -> `complex`). | `x : distance := 50m ; x += 20im ; x` | `50 + 20i` (`complex`, `distance`) |
| **Combined Type & Unit Lock (`x : type, unit := expr`)** | `x : real, distance := 50m` | `(target:type, unit, val:type, unit) -> type, unit` | Locks variable slot to both entity type `type` and physical unit `unit`. | `x : real, distance := 50m ; x += 250cm ; x` | `52.5` (`real`, `distance`) |
| **Parameterized Container Type Lock (`v : vec<T[, unit]> := ...`)** | `v : vec<vec2> := vec<vec2>()` | `(target:vec, val:vec) -> vec` | Declares a homogeneously typed (and optionally unit-locked) vector variable. | `v : vec<vec2> := vec<vec2>() ; vec2(3, 4) into v ; v[0].x + v[0].y` | `7` (`real`) |
| **Multi-Variable Scalar Broadcast (`x, y := scalar`)** | `x, y, z := val` | `(targets..., val:real) -> real` | Broadcasts a consuming scalar to multiple variables simultaneously. | `x, y, z := 7 ; x * 100 + y * 10 + z` | `777` (`real`) |
| **Multi-Variable Compound Assignment (`x, y += val`)** | `x, y += val` | `(targets..., val:T) -> T` | Applies compound update across all listed variables. | `x, y := 5 ; x, y += 3 ; x * 10 + y` | `88` (`real`) |
| **Struct Field Destructuring (`r1, r2 := struct_expr`)** | `x, y := vec2(3, 4)` | `(targets..., s:struct) -> field_type` | Unpacks flat struct fields positionally into variables. | `r1, r2 := quad_roots(1, -5, 6) ; r1 * 10 + r2` | `32` (`real`) |
| **Vector & Matrix Destructuring (`a, b, c := vec_or_mat`)** | `a, b, c := vec(10, 20, 30)` | `(targets..., v:vec|mat) -> T` | Unpacks elements of a `$vec` or `$mat` into variables. | `a, b, c := vec(10, 20, 30) ; a + b + c` | `60` (`real`) |

### 4. Generic `...` Ranges, Angle-Bracket Exclusivity (`>..`, `..<`, `>..<`), Nested Skip Ranges & Stepped Ranges (`by`)

MEP uses a single generic 3-dot range form (`...`) where `...` is inclusive-inclusive (`[a, b]`). There is no `..` operator; any exclusiveness is expressed by attaching angle brackets `<` and `>` to `...` (`>..`, `..<`, `>..<`). Combining `<` and `>` across chained `...` segments creates nested exclusion (skip) ranges and multi-segment ranges.

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Inclusive-Inclusive Range (`a...b`)** | `a...b` | `(a:real, T, b:real, T) -> vec<real, T>` | Closed interval $[a, b]$ including both endpoints (ascending if $a \le b$, descending if $a > b$). | `1...5` | `[1, 2, 3, 4, 5]` (`vec`) |
| **Exclusive-Inclusive Range (`a>..b`)** | `a>..b` | `(a:real, T, b:real, T) -> vec<real, T>` | Half-open interval $(a, b]$ excluding start $a$ and including end $b$. | `1>..5` | `[2, 3, 4, 5]` (`vec`) |
| **Inclusive-Exclusive Range (`a..<b`)** | `a..<b` | `(a:real, T, b:real, T) -> vec<real, T>` | Half-open interval $[a, b)$ including start $a$ and excluding end $b$. | `1..<5` | `[1, 2, 3, 4]` (`vec`) |
| **Exclusive-Exclusive Range (`a>..<b`)** | `a>..<b` | `(a:real, T, b:real, T) -> vec<real, T>` | Open interval $(a, b)$ excluding both endpoints $a$ and $b$. | `1>..<5` | `[2, 3, 4]` (`vec`) |
| **Descending Range (`a...b` with $a > b$)** | `5...1 / 5>..<1` | `(a:real, T, b:real, T) -> vec<real, T>` | Automatically steps by $-1$ when start exceeds stop. | `5...1` | `[5, 4, 3, 2, 1]` (`vec`) |
| **Single-Interval Skip Range (`a..<x...y>..b`)** | `1..<25...75>..100` | `(a:real, T, x:real, T, y:real, T, b:real, T) -> vec<real, T>` | Includes $[a, x-1]$ and $[y+1, b]$, skipping the closed interval $[x, y]$. | `1..<4...7>..10` | `[1, 2, 3, 8, 9, 10]` (`vec`) |
| **Single-Point Skip Range (`a..<x>..b`)** | `1..<5>..10` | `(a:real, T, x:real, T, b:real, T) -> vec<real, T>` | Includes $[a, x-1]$ and $[x+1, b]$, skipping the single point $x$. | `1..<5>..10` | `[1, 2, 3, 4, 6, 7, 8, 9, 10]` (`vec`) |
| **Outer-Exclusive Nested Skip Range (`a>..<x...y>..<b`)** | `1>..<5...8>..<12` | `(a:real, T, x:real, T, y:real, T, b:real, T) -> vec<real, T>` | Excludes outer endpoints $a$ and $b$ as well as inner interval $[x, y]$. | `1>..<5...8>..<12` | `[2, 3, 4, 9, 10, 11]` (`vec`) |
| **Multi-Interval Skip Range (`a..<x1...y1>..<x2...y2>..<b`)** | `1..<25...50>..<75...90>..<100` | `(bounds:real, T...) -> vec<real, T>` | Chains up to 8 disjoint range segments (e.g. `1..24`, `51..74`, `91..99`). | `v := 1..<25...50>..<75...90>..<100 ; v.len()` | `57` (`real`) |
| **Stepped Range (`a...b by step`)** | `a...b by s / a>..<b by s` | `(r:vec, step:real, T) -> vec<real, T>` | Generates arithmetic progression across single or nested ranges with custom positive, negative, or fractional step. | `1...10 by 2` | `[1, 3, 5, 7, 9]` (`vec`) |
| **Unit-Preserving Range (`(0 m)...(1 km) by (250 m)`)** | `(a:real, T)...(b:real, T) by (s:real, T)` | `(a:real, T, b:real, T, s:real, T) -> vec<real, T>` | Generates unit-tagged range vector in SI base units. | `((0 m)...(1 km) by (250 m))[2]` | `500` (`real`, `distance`) |
| **O(1) Range Membership (`x in a...b` / `x in a..<x1...y1>..b`)** | `val in rng` | `(val:real, T, rng:vec) -> real, none` | Tests membership across single, stepped, or multi-segment skip ranges in $O(1)$ memory without allocating a vector. | `(24 in 1..<25...75>..100) * 10 + (50 in 1..<25...75>..100)` | `10` (`real`) |

### 5. Blocks, Lexical Scopes (`()` vs `{}`), Multi-Line Sequencing & Tagged Scopes (`Tag : { ... }`)

Parentheses `( ... )` group statements in the current scope (variables declared inside remain visible outside), whereas curly braces `{ ... }` open an isolated lexical scope (inner declarations shadow outer variables without mutating them). Statements are separated by `;` or newlines (`\n`).

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Same-Scope Sequence Group (`(a ; b ; c)`)** | `(stmt1 ; stmt2 ; expr)` | `() -> T` | Executes multiple statements in the enclosing scope and returns the final expression. | `(a := 3 ; b := 4 ; a + b) * 10 + a` | `73` (`real`) |
| **Isolated Lexical Scope Block (`{ a ; b ; c }`)** | `{ stmt1 ; stmt2 ; expr }` | `() -> T` | Executes statements in an isolated lexical scope; inner `:=` declarations do not leak or overwrite outer variables. | `x := 10 ; y := { x := 99 ; x + 1 } ; x * 1000 + y` | `10100` (`real`) |
| **Empty Scope Block (`{}`)** | `{}` | `() -> void` | Evaluates to `<void>` (`MEM_TYPE_VOID`). | `{}` | `<void>` (`void`) |
| **Tagged Scope Block (`Tag : { ... }`)** | `Tag : { body }` | `() -> T` | Binds a label `Tag` (up to 16 per scope) that can be targeted by `jump(Tag)`, `run(Tag)`, `Tag()`, or `break(Tag[, val])`. | `i := 0 ; Loop : { i += 1 ; if (i < 5) { jump(Loop) } ; i * 10 }` | `50` (`real`) |
| **Inline Tagged Scope Reuse (`run(Tag)` / `Tag()`)** | `run(Tag) / Tag()` | `(tag) -> T` | Re-executes the body of labeled block `Tag` inline at the call site. | `x := 2 ; Double : { x *= 2 } ; run(Double) ; Double() ; x` | `16` (`real`) |

### 6. Conditionals (`? :`, `if / or / else`), Distributed Logic (`&`, `|`, `!`) & Pattern-Matched `switch`

MEP provides right-associative lazy ternary conditionals, multi-branch `if ... or ... else` chains (using `or` for else-if), distributed comparison combinators (`&`, `|`, `!`), and structural pattern-matched `switch` blocks.

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Lazy Ternary Conditional (`cond ? then : else`)** | `c ? a : b` | `(c:real, a:T, b:T) -> T` | Evaluates only the taken branch; chains right-associatively. | `s := 85 ; (s >= 90) ? 4 : (s >= 80) ? 3 : 2` | `3` (`real`) |
| **Multi-Branch `if ... or ... else` Block** | `if (c1) { b1 } or (c2) { b2 } else { b3 }` | `() -> T` | Evaluates conditions top-to-bottom and runs only the first matching branch (`or` is else-if). | `m := 2.5 ; if (m < 0.8) { "sub" } or (m < 5.0) { "super" } else { "hyper" }` | `"super"` (`str`) |
| **Functional `if(cond, then_expr, else_expr)`** | `if(c, a, b)` | `(c:real, a:T, b:T) -> T` | Lazy 3-argument functional conditional form. | `if(10 > 3, 42, 99)` | `42` (`real`) |
| **Distributed Comparison Combinators (`&`, `|`, `!`)** | `(x & y == 1 \| 2) / (x == !(1 \| 2))` | `(lhs, rhs) -> real, none` | Distributes relational operator across `&` (AND), `|` (OR), and `!` (NOT) operand sets. | `x := 1 ; y := 2 ; (x & y == 1 \| 2)` | `1` (`real`) |
| **Switch Value & Multi-Value Case (`case v1, v2 :`)** | `switch (e) { case a, b : { ... } else : { ... } }` | `() -> T` | Matches target expression against one or more values per case. | `switch ("Au") { case "Ag", "Cu" : { 10 } case "Au" : { 79 } else : { 0 } }` | `79` (`real`) |
| **Switch Relational Guard Case (`case >= 90 :`)** | `case < a, > b : { ... }` | `() -> T` | Matches target against unary relational patterns (`<`, `<=`, `>`, `>=`, `==`, `!=`, `~`). | `switch (95) { case < 0, > 100 : { -1 } case >= 90 : { 4 } else : { 0 } }` | `4` (`real`) |
| **Switch Range & Nested Skip Range Case (`case a...b :`)** | `case 1..<25...75>..100 : { ... }` | `() -> T` | Tests target membership in any inclusive, exclusive, or nested skip range in $O(1)$ time. | `switch (80) { case 1..<25...75>..100 : { 1 } else : { 0 } }` | `1` (`real`) |
| **Switch Struct Destructuring & `if` Guard (`case vec2(x, y) if ... :`)** | `case vec2(0, y) if y > 5 : { ... }` | `() -> T` | Checks runtime struct type, binds fields into pattern variables, and evaluates optional `if` guard. | `p := vec2(4, 4) ; switch (p) { case vec2(0, y) : { y } case vec2(x, y) if x == y : { x * 10 } else : { -1 } }` | `40` (`real`) |
| **Switch Vector/Matrix Destructuring (`case [a, b] if ... :`)** | `case [0, y] if y > 5 : { ... }` | `() -> T` | Matches 1D vector/matrix length, binds elements/literals, and evaluates optional `if` guard. | `arr := [0, 12] ; switch (arr) { case [0, y] if y > 5 : { y * 3 } else : { 0 } }` | `36` (`real`) |
| **Switch Early Exit (`break` / `break(val)`)** | `break / break(expr)` | `() -> T` | Exits the active `switch` case early, returning either the prior statement's value or `expr`. | `switch (2) { case 2 : { x := 25 ; if (x > 20) { break(x * 2) } ; 999 } else : { 0 } }` | `50` (`real`) |

### 7. Loops (`??`, `?!?`, `in`) & Vector Comprehensions (`[expr for ...]`)

MEP supports pre-tested while loops (`??`), post-tested do-while loops (`{ ... } ?? (cond)` and `?!?`), $O(1)$-memory range and nested-range loops, 1-variable and 2-variable (`i, x in ...`) collection/map/string loops, labeled `break`/`continue` with return values, and Python-style comprehensions.

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Pre-Tested While Loop (`(cond) ?? { body }`)** | `(cond) ?? { body }` | `(cond:real, body) -> T` | Repeatedly evaluates `body` while `cond` is non-zero. | `s := 0 ; i := 1 ; (i <= 5) ?? { s += i ; i += 1 } ; s` | `15` (`real`) |
| **Post-Tested Do-While Block (`{ body } ?? (cond)`)** | `{ body } ?? (cond)` | `(body, cond:real) -> T` | Executes `{ body }` once before checking `(cond)` at the bottom of each iteration. | `k := 10 ; { k += 1 } ?? (k < 5) ; k` | `11` (`real`) |
| **Post-Tested Do-While Operator (`(body) ?!? (cond)`)** | `(body) ?!? (cond)` | `(body, cond:real) -> T` | Dedicated post-tested loop operator for any single or parenthesized expression body. | `k := 10 ; (k += 5) ?!? (k < 5) ; k` | `15` (`real`) |
| **Keywordless Range Loop (`(i in a...b) { ... }` / `i in a...b { ... }`)** | `(i in 1...5) { body }` | `() -> T` | Iterates over inclusive, exclusive, or stepped ranges in $O(1)$ memory without allocating a vector (`??` and `for(...)` are optional). | `s := 0 ; (i in 1...5) { s += i } ; s` | `15` (`real`) |
| **Nested Skip Range Loop (`(x in 1..<25...75>..100) { ... }`)** | `(x in a..<x1...y1>..b) { body }` | `() -> T` | Iterates across multi-segment skip ranges in $O(1)$ memory. | `s := 0 ; (x in 1..<25...75>..100) { s += x } ; s` | `2500` (`real`) |
| **Stepped Range Loop (`(x in a...b by s) { ... }`)** | `(x in 1...100 by 2) { body }` | `() -> T` | Iterates with custom step stride in $O(1)$ memory. | `s := 0 ; (x in 1...10 by 2) { s += x } ; s` | `25` (`real`) |
| **Collection Element Loop (`(x in v) { ... }`)** | `(x in coll) { body }` | `() -> T` | Iterates over elements of `$vec`, `$mat`, `$str` (characters), or `$map` (keys). | `s := 0 ; (x in vec(10, 20, 30)) { s += x } ; s` | `60` (`real`) |
| **2-Variable Indexed / Key-Value Loop (`(i, x in coll) { ... }`)** | `(i, x in coll) { body }` | `() -> T` | Binds 0-based index `i` and element `x` (or key `k` and value `v` for `$map`). | `s := 0 ; (i, x in vec(10, 20, 30)) { s += (i + 1) * x } ; s` | `140` (`real`) |
| **Value-Returning & Labeled `break` / `continue`** | `break [val] / break(Tag[, val]) / continue[(Tag)]` | `() -> T` | Exits or advances innermost or `Tag`-labeled outer loop, optionally returning a value as the loop's result. | `Outer : (i in 1...5) ?? { (j in 1...5) ?? { if (i * j == 12) { break(Outer, i * 100 + j) } } }` | `304` (`real`) |
| **Vector Comprehension (`[expr for x in iter]`)** | `[expr for x in iter]` | `() -> vec` | Constructs a `$vec` by evaluating `expr` across any range, nested skip range, vector, matrix, string, or map. | `[x^2 for x in 1..<4...7>..10]` | `[1, 4, 9, 64, 81, 100]` (`vec`) |
| **Filtered & Indexed Comprehension (`[expr for i, x in iter if cond]`)** | `[expr for i, x in iter if cond]` | `() -> vec` | Comprehension with optional 2-variable `(i, x)` binding and `if` filter guard. | `[i * 10 + x for i, x in vec(3, 4, 5, 6) if i % 2 == 1]` | `[14, 36]` (`vec`) |

### 8. First-Class Closures (`=>`), Inline Functions (`f(x) := ...`), Default Arguments, Recursion & Pipe (`|>`) / UFCS (`.`)

Functions and anonymous lambdas are first-class lexical closures (`$fn`) supporting up to 16 parameters, optional default parameter values, single- or mutual-recursion (up to 128 levels), UFCS dot-chaining (`x.f(y)`), and functional piping (`x |> f(y)`).

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Single-Parameter Lambda (`x => expr`)** | `x => body` | `() -> fn` | Creates a 1-parameter lexical closure capturing outer scope variables. | `k := 10 ; f := x => x * k ; f(7)` | `70` (`real`) |
| **Multi-Parameter & Default-Argument Lambda (`(x, y = 10) => expr`)** | `(p1, p2 = def) => body` | `() -> fn` | Creates a multi-parameter closure with optional default parameter expressions. | `scale := (x, k = 3, off = 1) => x * k + off ; scale(4) * 100 + scale(4, 5, 2)` | `1322` (`real`) |
| **Inline Named Function Definition (`f(x, y = 1) := expr` or `= expr`)** | `fname(params...) := body` | `() -> fn` | Defines a named function in the current scope or session (persisted across session evaluations). | `sq(x, c = 1) := x^2 + c ; sq(5) * 100 + sq(5, 10)` | `2635` (`real`) |
| **Recursive & Mutually Recursive Inline Functions** | `fact(n) := n <= 1 ? 1 : n * fact(n - 1)` | `() -> T` | Supports self-recursion and forward-referenced mutual recursion up to 128 call frames deep. | `fact(n) := n <= 1 ? 1 : n * fact(n - 1) ; fact(6)` | `720` (`real`) |
| **Uniform Function Call Syntax (UFCS Dot-Chaining)** | `x.fn(a, b)` | `(x:T, args...) -> R` | Rewrites `x.fn(a, b)` into `fn(x, a, b)` when `x` has no struct field named `fn`. | `(1...5).map(x => x^2).filter(x => x > 5).sum()` | `50` (`real`) |
| **Functional Pipe Operator (`x |> fn` / `x |> fn(a, b)` / `x |> (a => ...)`)** | `lhs \|> rhs` | `(lhs:T, rhs) -> R` | Passes `lhs` as the first argument to function `rhs`, call `rhs(a, b)`, or lambda closure. | `vec(1, 2, 3, 4, 5) \|> map(x => x^2) \|> filter(x => x > 5) \|> sum` | `50` (`real`) |

### 9. Indexing & Slicing (`v[i]`, `v[a...b by s]`, `M[r, c]`, `m["k"]`, `s[a...b]`)

C-style bracket indexing `base[...]` works uniformly across flat structs (`$vec2`, `$vec3`, `$circle`, `$mat2`), dynamic vectors (`$vec`), N-D matrices (`$mat`), strings (`$str`), and dictionaries (`$map`), supporting negative indices (`-1` = last element), range/strided slices, and in-place mutation.

| Construct Variant | Syntax Form | Parameter & Return Annotation | Behavior & Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Struct Positional Field Indexing & Mutation (`v[0]`, `c[0][1] = 99`)** | `s[idx]` | `(s:struct, idx:real) -> field_type` | Reads or mutates flat struct fields by 0-based index (including chained nested structs). | `c := circle(vec2(3, 7), 5) ; c[0][1] = 99 ; c` | `circle(center=vec2(x=3, y=99), radius=5)` (`circle`) |
| **Vector Positive & Negative Indexing (`v[0]`, `v[-1]`)** | `v[idx] / v[idx] = val` | `(v:vec, idx:real) -> T` | Reads or mutates `$vec` element at `idx` (`idx < 0` indexes from the end). | `v := vec(10, 20, 30) ; v[-1] += 5 ; v` | `[10, 20, 35]` (`vec`) |
| **Vector Range, Nested-Skip & Strided Slicing (`v[a...b]`, `v[a...b by s]`)** | `v[rng]` | `(v:vec, rng:vec) -> vec` | Extracts sub-vector over any range, nested skip range, or stepped range; also supports in-place scalar slice assignment `v[1...3] = 99`. | `v := vec(10, 20, 30, 40, 50, 60) ; v[0...5 by 2]` | `[10, 30, 50]` (`vec`) |
| **Matrix Multi-Index & Chained Indexing (`M[r, c]`, `M[r][c]`, `T[i, j, k]`)** | `M[i, j, ...]` | `(M:mat, indices:real...) -> real|mat` | Reads or mutates elements or sub-tensors of 1D–4D `$mat` tensors. | `M := [10, 20 ; 30, 40] ; M[0, 1] + M[1][0]` | `50` (`real`) |
| **Matrix 2D Submatrix & Row/Column Range Slicing (`M[r1...r2, c1...c2]`)** | `M[r_rng, c_rng]` | `(M:mat, r_rng, c_rng) -> mat` | Extracts 1D row/column slices or 2D submatrices using range expressions. | `M := [1, 2, 3 ; 4, 5, 6 ; 7, 8, 9] ; M[0...1, 1...2]` | `[[2, 3], [5, 6]]` (`mat`) |
| **String Character Indexing & Substring Range Slicing (`s[i]`, `s[a...b by s]`)** | `s[idx_or_rng]` | `(s:str, idx_or_rng) -> str` | Extracts character at `i` or substring over any ascending, descending, or strided range. | `"abcdef"[1...4] + "_" + "stressed"[7...0 by -1]` | `"bcde_desserts"` (`str`) |
| **Dictionary Key Lookup, Insertion & Compound Update (`m["k"]`, `m["k"] += v`)** | `m[key] / m[key] = val` | `(m:map, key:str) -> T` | Reads, inserts, or updates key-value entries in a `$map` dictionary. | `m := dict("a", 10, "b", 20) ; m["a"] += 5 ; m["c"] = 30 ; m["a"] + m["c"]` | `45` (`real`) |

## Part II — Entity Types, Flat Structs, 7D SI Unit Dimensions & Global Constants

### 1. Registered Entity Types & Flat C-Style Structs (15 Types)

| Type ID | Type Name | Module | Category | Flat Slot Size (`f64`) | Fields (`name:type @ offset`) | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `1` | `$real` | `[boot_types]` | Consuming Literal | `1` | `re:real` (@0), `im:real` (@1), `x:real` (@0), `y:real` (@1), `x:real` (@0), `y:real` (@1), `z:real` (@2), `center:vec2` (@0), `radius:real` (@2), `a:real` (@0), `b:real` (@1), `c:real` (@2), `d:real` (@3), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0), `:void` (@0) | `type_of(42.5)` | `"real"` (`str`) |
| `2` | `$iota` | `[boot_types]` | Consuming Literal | `1` | — | `type_of(5i)` | `"iota"` (`str`) |
| `3` | `$str` | `[boot_types]` | Consuming Literal | `1` | `:void` (@0), `:void` (@0), `:void` (@0) | `type_of("MEP")` | `"str"` (`str`) |
| `4` | `$vec` | `[boot_types]` | Intrinsic Container / Wildcard | `4` | — | `type_of(vec(1, 2, 3))` | `"vec"` (`str`) |
| `5` | `$mat` | `[boot_types]` | Intrinsic Container / Wildcard | `8` | — | `type_of([1, 2 ; 3, 4])` | `"mat"` (`str`) |
| `6` | `$fn` | `[boot_types]` | Intrinsic Container / Wildcard | `4` | — | `type_of(x => x^2)` | `"fn"` (`str`) |
| `7` | `$map` | `[boot_types]` | Intrinsic Container / Wildcard | `4` | — | `type_of(dict("a", 1))` | `"map"` (`str`) |
| `8` | `$T` | `[boot_types]` | Intrinsic Container / Wildcard | `0` | — | `type_of(scale_unit(10m, 3))` | `"real"` (`str`) |
| `9` | `$any` | `[boot_types]` | Intrinsic Container / Wildcard | `0` | — | `type_of(unit_span(10m, 4m))` | `"real"` (`str`) |
| `10` | `$none` | `[boot_types]` | Intrinsic Container / Wildcard | `0` | — | `unit_of(raw_val(10m))` | `"none"` (`str`) |
| `11` | `$complex` | `[complex_ext]` | Flat C-Struct | `2` | `re:real` (@0), `im:real` (@1) | `z := 3 + 4i ; z.re * 10 + z.im` | `34` (`real`) |
| `12` | `$vec2` | `[geom_ext]` | Flat C-Struct | `2` | `x:real` (@0), `y:real` (@1) | `v := vec2(3, 4) ; v.x * 10 + v.y` | `34` (`real`) |
| `13` | `$vec3` | `[geom_ext]` | Flat C-Struct | `3` | `x:real` (@0), `y:real` (@1), `z:real` (@2) | `v := vec3(1, 2, 3) ; v.x + v.y + v.z` | `6` (`real`) |
| `14` | `$circle` | `[geom_ext]` | Flat C-Struct | `3` | `center:vec2` (@0), `radius:real` (@2) | `c := circle(vec2(1, 2), 5) ; c.center.y * 10 + c.radius` | `25` (`real`) |
| `15` | `$mat2` | `[algebra]` | Flat C-Struct | `4` | `a:real` (@0), `b:real` (@1), `c:real` (@2), `d:real` (@3) | `m := mat2(4, 7, 2, 6) ; m.a * m.d - m.b * m.c` | `10` (`real`) |

### 2. Registered 7D SI Physical Unit Groups (35 Unit Groups)

Every physical unit group is backed by a 7-dimensional SI exponent vector $(\text{m}, \text{kg}, \text{s}, \text{A}, \text{K}, \text{mol}, \text{cd})$ enabling automatic dimensional synthesis across `*`, `/`, `^`, `sqrt`, `cbrt`, derivatives, and integrals.

| Unit ID | Unit Group Name | 7D SI Exponents `(m, kg, s, A, K, mol, cd)` | SI Base Unit Equivalent | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `1` | `distance` | `(1, 0, 0, 0, 0, 0, 0)` | `m` | `unit_of(1.5 km) + ":" + (1.5 km)_` | `"distance:1500"` |
| `2` | `area` | `(2, 0, 0, 0, 0, 0, 0)` | `m^2` | `unit_of((5 m) * (4 m)) + ":" + ((5 m) * (4 m))_` | `"area:20"` |
| `3` | `volume` | `(3, 0, 0, 0, 0, 0, 0)` | `m^3` | `unit_of((2 m) ^ 3) + ":" + ((2 m) ^ 3)_` | `"volume:8"` |
| `4` | `mass` | `(0, 1, 0, 0, 0, 0, 0)` | `kg` | `unit_of(2 kg + 500 gram) + ":" + (2 kg + 500 gram)_` | `"mass:2.5"` |
| `5` | `density` | `(-3, 1, 0, 0, 0, 0, 0)` | `m^-3 * kg` | `unit_of((1000 kg) / (1 m3)) + ":" + (1 gcm3)_` | `"density:1000"` |
| `6` | `time` | `(0, 0, 1, 0, 0, 0, 0)` | `s` | `unit_of(2 min_t + 30 sec) + ":" + (2 min_t + 30 sec)_` | `"time:150"` |
| `7` | `time2` | `(0, 0, 2, 0, 0, 0, 0)` | `s^2` | `unit_of((3 sec) ^ 2) + ":" + ((3 sec) ^ 2)_` | `"time2:9"` |
| `8` | `frequency` | `(0, 0, -1, 0, 0, 0, 0)` | `s^-1` | `unit_of(1 / (0.02 sec)) + ":" + (1 / (0.02 sec))_` | `"frequency:50"` |
| `9` | `speed` | `(1, 0, -1, 0, 0, 0, 0)` | `m * s^-1` | `unit_of((100 m) / (10 sec)) + ":" + (36 kph)_` | `"speed:10"` |
| `10` | `acceleration` | `(1, 0, -2, 0, 0, 0, 0)` | `m * s^-2` | `unit_of((20 mps) / (4 sec)) + ":" + (1 g_acc)_` | `"acceleration:9.80665"` |
| `11` | `specific_energy` | `(2, 0, -2, 0, 0, 0, 0)` | `m^2 * s^-2` | `unit_of((100 J) / (2 kg)) + ":" + (50 Jpkg)_` | `"specific_energy:50"` |
| `12` | `momentum` | `(1, 1, -1, 0, 0, 0, 0)` | `m * kg * s^-1` | `unit_of((10 kg) * (5 mps)) + ":" + (50 Ns)_` | `"momentum:50"` |
| `13` | `force` | `(1, 1, -2, 0, 0, 0, 0)` | `m * kg * s^-2` | `unit_of((10 kg) * (9.8 mps2)) + ":" + ((10 kg) * (9.8 mps2))_` | `"force:98"` |
| `14` | `pressure` | `(-1, 1, -2, 0, 0, 0, 0)` | `m^-1 * kg * s^-2` | `unit_of((100 N) / (2 m2)) + ":" + (1 atm)_` | `"pressure:101325"` |
| `15` | `energy` | `(2, 1, -2, 0, 0, 0, 0)` | `m^2 * kg * s^-2` | `unit_of((50 N) * (4 m)) + ":" + (1 kcal)_` | `"energy:4184"` |
| `16` | `power` | `(2, 1, -3, 0, 0, 0, 0)` | `m^2 * kg * s^-3` | `unit_of((200 J) / (10 sec)) + ":" + (1 kW)_` | `"power:1000"` |
| `17` | `current` | `(0, 0, 0, 1, 0, 0, 0)` | `A` | `unit_of(500 mA) + ":" + (500 mA)_` | `"current:0.5"` |
| `18` | `charge` | `(0, 0, 1, 1, 0, 0, 0)` | `s * A` | `unit_of((2 A) * (10 sec)) + ":" + (1 Ah)_` | `"charge:3600"` |
| `19` | `voltage` | `(2, 1, -3, -1, 0, 0, 0)` | `m^2 * kg * s^-3 * A^-1` | `unit_of((3 A) * (4 ohm)) + ":" + (12 V)_` | `"voltage:12"` |
| `20` | `resistance` | `(2, 1, -3, -2, 0, 0, 0)` | `m^2 * kg * s^-3 * A^-2` | `unit_of((12 V) / (3 A)) + ":" + (2 kohm)_` | `"resistance:2000"` |
| `21` | `capacitance` | `(-2, -1, 4, 2, 0, 0, 0)` | `m^-2 * kg^-1 * s^4 * A^2` | `unit_of((6 C) / (12 V)) + ":" + (1000 mF)_` | `"capacitance:1"` |
| `22` | `temperature` | `(0, 0, 0, 0, 1, 0, 0)` | `K` | `unit_of(25 degC) + ":" + (25 degC)_` | `"temperature:298.15"` |
| `23` | `heat_capacity` | `(2, 1, -2, 0, -1, 0, 0)` | `m^2 * kg * s^-2 * K^-1` | `unit_of((1000 J) / (250 K)) + ":" + (4 JpK)_` | `"heat_capacity:4"` |
| `24` | `specific_heat` | `(2, 0, -2, 0, -1, 0, 0)` | `m^2 * s^-2 * K^-1` | `unit_of(((4184 J) / (1 kg)) / (1 K)) + ":" + (4184 JpkgK)_` | `"specific_heat:4184"` |
| `25` | `substance` | `(0, 0, 0, 0, 0, 1, 0)` | `mol` | `unit_of(2 mol + 500 mmol) + ":" + (2 mol + 500 mmol)_` | `"substance:2.5"` |
| `26` | `molar_mass` | `(0, 1, 0, 0, 0, -1, 0)` | `kg * mol^-1` | `unit_of((36 gram) / (2 mol)) + ":" + (18 gpmol)_` | `"molar_mass:0.018"` |
| `27` | `molarity` | `(-3, 0, 0, 0, 0, 1, 0)` | `m^-3 * mol` | `unit_of((0.5 mol) / (250 mL)) + ":" + (2 M_conc)_` | `"molarity:2000"` |
| `28` | `molar_energy` | `(2, 1, -2, 0, 0, -1, 0)` | `m^2 * kg * s^-2 * mol^-1` | `unit_of((50 kJ) / (2 mol)) + ":" + (25 kJpmol)_` | `"molar_energy:25000"` |
| `29` | `molar_heat_capacity` | `(2, 1, -2, 0, -1, -1, 0)` | `m^2 * kg * s^-2 * K^-1 * mol^-1` | `unit_of(((8.314 J) / (1 mol)) / (1 K)) + ":" + (8.314 JpmolK)_` | `"molar_heat_capacity:8.314"` |
| `30` | `catalytic_activity` | `(0, 0, -1, 0, 0, 1, 0)` | `s^-1 * mol` | `unit_of((0.5 mol) / (10 sec)) + ":" + (0.05 kat)_` | `"catalytic_activity:0.05"` |
| `31` | `luminous_intensity` | `(0, 0, 0, 0, 0, 0, 1)` | `cd` | `unit_of(800 lm) + ":" + (800 lm)_` | `"luminous_intensity:800"` |
| `32` | `illuminance` | `(-2, 0, 0, 0, 0, 0, 1)` | `m^-2 * cd` | `unit_of((800 lm) / (4 m2)) + ":" + (200 lx)_` | `"illuminance:200"` |
| `33` | `angle` | `(custom non-SI group)` | `rad / custom` | `unit_of(60 deg) + ":" + ((180 deg) to_rad ~ π)_` | `"angle:1"` |
| `34` | `data_size` | `(custom non-SI group)` | `rad / custom` | `unit_of(1 KB) + ":" + (1 KB)_` | `"data_size:1024"` |
| `35` | `bandwidth` | `(custom non-SI group)` | `rad / custom` | `unit_of((10 MB) / (2 sec)) + ":" + ((10 KB) / (2 sec))_` | `"bandwidth:5120"` |

### 3. Registered Global Mathematical, Physical & Chemical Constants (30 Constants)

| Const ID | Constant Name | Module | Return Type & Unit | Value / Tested Expression | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0` | `π` | `[arith]` | `-> real, none` | `π` | `3.141592654` |
| `1` | `pi` | `[arith]` | `-> real, none` | `pi` | `3.141592654` |
| `2` | `e` | `[arith]` | `-> real, none` | `e` | `2.718281828` |
| `3` | `phi` | `[arith]` | `-> real, none` | `phi` | `1.618033989` |
| `4` | `φ` | `[arith]` | `-> real, none` | `φ` | `1.618033989` |
| `5` | `ϕ` | `[arith]` | `-> real, none` | `ϕ` | `1.618033989` |
| `6` | `inf` | `[arith]` | `-> real, none` | `inf` | `inf` |
| `7` | `nan` | `[arith]` | `-> real, none` | `nan` | `nan` |
| `8` | `N_A` | `[chemistry]` | `-> real, none` | `N_A` | `6.02214076e+23` |
| `9` | `R_gas` | `[chemistry]` | `-> real, none` | `R_gas` | `8.314462618` |
| `10` | `R_atm` | `[chemistry]` | `-> real, none` | `R_atm` | `0.082057366` |
| `11` | `F_faraday` | `[chemistry]` | `-> real, none` | `F_faraday` | `96485.33212` |
| `12` | `T_stp` | `[chemistry]` | `-> real, none` | `T_stp` | `273.15` |
| `13` | `P_stp` | `[chemistry]` | `-> real, none` | `P_stp` | `101325` |
| `14` | `kw_water` | `[chemistry]` | `-> real, none` | `kw_water` | `1e-14` |
| `15` | `g0` | `[physics]` | `-> real, none` | `g0` | `9.80665` |
| `16` | `c0` | `[physics]` | `-> real, none` | `c0` | `299792458` |
| `17` | `G_grav` | `[physics]` | `-> real, none` | `G_grav` | `6.6743e-11` |
| `18` | `h_planck` | `[physics]` | `-> real, none` | `h_planck` | `6.62607015e-34` |
| `19` | `k_B` | `[physics]` | `-> real, none` | `k_B` | `1.380649e-23` |
| `20` | `ke_coulomb` | `[physics]` | `-> real, none` | `ke_coulomb` | `8987551792` |
| `21` | `eps0` | `[physics]` | `-> real, none` | `eps0` | `8.854187813e-12` |
| `22` | `mu0` | `[physics]` | `-> real, none` | `mu0` | `1.256637062e-06` |
| `23` | `q_e` | `[physics]` | `-> real, none` | `q_e` | `1.602176634e-19` |
| `24` | `m_e` | `[physics]` | `-> real, none` | `m_e` | `9.109383702e-31` |
| `25` | `m_p` | `[physics]` | `-> real, none` | `m_p` | `1.672621924e-27` |
| `26` | `sigma_sb` | `[physics]` | `-> real, none` | `sigma_sb` | `5.670374419e-08` |
| `27` | `b_wien` | `[physics]` | `-> real, none` | `b_wien` | `0.002897771955` |
| `28` | `rydberg_R` | `[physics]` | `-> real, none` | `rydberg_R` | `10973731.57` |
| `29` | `tau_const` | `[user_physics]` | `-> real, none` | `tau_const` | `6.283185307` |

## Part III — Complete Module-by-Module Operator & Function Overload Reference (All 1,380 Overloads)

Every single operator and function overload registered across all modules is documented below with its exact parameter list (`name:type, unit = default`), fixity and binding precedence, return annotation (`-> ret_type, ret_unit`), implementation summary, and a live-tested example with its exact engine output.

### Module `[boot_syntax]` (`Library/bootstrap.math` — 7 Overloads)

Core expression grouping `( )`, scope `{ }`, matrix `[ ]`, and argument `,` delimiters.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `0` | `"("` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@lparen` | `(2 + 3) * 4` | `20` (`real`) |
| `1` | `")"` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@rparen` | `(2 + 3) * 4` | `20` (`real`) |
| `2` | `"{"` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@lbrace` | `{ a := 3 ; b := 4 ; a + b }` | `7` (`real`) |
| `3` | `"}"` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@rbrace` | `{ a := 3 ; b := 4 ; a + b }` | `7` (`real`) |
| `4` | `"["` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@lbrack` | `[1, 2 ; 3, 4]` | `[[1, 2], [3, 4]]` (`mat`) |
| `5` | `"]"` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@rbrack` | `[1, 2 ; 3, 4]` | `[[1, 2], [3, 4]]` (`mat`) |
| `6` | `","` | `SYNTAX` | `()` (0 params) | `-> void, none` | `@comma` | `min(10, 25)` | `10` (`real`) |

### Module `[boot_control]` (`Library/bootstrap.math` — 61 Overloads)

Sequencing `;`, assignments `:=`/`=`/`+=`, lambdas `=>`, pipe `|>`, loops `??`/`?!?`, conditionals `if`/`switch`/`? :`, symbolic CAS, and introspection.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `7` | `(s) "." (f) :<95` | `INFIX_LEFT` (prec `95`) | `s:real, none`, `f:real, none` | `-> real, none` | `@field(s, f)` | `vec2(3, 4).x` | `3` (`real`) |
| `8` | `(a) ";" (b) :<1` | `INFIX_LEFT` (prec `1`) | `a:real, none`, `b:real, none` | `-> real, none` | `@seq(a, b)` | `a := 5 ; a * 3` | `15` (`real`) |
| `9` | `(target) ":=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, val, 1)` | `x := 42 ; x` | `42` (`real`) |
| `10` | `(target) "="  (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, val)` | `x := 10 ; x = 25 ; x` | `25` (`real`) |
| `11` | `(target) "+=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, @add(target, val))` | `x := 10 ; x += 5 ; x` | `15` (`real`) |
| `12` | `(target) "-=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, @sub(target, val))` | `x := 10 ; x -= 3 ; x` | `7` (`real`) |
| `13` | `(target) "*=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, @mul(target, val))` | `x := 10 ; x *= 4 ; x` | `40` (`real`) |
| `14` | `(target) "/=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, @div(target, val))` | `x := 20 ; x /= 4 ; x` | `5` (`real`) |
| `15` | `(target) "%=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, @mod(target, val))` | `x := 17 ; x %= 5 ; x` | `2` (`real`) |
| `16` | `(target) "^=" (val) :>5` | `INFIX_RIGHT` (prec `5`) | `target:real, none`, `val:real, none` | `-> real, none` | `@store(target, @pow(target, val))` | `x := 3 ; x ^= 4 ; x` | `81` (`real`) |
| `17` | `(params) "=>" (body) :>8  -> fn` | `INFIX_RIGHT` (prec `8`) | `params:real, none`, `body:real, none` | `-> fn, none` | `@lambda(params, body)` | `f := (x, y) => x * 10 + y ; f(3, 4)` | `34` (`real`) |
| `18` | `(x:T)    "\|>" (f:T)  :<12 -> T` | `INFIX_LEFT` (prec `12`) | `x:T, none`, `f:T, none` | `-> T, none` | `@pipe(x, f)` | `5 \|> (x => x^2 + 1)` | `26` (`real`) |
| `19` | `call(f:fn)` | `FUNC` | `f:fn, none` | `-> real, none` | `@fcall(f, 0, 0)` | `f := () => 42 ; call(f)` | `42` (`real`) |
| `20` | `call(f:fn, a:T)           -> T` | `FUNC` | `f:fn, none`, `a:T, none` | `-> T, none` | `@fcall(f, a, 0)` | `call(x => x^2 + 1, 5)` | `26` (`real`) |
| `21` | `call(f:fn, a:T, b:T)      -> T` | `FUNC` | `f:fn, none`, `a:T, none`, `b:T, none` | `-> T, none` | `@fcall(f, a, b)` | `call((a, b) => a + b * 10, 4, 3)` | `34` (`real`) |
| `22` | `call(f:fn, a:T, b:T, c:T) -> T` | `FUNC` | `f:fn, none`, `a:T, none`, `b:T, none`, `c:T, none` | `-> T, none` | `@fcall(f, a, b)` | `call((a, b, c) => a + b + c, 10, 20, 30)` | `60` (`real`) |
| `23` | `call(f:fn, a:T, b:T, c:T, d:T) -> T` | `FUNC` | `f:fn, none`, `a:T, none`, `b:T, none`, `c:T, none`, `d:T, none` | `-> T, none` | `@fcall(f, a, b)` | `call((a, b, c, d) => a + b + c + d, 1, 2, 3, 4)` | `10` (`real`) |
| `24` | `(cond) "??"  (body) :<3` | `INFIX_LEFT` (prec `3`) | `cond:real, none`, `body:real, none` | `-> real, none` | `@while(cond, body)` | `s := 0 ; i := 1 ; (i <= 5) ?? { s += i ; i += 1 } ; s` | `15` (`real`) |
| `25` | `(body) "?!?" (cond) :<3` | `INFIX_LEFT` (prec `3`) | `body:real, none`, `cond:real, none` | `-> real, none` | `@do_while(body, cond)` | `k := 10 ; (k += 5) ?!? (k < 5) ; k` | `15` (`real`) |
| `26` | `(cond) "?" (branches) :>10` | `INFIX_RIGHT` (prec `10`) | `cond:real, none`, `branches:real, none` | `-> real, none` | `@ternary(cond, branches)` | `(10 > 5) ? 42 : 99` | `42` (`real`) |
| `27` | `(a)    ":" (b)        :>10` | `INFIX_RIGHT` (prec `10`) | `a:real, none`, `b:real, none` | `-> real, none` | `@colon(a, b)` | `(10 < 5) ? 42 : 99` | `99` (`real`) |
| `28` | `"!" (a:real, none) :80` | `PREFIX` (prec `80`) | `a:real, none (locked)` | `-> real, none` | `@cnot(a)` | `!0 * 10 + !1` | `10` (`real`) |
| `29` | `(a) "&" (b)        :<35` | `INFIX_LEFT` (prec `35`) | `a:real, none`, `b:real, none` | `-> real, none` | `@cand(a, b)` | `x := 1 ; y := 2 ; (x & y == 1 \| 2)` | `1` (`real`) |
| `30` | `(a) "\|" (b)        :<34` | `INFIX_LEFT` (prec `34`) | `a:real, none`, `b:real, none` | `-> real, none` | `@cor(a, b)` | `x := 2 ; (x == 1 \| 2)` | `1` (`real`) |
| `31` | `if(cond, then_val, else_val)` | `FUNC` | `cond:real, none`, `then_val:real, none`, `else_val:real, none` | `-> real, none` | `@if(cond, then_val, else_val)` | `if (5 > 3) { 100 } else { 200 }` | `100` (`real`) |
| `32` | `or(cond, body)` | `FUNC` | `cond:real, none`, `body:real, none` | `-> real, none` | `@or_branch(cond, body)` | `x := 2 ; if (x == 1) { 10 } or (x == 2) { 20 } else { 30 }` | `20` (`real`) |
| `33` | `then(cond, body)` | `FUNC` | `cond:real, none`, `body:real, none` | `-> real, none` | `@or_branch(cond, body)` | `if (1) { 42 } else { 0 }` | `42` (`real`) |
| `34` | `else(body)` | `FUNC` | `body:real, none` | `-> real, none` | `@else_branch(body)` | `if (0) { 10 } else { 99 }` | `99` (`real`) |
| `35` | `default(body)` | `FUNC` | `body:real, none` | `-> real, none` | `@else_branch(body)` | `switch (9) { case 1 : { 10 } default : { 99 } }` | `99` (`real`) |
| `36` | `switch(val, body)` | `FUNC` | `val:real, none`, `body:real, none` | `-> real, none` | `@switch_block(val, body)` | `switch (2) { case 1 : { 10 } case 2 : { 20 } else : { 0 } }` | `20` (`real`) |
| `37` | `case(val, body)` | `FUNC` | `val:real, none`, `body:real, none` | `-> real, none` | `@case_branch(val, body)` | `switch ("Au") { case "Ag", "Au" : { 79 } else : { 0 } }` | `79` (`real`) |
| `38` | `for(expr, body)` | `FUNC` | `expr:real, none`, `body:real, none` | `-> real, none` | `@for_kw(expr, body)` | `[x^2 for x in 1...5]` | `[1, 4, 9, 16, 25]` (`vec`) |
| `39` | `(k:T) in (m:map)        :<42` | `INFIX_LEFT` (prec `42`) | `k:T, none`, `m:map, none` | `-> real, none` | `@map_has(m, k)` | `"a" in dict("a", 10, "b", 20)` | `1` (`real`) |
| `40` | `jump(tag)` | `FUNC` | `tag:real, none` | `-> real, none` | `@jump_tag(tag)` | `x := 1 ; if (x > 0) { jump(Done) } ; x = 999 ; Done : { x + 100 }` | `101` (`real`) |
| `41` | `run(tag)` | `FUNC` | `tag:real, none` | `-> real, none` | `@run_tag(tag)` | `x := 5 ; Step : { x += 10 } ; run(Step) ; x` | `25` (`real`) |
| `42` | `break()` | `FUNC` | `()` (0 params) | `-> real, none` | `@break_loop(0)` | `s := 0 ; (i in 1...10) { if (i == 5) { break } ; s += i } ; s` | `10` (`real`) |
| `43` | `break(tag)` | `FUNC` | `tag:real, none` | `-> real, none` | `@break_loop(tag)` | `found := (x in 1...20) ?? { if (x^2 > 50) { break(x) } } ; found` | `8` (`real`) |
| `44` | `break(tag, val:T)       -> T` | `FUNC` | `tag:real, none`, `val:T, none` | `-> T, none` | `@break_loop(tag, val)` | `Outer : (i in 1...5) ?? { (j in 1...5) ?? { if (i * j == 12) { break(Outer, i * 100 + j) } } }` | `304` (`real`) |
| `45` | `continue()` | `FUNC` | `()` (0 params) | `-> real, none` | `@continue_loop(0)` | `s := 0 ; (i in 1...5) { if (i % 2 == 0) { continue } ; s += i } ; s` | `9` (`real`) |
| `46` | `continue(tag)` | `FUNC` | `tag:real, none` | `-> real, none` | `@continue_loop(tag)` | `tot := 0 ; Outer : (i in 1...3) ?? { (j in 1...3) ?? { if (j == 2) { continue(Outer) } ; tot += i * 10 + j } } ; tot` | `63` (`real`) |
| `47` | `return()` | `FUNC` | `()` (0 params) | `-> real, none` | `@return_fn(0)` | `f(x) := { if (x < 0) { return } ; x * 2 } ; f(5)` | `10` (`real`) |
| `48` | `return(val:T)           -> T` | `FUNC` | `val:T, none` | `-> T, none` | `@return_fn(val)` | `f(x) := { if (x < 0) { return -1 } ; x * 10 } ; f(-3) * 100 + f(4)` | `-60` (`real`) |
| `49` | `sym_diff(var_x, expr)              -> str` | `FUNC` | `var_x:real, none`, `expr:real, none` | `-> str, none` | `@sym_diff(var_x, expr)` | `sym_diff(x, x^3 + 2 * sin(x))` | `"3 * x ^ 2 + 2 * cos(x)"` (`str`) |
| `50` | `sym_diff(var_x, expr, order)       -> str` | `FUNC` | `var_x:real, none`, `expr:real, none`, `order:real, none` | `-> str, none` | `@sym_diff(var_x, expr, order)` | `sym_diff(x, x^4 + 5 * x^2, 2)` | `"12 * x ^ 2 + 10"` (`str`) |
| `51` | `sym_diff_n(var_x, expr, order)     -> str` | `FUNC` | `var_x:real, none`, `expr:real, none`, `order:real, none` | `-> str, none` | `@sym_diff(var_x, expr, order)` | `sym_diff_n(x, x^4 + 5 * x^2, 2)` | `"12 * x ^ 2 + 10"` (`str`) |
| `52` | `sym_integ(var_x, expr)             -> str` | `FUNC` | `var_x:real, none`, `expr:real, none` | `-> str, none` | `@sym_integ(var_x, expr)` | `sym_integ(x, 3 * x^2 + 4 * x + 5)` | `"x ^ 3 + 2 * x ^ 2 + 5 * x"` (`str`) |
| `53` | `sym_solve(var_x, eq)               -> str` | `FUNC` | `var_x:real, none`, `eq:real, none` | `-> str, none` | `@sym_solve(var_x, eq)` | `sym_solve(x, 2 * x + 5 == 17)` | `"6"` (`str`) |
| `54` | `sym_simplify(expr:T)               -> str` | `FUNC` | `expr:T, none` | `-> str, none` | `@sym_simplify(expr)` | `sym_simplify((x + 0) * 1 + 2 * x + 3 * x)` | `"6 * x"` (`str`) |
| `55` | `simplify(expr:T)                   -> str` | `FUNC` | `expr:T, none` | `-> str, none` | `@sym_simplify(expr)` | `simplify(((x ^ 2) ^ 3) * (x ^ 0))` | `"x ^ 6"` (`str`) |
| `56` | `sym_eval(expr_str:str, var_x, val)` | `FUNC` | `expr_str:str, none`, `var_x:real, none`, `val:real, none` | `-> real, none` | `@sym_eval(expr_str, var_x, val)` | `df := sym_diff(x, x^3 + 4 * x) ; sym_eval(df, x, 3)` | `31` (`real`) |
| `57` | `raw_val(x)           -> real, none` | `FUNC` | `x:real, none` | `-> real, none (locked)` | `@raw(x)` | `raw_val(25 km)` | `25000` (`real`) |
| `58` | `with_unit(val, donor)` | `FUNC` | `val:real, none`, `donor:real, none` | `-> real, none` | `@raw(val, donor)` | `unit_of(with_unit(42, 10 m))` | `"distance"` (`str`) |
| `59` | `type_of(x:T)         -> str` | `FUNC` | `x:T, none` | `-> str, none` | `@type_name(x)` | `type_of(3 + 4i)` | `"complex"` (`str`) |
| `60` | `unit_of(x:T)         -> str` | `FUNC` | `x:T, none` | `-> str, none` | `@unit_name(x)` | `unit_of(100 N)` | `"force"` (`str`) |
| `61` | `unit_dim(x:T, axis)` | `FUNC` | `x:T, none`, `axis:real, none` | `-> real, none` | `@unit_name(x, axis)` | `unit_dim(5 J, 0)` | `2` (`real`) |
| `62` | `gc_compact()` | `FUNC` | `()` (0 params) | `-> real, none` | `@gc_compact(0)` | `v := vec(1, 2, 3) ; gc_compact() >= 0` | `1` (`real`) |
| `63` | `pool_used()` | `FUNC` | `()` (0 params) | `-> real, none` | `@pool_used(0)` | `pool_used() >= 0` | `1` (`real`) |
| `64` | `(x:T) "_"   :95      -> str` | `POSTFIX` (prec `95`) | `x:T, none` | `-> str, none` | `@tostr(x)` | `(3 + 4i)_` | `"3 + 4i"` (`str`) |
| `65` | `"_" (s:str) :85      -> T` | `PREFIX` (prec `85`) | `s:str, none` | `-> T, none` | `@fromstr(s)` | `_"42" + 8` | `50` (`real`) |
| `66` | `error(msg:str)` | `FUNC` | `msg:str, none` | `-> real, none` | `@err(msg)` | `1 ? 42 : error("domain error")` | `42` (`real`) |
| `67` | `assert(cond, msg:str)` | `FUNC` | `cond:real, none`, `msg:str, none` | `-> real, none` | `if(cond, 1, error(msg))` | `assert(2 + 2 == 4, "math broken")` | `1` (`real`) |

### Module `[arith]` (`Library/arith.math` — 134 Overloads)

Core arithmetic, bitwise operators, rounding, number theory, combinatorics, percentages, finance, work/time, progressions, and mathematical constants.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `68` | `(a:real, T) "+"    (b:real, T) :<50 -> real, T` | `INFIX_LEFT` (prec `50`) | `a:real, T`, `b:real, T` | `-> real, T` | `@add(a, b)` | `6 + 3` | `9` (`real`) |
| `69` | `(a:real, T) "-"    (b:real, T) :<50 -> real, T` | `INFIX_LEFT` (prec `50`) | `a:real, T`, `b:real, T` | `-> real, T` | `@sub(a, b)` | `6 - 3` | `3` (`real`) |
| `70` | `(a:real, T) into   (b:real, T) :>55 -> real, T` | `INFIX_RIGHT` (prec `55`) | `a:real, T`, `b:real, T` | `-> real, T` | `@add(a, b)` | `6 into 3` | `9` (`real`) |
| `71` | `(a)         "*"    (b)         :<60` | `INFIX_LEFT` (prec `60`) | `a:real, none`, `b:real, none` | `-> real, none` | `@mul(a, b)` | `6 * 3` | `18` (`real`) |
| `72` | `(a)         "/"    (b)         :<60` | `INFIX_LEFT` (prec `60`) | `a:real, none`, `b:real, none` | `-> real, none` | `@div(a, b)` | `6 / 3` | `2` (`real`) |
| `73` | `(a)         "%"    (b)         :<60` | `INFIX_LEFT` (prec `60`) | `a:real, none`, `b:real, none` | `-> real, none` | `@mod(a, b)` | `6 % 3` | `0` (`real`) |
| `74` | `(a) plus  (b) :<45` | `INFIX_LEFT` (prec `45`) | `a:real, none`, `b:real, none` | `-> real, none` | `a + b` | `6 plus 3` | `9` (`real`) |
| `75` | `(a) minus (b) :<45` | `INFIX_LEFT` (prec `45`) | `a:real, none`, `b:real, none` | `-> real, none` | `a - b` | `6 minus 3` | `3` (`real`) |
| `76` | `(a) "^" (b) :>85` | `INFIX_RIGHT` (prec `85`) | `a:real, none`, `b:real, none` | `-> real, none` | `@pow(a, b)` | `6 ^ 3` | `216` (`real`) |
| `77` | `(a)         "==" (b)         :<30` | `INFIX_LEFT` (prec `30`) | `a:real, none`, `b:real, none` | `-> real, none` | `@eq(a, b)` | `6 == 3` | `0` (`real`) |
| `78` | `(a)         "!=" (b)         :<30` | `INFIX_LEFT` (prec `30`) | `a:real, none`, `b:real, none` | `-> real, none` | `@ne(a, b)` | `6 != 3` | `1` (`real`) |
| `79` | `(a)         "<"  (b)         :<30` | `INFIX_LEFT` (prec `30`) | `a:real, none`, `b:real, none` | `-> real, none` | `@lt(a, b)` | `6 < 3` | `0` (`real`) |
| `80` | `(a)         "<=" (b)         :<30` | `INFIX_LEFT` (prec `30`) | `a:real, none`, `b:real, none` | `-> real, none` | `@le(a, b)` | `6 <= 3` | `0` (`real`) |
| `81` | `(a)         ">"  (b)         :<30` | `INFIX_LEFT` (prec `30`) | `a:real, none`, `b:real, none` | `-> real, none` | `@gt(a, b)` | `6 > 3` | `1` (`real`) |
| `82` | `(a)         ">=" (b)         :<30` | `INFIX_LEFT` (prec `30`) | `a:real, none`, `b:real, none` | `-> real, none` | `@ge(a, b)` | `6 >= 3` | `1` (`real`) |
| `83` | `(a:real, T) "~"  (b:real, T) :<30` | `INFIX_LEFT` (prec `30`) | `a:real, T`, `b:real, T` | `-> real, none` | `(@abs(a - b) <= 0.001 + 1e-12)` | `6 ~ 3` | `0` (`real`) |
| `84` | `not(x) :25` | `PREFIX` (prec `25`) | `x:real, none` | `-> real, none` | `!x` | `not(0)` | `1` (`real`) |
| `85` | `(a) "&&" (b) :<20` | `INFIX_LEFT` (prec `20`) | `a:real, none`, `b:real, none` | `-> real, none` | `(a != 0) ? ((b != 0) ? 1 : 0) : 0` | `6 && 3` | `1` (`real`) |
| `86` | `(a) and  (b) :<20` | `INFIX_LEFT` (prec `20`) | `a:real, none`, `b:real, none` | `-> real, none` | `(a != 0) ? ((b != 0) ? 1 : 0) : 0` | `6 and 3` | `1` (`real`) |
| `87` | `(a) nand (b) :<20` | `INFIX_LEFT` (prec `20`) | `a:real, none`, `b:real, none` | `-> real, none` | `(a != 0) ? ((b != 0) ? 0 : 1) : 1` | `6 nand 3` | `0` (`real`) |
| `88` | `(a) xor  (b) :<18` | `INFIX_LEFT` (prec `18`) | `a:real, none`, `b:real, none` | `-> real, none` | `((a != 0) != (b != 0))` | `6 xor 3` | `0` (`real`) |
| `89` | `(a) "^^" (b) :<18` | `INFIX_LEFT` (prec `18`) | `a:real, none`, `b:real, none` | `-> real, none` | `((a != 0) != (b != 0))` | `6 ^^ 3` | `0` (`real`) |
| `90` | `(a) "\|\|" (b) :<15` | `INFIX_LEFT` (prec `15`) | `a:real, none`, `b:real, none` | `-> real, none` | `(a != 0) ? 1 : ((b != 0) ? 1 : 0)` | `6 \|\| 3` | `1` (`real`) |
| `91` | `(a) or   (b) :<15` | `INFIX_LEFT` (prec `15`) | `a:real, none`, `b:real, none` | `-> real, none` | `(a != 0) ? 1 : ((b != 0) ? 1 : 0)` | `6 or 3` | `1` (`real`) |
| `92` | `(a) nor  (b) :<15` | `INFIX_LEFT` (prec `15`) | `a:real, none`, `b:real, none` | `-> real, none` | `(a == 0) ? ((b == 0) ? 1 : 0) : 0` | `6 nor 3` | `0` (`real`) |
| `93` | `"-" (x) :80` | `PREFIX` (prec `80`) | `x:real, none` | `-> real, none` | `@neg(x)` | `-6` | `-6` (`real`) |
| `94` | `"+" (x) :80` | `PREFIX` (prec `80`) | `x:real, none` | `-> real, none` | `x` | `+6` | `6` (`real`) |
| `95` | `"√" (x) :80` | `PREFIX` (prec `80`) | `x:real, none` | `-> real, none` | `@sqrt(x)` | `√6` | `2.449489743` (`real`) |
| `96` | `"~" (x:real, none) :80` | `PREFIX` (prec `80`) | `x:real, none (locked)` | `-> real, none` | `@bnot(x)` | `~6` | `-7` (`real`) |
| `97` | `(a:real, none) "<<" (b:real, none) :<40` | `INFIX_LEFT` (prec `40`) | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@bshl(a, b)` | `6 << 3` | `48` (`real`) |
| `98` | `(a:real, none) ">>" (b:real, none) :<40` | `INFIX_LEFT` (prec `40`) | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@bshr(a, b)` | `6 >> 3` | `0` (`real`) |
| `99` | `"++"  (x) :85` | `PREFIX` (prec `85`) | `x:real, none` | `-> real, none` | `x + 1` | `++6` | `7` (`real`) |
| `100` | `"+++" (x) :85` | `PREFIX` (prec `85`) | `x:real, none` | `-> real, none` | `x + 2` | `+++6` | `8` (`real`) |
| `101` | `"--"  (x) :85` | `PREFIX` (prec `85`) | `x:real, none` | `-> real, none` | `x - 1` | `--6` | `5` (`real`) |
| `102` | `"---" (x) :85` | `PREFIX` (prec `85`) | `x:real, none` | `-> real, none` | `x - 2` | `---6` | `4` (`real`) |
| `103` | `(x) "++"  :90` | `POSTFIX` (prec `90`) | `x:real, none` | `-> real, none` | `x + 1` | `6++` | `7` (`real`) |
| `104` | `(x) "+++" :90` | `POSTFIX` (prec `90`) | `x:real, none` | `-> real, none` | `x + 2` | `6+++` | `8` (`real`) |
| `105` | `(x) "--"  :90` | `POSTFIX` (prec `90`) | `x:real, none` | `-> real, none` | `x - 1` | `6--` | `5` (`real`) |
| `106` | `(x) "---" :90` | `POSTFIX` (prec `90`) | `x:real, none` | `-> real, none` | `x - 2` | `6---` | `4` (`real`) |
| `107` | `(n:real, none) "!" :90` | `POSTFIX` (prec `90`) | `n:real, none (locked)` | `-> real, none` | `{ if (n != n \|\| n < 0 \|\| n != @floor(n)) { error("Factorial '!' requi...` | `6!` | `720` (`real`) |
| `108` | `abs(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@abs(x)` | `abs(6)` | `6` (`real`) |
| `109` | `floor(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@floor(x)` | `floor(6)` | `6` (`real`) |
| `110` | `ceil(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@ceil(x)` | `ceil(6)` | `6` (`real`) |
| `111` | `round(x, digits = 0)` | `FUNC` | `x:real, none`, `digits:real, none = 0` | `-> real, none` | `{ if (digits != digits \|\| digits != @floor(digits)) { error("round() ...` | `round(6, 3)` | `6` (`real`) |
| `112` | `round(x) [default wrapper: digits=0]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls round(...) with digits=0` | `round(6)` | `6` (`real`) |
| `113` | `trunc(x)` | `FUNC` | `x:real, none` | `-> real, none` | `(x >= 0) ? @floor(x) : @ceil(x)` | `trunc(6)` | `6` (`real`) |
| `114` | `fract(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x - @floor(x)` | `fract(6)` | `0` (`real`) |
| `115` | `sign(x)` | `FUNC` | `x:real, none` | `-> real, none` | `(x != x) ? x : ((x > 0) ? 1 : (x < 0) ? -1 : 0)` | `sign(6)` | `1` (`real`) |
| `116` | `is_nan(x)` | `FUNC` | `x:real, none` | `-> real, none` | `(x != x)` | `is_nan(6)` | `0` (`real`) |
| `117` | `min(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `(a != a) ? a : ((b != b) ? b : ((a < b) ? a : b))` | `min(6, 3)` | `3` (`real`) |
| `118` | `max(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `(a != a) ? a : ((b != b) ? b : ((a > b) ? a : b))` | `max(6, 3)` | `6` (`real`) |
| `119` | `clamp(x, lo, hi)` | `FUNC` | `x:real, none`, `lo:real, none`, `hi:real, none` | `-> real, none` | `min(max(x, lo), hi)` | `clamp(6, 3, 3)` | `3` (`real`) |
| `120` | `abs_diff(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `(a < b) ? (b - a) : (a - b)` | `abs_diff(6, 3)` | `3` (`real`) |
| `121` | `is_even(n)` | `FUNC` | `n:real, none` | `-> real, none` | `(n % 2 == 0)` | `is_even(6)` | `1` (`real`) |
| `122` | `is_odd(n)` | `FUNC` | `n:real, none` | `-> real, none` | `(n % 2 != 0)` | `is_odd(6)` | `0` (`real`) |
| `123` | `is_close(a, b, tol = 0.000001)` | `FUNC` | `a:real, none`, `b:real, none`, `tol:real, none = 0.000001` | `-> real, none` | `(@abs(a - b) <= tol)` | `is_close(6, 3, 3)` | `1` (`real`) |
| `124` | `is_close(a, b) [default wrapper: tol=0.000001]` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `Calls is_close(...) with tol=0.000001` | `is_close(6, 3)` | `0` (`real`) |
| `125` | `gcd(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `{ x := @abs(a) y := @abs(b) (y > 0) ?? { t := x % y x = y y = t } x }` | `gcd(6, 3)` | `3` (`real`) |
| `126` | `lcm(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `(a == 0 \|\| b == 0) ? 0 : @abs(a * b) / gcd(a, b)` | `lcm(6, 3)` | `6` (`real`) |
| `127` | `npr(n, r)` | `FUNC` | `n:real, none`, `r:real, none` | `-> real, none` | `(r < 0 \|\| r > n) ? 0 : (n!) / ((n - r)!)` | `npr(6, 3)` | `120` (`real`) |
| `128` | `ncr(n, r)` | `FUNC` | `n:real, none`, `r:real, none` | `-> real, none` | `(r < 0 \|\| r > n) ? 0 : (n!) / ((r!) * ((n - r)!))` | `ncr(6, 3)` | `20` (`real`) |
| `129` | `fib(n)` | `FUNC` | `n:real, none` | `-> real, none` | `{ a := 0 b := 1 k := 0 (k < n) ?? { nxt := a + b a = b b = nxt k += 1...` | `fib(6)` | `8` (`real`) |
| `130` | `is_prime(n)` | `FUNC` | `n:real, none` | `-> real, none` | `{ if (n < 2) { return 0 } d := 2 (d^2 <= n) ?? { if (n % d == 0) { re...` | `is_prime(6)` | `0` (`real`) |
| `131` | `mod_pow(base, exp_n, m)` | `FUNC` | `base:real, none`, `exp_n:real, none`, `m:real, none` | `-> real, none` | `{ res := 1 b   := base % m e_k := exp_n (e_k > 0) ?? { if (e_k % 2 ==...` | `mod_pow(6, 3, 3)` | `0` (`real`) |
| `132` | `pct_of(part, whole)` | `FUNC` | `part:real, none`, `whole:real, none` | `-> real, none` | `(part / whole) * 100` | `pct_of(6, 3)` | `200` (`real`) |
| `133` | `pct_val(pct, total)` | `FUNC` | `pct:real, none`, `total:real, none` | `-> real, none` | `(pct / 100) * total` | `pct_val(6, 3)` | `0.18` (`real`) |
| `134` | `pct_change(old_v, new_v)` | `FUNC` | `old_v:real, none`, `new_v:real, none` | `-> real, none` | `((new_v - old_v) / old_v) * 100` | `pct_change(6, 3)` | `-50` (`real`) |
| `135` | `pct_inc(val, pct)` | `FUNC` | `val:real, none`, `pct:real, none` | `-> real, none` | `val * (1 + pct / 100)` | `pct_inc(6, 3)` | `6.18` (`real`) |
| `136` | `pct_dec(val, pct)` | `FUNC` | `val:real, none`, `pct:real, none` | `-> real, none` | `val * (1 - pct / 100)` | `pct_dec(6, 3)` | `5.82` (`real`) |
| `137` | `pct_error(measured, exact)` | `FUNC` | `measured:real, none`, `exact:real, none` | `-> real, none` | `(@abs(measured - exact) / @abs(exact)) * 100` | `pct_error(6, 3)` | `100` (`real`) |
| `138` | `successive_pct(p1, p2)` | `FUNC` | `p1:real, none`, `p2:real, none` | `-> real, none` | `p1 + p2 + (p1 * p2) / 100` | `successive_pct(6, 3)` | `9.18` (`real`) |
| `139` | `profit(cp, sp)` | `FUNC` | `cp:real, none`, `sp:real, none` | `-> real, none` | `sp - cp` | `profit(6, 3)` | `-3` (`real`) |
| `140` | `loss(cp, sp)` | `FUNC` | `cp:real, none`, `sp:real, none` | `-> real, none` | `cp - sp` | `loss(6, 3)` | `3` (`real`) |
| `141` | `profit_pct(cp, sp)` | `FUNC` | `cp:real, none`, `sp:real, none` | `-> real, none` | `((sp - cp) / cp) * 100` | `profit_pct(6, 3)` | `-50` (`real`) |
| `142` | `loss_pct(cp, sp)` | `FUNC` | `cp:real, none`, `sp:real, none` | `-> real, none` | `((cp - sp) / cp) * 100` | `loss_pct(6, 3)` | `50` (`real`) |
| `143` | `sp_from_profit_pct(cp, p_pct)` | `FUNC` | `cp:real, none`, `p_pct:real, none` | `-> real, none` | `cp * (1 + p_pct / 100)` | `sp_from_profit_pct(6, 3)` | `6.18` (`real`) |
| `144` | `sp_from_loss_pct(cp, l_pct)` | `FUNC` | `cp:real, none`, `l_pct:real, none` | `-> real, none` | `cp * (1 - l_pct / 100)` | `sp_from_loss_pct(6, 3)` | `5.82` (`real`) |
| `145` | `cp_from_profit_pct(sp, p_pct)` | `FUNC` | `sp:real, none`, `p_pct:real, none` | `-> real, none` | `sp / (1 + p_pct / 100)` | `cp_from_profit_pct(6, 3)` | `5.825242718` (`real`) |
| `146` | `cp_from_loss_pct(sp, l_pct)` | `FUNC` | `sp:real, none`, `l_pct:real, none` | `-> real, none` | `sp / (1 - l_pct / 100)` | `cp_from_loss_pct(6, 3)` | `6.18556701` (`real`) |
| `147` | `discount_amt(marked_p, d_pct)` | `FUNC` | `marked_p:real, none`, `d_pct:real, none` | `-> real, none` | `marked_p * (d_pct / 100)` | `discount_amt(6, 3)` | `0.18` (`real`) |
| `148` | `sp_after_discount(marked_p, d_pct)` | `FUNC` | `marked_p:real, none`, `d_pct:real, none` | `-> real, none` | `marked_p * (1 - d_pct / 100)` | `sp_after_discount(6, 3)` | `5.82` (`real`) |
| `149` | `successive_discount(d1, d2)` | `FUNC` | `d1:real, none`, `d2:real, none` | `-> real, none` | `d1 + d2 - (d1 * d2) / 100` | `successive_discount(6, 3)` | `8.82` (`real`) |
| `150` | `simple_interest(p, r_pct, t_yr)` | `FUNC` | `p:real, none`, `r_pct:real, none`, `t_yr:real, none` | `-> real, none` | `(p * r_pct * t_yr) / 100` | `simple_interest(6, 3, 3)` | `0.54` (`real`) |
| `151` | `si_amount(p, r_pct, t_yr)` | `FUNC` | `p:real, none`, `r_pct:real, none`, `t_yr:real, none` | `-> real, none` | `p + simple_interest(p, r_pct, t_yr)` | `si_amount(6, 3, 3)` | `6.54` (`real`) |
| `152` | `compound_amount(p, r_pct, t_yr)` | `FUNC` | `p:real, none`, `r_pct:real, none`, `t_yr:real, none` | `-> real, none` | `p * ((1 + r_pct / 100) ^ t_yr)` | `compound_amount(6, 3, 3)` | `6.556362` (`real`) |
| `153` | `compound_interest(p, r_pct, t_yr)` | `FUNC` | `p:real, none`, `r_pct:real, none`, `t_yr:real, none` | `-> real, none` | `compound_amount(p, r_pct, t_yr) - p` | `compound_interest(6, 3, 3)` | `0.556362` (`real`) |
| `154` | `ci_amount_n(p, r_pct, t_yr, n_per_yr)` | `FUNC` | `p:real, none`, `r_pct:real, none`, `t_yr:real, none`, `n_per_yr:real, none` | `-> real, none` | `p * ((1 + (r_pct / 100) / n_per_yr) ^ (n_per_yr * t_yr))` | `ci_amount_n(6, 3, 3, 3)` | `6.562111636` (`real`) |
| `155` | `ci_si_diff_2yr(p, r_pct)` | `FUNC` | `p:real, none`, `r_pct:real, none` | `-> real, none` | `p * ((r_pct / 100) ^ 2)` | `ci_si_diff_2yr(6, 3)` | `0.0054` (`real`) |
| `156` | `depreciated_val(p0, r_pct, t_yr)` | `FUNC` | `p0:real, none`, `r_pct:real, none`, `t_yr:real, none` | `-> real, none` | `p0 * ((1 - r_pct / 100) ^ t_yr)` | `depreciated_val(6, 3, 3)` | `5.476038` (`real`) |
| `157` | `emi(principal, annual_rate_pct, months)` | `FUNC` | `principal:real, none`, `annual_rate_pct:real, none`, `months:real, none` | `-> real, none` | `{ r_m := (annual_rate_pct / 12) / 100 f   := (1 + r_m) ^ months (prin...` | `emi(6, 3, 3)` | `2.010008323` (`real`) |
| `158` | `work_rate(days)` | `FUNC` | `days:real, none` | `-> real, none` | `1 / days` | `work_rate(6)` | `0.1666666667` (`real`) |
| `159` | `work_together_2(d1, d2)` | `FUNC` | `d1:real, none`, `d2:real, none` | `-> real, none` | `(d1 * d2) / (d1 + d2)` | `work_together_2(6, 3)` | `2` (`real`) |
| `160` | `work_together_3(d1, d2, d3)` | `FUNC` | `d1:real, none`, `d2:real, none`, `d3:real, none` | `-> real, none` | `(d1 * d2 * d3) / (d1 * d2 + d2 * d3 + d3 * d1)` | `work_together_3(6, 3, 3)` | `1.2` (`real`) |
| `161` | `work_other_person(d_both, d_one)` | `FUNC` | `d_both:real, none`, `d_one:real, none` | `-> real, none` | `(d_both * d_one) / (d_one - d_both)` | `work_other_person(4, 12)` | `6` (`real`) |
| `162` | `work_rem_after(d_total, days_worked)` | `FUNC` | `d_total:real, none`, `days_worked:real, none` | `-> real, none` | `1 - (days_worked / d_total)` | `work_rem_after(6, 3)` | `0.5` (`real`) |
| `163` | `pipe_fill_with_leak(t_fill, t_empty)` | `FUNC` | `t_fill:real, none`, `t_empty:real, none` | `-> real, none` | `(t_fill * t_empty) / (t_empty - t_fill)` | `pipe_fill_with_leak(4, 12)` | `6` (`real`) |
| `164` | `man_days_solve(m1, d1, w1, m2, w2)` | `FUNC` | `m1:real, none`, `d1:real, none`, `w1:real, none`, `m2:real, none`, `w2:real, none` | `-> real, none` | `(m1 * d1 * w2) / (w1 * m2)` | `man_days_solve(10, 6, 1, 15, 2)` | `8` (`real`) |
| `165` | `ap_nth(a, d, n)` | `FUNC` | `a:real, none`, `d:real, none`, `n:real, none` | `-> real, none` | `a + (n - 1) * d` | `ap_nth(6, 3, 3)` | `12` (`real`) |
| `166` | `ap_sum(a, d, n)` | `FUNC` | `a:real, none`, `d:real, none`, `n:real, none` | `-> real, none` | `(n / 2) * (2a + (n - 1) * d)` | `ap_sum(6, 3, 3)` | `27` (`real`) |
| `167` | `gp_nth(a, r, n)` | `FUNC` | `a:real, none`, `r:real, none`, `n:real, none` | `-> real, none` | `a * (r ^ (n - 1))` | `gp_nth(6, 3, 3)` | `54` (`real`) |
| `168` | `gp_sum(a, r, n)` | `FUNC` | `a:real, none`, `r:real, none`, `n:real, none` | `-> real, none` | `(r == 1) ? (a * n) : a * ((r ^ n) - 1) / (r - 1)` | `gp_sum(6, 3, 3)` | `78` (`real`) |
| `169` | `gp_inf_sum(a, r)` | `FUNC` | `a:real, none`, `r:real, none` | `-> real, none` | `a / (1 - r)` | `gp_inf_sum(10, 0.5)` | `20` (`real`) |
| `170` | `hp_nth(a, d, n)` | `FUNC` | `a:real, none`, `d:real, none`, `n:real, none` | `-> real, none` | `1 / (a + (n - 1) * d)` | `hp_nth(6, 3, 3)` | `0.08333333333` (`real`) |
| `171` | `arith_mean(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `(a + b) / 2` | `arith_mean(6, 3)` | `4.5` (`real`) |
| `172` | `geom_mean(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `√(a * b)` | `geom_mean(6, 3)` | `4.242640687` (`real`) |
| `173` | `harm_mean(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `2a * b / (a + b)` | `harm_mean(6, 3)` | `4` (`real`) |
| `174` | `fourth_proportional(a, b, c)` | `FUNC` | `a:real, none`, `b:real, none`, `c:real, none` | `-> real, none` | `(b * c) / a` | `fourth_proportional(6, 3, 3)` | `1.5` (`real`) |
| `175` | `third_proportional(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `(b^2) / a` | `third_proportional(6, 3)` | `1.5` (`real`) |
| `176` | `mean_proportional(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `√(a * b)` | `mean_proportional(6, 3)` | `4.242640687` (`real`) |
| `177` | `alligation_ratio(cheaper, dearer, mean_p)` | `FUNC` | `cheaper:real, none`, `dearer:real, none`, `mean_p:real, none` | `-> real, none` | `(dearer - mean_p) / (mean_p - cheaper)` | `alligation_ratio(10, 20, 14)` | `1.5` (`real`) |
| `178` | `avg_speed_2(s1, s2)` | `FUNC` | `s1:real, none`, `s2:real, none` | `-> real, none` | `2s1 * s2 / (s1 + s2)` | `avg_speed_2(6, 3)` | `4` (`real`) |
| `179` | `rel_speed_same(s1, s2)` | `FUNC` | `s1:real, none`, `s2:real, none` | `-> real, none` | `@abs(s1 - s2)` | `rel_speed_same(6, 3)` | `3` (`real`) |
| `180` | `rel_speed_opp(s1, s2)` | `FUNC` | `s1:real, none`, `s2:real, none` | `-> real, none` | `s1 + s2` | `rel_speed_opp(6, 3)` | `9` (`real`) |
| `181` | `boat_downstream(u_boat, v_stream)` | `FUNC` | `u_boat:real, none`, `v_stream:real, none` | `-> real, none` | `u_boat + v_stream` | `boat_downstream(6, 3)` | `9` (`real`) |
| `182` | `boat_upstream(u_boat, v_stream)` | `FUNC` | `u_boat:real, none`, `v_stream:real, none` | `-> real, none` | `u_boat - v_stream` | `boat_upstream(6, 3)` | `3` (`real`) |
| `183` | `boat_still_speed(d_spd, u_spd)` | `FUNC` | `d_spd:real, none`, `u_spd:real, none` | `-> real, none` | `(d_spd + u_spd) / 2` | `boat_still_speed(6, 3)` | `4.5` (`real`) |
| `184` | `stream_speed(d_spd, u_spd)` | `FUNC` | `d_spd:real, none`, `u_spd:real, none` | `-> real, none` | `(d_spd - u_spd) / 2` | `stream_speed(6, 3)` | `1.5` (`real`) |
| `185` | `sum_n(n)` | `FUNC` | `n:real, none` | `-> real, none` | `n * (n + 1) / 2` | `sum_n(6)` | `21` (`real`) |
| `186` | `sum_sq_n(n)` | `FUNC` | `n:real, none` | `-> real, none` | `n * (n + 1) * (2n + 1) / 6` | `sum_sq_n(6)` | `91` (`real`) |
| `187` | `sum_cube_n(n)` | `FUNC` | `n:real, none` | `-> real, none` | `(n * (n + 1) / 2) ^ 2` | `sum_cube_n(6)` | `441` (`real`) |
| `188` | `bit_and(a:real, none, b:real, none)` | `FUNC` | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@band(a, b)` | `bit_and(6, 3)` | `2` (`real`) |
| `189` | `bit_or(a:real, none, b:real, none)` | `FUNC` | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@bor(a, b)` | `bit_or(6, 3)` | `7` (`real`) |
| `190` | `bit_xor(a:real, none, b:real, none)` | `FUNC` | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@bxor(a, b)` | `bit_xor(6, 3)` | `5` (`real`) |
| `191` | `bit_not(a:real, none)` | `FUNC` | `a:real, none (locked)` | `-> real, none` | `@bnot(a)` | `bit_not(6)` | `-7` (`real`) |
| `192` | `bit_shl(a:real, none, b:real, none)` | `FUNC` | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@bshl(a, b)` | `bit_shl(6, 3)` | `48` (`real`) |
| `193` | `bit_shr(a:real, none, b:real, none)` | `FUNC` | `a:real, none (locked)`, `b:real, none (locked)` | `-> real, none` | `@bshr(a, b)` | `bit_shr(6, 3)` | `0` (`real`) |
| `194` | `popcount(n:real, none)` | `FUNC` | `n:real, none (locked)` | `-> real, none` | `{ if (n < 0 \|\| n != @floor(n)) { error("popcount() requires a non-neg...` | `popcount(6)` | `2` (`real`) |
| `195` | `bit_len(n:real, none)` | `FUNC` | `n:real, none (locked)` | `-> real, none` | `{ if (n < 0 \|\| n != @floor(n)) { error("bit_len() requires a non-nega...` | `bit_len(6)` | `3` (`real`) |
| `196` | `binom_pmf(n, k, p)` | `FUNC` | `n:real, none`, `k:real, none`, `p:real, none` | `-> real, none` | `ncr(n, k) * (p ^ k) * ((1 - p) ^ (n - k))` | `binom_pmf(10, 5, 0.5)` | `0.24609375` (`real`) |
| `197` | `poisson_pmf(k, lam)` | `FUNC` | `k:real, none`, `lam:real, none` | `-> real, none` | `((lam ^ k) * @exp(-lam)) / (k!)` | `poisson_pmf(6, 3)` | `0.05040940672` (`real`) |
| `198` | `geom_pmf(k, p)` | `FUNC` | `k:real, none`, `p:real, none` | `-> real, none` | `((1 - p) ^ (k - 1)) * p` | `geom_pmf(3, 0.5)` | `0.125` (`real`) |
| `199` | `exp_pdf(x, lam)` | `FUNC` | `x:real, none`, `lam:real, none` | `-> real, none` | `(x < 0) ? 0 : lam * @exp(-lam * x)` | `exp_pdf(6, 3)` | `4.568993923e-08` (`real`) |
| `200` | `exp_cdf(x, lam)` | `FUNC` | `x:real, none`, `lam:real, none` | `-> real, none` | `(x < 0) ? 0 : 1 - @exp(-lam * x)` | `exp_cdf(6, 3)` | `0.9999999848` (`real`) |
| `201` | `is_inf(x)` | `FUNC` | `x:real, none` | `-> real, none` | `(@abs(x) == inf)` | `is_inf(6)` | `0` (`real`) |

### Module `[trig]` (`Library/trig.math` — 37 Overloads)

Circular, inverse, and hyperbolic trigonometry, exponentials/logarithms, roots, error functions, Gamma/Beta, and Gaussian normal distribution.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `202` | `sin(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@sin(x)` | `sin(6)` | `-0.2794154982` (`real`) |
| `203` | `cos(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@cos(x)` | `cos(6)` | `0.9601702867` (`real`) |
| `204` | `tan(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@tan(x)` | `tan(6)` | `-0.2910061914` (`real`) |
| `205` | `sec(x)` | `FUNC` | `x:real, none` | `-> real, none` | `1 / @cos(x)` | `sec(6)` | `1.041481927` (`real`) |
| `206` | `csc(x)` | `FUNC` | `x:real, none` | `-> real, none` | `1 / @sin(x)` | `csc(6)` | `-3.578899547` (`real`) |
| `207` | `cot(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@cos(x) / @sin(x)` | `cot(6)` | `-3.436353004` (`real`) |
| `208` | `asin(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@asin(x)` | `asin(0.5)` | `0.5235987756` (`real`) |
| `209` | `acos(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@acos(x)` | `acos(0.5)` | `1.047197551` (`real`) |
| `210` | `atan(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@atan(x)` | `atan(6)` | `1.405647649` (`real`) |
| `211` | `atan2(y, x)` | `FUNC` | `y:real, none`, `x:real, none` | `-> real, none` | `@atan2(y, x)` | `atan2(6, 3)` | `1.107148718` (`real`) |
| `212` | `sinh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `0.5 * (@exp(x) - @exp(-x))` | `sinh(6)` | `201.7131574` (`real`) |
| `213` | `cosh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `0.5 * (@exp(x) + @exp(-x))` | `cosh(6)` | `201.7156361` (`real`) |
| `214` | `tanh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `(@exp(x) - @exp(-x)) / (@exp(x) + @exp(-x))` | `tanh(6)` | `0.9999877117` (`real`) |
| `215` | `asinh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@ln(x + √(x^2 + 1))` | `asinh(6)` | `2.491779853` (`real`) |
| `216` | `acosh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@ln(x + √(x^2 - 1))` | `acosh(2)` | `1.316957897` (`real`) |
| `217` | `atanh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `0.5 * @ln((1 + x) / (1 - x))` | `atanh(0.5)` | `0.5493061443` (`real`) |
| `218` | `ln(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@ln(x)` | `ln(6)` | `1.791759469` (`real`) |
| `219` | `log10(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@ln(x) / @ln(10)` | `log10(6)` | `0.7781512504` (`real`) |
| `220` | `log2(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@ln(x) / @ln(2)` | `log2(6)` | `2.584962501` (`real`) |
| `221` | `logb(x, base)` | `FUNC` | `x:real, none`, `base:real, none` | `-> real, none` | `@ln(x) / @ln(base)` | `logb(6, 3)` | `1.630929754` (`real`) |
| `222` | `exp(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@exp(x)` | `exp(6)` | `403.4287935` (`real`) |
| `223` | `sqrt(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@sqrt(x)` | `sqrt(6)` | `2.449489743` (`real`) |
| `224` | `cbrt(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@cbrt(x)` | `cbrt(6)` | `1.817120593` (`real`) |
| `225` | `root(x, n)` | `FUNC` | `x:real, none`, `n:real, none` | `-> real, none` | `x ^ (1 / n)` | `root(6, 3)` | `1.817120593` (`real`) |
| `226` | `hypot(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `√(a^2 + b^2)` | `hypot(6, 3)` | `6.708203932` (`real`) |
| `227` | `sigmoid(x)` | `FUNC` | `x:real, none` | `-> real, none` | `1 / (1 + @exp(-x))` | `sigmoid(6)` | `0.9975273768` (`real`) |
| `228` | `erf(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@erf(x)` | `erf(6)` | `1` (`real`) |
| `229` | `erfc(x)` | `FUNC` | `x:real, none` | `-> real, none` | `1 - @erf(x)` | `erfc(6)` | `0` (`real`) |
| `230` | `gamma(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@gamma(x)` | `gamma(6)` | `120` (`real`) |
| `231` | `lgamma(x)` | `FUNC` | `x:real, none` | `-> real, none` | `@lgamma(x)` | `lgamma(6)` | `4.787491743` (`real`) |
| `232` | `beta_fn(a, b)` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `@exp(@lgamma(a) + @lgamma(b) - @lgamma(a + b))` | `beta_fn(6, 3)` | `0.005952380952` (`real`) |
| `233` | `norm_pdf(x, mu = 0, s = 1)` | `FUNC` | `x:real, none`, `mu:real, none = 0`, `s:real, none = 1` | `-> real, none` | `(1 / (s * √(2π))) * @exp(-0.5 * (((x - mu) / s) ^ 2))` | `norm_pdf(6, 3, 3)` | `0.08065690817` (`real`) |
| `234` | `norm_pdf(x) [default wrapper: mu=0, s=1]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls norm_pdf(...) with mu=0, s=1` | `norm_pdf(6)` | `6.07588285e-09` (`real`) |
| `235` | `norm_pdf(x, mu) [default wrapper: s=1]` | `FUNC` | `x:real, none`, `mu:real, none` | `-> real, none` | `Calls norm_pdf(...) with s=1` | `norm_pdf(6, 3)` | `0.004431848412` (`real`) |
| `236` | `norm_cdf(x, mu = 0, s = 1)` | `FUNC` | `x:real, none`, `mu:real, none = 0`, `s:real, none = 1` | `-> real, none` | `0.5 * (1 + @erf((x - mu) / (s * √2)))` | `norm_cdf(6, 3, 3)` | `0.8413447461` (`real`) |
| `237` | `norm_cdf(x) [default wrapper: mu=0, s=1]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls norm_cdf(...) with mu=0, s=1` | `norm_cdf(6)` | `0.999999999` (`real`) |
| `238` | `norm_cdf(x, mu) [default wrapper: s=1]` | `FUNC` | `x:real, none`, `mu:real, none` | `-> real, none` | `Calls norm_cdf(...) with s=1` | `norm_cdf(6, 3)` | `0.998650102` (`real`) |

### Module `[complex_ext]` (`Library/complex.math` — 74 Overloads)

Postfix imaginary unit `i`, rectangular `$complex(re, im)` struct, mixed real/iota/complex algebra, polar form, and complex transcendental functions.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `239` | `(x:real, T)    i :95 -> iota, T` | `POSTFIX` (prec `95`) | `x:real, T` | `-> iota, T` | `@raw(x)` | `6i` | `6i` (`iota`) |
| `240` | `(x:iota, T)    i :95 -> real, T` | `POSTFIX` (prec `95`) | `x:iota, T` | `-> real, T` | `-@raw(x)` | `(3i)i` | `-3` (`real`) |
| `241` | `(z:complex, T) i :95 -> complex, T` | `POSTFIX` (prec `95`) | `z:complex, T` | `-> complex, T` | `complex(-z.im, z.re)` | `(3 + 4i)i` | `-4 + 3i` (`complex`) |
| `242` | `"-" (a:iota, T)             :80  -> iota, T` | `PREFIX` (prec `80`) | `a:iota, T` | `-> iota, T` | `@neg(a)` | `-3i` | `-3i` (`iota`) |
| `243` | `(a:iota, T) "+"  (b:iota, T):<50 -> iota, T` | `INFIX_LEFT` (prec `50`) | `a:iota, T`, `b:iota, T` | `-> iota, T` | `@add(a, b)` | `3i + 2i` | `5i` (`iota`) |
| `244` | `(a:iota, T) "-"  (b:iota, T):<50 -> iota, T` | `INFIX_LEFT` (prec `50`) | `a:iota, T`, `b:iota, T` | `-> iota, T` | `@sub(a, b)` | `3i - 2i` | `1i` (`iota`) |
| `245` | `(a:iota, T) into (b:iota, T):>55 -> iota, T` | `INFIX_RIGHT` (prec `55`) | `a:iota, T`, `b:iota, T` | `-> iota, T` | `@add(a, b)` | `3i into 2i` | `5i` (`iota`) |
| `246` | `(a:iota)    "*"  (b:iota)   :<60` | `INFIX_LEFT` (prec `60`) | `a:iota, none`, `b:iota, none` | `-> real, none` | `-(@mul(a, b))` | `3i * 2i` | `-6` (`real`) |
| `247` | `(a)         "*"  (b:iota)   :<60 -> iota` | `INFIX_LEFT` (prec `60`) | `a:real, none`, `b:iota, none` | `-> iota, none` | `@mul(a, b)` | `6 * 2i` | `12i` (`iota`) |
| `248` | `(a:iota)    "*"  (b)        :<60 -> iota` | `INFIX_LEFT` (prec `60`) | `a:iota, none`, `b:real, none` | `-> iota, none` | `@mul(a, b)` | `3i * 3` | `9i` (`iota`) |
| `249` | `(a:iota)    "/"  (b:iota)   :<60` | `INFIX_LEFT` (prec `60`) | `a:iota, none`, `b:iota, none` | `-> real, none` | `@div(a, b)` | `3i / 2i` | `1.5` (`real`) |
| `250` | `(a:iota, T) "/"  (b:real)   :<60 -> iota, T` | `INFIX_LEFT` (prec `60`) | `a:iota, T`, `b:real, none` | `-> iota, T` | `@div(a, b)` | `3i / 3` | `1i` (`iota`) |
| `251` | `(a:real, T) "/"  (b:iota)   :<60 -> iota, T` | `INFIX_LEFT` (prec `60`) | `a:real, T`, `b:iota, none` | `-> iota, T` | `-(@div(a, b))` | `6 / 2i` | `-3i` (`iota`) |
| `252` | `(a:iota, T) "==" (b:iota, T):<30` | `INFIX_LEFT` (prec `30`) | `a:iota, T`, `b:iota, T` | `-> real, none` | `@eq(a, b)` | `3i == 2i` | `0` (`real`) |
| `253` | `(a:iota, T) "!=" (b:iota, T):<30` | `INFIX_LEFT` (prec `30`) | `a:iota, T`, `b:iota, T` | `-> real, none` | `@ne(a, b)` | `3i != 2i` | `1` (`real`) |
| `254` | `(a:iota, T) "~"  (b:iota, T):<30` | `INFIX_LEFT` (prec `30`) | `a:iota, T`, `b:iota, T` | `-> real, none` | `(@abs(@raw(a) - @raw(b)) <= 0.001 + 1e-12)` | `3i ~ 2i` | `0` (`real`) |
| `255` | `(a:real, T)    "+" (b:iota, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:real, T`, `b:iota, T` | `-> complex, T` | `complex(a, @raw(b))` | `6 + 2i` | `6 + 2i` (`complex`) |
| `256` | `(a:real, T)    "-" (b:iota, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:real, T`, `b:iota, T` | `-> complex, T` | `complex(a, -@raw(b))` | `6 - 2i` | `6 - 2i` (`complex`) |
| `257` | `(a:iota, T)    "+" (b:real, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:iota, T`, `b:real, T` | `-> complex, T` | `complex(b, @raw(a))` | `3i + 3` | `3 + 3i` (`complex`) |
| `258` | `(a:iota, T)    "-" (b:real, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:iota, T`, `b:real, T` | `-> complex, T` | `complex(-b, @raw(a))` | `3i - 3` | `-3 + 3i` (`complex`) |
| `259` | `(z:complex, T) "+" (b:iota, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `z:complex, T`, `b:iota, T` | `-> complex, T` | `complex(z.re, z.im + @raw(b))` | `(3 + 4i) + 2i` | `3 + 6i` (`complex`) |
| `260` | `(z:complex, T) "-" (b:iota, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `z:complex, T`, `b:iota, T` | `-> complex, T` | `complex(z.re, z.im - @raw(b))` | `(3 + 4i) - 2i` | `3 + 2i` (`complex`) |
| `261` | `(a:iota, T)    "+" (z:complex, T) :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:iota, T`, `z:complex, T` | `-> complex, T` | `complex(z.re, @raw(a) + z.im)` | `3i + (1 + 2i)` | `1 + 5i` (`complex`) |
| `262` | `(a:iota, T)    "-" (z:complex, T) :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:iota, T`, `z:complex, T` | `-> complex, T` | `complex(-z.re, @raw(a) - z.im)` | `3i - (1 + 2i)` | `-1 + 1i` (`complex`) |
| `263` | `(z:complex, T) "+" (b:real, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `z:complex, T`, `b:real, T` | `-> complex, T` | `complex(z.re + b, z.im)` | `(3 + 4i) + 3` | `6 + 4i` (`complex`) |
| `264` | `(z:complex, T) "-" (b:real, T)    :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `z:complex, T`, `b:real, T` | `-> complex, T` | `complex(z.re - b, z.im)` | `(3 + 4i) - 3` | `0 + 4i` (`complex`) |
| `265` | `(a:real, T)    "+" (z:complex, T) :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:real, T`, `z:complex, T` | `-> complex, T` | `complex(a + z.re, z.im)` | `6 + (1 + 2i)` | `7 + 2i` (`complex`) |
| `266` | `(a:real, T)    "-" (z:complex, T) :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:real, T`, `z:complex, T` | `-> complex, T` | `complex(a - z.re, -z.im)` | `6 - (1 + 2i)` | `5 - 2i` (`complex`) |
| `267` | `(a:real, T)    into (b:iota, T)    :>55 -> complex, T` | `INFIX_RIGHT` (prec `55`) | `a:real, T`, `b:iota, T` | `-> complex, T` | `complex(a, @raw(b))` | `6 into 2i` | `6 + 2i` (`complex`) |
| `268` | `(a:iota, T)    into (b:real, T)    :>55 -> complex, T` | `INFIX_RIGHT` (prec `55`) | `a:iota, T`, `b:real, T` | `-> complex, T` | `complex(b, @raw(a))` | `3i into 3` | `3 + 3i` (`complex`) |
| `269` | `(a:real, T)    into (z:complex, T) :>55 -> complex, T` | `INFIX_RIGHT` (prec `55`) | `a:real, T`, `z:complex, T` | `-> complex, T` | `complex(z.re + a, z.im)` | `6 into (1 + 2i)` | `7 + 2i` (`complex`) |
| `270` | `(a:iota, T)    into (z:complex, T) :>55 -> complex, T` | `INFIX_RIGHT` (prec `55`) | `a:iota, T`, `z:complex, T` | `-> complex, T` | `complex(z.re, z.im + @raw(a))` | `3i into (1 + 2i)` | `1 + 5i` (`complex`) |
| `271` | `(a:complex, T) into (b:complex, T) :>55 -> complex, T` | `INFIX_RIGHT` (prec `55`) | `a:complex, T`, `b:complex, T` | `-> complex, T` | `complex(a.re + b.re, a.im + b.im)` | `(3 + 4i) into (1 + 2i)` | `4 + 6i` (`complex`) |
| `272` | `"-" (z:complex, T)                 :80  -> complex, T` | `PREFIX` (prec `80`) | `z:complex, T` | `-> complex, T` | `complex(-z.re, -z.im)` | `-(3 + 4i)` | `-3 - 4i` (`complex`) |
| `273` | `(a:complex, T) "+" (b:complex, T) :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:complex, T`, `b:complex, T` | `-> complex, T` | `complex(a.re + b.re, a.im + b.im)` | `(3 + 4i) + (1 + 2i)` | `4 + 6i` (`complex`) |
| `274` | `(a:complex, T) "-" (b:complex, T) :<50 -> complex, T` | `INFIX_LEFT` (prec `50`) | `a:complex, T`, `b:complex, T` | `-> complex, T` | `complex(a.re - b.re, a.im - b.im)` | `(3 + 4i) - (1 + 2i)` | `2 + 2i` (`complex`) |
| `275` | `(s:real)       "*" (z:complex, T) :<60 -> complex, T` | `INFIX_LEFT` (prec `60`) | `s:real, none`, `z:complex, T` | `-> complex, T` | `complex(s * z.re, s * z.im)` | `6 * (1 + 2i)` | `6 + 12i` (`complex`) |
| `276` | `(z:complex, T) "*" (s:real)       :<60 -> complex, T` | `INFIX_LEFT` (prec `60`) | `z:complex, T`, `s:real, none` | `-> complex, T` | `complex(z.re * s, z.im * s)` | `(3 + 4i) * 3` | `9 + 12i` (`complex`) |
| `277` | `(z:complex, T) "/" (s:real)       :<60 -> complex, T` | `INFIX_LEFT` (prec `60`) | `z:complex, T`, `s:real, none` | `-> complex, T` | `complex(z.re / s, z.im / s)` | `(3 + 4i) / 3` | `1 + 1.33333i` (`complex`) |
| `278` | `(a:complex)    "*" (b:complex)    :<60 -> complex` | `INFIX_LEFT` (prec `60`) | `a:complex, none`, `b:complex, none` | `-> complex, none` | `complex(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re)` | `(3 + 4i) * (1 + 2i)` | `-5 + 10i` (`complex`) |
| `279` | `(a:complex)    "/" (b:complex)    :<60 -> complex` | `INFIX_LEFT` (prec `60`) | `a:complex, none`, `b:complex, none` | `-> complex, none` | `{ denom := b.re^2 + b.im^2 complex((a.re * b.re + a.im * b.im) / deno...` | `(3 + 4i) / (1 + 2i)` | `2.2 - 0.4i` (`complex`) |
| `280` | `conj(z:complex, T)      -> complex, T` | `FUNC` | `z:complex, T` | `-> complex, T` | `complex(z.re, -z.im)` | `conj((3 + 4i))` | `3 - 4i` (`complex`) |
| `281` | `cabs(z:complex, T)      -> real, T` | `FUNC` | `z:complex, T` | `-> real, T` | `√(z.re^2 + z.im^2)` | `cabs((3 + 4i))` | `5` (`real`) |
| `282` | `carg(z:complex)` | `FUNC` | `z:complex, none` | `-> real, none` | `atan2(z.im, z.re)` | `carg((3 + 4i))` | `0.927295218` (`real`) |
| `283` | `cexp_i(theta)           -> complex` | `FUNC` | `theta:real, none` | `-> complex, none` | `complex(cos(theta), sin(theta))` | `cexp_i(6)` | `0.96017 - 0.279415i` (`complex`) |
| `284` | `polar(r:real, T, theta) -> complex, T` | `FUNC` | `r:real, T`, `theta:real, none` | `-> complex, T` | `complex(r * cos(theta), r * sin(theta))` | `polar(6, 3)` | `-5.93995 + 0.84672i` (`complex`) |
| `285` | `(s:real)       "/" (z:complex)    :<60 -> complex` | `INFIX_LEFT` (prec `60`) | `s:real, none`, `z:complex, none` | `-> complex, none` | `complex(s, 0) / z` | `6 / (1 + 2i)` | `1.2 - 2.4i` (`complex`) |
| `286` | `(a:iota)       "*" (z:complex, T) :<60 -> complex, T` | `INFIX_LEFT` (prec `60`) | `a:iota, none`, `z:complex, T` | `-> complex, T` | `complex(-@raw(a) * z.im, @raw(a) * z.re)` | `3i * (1 + 2i)` | `-6 + 3i` (`complex`) |
| `287` | `(z:complex, T) "*" (b:iota)       :<60 -> complex, T` | `INFIX_LEFT` (prec `60`) | `z:complex, T`, `b:iota, none` | `-> complex, T` | `complex(-z.im * @raw(b), z.re * @raw(b))` | `(3 + 4i) * 2i` | `-8 + 6i` (`complex`) |
| `288` | `(z:complex, T) "/" (b:iota)       :<60 -> complex, T` | `INFIX_LEFT` (prec `60`) | `z:complex, T`, `b:iota, none` | `-> complex, T` | `complex(z.im / @raw(b), -z.re / @raw(b))` | `(3 + 4i) / 2i` | `2 - 1.5i` (`complex`) |
| `289` | `(a:iota)       "/" (z:complex)    :<60 -> complex` | `INFIX_LEFT` (prec `60`) | `a:iota, none`, `z:complex, none` | `-> complex, none` | `complex(0, @raw(a)) / z` | `3i / (1 + 2i)` | `1.2 + 0.6i` (`complex`) |
| `290` | `re(z:complex, T)        -> real, T` | `FUNC` | `z:complex, T` | `-> real, T` | `z.re` | `re((3 + 4i))` | `3` (`real`) |
| `291` | `im(z:complex, T)        -> real, T` | `FUNC` | `z:complex, T` | `-> real, T` | `z.im` | `im((3 + 4i))` | `4` (`real`) |
| `292` | `exp(z:complex)                    -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `polar(exp(z.re), z.im)` | `exp((3 + 4i))` | `-13.1288 - 15.2008i` (`complex`) |
| `293` | `ln(z:complex)                     -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `complex(ln(cabs(z)), carg(z))` | `ln((3 + 4i))` | `1.60944 + 0.927295i` (`complex`) |
| `294` | `(z:complex) "^" (p:real)    :>85  -> complex` | `INFIX_RIGHT` (prec `85`) | `z:complex, none`, `p:real, none` | `-> complex, none` | `polar(cabs(z) ^ p, carg(z) * p)` | `(3 + 4i) ^ 3` | `-117 + 44i` (`complex`) |
| `295` | `(z:complex) "^" (w:complex) :>85  -> complex` | `INFIX_RIGHT` (prec `85`) | `z:complex, none`, `w:complex, none` | `-> complex, none` | `exp(w * ln(z))` | `(3 + 4i) ^ (1 + 2i)` | `-0.419813 - 0.660452i` (`complex`) |
| `296` | `(a:real)    "^" (w:complex) :>85  -> complex` | `INFIX_RIGHT` (prec `85`) | `a:real, none`, `w:complex, none` | `-> complex, none` | `exp(w * ln(a))` | `6 ^ (1 + 2i)` | `-5.42358 - 2.56609i` (`complex`) |
| `297` | `"√" (z:complex)             :80   -> complex` | `PREFIX` (prec `80`) | `z:complex, none` | `-> complex, none` | `polar(√cabs(z), carg(z) / 2)` | `√(3 + 4i)` | `2 + 1i` (`complex`) |
| `298` | `sqrt(z:complex)                   -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `√z` | `sqrt((3 + 4i))` | `2 + 1i` (`complex`) |
| `299` | `sin(z:complex)                    -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `complex(sin(z.re) * cosh(z.im), cos(z.re) * sinh(z.im))` | `sin((3 + 4i))` | `3.85374 - 27.0168i` (`complex`) |
| `300` | `cos(z:complex)                    -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `complex(cos(z.re) * cosh(z.im), -sin(z.re) * sinh(z.im))` | `cos((3 + 4i))` | `-27.0349 - 3.85115i` (`complex`) |
| `301` | `tan(z:complex)                    -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `sin(z) / cos(z)` | `tan((3 + 4i))` | `-0.000187346 + 0.999356i` (`complex`) |
| `302` | `sinh(z:complex)                   -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `complex(sinh(z.re) * cos(z.im), cosh(z.re) * sin(z.im))` | `sinh((3 + 4i))` | `-6.54812 - 7.61923i` (`complex`) |
| `303` | `cosh(z:complex)                   -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `complex(cosh(z.re) * cos(z.im), sinh(z.re) * sin(z.im))` | `cosh((3 + 4i))` | `-6.58066 - 7.58155i` (`complex`) |
| `304` | `tanh(z:complex)                   -> complex` | `FUNC` | `z:complex, none` | `-> complex, none` | `sinh(z) / cosh(z)` | `tanh((3 + 4i))` | `1.00071 + 0.00490826i` (`complex`) |
| `305` | `(a:complex, T) "==" (b:complex, T) :<30` | `INFIX_LEFT` (prec `30`) | `a:complex, T`, `b:complex, T` | `-> real, none` | `(a.re == b.re) && (a.im == b.im)` | `(3 + 4i) == (1 + 2i)` | `0` (`real`) |
| `306` | `(a:complex, T) "!=" (b:complex, T) :<30` | `INFIX_LEFT` (prec `30`) | `a:complex, T`, `b:complex, T` | `-> real, none` | `(a.re != b.re) \|\| (a.im != b.im)` | `(3 + 4i) != (1 + 2i)` | `1` (`real`) |
| `307` | `(a:complex, T) "~"  (b:complex, T) :<30` | `INFIX_LEFT` (prec `30`) | `a:complex, T`, `b:complex, T` | `-> real, none` | `(a.re ~ b.re) && (a.im ~ b.im)` | `(3 + 4i) ~ (1 + 2i)` | `0` (`real`) |
| `308` | `(a:iota)    "^" (p:real)    :>85 -> complex` | `INFIX_RIGHT` (prec `85`) | `a:iota, none`, `p:real, none` | `-> complex, none` | `complex(0, @raw(a)) ^ p` | `3i ^ 3` | `-4.95982e-15 - 27i` (`complex`) |
| `309` | `(a:real)    "^" (b:iota)    :>85 -> complex` | `INFIX_RIGHT` (prec `85`) | `a:real, none`, `b:iota, none` | `-> complex, none` | `a ^ complex(0, @raw(b))` | `6 ^ 2i` | `-0.90393 - 0.427681i` (`complex`) |
| `310` | `(a:iota)    "^" (b:iota)    :>85 -> complex` | `INFIX_RIGHT` (prec `85`) | `a:iota, none`, `b:iota, none` | `-> complex, none` | `complex(0, @raw(a)) ^ complex(0, @raw(b))` | `3i ^ 2i` | `-0.0253344 + 0.0350087i` (`complex`) |
| `311` | `(z:complex) "^" (b:iota)    :>85 -> complex` | `INFIX_RIGHT` (prec `85`) | `z:complex, none`, `b:iota, none` | `-> complex, none` | `z ^ complex(0, @raw(b))` | `(3 + 4i) ^ 2i` | `-0.15605 - 0.0120841i` (`complex`) |
| `312` | `(a:iota)    "^" (w:complex) :>85 -> complex` | `INFIX_RIGHT` (prec `85`) | `a:iota, none`, `w:complex, none` | `-> complex, none` | `complex(0, @raw(a)) ^ w` | `3i ^ (1 + 2i)` | `-0.105026 - 0.0760031i` (`complex`) |

### Module `[units]` (`Library/units.math` — 169 Overloads)

32 7D SI dimensional unit declarations (`@dim`) and 160+ postfix SI/imperial unit suffixes across 32 physical unit groups.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `313` | `(d) deg    :95 -> real, angle` | `POSTFIX` (prec `95`) | `d:real, none` | `-> real, angle` | `d * π / 180` | `(6) deg` | `0.1047197551 [angle]` (`real`) |
| `314` | `(d) "°"    :95 -> real, angle` | `POSTFIX` (prec `95`) | `d:real, none` | `-> real, angle` | `d * π / 180` | `6°` | `0.1047197551 [angle]` (`real`) |
| `315` | `(r) rad    :95 -> real, angle` | `POSTFIX` (prec `95`) | `r:real, none` | `-> real, angle` | `r` | `(6) rad` | `6 [angle]` (`real`) |
| `316` | `(m) arcmin :95 -> real, angle` | `POSTFIX` (prec `95`) | `m:real, none` | `-> real, angle` | `m * π / (180 * 60)` | `(6) arcmin` | `0.001745329252 [angle]` (`real`) |
| `317` | `(s) arcsec :95 -> real, angle` | `POSTFIX` (prec `95`) | `s:real, none` | `-> real, angle` | `s * π / (180 * 3600)` | `(6) arcsec` | `2.908882087e-05 [angle]` (`real`) |
| `318` | `(x) pct    :95` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, none` | `x * 0.01` | `(6) pct` | `0.06` (`real`) |
| `319` | `(x) dB     :95` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, none` | `10 ^ (x / 20)` | `(6) dB` | `1.995262315` (`real`) |
| `320` | `(x) dBm    :95` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, none` | `0.001 * (10 ^ (x / 10))` | `(6) dBm` | `0.003981071706` (`real`) |
| `321` | `deg2rad(deg) -> real, angle` | `FUNC` | `deg:real, none` | `-> real, angle` | `deg * π / 180` | `deg2rad(6)` | `0.1047197551 [angle]` (`real`) |
| `322` | `rad2deg(rad)` | `FUNC` | `rad:real, none` | `-> real, none` | `rad * 180 / π` | `rad2deg(6)` | `343.7746771` (`real`) |
| `323` | `(x)      m    :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x` | `(6) m` | `6 [distance]` (`real`) |
| `324` | `(x:iota) m    :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:iota, none` | `-> iota, distance` | `@raw(x)` | `(3i) m` | `3i [distance]` (`iota`) |
| `325` | `(x)      km   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 1000` | `(6) km` | `6000 [distance]` (`real`) |
| `326` | `(x:iota) km   :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:iota, none` | `-> iota, distance` | `@mul(x, 1000)` | `(3i) km` | `3000i [distance]` (`iota`) |
| `327` | `(x)      cm   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 0.01` | `(6) cm` | `0.06 [distance]` (`real`) |
| `328` | `(x:iota) cm   :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:iota, none` | `-> iota, distance` | `@mul(x, 0.01)` | `(3i) cm` | `0.03i [distance]` (`iota`) |
| `329` | `(x)      mm   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 0.001` | `(6) mm` | `0.006 [distance]` (`real`) |
| `330` | `(x:iota) mm   :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:iota, none` | `-> iota, distance` | `@mul(x, 0.001)` | `(3i) mm` | `0.003i [distance]` (`iota`) |
| `331` | `(x)      im   :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> iota, distance` | `@raw(x)` | `(6) im` | `6i [distance]` (`iota`) |
| `332` | `(x)      ikm  :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> iota, distance` | `@mul(x, 1000)` | `(6) ikm` | `6000i [distance]` (`iota`) |
| `333` | `(x)      icm  :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> iota, distance` | `@mul(x, 0.01)` | `(6) icm` | `0.06i [distance]` (`iota`) |
| `334` | `(x)      imm  :95 -> iota, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> iota, distance` | `@mul(x, 0.001)` | `(6) imm` | `0.006i [distance]` (`iota`) |
| `335` | `(x)      um   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * (10 ^ -6)` | `(6) um` | `6e-06 [distance]` (`real`) |
| `336` | `(x)      nm   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * (10 ^ -9)` | `(6) nm` | `6e-09 [distance]` (`real`) |
| `337` | `(x)      mi   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 1609.344` | `(6) mi` | `9656.064 [distance]` (`real`) |
| `338` | `(x)      nmi  :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 1852` | `(6) nmi` | `11112 [distance]` (`real`) |
| `339` | `(x)      yd   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 0.9144` | `(6) yd` | `5.4864 [distance]` (`real`) |
| `340` | `(x)      ft   :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 0.3048` | `(6) ft` | `1.8288 [distance]` (`real`) |
| `341` | `(x)      inch :95 -> real, distance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, distance` | `x * 0.0254` | `(6) inch` | `0.1524 [distance]` (`real`) |
| `342` | `m_to_km(m)` | `FUNC` | `m:real, none` | `-> real, none` | `m / 1000` | `m_to_km(6)` | `0.006` (`real`) |
| `343` | `m_to_ft(m)` | `FUNC` | `m:real, none` | `-> real, none` | `m / 0.3048` | `m_to_ft(6)` | `19.68503937` (`real`) |
| `344` | `m_to_mi(m)` | `FUNC` | `m:real, none` | `-> real, none` | `m / 1609.344` | `m_to_mi(6)` | `0.003728227153` (`real`) |
| `345` | `ft_to_m(ft_v) -> real, distance` | `FUNC` | `ft_v:real, none` | `-> real, distance` | `ft_v * 0.3048` | `ft_to_m(6)` | `1.8288 [distance]` (`real`) |
| `346` | `in_to_cm(in_v)` | `FUNC` | `in_v:real, none` | `-> real, none` | `in_v * 2.54` | `in_to_cm(6)` | `15.24` (`real`) |
| `347` | `cm_to_in(cm_v)` | `FUNC` | `cm_v:real, none` | `-> real, none` | `cm_v / 2.54` | `cm_to_in(6)` | `2.362204724` (`real`) |
| `348` | `(x) m2   :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x` | `(6) m2` | `6 [area]` (`real`) |
| `349` | `(x) km2  :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 1000000` | `(6) km2` | `6000000 [area]` (`real`) |
| `350` | `(x) cm2  :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 0.0001` | `(6) cm2` | `0.0006 [area]` (`real`) |
| `351` | `(x) mm2  :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * (10 ^ -6)` | `(6) mm2` | `6e-06 [area]` (`real`) |
| `352` | `(x) ha   :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 10000` | `(6) ha` | `60000 [area]` (`real`) |
| `353` | `(x) acre :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 4046.8564224` | `(6) acre` | `24281.13853 [area]` (`real`) |
| `354` | `(x) yd2  :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 0.83612736` | `(6) yd2` | `5.01676416 [area]` (`real`) |
| `355` | `(x) ft2  :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 0.09290304` | `(6) ft2` | `0.55741824 [area]` (`real`) |
| `356` | `(x) in2  :95 -> real, area` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, area` | `x * 0.00064516` | `(6) in2` | `0.00387096 [area]` (`real`) |
| `357` | `(x) m3    :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x` | `(6) m3` | `6 [volume]` (`real`) |
| `358` | `(x) cm3   :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * (10 ^ -6)` | `(6) cm3` | `6e-06 [volume]` (`real`) |
| `359` | `(x) mm3   :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * (10 ^ -9)` | `(6) mm3` | `6e-09 [volume]` (`real`) |
| `360` | `(x) L     :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * 0.001` | `(6) L` | `0.006 [volume]` (`real`) |
| `361` | `(x) liter :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * 0.001` | `(6) liter` | `0.006 [volume]` (`real`) |
| `362` | `(x) mL    :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * (10 ^ -6)` | `(6) mL` | `6e-06 [volume]` (`real`) |
| `363` | `(x) gal   :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * 0.003785411784` | `(6) gal` | `0.0227124707 [volume]` (`real`) |
| `364` | `(x) ft3   :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * 0.028316846592` | `(6) ft3` | `0.1699010796 [volume]` (`real`) |
| `365` | `(x) in3   :95 -> real, volume` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, volume` | `x * 0.000016387064` | `(6) in3` | `9.8322384e-05 [volume]` (`real`) |
| `366` | `(x) kg   :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x` | `(6) kg` | `6 [mass]` (`real`) |
| `367` | `(x) gram :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x * 0.001` | `(6) gram` | `0.006 [mass]` (`real`) |
| `368` | `(x) mg   :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x * (10 ^ -6)` | `(6) mg` | `6e-06 [mass]` (`real`) |
| `369` | `(x) ton  :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x * 1000` | `(6) ton` | `6000 [mass]` (`real`) |
| `370` | `(x) lb   :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x * 0.45359237` | `(6) lb` | `2.72155422 [mass]` (`real`) |
| `371` | `(x) oz   :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x * 0.028349523125` | `(6) oz` | `0.1700971388 [mass]` (`real`) |
| `372` | `(x) amu  :95 -> real, mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, mass` | `x * 1.66053906660 * (10 ^ -27)` | `(6) amu` | `9.9632344e-27 [mass]` (`real`) |
| `373` | `(x) kgm3 :95 -> real, density` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, density` | `x` | `(6) kgm3` | `6 [density]` (`real`) |
| `374` | `(x) gcm3 :95 -> real, density` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, density` | `x * 1000` | `(6) gcm3` | `6000 [density]` (`real`) |
| `375` | `kg_to_lb(kg_v)` | `FUNC` | `kg_v:real, none` | `-> real, none` | `kg_v / 0.45359237` | `kg_to_lb(6)` | `13.22773573` (`real`) |
| `376` | `lb_to_kg(lb_v) -> real, mass` | `FUNC` | `lb_v:real, none` | `-> real, mass` | `lb_v * 0.45359237` | `lb_to_kg(6)` | `2.72155422 [mass]` (`real`) |
| `377` | `(x) ms    :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x * 0.001` | `(6) ms` | `0.006 [time]` (`real`) |
| `378` | `(x) us    :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x * (10 ^ -6)` | `(6) us` | `6e-06 [time]` (`real`) |
| `379` | `(x) ns    :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x * (10 ^ -9)` | `(6) ns` | `6e-09 [time]` (`real`) |
| `380` | `(x) sec   :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x` | `(6) sec` | `6 [time]` (`real`) |
| `381` | `(x) min_t :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x * 60` | `(6) min_t` | `360 [time]` (`real`) |
| `382` | `(x) hr    :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x * 3600` | `(6) hr` | `21600 [time]` (`real`) |
| `383` | `(x) day   :95 -> real, time` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time` | `x * 86400` | `(6) day` | `518400 [time]` (`real`) |
| `384` | `(x) sec2  :95 -> real, time2` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, time2` | `x` | `(6) sec2` | `6 [time2]` (`real`) |
| `385` | `(x) Hz    :95 -> real, frequency` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, frequency` | `x` | `(6) Hz` | `6 [frequency]` (`real`) |
| `386` | `(x) kHz   :95 -> real, frequency` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, frequency` | `x * 1000` | `(6) kHz` | `6000 [frequency]` (`real`) |
| `387` | `(x) MHz   :95 -> real, frequency` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, frequency` | `x * 1000000` | `(6) MHz` | `6000000 [frequency]` (`real`) |
| `388` | `(x) GHz   :95 -> real, frequency` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, frequency` | `x * (10 ^ 9)` | `(6) GHz` | `6000000000 [frequency]` (`real`) |
| `389` | `(x) rpm   :95 -> real, frequency` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, frequency` | `x / 60` | `(6) rpm` | `0.1 [frequency]` (`real`) |
| `390` | `(x) mps   :95 -> real, speed` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, speed` | `x` | `(6) mps` | `6 [speed]` (`real`) |
| `391` | `(x) kph   :95 -> real, speed` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, speed` | `x / 3.6` | `(6) kph` | `1.666666667 [speed]` (`real`) |
| `392` | `(x) mph   :95 -> real, speed` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, speed` | `x * 0.44704` | `(6) mph` | `2.68224 [speed]` (`real`) |
| `393` | `(x) knots :95 -> real, speed` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, speed` | `x * (1852 / 3600)` | `(6) knots` | `3.086666667 [speed]` (`real`) |
| `394` | `(x) mps2  :95 -> real, acceleration` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, acceleration` | `x` | `(6) mps2` | `6 [acceleration]` (`real`) |
| `395` | `(x) g_acc :95 -> real, acceleration` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, acceleration` | `x * 9.80665` | `(6) g_acc` | `58.8399 [acceleration]` (`real`) |
| `396` | `(x) cmps2 :95 -> real, acceleration` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, acceleration` | `x * 0.01` | `(6) cmps2` | `0.06 [acceleration]` (`real`) |
| `397` | `(x) fps2  :95 -> real, acceleration` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, acceleration` | `x * 0.3048` | `(6) fps2` | `1.8288 [acceleration]` (`real`) |
| `398` | `(x) Jpkg  :95 -> real, specific_energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, specific_energy` | `x` | `(6) Jpkg` | `6 [specific_energy]` (`real`) |
| `399` | `mps_to_kph(v)` | `FUNC` | `v:real, none` | `-> real, none` | `v * 3.6` | `mps_to_kph(6)` | `21.6` (`real`) |
| `400` | `mps_to_mph(v)` | `FUNC` | `v:real, none` | `-> real, none` | `v / 0.44704` | `mps_to_mph(6)` | `13.42161775` (`real`) |
| `401` | `(x) N     :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x` | `(6) N` | `6 [force]` (`real`) |
| `402` | `(x) kN    :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x * 1000` | `(6) kN` | `6000 [force]` (`real`) |
| `403` | `(x) MN    :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x * 1000000` | `(6) MN` | `6000000 [force]` (`real`) |
| `404` | `(x) mN    :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x * 0.001` | `(6) mN` | `0.006 [force]` (`real`) |
| `405` | `(x) dyn   :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x * 0.00001` | `(6) dyn` | `6e-05 [force]` (`real`) |
| `406` | `(x) lbf   :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x * 4.4482216152605` | `(6) lbf` | `26.68932969 [force]` (`real`) |
| `407` | `(x) kgf   :95 -> real, force` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, force` | `x * 9.80665` | `(6) kgf` | `58.8399 [force]` (`real`) |
| `408` | `(x) Ns    :95 -> real, momentum` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, momentum` | `x` | `(6) Ns` | `6 [momentum]` (`real`) |
| `409` | `(x) kgmps :95 -> real, momentum` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, momentum` | `x` | `(6) kgmps` | `6 [momentum]` (`real`) |
| `410` | `(x) Pa   :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x` | `(6) Pa` | `6 [pressure]` (`real`) |
| `411` | `(x) kPa  :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x * 1000` | `(6) kPa` | `6000 [pressure]` (`real`) |
| `412` | `(x) MPa  :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x * 1000000` | `(6) MPa` | `6000000 [pressure]` (`real`) |
| `413` | `(x) bar  :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x * 100000` | `(6) bar` | `600000 [pressure]` (`real`) |
| `414` | `(x) atm  :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x * 101325` | `(6) atm` | `607950 [pressure]` (`real`) |
| `415` | `(x) psi  :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x * 6894.757293168` | `(6) psi` | `41368.54376 [pressure]` (`real`) |
| `416` | `(x) torr :95 -> real, pressure` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, pressure` | `x * (101325 / 760)` | `(6) torr` | `799.9342105 [pressure]` (`real`) |
| `417` | `(x) J    :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x` | `(6) J` | `6 [energy]` (`real`) |
| `418` | `(x) kJ   :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 1000` | `(6) kJ` | `6000 [energy]` (`real`) |
| `419` | `(x) MJ   :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 1000000` | `(6) MJ` | `6000000 [energy]` (`real`) |
| `420` | `(x) cal  :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 4.184` | `(6) cal` | `25.104 [energy]` (`real`) |
| `421` | `(x) kcal :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 4184` | `(6) kcal` | `25104 [energy]` (`real`) |
| `422` | `(x) Wh   :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 3600` | `(6) Wh` | `21600 [energy]` (`real`) |
| `423` | `(x) kWh  :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 3600000` | `(6) kWh` | `21600000 [energy]` (`real`) |
| `424` | `(x) eV   :95 -> real, energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, energy` | `x * 1.602176634 * (10 ^ -19)` | `(6) eV` | `9.613059804e-19 [energy]` (`real`) |
| `425` | `(x) W    :95 -> real, power` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, power` | `x` | `(6) W` | `6 [power]` (`real`) |
| `426` | `(x) kW   :95 -> real, power` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, power` | `x * 1000` | `(6) kW` | `6000 [power]` (`real`) |
| `427` | `(x) MW   :95 -> real, power` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, power` | `x * 1000000` | `(6) MW` | `6000000 [power]` (`real`) |
| `428` | `(x) hp   :95 -> real, power` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, power` | `x * 745.699872` | `(6) hp` | `4474.199232 [power]` (`real`) |
| `429` | `(x) A     :95 -> real, current` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, current` | `x` | `(6) A` | `6 [current]` (`real`) |
| `430` | `(x) mA    :95 -> real, current` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, current` | `x * 0.001` | `(6) mA` | `0.006 [current]` (`real`) |
| `431` | `(x) uA    :95 -> real, current` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, current` | `x * (10 ^ -6)` | `(6) uA` | `6e-06 [current]` (`real`) |
| `432` | `(x) kA    :95 -> real, current` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, current` | `x * 1000` | `(6) kA` | `6000 [current]` (`real`) |
| `433` | `(x) C     :95 -> real, charge` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, charge` | `x` | `(6) C` | `6 [charge]` (`real`) |
| `434` | `(x) mC    :95 -> real, charge` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, charge` | `x * 0.001` | `(6) mC` | `0.006 [charge]` (`real`) |
| `435` | `(x) uC    :95 -> real, charge` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, charge` | `x * (10 ^ -6)` | `(6) uC` | `6e-06 [charge]` (`real`) |
| `436` | `(x) Ah    :95 -> real, charge` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, charge` | `x * 3600` | `(6) Ah` | `21600 [charge]` (`real`) |
| `437` | `(x) mAh   :95 -> real, charge` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, charge` | `x * 3.6` | `(6) mAh` | `21.6 [charge]` (`real`) |
| `438` | `(x) V     :95 -> real, voltage` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, voltage` | `x` | `(6) V` | `6 [voltage]` (`real`) |
| `439` | `(x) mV    :95 -> real, voltage` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, voltage` | `x * 0.001` | `(6) mV` | `0.006 [voltage]` (`real`) |
| `440` | `(x) kV    :95 -> real, voltage` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, voltage` | `x * 1000` | `(6) kV` | `6000 [voltage]` (`real`) |
| `441` | `(x) ohm   :95 -> real, resistance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, resistance` | `x` | `(6) ohm` | `6 [resistance]` (`real`) |
| `442` | `(x) kohm  :95 -> real, resistance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, resistance` | `x * 1000` | `(6) kohm` | `6000 [resistance]` (`real`) |
| `443` | `(x) Mohm  :95 -> real, resistance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, resistance` | `x * 1000000` | `(6) Mohm` | `6000000 [resistance]` (`real`) |
| `444` | `(x) F_cap :95 -> real, capacitance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, capacitance` | `x` | `(6) F_cap` | `6 [capacitance]` (`real`) |
| `445` | `(x) mF    :95 -> real, capacitance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, capacitance` | `x * 0.001` | `(6) mF` | `0.006 [capacitance]` (`real`) |
| `446` | `(x) uF    :95 -> real, capacitance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, capacitance` | `x * (10 ^ -6)` | `(6) uF` | `6e-06 [capacitance]` (`real`) |
| `447` | `(x) nF    :95 -> real, capacitance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, capacitance` | `x * (10 ^ -9)` | `(6) nF` | `6e-09 [capacitance]` (`real`) |
| `448` | `(x) pF    :95 -> real, capacitance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, capacitance` | `x * (10 ^ -12)` | `(6) pF` | `6e-12 [capacitance]` (`real`) |
| `449` | `(x) K      :95 -> real, temperature` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, temperature` | `x` | `(6) K` | `6 [temperature]` (`real`) |
| `450` | `(x) kelvin :95 -> real, temperature` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, temperature` | `x` | `(6) kelvin` | `6 [temperature]` (`real`) |
| `451` | `(x) mK     :95 -> real, temperature` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, temperature` | `x * 0.001` | `(6) mK` | `0.006 [temperature]` (`real`) |
| `452` | `(x) degC   :95 -> real, temperature` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, temperature` | `x + 273.15` | `(6) degC` | `279.15 [temperature]` (`real`) |
| `453` | `(x) degF   :95 -> real, temperature` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, temperature` | `(x - 32) / 1.8 + 273.15` | `(6) degF` | `258.7055556 [temperature]` (`real`) |
| `454` | `(x) JpK    :95 -> real, heat_capacity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, heat_capacity` | `x` | `(6) JpK` | `6 [heat_capacity]` (`real`) |
| `455` | `(x) JpkgK  :95 -> real, specific_heat` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, specific_heat` | `x` | `(6) JpkgK` | `6 [specific_heat]` (`real`) |
| `456` | `c_to_k(c)` | `FUNC` | `c:real, none` | `-> real, none` | `c + 273.15` | `c_to_k(6)` | `279.15` (`real`) |
| `457` | `k_to_c(k)` | `FUNC` | `k:real, none` | `-> real, none` | `k - 273.15` | `k_to_c(6)` | `-267.15` (`real`) |
| `458` | `c_to_f(c)` | `FUNC` | `c:real, none` | `-> real, none` | `c * 1.8 + 32` | `c_to_f(6)` | `42.8` (`real`) |
| `459` | `f_to_c(f)` | `FUNC` | `f:real, none` | `-> real, none` | `(f - 32) / 1.8` | `f_to_c(6)` | `-14.44444444` (`real`) |
| `460` | `f_to_k(f)` | `FUNC` | `f:real, none` | `-> real, none` | `(f - 32) / 1.8 + 273.15` | `f_to_k(6)` | `258.7055556` (`real`) |
| `461` | `k_to_f(k)` | `FUNC` | `k:real, none` | `-> real, none` | `(k - 273.15) * 1.8 + 32` | `k_to_f(6)` | `-448.87` (`real`) |
| `462` | `(x) mol     :95 -> real, substance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, substance` | `x` | `(6) mol` | `6 [substance]` (`real`) |
| `463` | `(x) mmol    :95 -> real, substance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, substance` | `x * 0.001` | `(6) mmol` | `0.006 [substance]` (`real`) |
| `464` | `(x) umol    :95 -> real, substance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, substance` | `x * 1e-6` | `(6) umol` | `6e-06 [substance]` (`real`) |
| `465` | `(x) nmol    :95 -> real, substance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, substance` | `x * 1e-9` | `(6) nmol` | `6e-09 [substance]` (`real`) |
| `466` | `(x) kmol    :95 -> real, substance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, substance` | `x * 1000` | `(6) kmol` | `6000 [substance]` (`real`) |
| `467` | `(x) kgpmol  :95 -> real, molar_mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molar_mass` | `x` | `(6) kgpmol` | `6 [molar_mass]` (`real`) |
| `468` | `(x) gpmol   :95 -> real, molar_mass` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molar_mass` | `x * 0.001` | `(6) gpmol` | `0.006 [molar_mass]` (`real`) |
| `469` | `(x) molm3   :95 -> real, molarity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molarity` | `x` | `(6) molm3` | `6 [molarity]` (`real`) |
| `470` | `(x) M_conc  :95 -> real, molarity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molarity` | `x * 1000` | `(6) M_conc` | `6000 [molarity]` (`real`) |
| `471` | `(x) mM_conc :95 -> real, molarity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molarity` | `x` | `(6) mM_conc` | `6 [molarity]` (`real`) |
| `472` | `(x) Jpmol   :95 -> real, molar_energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molar_energy` | `x` | `(6) Jpmol` | `6 [molar_energy]` (`real`) |
| `473` | `(x) kJpmol  :95 -> real, molar_energy` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molar_energy` | `x * 1000` | `(6) kJpmol` | `6000 [molar_energy]` (`real`) |
| `474` | `(x) JpmolK  :95 -> real, molar_heat_capacity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, molar_heat_capacity` | `x` | `(6) JpmolK` | `6 [molar_heat_capacity]` (`real`) |
| `475` | `(x) kat     :95 -> real, catalytic_activity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, catalytic_activity` | `x` | `(6) kat` | `6 [catalytic_activity]` (`real`) |
| `476` | `(x) mkat    :95 -> real, catalytic_activity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, catalytic_activity` | `x * 0.001` | `(6) mkat` | `0.006 [catalytic_activity]` (`real`) |
| `477` | `(x) ukat    :95 -> real, catalytic_activity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, catalytic_activity` | `x * 1e-6` | `(6) ukat` | `6e-06 [catalytic_activity]` (`real`) |
| `478` | `(x) cd      :95 -> real, luminous_intensity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, luminous_intensity` | `x` | `(6) cd` | `6 [luminous_intensity]` (`real`) |
| `479` | `(x) lm      :95 -> real, luminous_intensity` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, luminous_intensity` | `x` | `(6) lm` | `6 [luminous_intensity]` (`real`) |
| `480` | `(x) lx      :95 -> real, illuminance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, illuminance` | `x` | `(6) lx` | `6 [illuminance]` (`real`) |
| `481` | `(x) klx     :95 -> real, illuminance` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, illuminance` | `x * 1000` | `(6) klx` | `6000 [illuminance]` (`real`) |

### Module `[chemistry]` (`Library/chemistry.math` — 61 Overloads)

Fundamental chemical constants, stoichiometry, molarity, colligative properties, gas laws, acid-base pH/buffers, kinetics, nuclear decay, and electrochemistry.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `482` | `moles(mass_g, molar_mass)` | `FUNC` | `mass_g:real, none`, `molar_mass:real, none` | `-> real, none` | `mass_g / molar_mass` | `moles(6, 3)` | `2` (`real`) |
| `483` | `mass_from_moles(n_mol, molar_mass)` | `FUNC` | `n_mol:real, none`, `molar_mass:real, none` | `-> real, none` | `n_mol * molar_mass` | `mass_from_moles(6, 3)` | `18` (`real`) |
| `484` | `particles_from_moles(n_mol)` | `FUNC` | `n_mol:real, none` | `-> real, none` | `n_mol * N_A` | `particles_from_moles(6)` | `3.613284456e+24` (`real`) |
| `485` | `percent_composition(mass_elem, molar_mass)` | `FUNC` | `mass_elem:real, none`, `molar_mass:real, none` | `-> real, none` | `(mass_elem / molar_mass) * 100` | `percent_composition(6, 3)` | `200` (`real`) |
| `486` | `percent_yield(actual_y, theor_y)` | `FUNC` | `actual_y:real, none`, `theor_y:real, none` | `-> real, none` | `(actual_y / theor_y) * 100` | `percent_yield(6, 3)` | `200` (`real`) |
| `487` | `atom_economy(mw_desired, mw_reactants)` | `FUNC` | `mw_desired:real, none`, `mw_reactants:real, none` | `-> real, none` | `(mw_desired / mw_reactants) * 100` | `atom_economy(6, 3)` | `200` (`real`) |
| `488` | `molarity(n_mol, vol_liters)` | `FUNC` | `n_mol:real, none`, `vol_liters:real, none` | `-> real, none` | `n_mol / vol_liters` | `molarity(6, 3)` | `2` (`real`) |
| `489` | `normality_from_molarity(molarity_m, n_factor)` | `FUNC` | `molarity_m:real, none`, `n_factor:real, none` | `-> real, none` | `molarity_m * n_factor` | `normality_from_molarity(6, 3)` | `18` (`real`) |
| `490` | `dilution_v1(c1, c2, v2)` | `FUNC` | `c1:real, none`, `c2:real, none`, `v2:real, none` | `-> real, none` | `(c2 * v2) / c1` | `dilution_v1(6, 3, 3)` | `1.5` (`real`) |
| `491` | `molality(n_solute, kg_solvent)` | `FUNC` | `n_solute:real, none`, `kg_solvent:real, none` | `-> real, none` | `n_solute / kg_solvent` | `molality(6, 3)` | `2` (`real`) |
| `492` | `mole_fraction(n_component, n_total)` | `FUNC` | `n_component:real, none`, `n_total:real, none` | `-> real, none` | `n_component / n_total` | `mole_fraction(6, 3)` | `2` (`real`) |
| `493` | `ppm_conc(mass_solute_g, mass_soln_g)` | `FUNC` | `mass_solute_g:real, none`, `mass_soln_g:real, none` | `-> real, none` | `(mass_solute_g / mass_soln_g) * (10 ^ 6)` | `ppm_conc(6, 3)` | `2000000` (`real`) |
| `494` | `raoult_p_soln(x_solvent, p0_solvent)` | `FUNC` | `x_solvent:real, none`, `p0_solvent:real, none` | `-> real, none` | `x_solvent * p0_solvent` | `raoult_p_soln(6, 3)` | `18` (`real`) |
| `495` | `bp_elevation(i_vant, kb, molality_m)` | `FUNC` | `i_vant:real, none`, `kb:real, none`, `molality_m:real, none` | `-> real, none` | `i_vant * kb * molality_m` | `bp_elevation(6, 3, 3)` | `54` (`real`) |
| `496` | `fp_depression(i_vant, kf, molality_m)` | `FUNC` | `i_vant:real, none`, `kf:real, none`, `molality_m:real, none` | `-> real, none` | `i_vant * kf * molality_m` | `fp_depression(6, 3, 3)` | `54` (`real`) |
| `497` | `osmotic_pressure(molarity_m, temp_k)` | `FUNC` | `molarity_m:real, none`, `temp_k:real, none` | `-> real, none` | `molarity_m * R_atm * temp_k` | `osmotic_pressure(6, 3)` | `1.477032588` (`real`) |
| `498` | `vant_hoff_osmotic(i_vant, molarity_m, temp_k)` | `FUNC` | `i_vant:real, none`, `molarity_m:real, none`, `temp_k:real, none` | `-> real, none` | `i_vant * molarity_m * R_atm * temp_k` | `vant_hoff_osmotic(6, 3, 3)` | `4.431097764` (`real`) |
| `499` | `boyle_p2(p1, v1, v2)` | `FUNC` | `p1:real, none`, `v1:real, none`, `v2:real, none` | `-> real, none` | `(p1 * v1) / v2` | `boyle_p2(6, 3, 3)` | `6` (`real`) |
| `500` | `charles_v2(v1, t1_k, t2_k)` | `FUNC` | `v1:real, none`, `t1_k:real, none`, `t2_k:real, none` | `-> real, none` | `(v1 * t2_k) / t1_k` | `charles_v2(6, 3, 3)` | `6` (`real`) |
| `501` | `combined_gas_v2(p1, v1, t1_k, p2, t2_k)` | `FUNC` | `p1:real, none`, `v1:real, none`, `t1_k:real, none`, `p2:real, none`, `t2_k:real, none` | `-> real, none` | `(p1 * v1 * t2_k) / (p2 * t1_k)` | `combined_gas_v2(6, 3, 3, 3, 3)` | `6` (`real`) |
| `502` | `dalton_partial_p(mole_frac, p_total)` | `FUNC` | `mole_frac:real, none`, `p_total:real, none` | `-> real, none` | `mole_frac * p_total` | `dalton_partial_p(6, 3)` | `18` (`real`) |
| `503` | `ideal_gas_p(n_mol, temp_k, vol_m3)` | `FUNC` | `n_mol:real, none`, `temp_k:real, none`, `vol_m3:real, none` | `-> real, none` | `(n_mol * R_gas * temp_k) / vol_m3` | `ideal_gas_p(6, 3, 3)` | `49.88677571` (`real`) |
| `504` | `ideal_gas_v(n_mol, temp_k, press_pa)` | `FUNC` | `n_mol:real, none`, `temp_k:real, none`, `press_pa:real, none` | `-> real, none` | `(n_mol * R_gas * temp_k) / press_pa` | `ideal_gas_v(6, 3, 3)` | `49.88677571` (`real`) |
| `505` | `ideal_gas_n(press_pa, vol_m3, temp_k)` | `FUNC` | `press_pa:real, none`, `vol_m3:real, none`, `temp_k:real, none` | `-> real, none` | `(press_pa * vol_m3) / (R_gas * temp_k)` | `ideal_gas_n(6, 3, 3)` | `0.7216341303` (`real`) |
| `506` | `ideal_gas_t(press_pa, vol_m3, n_mol)` | `FUNC` | `press_pa:real, none`, `vol_m3:real, none`, `n_mol:real, none` | `-> real, none` | `(press_pa * vol_m3) / (n_mol * R_gas)` | `ideal_gas_t(6, 3, 3)` | `0.7216341303` (`real`) |
| `507` | `van_der_waals_p(n_mol, temp_k, vol_l, a_vdw, b_vdw)` | `FUNC` | `n_mol:real, none`, `temp_k:real, none`, `vol_l:real, none`, `a_vdw:real, none`, `b_vdw:real, none` | `-> real, none` | `((n_mol * R_atm * temp_k) / (vol_l - n_mol * b_vdw)) - a_vdw * ((n_mo...` | `van_der_waals_p(6, 3, 3, 3, 3)` | `-12.09846884` (`real`) |
| `508` | `rms_mol_speed(temp_k, molar_mass_kg)` | `FUNC` | `temp_k:real, none`, `molar_mass_kg:real, none` | `-> real, none` | `√(3 * R_gas * temp_k / molar_mass_kg)` | `rms_mol_speed(6, 3)` | `7.063057108` (`real`) |
| `509` | `graham_rate_ratio(molar_mass1, molar_mass2)` | `FUNC` | `molar_mass1:real, none`, `molar_mass2:real, none` | `-> real, none` | `√(molar_mass2 / molar_mass1)` | `graham_rate_ratio(6, 3)` | `0.7071067812` (`real`) |
| `510` | `ph_from_h(h_conc)` | `FUNC` | `h_conc:real, none` | `-> real, none` | `-log10(h_conc)` | `ph_from_h(6)` | `-0.7781512504` (`real`) |
| `511` | `h_from_ph(ph_val)` | `FUNC` | `ph_val:real, none` | `-> real, none` | `10 ^ (-ph_val)` | `h_from_ph(6)` | `1e-06` (`real`) |
| `512` | `poh_from_oh(oh_conc)` | `FUNC` | `oh_conc:real, none` | `-> real, none` | `-log10(oh_conc)` | `poh_from_oh(6)` | `-0.7781512504` (`real`) |
| `513` | `oh_from_poh(poh_val)` | `FUNC` | `poh_val:real, none` | `-> real, none` | `10 ^ (-poh_val)` | `oh_from_poh(6)` | `1e-06` (`real`) |
| `514` | `poh_from_ph(ph_val)` | `FUNC` | `ph_val:real, none` | `-> real, none` | `14 - ph_val` | `poh_from_ph(6)` | `8` (`real`) |
| `515` | `ph_from_poh(poh_val)` | `FUNC` | `poh_val:real, none` | `-> real, none` | `14 - poh_val` | `ph_from_poh(6)` | `8` (`real`) |
| `516` | `ph_from_oh(oh_conc)` | `FUNC` | `oh_conc:real, none` | `-> real, none` | `14 - (-log10(oh_conc))` | `ph_from_oh(6)` | `14.77815125` (`real`) |
| `517` | `pka_from_ka(ka)` | `FUNC` | `ka:real, none` | `-> real, none` | `-log10(ka)` | `pka_from_ka(6)` | `-0.7781512504` (`real`) |
| `518` | `ka_from_pka(pka)` | `FUNC` | `pka:real, none` | `-> real, none` | `10 ^ (-pka)` | `ka_from_pka(6)` | `1e-06` (`real`) |
| `519` | `weak_acid_h(ka, conc_c)` | `FUNC` | `ka:real, none`, `conc_c:real, none` | `-> real, none` | `√(ka * conc_c)` | `weak_acid_h(6, 3)` | `4.242640687` (`real`) |
| `520` | `weak_acid_ph(ka, conc_c)` | `FUNC` | `ka:real, none`, `conc_c:real, none` | `-> real, none` | `-log10(√(ka * conc_c))` | `weak_acid_ph(6, 3)` | `-0.6276362526` (`real`) |
| `521` | `henderson_hasselbalch(pka, conj_base, weak_acid)` | `FUNC` | `pka:real, none`, `conj_base:real, none`, `weak_acid:real, none` | `-> real, none` | `pka + log10(conj_base / weak_acid)` | `henderson_hasselbalch(6, 3, 3)` | `6` (`real`) |
| `522` | `solubility_ksp_ab(ksp)` | `FUNC` | `ksp:real, none` | `-> real, none` | `√ksp` | `solubility_ksp_ab(6)` | `2.449489743` (`real`) |
| `523` | `solubility_ksp_ab2(ksp)` | `FUNC` | `ksp:real, none` | `-> real, none` | `(ksp / 4) ^ (1 / 3)` | `solubility_ksp_ab2(6)` | `1.144714243` (`real`) |
| `524` | `arrhenius_k(pre_exp_a, ea_j_mol, temp_k)` | `FUNC` | `pre_exp_a:real, none`, `ea_j_mol:real, none`, `temp_k:real, none` | `-> real, none` | `pre_exp_a * exp(-ea_j_mol / (R_gas * temp_k))` | `arrhenius_k(6, 3, 3)` | `5.320073474` (`real`) |
| `525` | `arrhenius_k2(k1, ea_j_mol, t1_k, t2_k)` | `FUNC` | `k1:real, none`, `ea_j_mol:real, none`, `t1_k:real, none`, `t2_k:real, none` | `-> real, none` | `k1 * exp((ea_j_mol / R_gas) * ((1 / t1_k) - (1 / t2_k)))` | `arrhenius_k2(6, 3, 3, 3)` | `6` (`real`) |
| `526` | `rate_const_1st(t_sec, c0, ct)` | `FUNC` | `t_sec:real, none`, `c0:real, none`, `ct:real, none` | `-> real, none` | `(1 / t_sec) * ln(c0 / ct)` | `rate_const_1st(6, 3, 3)` | `0` (`real`) |
| `527` | `half_life_1st(rate_k)` | `FUNC` | `rate_k:real, none` | `-> real, none` | `ln(2) / rate_k` | `half_life_1st(6)` | `0.1155245301` (`real`) |
| `528` | `zero_order_half_life(c0, rate_k)` | `FUNC` | `c0:real, none`, `rate_k:real, none` | `-> real, none` | `c0 / (2 * rate_k)` | `zero_order_half_life(6, 3)` | `1` (`real`) |
| `529` | `second_order_half_life(c0, rate_k)` | `FUNC` | `c0:real, none`, `rate_k:real, none` | `-> real, none` | `1 / (rate_k * c0)` | `second_order_half_life(6, 3)` | `0.05555555556` (`real`) |
| `530` | `decay_rem(n0, time_t, half_life)` | `FUNC` | `n0:real, none`, `time_t:real, none`, `half_life:real, none` | `-> real, none` | `n0 * (0.5 ^ (time_t / half_life))` | `decay_rem(6, 3, 3)` | `3` (`real`) |
| `531` | `radiocarbon_age(fraction_rem)` | `FUNC` | `fraction_rem:real, none` | `-> real, none` | `-5730 * (ln(fraction_rem) / ln(2))` | `radiocarbon_age(0.5)` | `5730` (`real`) |
| `532` | `enthalpy_rxn(sum_h_prod, sum_h_react)` | `FUNC` | `sum_h_prod:real, none`, `sum_h_react:real, none` | `-> real, none` | `sum_h_prod - sum_h_react` | `enthalpy_rxn(6, 3)` | `3` (`real`) |
| `533` | `kp_from_kc(kc, temp_k, delta_n_gas)` | `FUNC` | `kc:real, none`, `temp_k:real, none`, `delta_n_gas:real, none` | `-> real, none` | `kc * ((R_atm * temp_k) ^ delta_n_gas)` | `kp_from_kc(6, 3, 3)` | `0.08950921146` (`real`) |
| `534` | `gibbs_free_energy(dh_j, temp_k, ds_j_k)` | `FUNC` | `dh_j:real, none`, `temp_k:real, none`, `ds_j_k:real, none` | `-> real, none` | `dh_j - temp_k * ds_j_k` | `gibbs_free_energy(6, 3, 3)` | `-3` (`real`) |
| `535` | `gibbs_from_keq(temp_k, keq)` | `FUNC` | `temp_k:real, none`, `keq:real, none` | `-> real, none` | `-R_gas * temp_k * ln(keq)` | `gibbs_from_keq(6, 3)` | `-54.80622483` (`real`) |
| `536` | `cell_emf(e_cathode, e_anode)` | `FUNC` | `e_cathode:real, none`, `e_anode:real, none` | `-> real, none` | `e_cathode - e_anode` | `cell_emf(6, 3)` | `3` (`real`) |
| `537` | `gibbs_electro(n_elec, e_cell_v)` | `FUNC` | `n_elec:real, none`, `e_cell_v:real, none` | `-> real, none` | `-n_elec * F_faraday * e_cell_v` | `gibbs_electro(6, 3)` | `-1736735.978` (`real`) |
| `538` | `faraday_mass(current_i, time_s, molar_mass, n_elec)` | `FUNC` | `current_i:real, none`, `time_s:real, none`, `molar_mass:real, none`, `n_elec:real, none` | `-> real, none` | `(current_i * time_s * molar_mass) / (n_elec * F_faraday)` | `faraday_mass(6, 3, 3, 3)` | `0.0001865568538` (`real`) |
| `539` | `molar_conductivity(kappa_s_cm, molarity_m)` | `FUNC` | `kappa_s_cm:real, none`, `molarity_m:real, none` | `-> real, none` | `(kappa_s_cm * 1000) / molarity_m` | `molar_conductivity(6, 3)` | `2000` (`real`) |
| `540` | `nernst_cell(e0_volts, n_elec, reaction_q)` | `FUNC` | `e0_volts:real, none`, `n_elec:real, none`, `reaction_q:real, none` | `-> real, none` | `e0_volts - (R_gas * 298.15 / (n_elec * F_faraday)) * ln(reaction_q)` | `nernst_cell(6, 3, 3)` | `5.990591272` (`real`) |
| `541` | `crystal_density(z_atoms, molar_mass, a_edge_cm)` | `FUNC` | `z_atoms:real, none`, `molar_mass:real, none`, `a_edge_cm:real, none` | `-> real, none` | `(z_atoms * molar_mass) / ((a_edge_cm ^ 3) * N_A)` | `crystal_density(6, 3, 3)` | `1.107026045e-24` (`real`) |
| `542` | `bragg_d_spacing(n_order, wavelength, theta_deg)` | `FUNC` | `n_order:real, none`, `wavelength:real, none`, `theta_deg:real, none` | `-> real, none` | `(n_order * wavelength) / (2 * sin(deg2rad(theta_deg)))` | `bragg_d_spacing(6, 3, 3)` | `171.9659035` (`real`) |

### Module `[conversions]` (`Library/conversions.math` — 239 Overloads)

Universal infix `in` conversion operator, postfix `to_<unit>` extractors, and pairwise unit conversion functions.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `543` | `(val:real, T) in (unit:real, T) :<42 -> real, T` | `INFIX_LEFT` (prec `42`) | `val:real, T`, `unit:real, T` | `-> real, T` | `@div(val, unit)` | `(2.5 km) in (1 m)` | `2500` (`real`) |
| `544` | `(x:real, distance) to_km   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x / 1000` | `((100 m)) to_km` | `0.1` (`real`) |
| `545` | `(x:real, distance) to_m    :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x` | `((100 m)) to_m` | `100` (`real`) |
| `546` | `(x:real, distance) to_cm   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x * 100` | `((100 m)) to_cm` | `10000` (`real`) |
| `547` | `(x:real, distance) to_mm   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x * 1000` | `((100 m)) to_mm` | `100000` (`real`) |
| `548` | `(x:real, distance) to_um   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x * (10 ^ 6)` | `((100 m)) to_um` | `100000000` (`real`) |
| `549` | `(x:real, distance) to_nm   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x * (10 ^ 9)` | `((100 m)) to_nm` | `1e+11` (`real`) |
| `550` | `(x:real, distance) to_mi   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x / 1609.344` | `((100 m)) to_mi` | `0.06213711922` (`real`) |
| `551` | `(x:real, distance) to_nmi  :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x / 1852` | `((100 m)) to_nmi` | `0.05399568035` (`real`) |
| `552` | `(x:real, distance) to_yd   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x / 0.9144` | `((100 m)) to_yd` | `109.3613298` (`real`) |
| `553` | `(x:real, distance) to_ft   :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x / 0.3048` | `((100 m)) to_ft` | `328.0839895` (`real`) |
| `554` | `(x:real, distance) to_inch :90 -> real, none` | `POSTFIX` (prec `90`) | `x:real, distance` | `-> real, none (locked)` | `x / 0.0254` | `((100 m)) to_inch` | `3937.007874` (`real`) |
| `555` | `km_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `km_to_m(6)` | `6000` (`real`) |
| `556` | `m_to_cm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 100` | `m_to_cm(6)` | `600` (`real`) |
| `557` | `cm_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.01` | `cm_to_m(6)` | `0.06` (`real`) |
| `558` | `m_to_mm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `m_to_mm(6)` | `6000` (`real`) |
| `559` | `mm_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `mm_to_m(6)` | `0.006` (`real`) |
| `560` | `m_to_um(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ 6)` | `m_to_um(6)` | `6000000` (`real`) |
| `561` | `um_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ -6)` | `um_to_m(6)` | `6e-06` (`real`) |
| `562` | `m_to_nm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ 9)` | `m_to_nm(6)` | `6000000000` (`real`) |
| `563` | `nm_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ -9)` | `nm_to_m(6)` | `6e-09` (`real`) |
| `564` | `km_to_cm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 100000` | `km_to_cm(6)` | `600000` (`real`) |
| `565` | `cm_to_km(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 100000` | `cm_to_km(6)` | `6e-05` (`real`) |
| `566` | `km_to_mm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ 6)` | `km_to_mm(6)` | `6000000` (`real`) |
| `567` | `mm_to_km(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ -6)` | `mm_to_km(6)` | `6e-06` (`real`) |
| `568` | `km_to_nm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ 12)` | `km_to_nm(6)` | `6e+12` (`real`) |
| `569` | `nm_to_km(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ -12)` | `nm_to_km(6)` | `6e-12` (`real`) |
| `570` | `cm_to_mm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 10` | `cm_to_mm(6)` | `60` (`real`) |
| `571` | `mm_to_cm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.1` | `mm_to_cm(6)` | `0.6` (`real`) |
| `572` | `mm_to_um(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `mm_to_um(6)` | `6000` (`real`) |
| `573` | `um_to_mm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `um_to_mm(6)` | `0.006` (`real`) |
| `574` | `um_to_nm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `um_to_nm(6)` | `6000` (`real`) |
| `575` | `nm_to_um(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `nm_to_um(6)` | `0.006` (`real`) |
| `576` | `mi_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1609.344` | `mi_to_m(6)` | `9656.064` (`real`) |
| `577` | `mi_to_km(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1.609344` | `mi_to_km(6)` | `9.656064` (`real`) |
| `578` | `km_to_mi(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1.609344` | `km_to_mi(6)` | `3.728227153` (`real`) |
| `579` | `yd_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.9144` | `yd_to_m(6)` | `5.4864` (`real`) |
| `580` | `m_to_yd(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 0.9144` | `m_to_yd(6)` | `6.56167979` (`real`) |
| `581` | `ft_to_in(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 12` | `ft_to_in(6)` | `72` (`real`) |
| `582` | `in_to_ft(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 12` | `in_to_ft(6)` | `0.5` (`real`) |
| `583` | `m_to_in(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 0.0254` | `m_to_in(6)` | `236.2204724` (`real`) |
| `584` | `in_to_m(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.0254` | `in_to_m(6)` | `0.1524` (`real`) |
| `585` | `(a:real, angle) to_rad    :90 -> real, none` | `POSTFIX` (prec `90`) | `a:real, angle` | `-> real, none (locked)` | `a` | `((60 deg)) to_rad` | `1.047197551` (`real`) |
| `586` | `(a:real, angle) to_deg    :90 -> real, none` | `POSTFIX` (prec `90`) | `a:real, angle` | `-> real, none (locked)` | `a * 180 / π` | `((60 deg)) to_deg` | `60` (`real`) |
| `587` | `(a:real, angle) to_arcmin :90 -> real, none` | `POSTFIX` (prec `90`) | `a:real, angle` | `-> real, none (locked)` | `a * (180 * 60) / π` | `((60 deg)) to_arcmin` | `3600` (`real`) |
| `588` | `(a:real, angle) to_arcsec :90 -> real, none` | `POSTFIX` (prec `90`) | `a:real, angle` | `-> real, none (locked)` | `a * (180 * 3600) / π` | `((60 deg)) to_arcsec` | `216000` (`real`) |
| `589` | `deg_to_rad(d)` | `FUNC` | `d:real, none` | `-> real, none` | `d * π / 180` | `deg_to_rad(6)` | `0.1047197551` (`real`) |
| `590` | `rad_to_deg(r)` | `FUNC` | `r:real, none` | `-> real, none` | `r * 180 / π` | `rad_to_deg(6)` | `343.7746771` (`real`) |
| `591` | `deg_to_arcmin(d)` | `FUNC` | `d:real, none` | `-> real, none` | `d * 60` | `deg_to_arcmin(6)` | `360` (`real`) |
| `592` | `arcmin_to_deg(m)` | `FUNC` | `m:real, none` | `-> real, none` | `m / 60` | `arcmin_to_deg(6)` | `0.1` (`real`) |
| `593` | `deg_to_arcsec(d)` | `FUNC` | `d:real, none` | `-> real, none` | `d * 3600` | `deg_to_arcsec(6)` | `21600` (`real`) |
| `594` | `arcsec_to_deg(s)` | `FUNC` | `s:real, none` | `-> real, none` | `s / 3600` | `arcsec_to_deg(6)` | `0.001666666667` (`real`) |
| `595` | `rad_to_arcsec(r)` | `FUNC` | `r:real, none` | `-> real, none` | `r * (180 * 3600) / π` | `rad_to_arcsec(6)` | `1237588.837` (`real`) |
| `596` | `arcsec_to_rad(s)` | `FUNC` | `s:real, none` | `-> real, none` | `s * π / (180 * 3600)` | `arcsec_to_rad(6)` | `2.908882087e-05` (`real`) |
| `597` | `(m:real, mass) to_kg   :90 -> real, none` | `POSTFIX` (prec `90`) | `m:real, mass` | `-> real, none (locked)` | `m` | `((10 kg)) to_kg` | `10` (`real`) |
| `598` | `(m:real, mass) to_gram :90 -> real, none` | `POSTFIX` (prec `90`) | `m:real, mass` | `-> real, none (locked)` | `m * 1000` | `((10 kg)) to_gram` | `10000` (`real`) |
| `599` | `(m:real, mass) to_mg   :90 -> real, none` | `POSTFIX` (prec `90`) | `m:real, mass` | `-> real, none (locked)` | `m * (10 ^ 6)` | `((10 kg)) to_mg` | `10000000` (`real`) |
| `600` | `(m:real, mass) to_ton  :90 -> real, none` | `POSTFIX` (prec `90`) | `m:real, mass` | `-> real, none (locked)` | `m / 1000` | `((10 kg)) to_ton` | `0.01` (`real`) |
| `601` | `(m:real, mass) to_lb   :90 -> real, none` | `POSTFIX` (prec `90`) | `m:real, mass` | `-> real, none (locked)` | `m / 0.45359237` | `((10 kg)) to_lb` | `22.04622622` (`real`) |
| `602` | `(m:real, mass) to_oz   :90 -> real, none` | `POSTFIX` (prec `90`) | `m:real, mass` | `-> real, none (locked)` | `m / 0.028349523125` | `((10 kg)) to_oz` | `352.7396195` (`real`) |
| `603` | `kg_to_g(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `kg_to_g(6)` | `6000` (`real`) |
| `604` | `g_to_kg(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `g_to_kg(6)` | `0.006` (`real`) |
| `605` | `kg_to_mg(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ 6)` | `kg_to_mg(6)` | `6000000` (`real`) |
| `606` | `mg_to_kg(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ -6)` | `mg_to_kg(6)` | `6e-06` (`real`) |
| `607` | `g_to_mg(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `g_to_mg(6)` | `6000` (`real`) |
| `608` | `mg_to_g(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `mg_to_g(6)` | `0.006` (`real`) |
| `609` | `ton_to_kg(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `ton_to_kg(6)` | `6000` (`real`) |
| `610` | `kg_to_ton(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `kg_to_ton(6)` | `0.006` (`real`) |
| `611` | `lb_to_oz(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 16` | `lb_to_oz(6)` | `96` (`real`) |
| `612` | `oz_to_lb(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 16` | `oz_to_lb(6)` | `0.375` (`real`) |
| `613` | `g_to_oz(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 28.349523125` | `g_to_oz(6)` | `0.2116437717` (`real`) |
| `614` | `oz_to_g(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 28.349523125` | `oz_to_g(6)` | `170.0971387` (`real`) |
| `615` | `(t:real, time) to_sec :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t` | `((5 sec)) to_sec` | `5` (`real`) |
| `616` | `(t:real, time) to_ms  :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t * 1000` | `((5 sec)) to_ms` | `5000` (`real`) |
| `617` | `(t:real, time) to_us  :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t * (10 ^ 6)` | `((5 sec)) to_us` | `5000000` (`real`) |
| `618` | `(t:real, time) to_ns  :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t * (10 ^ 9)` | `((5 sec)) to_ns` | `5000000000` (`real`) |
| `619` | `(t:real, time) to_min :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t / 60` | `((5 sec)) to_min` | `0.08333333333` (`real`) |
| `620` | `(t:real, time) to_hr  :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t / 3600` | `((5 sec)) to_hr` | `0.001388888889` (`real`) |
| `621` | `(t:real, time) to_day :90 -> real, none` | `POSTFIX` (prec `90`) | `t:real, time` | `-> real, none (locked)` | `t / 86400` | `((5 sec)) to_day` | `5.787037037e-05` (`real`) |
| `622` | `hr_to_min(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 60` | `hr_to_min(6)` | `360` (`real`) |
| `623` | `min_to_hr(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 60` | `min_to_hr(6)` | `0.1` (`real`) |
| `624` | `hr_to_sec(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 3600` | `hr_to_sec(6)` | `21600` (`real`) |
| `625` | `sec_to_hr(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 3600` | `sec_to_hr(6)` | `0.001666666667` (`real`) |
| `626` | `min_to_sec(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 60` | `min_to_sec(6)` | `360` (`real`) |
| `627` | `sec_to_min(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 60` | `sec_to_min(6)` | `0.1` (`real`) |
| `628` | `sec_to_ms(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `sec_to_ms(6)` | `6000` (`real`) |
| `629` | `ms_to_sec(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `ms_to_sec(6)` | `0.006` (`real`) |
| `630` | `ms_to_us(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `ms_to_us(6)` | `6000` (`real`) |
| `631` | `us_to_ms(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `us_to_ms(6)` | `0.006` (`real`) |
| `632` | `us_to_ns(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `us_to_ns(6)` | `6000` (`real`) |
| `633` | `ns_to_us(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `ns_to_us(6)` | `0.006` (`real`) |
| `634` | `sec_to_ns(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ 9)` | `sec_to_ns(6)` | `6000000000` (`real`) |
| `635` | `ns_to_sec(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (10 ^ -9)` | `ns_to_sec(6)` | `6e-09` (`real`) |
| `636` | `day_to_hr(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 24` | `day_to_hr(6)` | `144` (`real`) |
| `637` | `hr_to_day(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 24` | `hr_to_day(6)` | `0.25` (`real`) |
| `638` | `day_to_sec(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 86400` | `day_to_sec(6)` | `518400` (`real`) |
| `639` | `sec_to_day(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 86400` | `sec_to_day(6)` | `6.944444444e-05` (`real`) |
| `640` | `(v:real, speed) to_mps   :90 -> real, none` | `POSTFIX` (prec `90`) | `v:real, speed` | `-> real, none (locked)` | `v` | `((20 mps)) to_mps` | `20` (`real`) |
| `641` | `(v:real, speed) to_kph   :90 -> real, none` | `POSTFIX` (prec `90`) | `v:real, speed` | `-> real, none (locked)` | `v * 3.6` | `((20 mps)) to_kph` | `72` (`real`) |
| `642` | `(v:real, speed) to_mph   :90 -> real, none` | `POSTFIX` (prec `90`) | `v:real, speed` | `-> real, none (locked)` | `v / 0.44704` | `((20 mps)) to_mph` | `44.73872584` (`real`) |
| `643` | `(v:real, speed) to_knots :90 -> real, none` | `POSTFIX` (prec `90`) | `v:real, speed` | `-> real, none (locked)` | `v * (3600 / 1852)` | `((20 mps)) to_knots` | `38.87688985` (`real`) |
| `644` | `kph_to_mps(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 3.6` | `kph_to_mps(6)` | `1.666666667` (`real`) |
| `645` | `mph_to_mps(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.44704` | `mph_to_mps(6)` | `2.68224` (`real`) |
| `646` | `kph_to_mph(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1.609344` | `kph_to_mph(6)` | `3.728227153` (`real`) |
| `647` | `mph_to_kph(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1.609344` | `mph_to_kph(6)` | `9.656064` (`real`) |
| `648` | `knots_to_mps(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (1852 / 3600)` | `knots_to_mps(6)` | `3.086666667` (`real`) |
| `649` | `mps_to_knots(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (3600 / 1852)` | `mps_to_knots(6)` | `11.66306695` (`real`) |
| `650` | `knots_to_kph(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1.852` | `knots_to_kph(6)` | `11.112` (`real`) |
| `651` | `kph_to_knots(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1.852` | `kph_to_knots(6)` | `3.239740821` (`real`) |
| `652` | `(p:real, pressure) to_Pa   :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p` | `((101325 Pa)) to_Pa` | `101325` (`real`) |
| `653` | `(p:real, pressure) to_kPa  :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p / 1000` | `((101325 Pa)) to_kPa` | `101.325` (`real`) |
| `654` | `(p:real, pressure) to_MPa  :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p / 1000000` | `((101325 Pa)) to_MPa` | `0.101325` (`real`) |
| `655` | `(p:real, pressure) to_bar  :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p / 100000` | `((101325 Pa)) to_bar` | `1.01325` (`real`) |
| `656` | `(p:real, pressure) to_atm  :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p / 101325` | `((101325 Pa)) to_atm` | `1` (`real`) |
| `657` | `(p:real, pressure) to_psi  :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p / 6894.757293168` | `((101325 Pa)) to_psi` | `14.69594878` (`real`) |
| `658` | `(p:real, pressure) to_torr :90 -> real, none` | `POSTFIX` (prec `90`) | `p:real, pressure` | `-> real, none (locked)` | `p * (760 / 101325)` | `((101325 Pa)) to_torr` | `760` (`real`) |
| `659` | `atm_to_Pa(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 101325` | `atm_to_Pa(6)` | `607950` (`real`) |
| `660` | `Pa_to_atm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 101325` | `Pa_to_atm(6)` | `5.9215396e-05` (`real`) |
| `661` | `bar_to_Pa(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 100000` | `bar_to_Pa(6)` | `600000` (`real`) |
| `662` | `Pa_to_bar(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 100000` | `Pa_to_bar(6)` | `6e-05` (`real`) |
| `663` | `kPa_to_Pa(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `kPa_to_Pa(6)` | `6000` (`real`) |
| `664` | `Pa_to_kPa(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1000` | `Pa_to_kPa(6)` | `0.006` (`real`) |
| `665` | `psi_to_Pa(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 6894.757293168` | `psi_to_Pa(6)` | `41368.54376` (`real`) |
| `666` | `Pa_to_psi(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 6894.757293168` | `Pa_to_psi(6)` | `0.0008702264264` (`real`) |
| `667` | `atm_to_psi(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (101325 / 6894.757293168)` | `atm_to_psi(6)` | `88.17569265` (`real`) |
| `668` | `psi_to_atm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * (6894.757293168 / 101325)` | `psi_to_atm(6)` | `0.4082757835` (`real`) |
| `669` | `atm_to_bar(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1.01325` | `atm_to_bar(6)` | `6.0795` (`real`) |
| `670` | `bar_to_atm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1.01325` | `bar_to_atm(6)` | `5.9215396` (`real`) |
| `671` | `atm_to_torr(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 760` | `atm_to_torr(6)` | `4560` (`real`) |
| `672` | `torr_to_atm(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 760` | `torr_to_atm(6)` | `0.007894736842` (`real`) |
| `673` | `(e_v:real, energy) to_J    :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v` | `((500 J)) to_J` | `500` (`real`) |
| `674` | `(e_v:real, energy) to_kJ   :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / 1000` | `((500 J)) to_kJ` | `0.5` (`real`) |
| `675` | `(e_v:real, energy) to_MJ   :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / 1000000` | `((500 J)) to_MJ` | `0.0005` (`real`) |
| `676` | `(e_v:real, energy) to_cal  :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / 4.184` | `((500 J)) to_cal` | `119.5028681` (`real`) |
| `677` | `(e_v:real, energy) to_kcal :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / 4184` | `((500 J)) to_kcal` | `0.1195028681` (`real`) |
| `678` | `(e_v:real, energy) to_Wh   :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / 3600` | `((500 J)) to_Wh` | `0.1388888889` (`real`) |
| `679` | `(e_v:real, energy) to_kWh  :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / 3600000` | `((500 J)) to_kWh` | `0.0001388888889` (`real`) |
| `680` | `(e_v:real, energy) to_eV   :90 -> real, none` | `POSTFIX` (prec `90`) | `e_v:real, energy` | `-> real, none (locked)` | `e_v / (1.602176634 * (10 ^ -19))` | `((500 J)) to_eV` | `3.120754537e+21` (`real`) |
| `681` | `(p_v:real, power)  to_W    :90 -> real, none` | `POSTFIX` (prec `90`) | `p_v:real, power` | `-> real, none (locked)` | `p_v` | `((250 W)) to_W` | `250` (`real`) |
| `682` | `(p_v:real, power)  to_kW   :90 -> real, none` | `POSTFIX` (prec `90`) | `p_v:real, power` | `-> real, none (locked)` | `p_v / 1000` | `((250 W)) to_kW` | `0.25` (`real`) |
| `683` | `(p_v:real, power)  to_MW   :90 -> real, none` | `POSTFIX` (prec `90`) | `p_v:real, power` | `-> real, none (locked)` | `p_v / 1000000` | `((250 W)) to_MW` | `0.00025` (`real`) |
| `684` | `(p_v:real, power)  to_hp   :90 -> real, none` | `POSTFIX` (prec `90`) | `p_v:real, power` | `-> real, none (locked)` | `p_v / 745.699872` | `((250 W)) to_hp` | `0.3352555222` (`real`) |
| `685` | `kJ_to_J(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `kJ_to_J(6)` | `6000` (`real`) |
| `686` | `J_to_kJ(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1000` | `J_to_kJ(6)` | `0.006` (`real`) |
| `687` | `kcal_to_J(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 4184` | `kcal_to_J(6)` | `25104` (`real`) |
| `688` | `J_to_kcal(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 4184` | `J_to_kcal(6)` | `0.001434034417` (`real`) |
| `689` | `kcal_to_kJ(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 4.184` | `kcal_to_kJ(6)` | `25.104` (`real`) |
| `690` | `kJ_to_kcal(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 4.184` | `kJ_to_kcal(6)` | `1.434034417` (`real`) |
| `691` | `cal_to_J(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 4.184` | `cal_to_J(6)` | `25.104` (`real`) |
| `692` | `J_to_cal(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 4.184` | `J_to_cal(6)` | `1.434034417` (`real`) |
| `693` | `kWh_to_J(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 3600000` | `kWh_to_J(6)` | `21600000` (`real`) |
| `694` | `J_to_kWh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 3600000` | `J_to_kWh(6)` | `1.666666667e-06` (`real`) |
| `695` | `Wh_to_J(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 3600` | `Wh_to_J(6)` | `21600` (`real`) |
| `696` | `J_to_Wh(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 3600` | `J_to_Wh(6)` | `0.001666666667` (`real`) |
| `697` | `kW_to_W(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `kW_to_W(6)` | `6000` (`real`) |
| `698` | `W_to_kW(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1000` | `W_to_kW(6)` | `0.006` (`real`) |
| `699` | `MW_to_W(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000000` | `MW_to_W(6)` | `6000000` (`real`) |
| `700` | `W_to_MW(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1000000` | `W_to_MW(6)` | `6e-06` (`real`) |
| `701` | `hp_to_W(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 745.699872` | `hp_to_W(6)` | `4474.199232` (`real`) |
| `702` | `W_to_hp(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 745.699872` | `W_to_hp(6)` | `0.008046132533` (`real`) |
| `703` | `hp_to_kW(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.745699872` | `hp_to_kW(6)` | `4.474199232` (`real`) |
| `704` | `kW_to_hp(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 0.745699872` | `kW_to_hp(6)` | `8.046132533` (`real`) |
| `705` | `(a_v:real, area) to_m2   :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v` | `((25 m2)) to_m2` | `25` (`real`) |
| `706` | `(a_v:real, area) to_km2  :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v / 1000000` | `((25 m2)) to_km2` | `2.5e-05` (`real`) |
| `707` | `(a_v:real, area) to_cm2  :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v * 10000` | `((25 m2)) to_cm2` | `250000` (`real`) |
| `708` | `(a_v:real, area) to_mm2  :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v * 1000000` | `((25 m2)) to_mm2` | `25000000` (`real`) |
| `709` | `(a_v:real, area) to_ha   :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v / 10000` | `((25 m2)) to_ha` | `0.0025` (`real`) |
| `710` | `(a_v:real, area) to_acre :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v / 4046.8564224` | `((25 m2)) to_acre` | `0.006177634537` (`real`) |
| `711` | `(a_v:real, area) to_ft2  :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v / 0.09290304` | `((25 m2)) to_ft2` | `269.0977604` (`real`) |
| `712` | `(a_v:real, area) to_in2  :90 -> real, none` | `POSTFIX` (prec `90`) | `a_v:real, area` | `-> real, none (locked)` | `a_v / 0.00064516` | `((25 m2)) to_in2` | `38750.0775` (`real`) |
| `713` | `(v_v:real, volume) to_m3  :90 -> real, none` | `POSTFIX` (prec `90`) | `v_v:real, volume` | `-> real, none (locked)` | `v_v` | `((2 m3)) to_m3` | `2` (`real`) |
| `714` | `(v_v:real, volume) to_cm3 :90 -> real, none` | `POSTFIX` (prec `90`) | `v_v:real, volume` | `-> real, none (locked)` | `v_v * 1000000` | `((2 m3)) to_cm3` | `2000000` (`real`) |
| `715` | `(v_v:real, volume) to_L   :90 -> real, none` | `POSTFIX` (prec `90`) | `v_v:real, volume` | `-> real, none (locked)` | `v_v * 1000` | `((2 m3)) to_L` | `2000` (`real`) |
| `716` | `(v_v:real, volume) to_mL  :90 -> real, none` | `POSTFIX` (prec `90`) | `v_v:real, volume` | `-> real, none (locked)` | `v_v * 1000000` | `((2 m3)) to_mL` | `2000000` (`real`) |
| `717` | `(v_v:real, volume) to_gal :90 -> real, none` | `POSTFIX` (prec `90`) | `v_v:real, volume` | `-> real, none (locked)` | `v_v / 0.003785411784` | `((2 m3)) to_gal` | `528.3441047` (`real`) |
| `718` | `(v_v:real, volume) to_ft3 :90 -> real, none` | `POSTFIX` (prec `90`) | `v_v:real, volume` | `-> real, none (locked)` | `v_v / 0.028316846592` | `((2 m3)) to_ft3` | `70.62933344` (`real`) |
| `719` | `(acc_v:real, acceleration) to_mps2  :90 -> real, none` | `POSTFIX` (prec `90`) | `acc_v:real, acceleration` | `-> real, none (locked)` | `acc_v` | `((9.8 mps2)) to_mps2` | `9.8` (`real`) |
| `720` | `(acc_v:real, acceleration) to_g_acc :90 -> real, none` | `POSTFIX` (prec `90`) | `acc_v:real, acceleration` | `-> real, none (locked)` | `acc_v / 9.80665` | `((9.8 mps2)) to_g_acc` | `0.9993218887` (`real`) |
| `721` | `(acc_v:real, acceleration) to_fps2  :90 -> real, none` | `POSTFIX` (prec `90`) | `acc_v:real, acceleration` | `-> real, none (locked)` | `acc_v / 0.3048` | `((9.8 mps2)) to_fps2` | `32.15223097` (`real`) |
| `722` | `(f_v:real, force) to_N   :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v` | `((100 N)) to_N` | `100` (`real`) |
| `723` | `(f_v:real, force) to_kN  :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v / 1000` | `((100 N)) to_kN` | `0.1` (`real`) |
| `724` | `(f_v:real, force) to_MN  :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v / 1000000` | `((100 N)) to_MN` | `0.0001` (`real`) |
| `725` | `(f_v:real, force) to_mN  :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v * 1000` | `((100 N)) to_mN` | `100000` (`real`) |
| `726` | `(f_v:real, force) to_dyn :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v * 100000` | `((100 N)) to_dyn` | `10000000` (`real`) |
| `727` | `(f_v:real, force) to_lbf :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v / 4.4482216152605` | `((100 N)) to_lbf` | `22.48089431` (`real`) |
| `728` | `(f_v:real, force) to_kgf :90 -> real, none` | `POSTFIX` (prec `90`) | `f_v:real, force` | `-> real, none (locked)` | `f_v / 9.80665` | `((100 N)) to_kgf` | `10.19716213` (`real`) |
| `729` | `(fr_v:real, frequency) to_Hz  :90 -> real, none` | `POSTFIX` (prec `90`) | `fr_v:real, frequency` | `-> real, none (locked)` | `fr_v` | `((50 Hz)) to_Hz` | `50` (`real`) |
| `730` | `(fr_v:real, frequency) to_kHz :90 -> real, none` | `POSTFIX` (prec `90`) | `fr_v:real, frequency` | `-> real, none (locked)` | `fr_v / 1000` | `((50 Hz)) to_kHz` | `0.05` (`real`) |
| `731` | `(fr_v:real, frequency) to_MHz :90 -> real, none` | `POSTFIX` (prec `90`) | `fr_v:real, frequency` | `-> real, none (locked)` | `fr_v / 1000000` | `((50 Hz)) to_MHz` | `5e-05` (`real`) |
| `732` | `(fr_v:real, frequency) to_GHz :90 -> real, none` | `POSTFIX` (prec `90`) | `fr_v:real, frequency` | `-> real, none (locked)` | `fr_v / (10 ^ 9)` | `((50 Hz)) to_GHz` | `5e-08` (`real`) |
| `733` | `(fr_v:real, frequency) to_rpm :90 -> real, none` | `POSTFIX` (prec `90`) | `fr_v:real, frequency` | `-> real, none (locked)` | `fr_v * 60` | `((50 Hz)) to_rpm` | `3000` (`real`) |
| `734` | `(p_m:real, momentum) to_Ns    :90 -> real, none` | `POSTFIX` (prec `90`) | `p_m:real, momentum` | `-> real, none (locked)` | `p_m` | `((50 Ns)) to_Ns` | `50` (`real`) |
| `735` | `(p_m:real, momentum) to_kgmps :90 -> real, none` | `POSTFIX` (prec `90`) | `p_m:real, momentum` | `-> real, none (locked)` | `p_m` | `((50 Ns)) to_kgmps` | `50` (`real`) |
| `736` | `(d_v:real, density) to_kgm3 :90 -> real, none` | `POSTFIX` (prec `90`) | `d_v:real, density` | `-> real, none (locked)` | `d_v` | `((1000 kgm3)) to_kgm3` | `1000` (`real`) |
| `737` | `(d_v:real, density) to_gcm3 :90 -> real, none` | `POSTFIX` (prec `90`) | `d_v:real, density` | `-> real, none (locked)` | `d_v / 1000` | `((1000 kgm3)) to_gcm3` | `1` (`real`) |
| `738` | `(i_v:real, current)     to_A    :90 -> real, none` | `POSTFIX` (prec `90`) | `i_v:real, current` | `-> real, none (locked)` | `i_v` | `((3 A)) to_A` | `3` (`real`) |
| `739` | `(i_v:real, current)     to_mA   :90 -> real, none` | `POSTFIX` (prec `90`) | `i_v:real, current` | `-> real, none (locked)` | `i_v * 1000` | `((3 A)) to_mA` | `3000` (`real`) |
| `740` | `(q_v:real, charge)      to_C    :90 -> real, none` | `POSTFIX` (prec `90`) | `q_v:real, charge` | `-> real, none (locked)` | `q_v` | `((12 C)) to_C` | `12` (`real`) |
| `741` | `(q_v:real, charge)      to_mAh  :90 -> real, none` | `POSTFIX` (prec `90`) | `q_v:real, charge` | `-> real, none (locked)` | `q_v / 3.6` | `((12 C)) to_mAh` | `3.333333333` (`real`) |
| `742` | `(u_v:real, voltage)     to_V    :90 -> real, none` | `POSTFIX` (prec `90`) | `u_v:real, voltage` | `-> real, none (locked)` | `u_v` | `((24 V)) to_V` | `24` (`real`) |
| `743` | `(u_v:real, voltage)     to_mV   :90 -> real, none` | `POSTFIX` (prec `90`) | `u_v:real, voltage` | `-> real, none (locked)` | `u_v * 1000` | `((24 V)) to_mV` | `24000` (`real`) |
| `744` | `(u_v:real, voltage)     to_kV   :90 -> real, none` | `POSTFIX` (prec `90`) | `u_v:real, voltage` | `-> real, none (locked)` | `u_v / 1000` | `((24 V)) to_kV` | `0.024` (`real`) |
| `745` | `(r_v:real, resistance)  to_ohm  :90 -> real, none` | `POSTFIX` (prec `90`) | `r_v:real, resistance` | `-> real, none (locked)` | `r_v` | `((8 ohm)) to_ohm` | `8` (`real`) |
| `746` | `(r_v:real, resistance)  to_kohm :90 -> real, none` | `POSTFIX` (prec `90`) | `r_v:real, resistance` | `-> real, none (locked)` | `r_v / 1000` | `((8 ohm)) to_kohm` | `0.008` (`real`) |
| `747` | `N_to_kN(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 1000` | `N_to_kN(6)` | `0.006` (`real`) |
| `748` | `kN_to_N(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `kN_to_N(6)` | `6000` (`real`) |
| `749` | `N_to_lbf(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 4.4482216152605` | `N_to_lbf(6)` | `1.348853659` (`real`) |
| `750` | `lbf_to_N(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 4.4482216152605` | `lbf_to_N(6)` | `26.68932969` (`real`) |
| `751` | `N_to_dyn(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 100000` | `N_to_dyn(6)` | `600000` (`real`) |
| `752` | `dyn_to_N(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 100000` | `dyn_to_N(6)` | `6e-05` (`real`) |
| `753` | `L_to_m3(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `L_to_m3(6)` | `0.006` (`real`) |
| `754` | `m3_to_L(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `m3_to_L(6)` | `6000` (`real`) |
| `755` | `L_to_mL(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 1000` | `L_to_mL(6)` | `6000` (`real`) |
| `756` | `mL_to_L(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 0.001` | `mL_to_L(6)` | `0.006` (`real`) |
| `757` | `gal_to_L(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x * 3.785411784` | `gal_to_L(6)` | `22.7124707` (`real`) |
| `758` | `L_to_gal(x)` | `FUNC` | `x:real, none` | `-> real, none` | `x / 3.785411784` | `L_to_gal(6)` | `1.585032314` (`real`) |
| `759` | `(t_k:real, temperature)         to_K      :90 -> real, none` | `POSTFIX` (prec `90`) | `t_k:real, temperature` | `-> real, none (locked)` | `t_k` | `((300 K)) to_K` | `300` (`real`) |
| `760` | `(t_k:real, temperature)         to_mK     :90 -> real, none` | `POSTFIX` (prec `90`) | `t_k:real, temperature` | `-> real, none (locked)` | `t_k * 1000` | `((300 K)) to_mK` | `300000` (`real`) |
| `761` | `(t_k:real, temperature)         to_degC   :90 -> real, none` | `POSTFIX` (prec `90`) | `t_k:real, temperature` | `-> real, none (locked)` | `t_k - 273.15 K` | `((300 K)) to_degC` | `26.85` (`real`) |
| `762` | `(t_k:real, temperature)         to_degF   :90 -> real, none` | `POSTFIX` (prec `90`) | `t_k:real, temperature` | `-> real, none (locked)` | `(t_k - 273.15 K) * 1.8 + 32 K` | `((300 K)) to_degF` | `80.33` (`real`) |
| `763` | `(hc_v:real, heat_capacity)      to_JpK    :90 -> real, none` | `POSTFIX` (prec `90`) | `hc_v:real, heat_capacity` | `-> real, none (locked)` | `hc_v` | `((40 JpK)) to_JpK` | `40` (`real`) |
| `764` | `(sh_v:real, specific_heat)      to_JpkgK  :90 -> real, none` | `POSTFIX` (prec `90`) | `sh_v:real, specific_heat` | `-> real, none (locked)` | `sh_v` | `((4184 JpkgK)) to_JpkgK` | `4184` (`real`) |
| `765` | `(n_v:real, substance)           to_mol    :90 -> real, none` | `POSTFIX` (prec `90`) | `n_v:real, substance` | `-> real, none (locked)` | `n_v` | `((2 mol)) to_mol` | `2` (`real`) |
| `766` | `(n_v:real, substance)           to_mmol   :90 -> real, none` | `POSTFIX` (prec `90`) | `n_v:real, substance` | `-> real, none (locked)` | `n_v * 1000` | `((2 mol)) to_mmol` | `2000` (`real`) |
| `767` | `(n_v:real, substance)           to_umol   :90 -> real, none` | `POSTFIX` (prec `90`) | `n_v:real, substance` | `-> real, none (locked)` | `n_v * 1e6` | `((2 mol)) to_umol` | `2000000` (`real`) |
| `768` | `(n_v:real, substance)           to_kmol   :90 -> real, none` | `POSTFIX` (prec `90`) | `n_v:real, substance` | `-> real, none (locked)` | `n_v / 1000` | `((2 mol)) to_kmol` | `0.002` (`real`) |
| `769` | `(mm_v:real, molar_mass)         to_kgpmol :90 -> real, none` | `POSTFIX` (prec `90`) | `mm_v:real, molar_mass` | `-> real, none (locked)` | `mm_v` | `((0.018 kgpmol)) to_kgpmol` | `0.018` (`real`) |
| `770` | `(mm_v:real, molar_mass)         to_gpmol  :90 -> real, none` | `POSTFIX` (prec `90`) | `mm_v:real, molar_mass` | `-> real, none (locked)` | `mm_v * 1000` | `((0.018 kgpmol)) to_gpmol` | `18` (`real`) |
| `771` | `(c_v:real, molarity)            to_molm3  :90 -> real, none` | `POSTFIX` (prec `90`) | `c_v:real, molarity` | `-> real, none (locked)` | `c_v` | `((1000 molm3)) to_molm3` | `1000` (`real`) |
| `772` | `(c_v:real, molarity)            to_M_conc :90 -> real, none` | `POSTFIX` (prec `90`) | `c_v:real, molarity` | `-> real, none (locked)` | `c_v / 1000` | `((1000 molm3)) to_M_conc` | `1` (`real`) |
| `773` | `(c_v:real, molarity)            to_mM_conc:90 -> real, none` | `POSTFIX` (prec `90`) | `c_v:real, molarity` | `-> real, none (locked)` | `c_v` | `((1000 molm3)) to_mM_conc` | `1000` (`real`) |
| `774` | `(me_v:real, molar_energy)       to_Jpmol  :90 -> real, none` | `POSTFIX` (prec `90`) | `me_v:real, molar_energy` | `-> real, none (locked)` | `me_v` | `((5000 Jpmol)) to_Jpmol` | `5000` (`real`) |
| `775` | `(me_v:real, molar_energy)       to_kJpmol :90 -> real, none` | `POSTFIX` (prec `90`) | `me_v:real, molar_energy` | `-> real, none (locked)` | `me_v / 1000` | `((5000 Jpmol)) to_kJpmol` | `5` (`real`) |
| `776` | `(mhc_v:real, molar_heat_capacity) to_JpmolK :90 -> real, none` | `POSTFIX` (prec `90`) | `mhc_v:real, molar_heat_capacity` | `-> real, none (locked)` | `mhc_v` | `((8.314 JpmolK)) to_JpmolK` | `8.314` (`real`) |
| `777` | `(kat_v:real, catalytic_activity)  to_kat  :90 -> real, none` | `POSTFIX` (prec `90`) | `kat_v:real, catalytic_activity` | `-> real, none (locked)` | `kat_v` | `((0.5 kat)) to_kat` | `0.5` (`real`) |
| `778` | `(li_v:real, luminous_intensity) to_cd     :90 -> real, none` | `POSTFIX` (prec `90`) | `li_v:real, luminous_intensity` | `-> real, none (locked)` | `li_v` | `((800 lm)) to_cd` | `800` (`real`) |
| `779` | `(li_v:real, luminous_intensity) to_lm     :90 -> real, none` | `POSTFIX` (prec `90`) | `li_v:real, luminous_intensity` | `-> real, none (locked)` | `li_v` | `((800 lm)) to_lm` | `800` (`real`) |
| `780` | `(il_v:real, illuminance)        to_lx     :90 -> real, none` | `POSTFIX` (prec `90`) | `il_v:real, illuminance` | `-> real, none (locked)` | `il_v` | `((200 lx)) to_lx` | `200` (`real`) |
| `781` | `(il_v:real, illuminance)        to_klx    :90 -> real, none` | `POSTFIX` (prec `90`) | `il_v:real, illuminance` | `-> real, none (locked)` | `il_v / 1000` | `((200 lx)) to_klx` | `0.2` (`real`) |

### Module `[vec]` (`Library/vec.math` — 109 Overloads)

Generic `$vec`/`list`/`stack`/`queue`, generic `...`/`>..`/`..<`/`>..<` ranges, `by` stepping, higher-order pipelines, Hadamard ops, statistics, OLS regression, and `$map` dictionaries.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `782` | `vec()                         -> vec` | `FUNC` | `()` (0 params) | `-> vec, none` | `@vnew(0)` | `vec()` | `[]` (`vec`) |
| `783` | `vec(cap:real, none)           -> vec` | `FUNC` | `cap:real, none (locked)` | `-> vec, none` | `@vnew(cap)` | `vec(6)` | `[]` (`vec`) |
| `784` | `vec(count:real, none, init:T) -> vec` | `FUNC` | `count:real, none (locked)`, `init:T, none` | `-> vec, none` | `@vfill(count, init)` | `vec(6, 3)` | `[3, 3, 3, 3, 3, 3]` (`vec`) |
| `785` | `list()                  -> vec` | `FUNC` | `()` (0 params) | `-> vec, none` | `@vnew(0)` | `list()` | `[]` (`vec`) |
| `786` | `list(item:T)            -> vec` | `FUNC` | `item:T, none` | `-> vec, none` | `@vfill(1, item)` | `list(6)` | `[6]` (`vec`) |
| `787` | `stack()                 -> vec` | `FUNC` | `()` (0 params) | `-> vec, none` | `@vnew(0)` | `stack()` | `[]` (`vec`) |
| `788` | `stack(item:T)            -> vec` | `FUNC` | `item:T, none` | `-> vec, none` | `@vfill(1, item)` | `stack(6)` | `[6]` (`vec`) |
| `789` | `queue()                 -> vec` | `FUNC` | `()` (0 params) | `-> vec, none` | `@vnew(0)` | `queue()` | `[]` (`vec`) |
| `790` | `queue(item:T)            -> vec` | `FUNC` | `item:T, none` | `-> vec, none` | `@vfill(1, item)` | `queue(6)` | `[6]` (`vec`) |
| `791` | `len(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `@vlen(v)` | `len(vec(2, 4, 6))` | `3` (`real`) |
| `792` | `cap(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `@vcap(v)` | `cap(vec(2, 4, 6))` | `4` (`real`) |
| `793` | `is_empty(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `(len(v) == 0)` | `is_empty(vec(2, 4, 6))` | `0` (`real`) |
| `794` | `at(v:vec, idx)          -> T` | `FUNC` | `v:vec, none`, `idx:real, none` | `-> T, none` | `@vget(v, idx)` | `at(vec(10, 20, 30), 1)` | `20` (`real`) |
| `795` | `set(v:vec, idx, val:T)  -> vec` | `FUNC` | `v:vec, none`, `idx:real, none`, `val:T, none` | `-> vec, none` | `@vset(v, idx, val)` | `set(vec(10, 20, 30), 1, 99)` | `[10, 99, 30]` (`vec`) |
| `796` | `resize(v:vec, new_len)  -> vec` | `FUNC` | `v:vec, none`, `new_len:real, none` | `-> vec, none` | `@vresize(v, new_len)` | `resize(vec(2, 4, 6), 3)` | `[2, 4, 6]` (`vec`) |
| `797` | `shrink(v:vec)           -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `@vshrink(v)` | `shrink(vec(2, 4, 6))` | `[2, 4, 6]` (`vec`) |
| `798` | `clear(v:vec)            -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `@vclear(v)` | `clear(vec(2, 4, 6))` | `[]` (`vec`) |
| `799` | `push(v:vec, item:T)     -> vec` | `FUNC` | `v:vec, none`, `item:T, none` | `-> vec, none` | `@vpush(v, item)` | `push(vec(2, 4, 6), 3)` | `[2, 4, 6, 3]` (`vec`) |
| `800` | `pop(v:vec)              -> T` | `FUNC` | `v:vec, none` | `-> T, none` | `@vpop(v)` | `pop(vec(2, 4, 6))` | `6` (`real`) |
| `801` | `enqueue(q:vec, item:T)  -> vec` | `FUNC` | `q:vec, none`, `item:T, none` | `-> vec, none` | `@vpush(q, item)` | `enqueue(vec(2, 4, 6), 3)` | `[2, 4, 6, 3]` (`vec`) |
| `802` | `dequeue(q:vec)          -> T` | `FUNC` | `q:vec, none` | `-> T, none` | `@vshift(q)` | `dequeue(vec(2, 4, 6))` | `2` (`real`) |
| `803` | `shift(q:vec)            -> T` | `FUNC` | `q:vec, none` | `-> T, none` | `@vshift(q)` | `shift(vec(2, 4, 6))` | `2` (`real`) |
| `804` | `pop_front(q:vec)        -> T` | `FUNC` | `q:vec, none` | `-> T, none` | `@vshift(q)` | `pop_front(vec(2, 4, 6))` | `2` (`real`) |
| `805` | `peek(s:vec)             -> T` | `FUNC` | `s:vec, none` | `-> T, none` | `(s.len() == 0) ? error("Cannot peek into an empty vector") : s[-1]` | `peek(vec(2, 4, 6))` | `6` (`real`) |
| `806` | `top(s:vec)              -> T` | `FUNC` | `s:vec, none` | `-> T, none` | `peek(s)` | `top(vec(2, 4, 6))` | `6` (`real`) |
| `807` | `back(s:vec)             -> T` | `FUNC` | `s:vec, none` | `-> T, none` | `peek(s)` | `back(vec(2, 4, 6))` | `6` (`real`) |
| `808` | `front(q:vec)            -> T` | `FUNC` | `q:vec, none` | `-> T, none` | `(q.len() == 0) ? error("Cannot peek into an empty vector") : q[0]` | `front(vec(2, 4, 6))` | `2` (`real`) |
| `809` | `(a:real, T) "..."  (b:real, T) :<45 -> vec` | `INFIX_LEFT` (prec `45`) | `a:real, T`, `b:real, T` | `-> vec, none` | `@vrange(a, b, 0)` | `1...5` | `[1, 2, 3, 4, 5]` (`vec`) |
| `810` | `(a:real, T) "..<"  (b:real, T) :<45 -> vec` | `INFIX_LEFT` (prec `45`) | `a:real, T`, `b:real, T` | `-> vec, none` | `@vrange(a, b, 1)` | `1..<5` | `[1, 2, 3, 4]` (`vec`) |
| `811` | `(a:real, T) ">.."  (b:real, T) :<45 -> vec` | `INFIX_LEFT` (prec `45`) | `a:real, T`, `b:real, T` | `-> vec, none` | `@vrange(a, b, 2)` | `1>..5` | `[2, 3, 4, 5]` (`vec`) |
| `812` | `(a:real, T) ">..<" (b:real, T) :<45 -> vec` | `INFIX_LEFT` (prec `45`) | `a:real, T`, `b:real, T` | `-> vec, none` | `@vrange(a, b, 3)` | `1>..<5` | `[2, 3, 4]` (`vec`) |
| `813` | `(r:vec)     by    (step:real) :<44 -> vec` | `INFIX_LEFT` (prec `44`) | `r:vec, none`, `step:real, none` | `-> vec, none` | `@vrange_step(r, step, 0)` | `1...10 by 2` | `[1, 3, 5, 7, 9]` (`vec`) |
| `814` | `(item:T)    into  (v:vec)     :>55 -> vec` | `INFIX_RIGHT` (prec `55`) | `item:T, none`, `v:vec, none` | `-> vec, none` | `@vpush(v, item)` | `40 into vec(10, 20, 30)` | `[10, 20, 30, 40]` (`vec`) |
| `815` | `(item:T) in (v:vec) :<42` | `INFIX_LEFT` (prec `42`) | `item:T, none`, `v:vec, none` | `-> real, none` | `{ u_it := unit_of(item) t_it := type_of(item) (x in v) { if (@eq(type...` | `20 in vec(10, 20, 30)` | `1` (`real`) |
| `816` | `range(start:real, T, stop:real, T, step = 1) -> vec` | `FUNC` | `start:real, T`, `stop:real, T`, `step:real, none = 1` | `-> vec, none` | `{ if (step == 0) { error("Range step cannot be zero") } out := vec<re...` | `range(6, 3, 3)` | `[]` (`vec`) |
| `817` | `range(start, stop) [default wrapper: step=1]` | `FUNC` | `start:real, T`, `stop:real, T` | `-> vec, none` | `Calls range(...) with step=1` | `range(6, 3)` | `[]` (`vec`) |
| `818` | `map(v:vec, f:fn)            -> vec` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> vec, none` | `[f(x) for x in v]` | `map(vec(2, 4, 6), (x => x^2 + 1))` | `[5, 17, 37]` (`vec`) |
| `819` | `filter(v:vec, f:fn)         -> vec` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> vec, none` | `[x for x in v if f(x)]` | `filter(vec(1, 2, 3, 4, 5, 6), x => x % 2 == 0)` | `[2, 4, 6]` (`vec`) |
| `820` | `reduce(v:vec, f:fn)         -> T` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> T, none` | `{ if (v.len() == 0) { error("Cannot reduce an empty collection withou...` | `reduce(vec(1, 2, 3, 4), (a, b) => a + b)` | `10` (`real`) |
| `821` | `reduce(v:vec, init:T, f:fn) -> T` | `FUNC` | `v:vec, none`, `init:T, none`, `f:fn, none` | `-> T, none` | `{ acc := init (x in v) { acc = f(acc, x) } acc }` | `reduce(vec(1, 2, 3, 4), 10, (a, b) => a + b)` | `20` (`real`) |
| `822` | `fold(v:vec, init:T, f:fn)   -> T` | `FUNC` | `v:vec, none`, `init:T, none`, `f:fn, none` | `-> T, none` | `reduce(v, init, f)` | `fold(vec(1, 2, 3, 4), 10, (a, b) => a + b)` | `20` (`real`) |
| `823` | `any(v:vec, f:fn)` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> real, none` | `{ (x in v) { if (f(x)) { return 1 } } 0 }` | `any(vec(1, 4, 5), x => x % 2 == 0)` | `1` (`real`) |
| `824` | `all(v:vec, f:fn)` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> real, none` | `{ (x in v) { if (!f(x)) { return 0 } } 1 }` | `all(vec(2, 4, 6), x => x % 2 == 0)` | `1` (`real`) |
| `825` | `find(v:vec, f:fn)           -> T` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> T, none` | `{ (x in v) { if (f(x)) { return x } } 0 }` | `find(vec(2, 4, 6), (x => x^2 + 1))` | `2` (`real`) |
| `826` | `count_if(v:vec, f:fn)` | `FUNC` | `v:vec, none`, `f:fn, none` | `-> real, none` | `{ c := 0 (x in v) { if (f(x)) { c += 1 } } c }` | `count_if(vec(2, 4, 7, 8), x => x >= 4)` | `3` (`real`) |
| `827` | `(a:vec) "+" (b:vec) :<50 -> vec` | `INFIX_LEFT` (prec `50`) | `a:vec, none`, `b:vec, none` | `-> vec, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '+'") }...` | `vec(2, 4, 6) + vec(1, 3, 5)` | `[3, 7, 11]` (`vec`) |
| `828` | `(a:vec) "-" (b:vec) :<50 -> vec` | `INFIX_LEFT` (prec `50`) | `a:vec, none`, `b:vec, none` | `-> vec, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '-'") }...` | `vec(2, 4, 6) - vec(1, 3, 5)` | `[1, 1, 1]` (`vec`) |
| `829` | `(a:vec) ".*" (b:vec) :<60 -> vec` | `INFIX_LEFT` (prec `60`) | `a:vec, none`, `b:vec, none` | `-> vec, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '.*'") ...` | `vec(2, 4, 6) .* vec(1, 3, 5)` | `[2, 12, 30]` (`vec`) |
| `830` | `(a:vec) "./" (b:vec) :<60 -> vec` | `INFIX_LEFT` (prec `60`) | `a:vec, none`, `b:vec, none` | `-> vec, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for './'") ...` | `vec(2, 4, 6) ./ vec(1, 3, 5)` | `[2, 1.33333, 1.2]` (`vec`) |
| `831` | `(a:vec) ".^" (b:vec) :>85 -> vec` | `INFIX_RIGHT` (prec `85`) | `a:vec, none`, `b:vec, none` | `-> vec, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '.^'") ...` | `vec(2, 4, 6) .^ vec(1, 3, 5)` | `[2, 64, 7776]` (`vec`) |
| `832` | `(a:vec)  ".^" (p:real) :>85 -> vec` | `INFIX_RIGHT` (prec `85`) | `a:vec, none`, `p:real, none` | `-> vec, none` | `[x ^ p for x in a]` | `vec(2, 4, 6) .^ 3` | `[8, 64, 216]` (`vec`) |
| `833` | `(s:real) ".^" (a:vec)  :>85 -> vec` | `INFIX_RIGHT` (prec `85`) | `s:real, none`, `a:vec, none` | `-> vec, none` | `[s ^ x for x in a]` | `6 .^ vec(1, 3, 5)` | `[6, 216, 7776]` (`vec`) |
| `834` | `vec_sum(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ s := 0 (x in v) { s += x } s }` | `vec_sum(vec(2, 4, 6))` | `12` (`real`) |
| `835` | `sum(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `vec_sum(v)` | `sum(vec(2, 4, 6))` | `12` (`real`) |
| `836` | `mean(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `vec_sum(v) / v.len()` | `mean(vec(2, 4, 6))` | `4` (`real`) |
| `837` | `variance(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ mu  := mean(v) acc := 0 (x in v) { d := x - mu acc += d^2 } acc / v...` | `variance(vec(2, 4, 6))` | `2.666666667` (`real`) |
| `838` | `stddev(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `√variance(v)` | `stddev(vec(2, 4, 6))` | `1.632993162` (`real`) |
| `839` | `sma(v:vec, window)` | `FUNC` | `v:vec, none`, `window:real, none` | `-> real, none` | `{ n     := v.len() if (n == 0) { error("Cannot compute sma() of an em...` | `sma(vec(2, 4, 6), 3)` | `4` (`real`) |
| `840` | `sort(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ n := v.len() if (n <= 1) { return [x for x in v] } out := [x for x ...` | `sort(vec(2, 4, 6))` | `[2, 4, 6]` (`vec`) |
| `841` | `sort_desc(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ n := v.len() if (n <= 1) { return [x for x in v] } out := [x for x ...` | `sort_desc(vec(2, 4, 6))` | `[6, 4, 2]` (`vec`) |
| `842` | `reverse(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ n := v.len() if (n <= 1) { return v } v[(n - 1)...0] }` | `reverse(vec(2, 4, 6))` | `[6, 4, 2]` (`vec`) |
| `843` | `vec_min(v:vec) -> T` | `FUNC` | `v:vec, none` | `-> T, none` | `{ if (v.len() == 0) { error("Cannot compute min of an empty vector") ...` | `vec_min(vec(2, 4, 6))` | `2` (`real`) |
| `844` | `vec_max(v:vec) -> T` | `FUNC` | `v:vec, none` | `-> T, none` | `{ if (v.len() == 0) { error("Cannot compute max of an empty vector") ...` | `vec_max(vec(2, 4, 6))` | `6` (`real`) |
| `845` | `min(v:vec) -> T` | `FUNC` | `v:vec, none` | `-> T, none` | `vec_min(v)` | `min(vec(2, 4, 6))` | `2` (`real`) |
| `846` | `max(v:vec) -> T` | `FUNC` | `v:vec, none` | `-> T, none` | `vec_max(v)` | `max(vec(2, 4, 6))` | `6` (`real`) |
| `847` | `argmin(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ if (v.len() == 0) { error("Cannot compute argmin of an empty vector...` | `argmin(vec(2, 4, 6))` | `0` (`real`) |
| `848` | `argmax(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ if (v.len() == 0) { error("Cannot compute argmax of an empty vector...` | `argmax(vec(2, 4, 6))` | `2` (`real`) |
| `849` | `cumsum(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ out := vec() acc := 0 (i, x in v) { acc = (i == 0) ? x : (acc + x) ...` | `cumsum(vec(2, 4, 6))` | `[2, 6, 12]` (`vec`) |
| `850` | `cumprod(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ out := vec() acc := 1 (x in v) { acc = acc * x acc into out } out }` | `cumprod(vec(2, 4, 6))` | `[2, 8, 48]` (`vec`) |
| `851` | `diff_vec(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ n := v.len() if (n <= 1) { return vec() } [v[i + 1] - v[i] for i in...` | `diff_vec(vec(2, 4, 6))` | `[2, 2]` (`vec`) |
| `852` | `dot(a:vec, b:vec)` | `FUNC` | `a:vec, none`, `b:vec, none` | `-> real, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for dot()")...` | `dot(vec(2, 4, 6), vec(1, 3, 5))` | `44` (`real`) |
| `853` | `norm(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `√dot(v, v)` | `norm(vec(2, 4, 6))` | `7.483314774` (`real`) |
| `854` | `linspace(start:real, T, stop:real, T, n = 50) -> vec` | `FUNC` | `start:real, T`, `stop:real, T`, `n:real, none = 50` | `-> vec, none` | `{ if (n < 1) { error("linspace() count must be >= 1") } if (n == 1) {...` | `linspace(6, 3, 3)` | `[6, 4.5, 3]` (`vec`) |
| `855` | `linspace(start, stop) [default wrapper: n=50]` | `FUNC` | `start:real, T`, `stop:real, T` | `-> vec, none` | `Calls linspace(...) with n=50` | `linspace(6, 3)` | `[6, 5.93878, 5.87755, 5.81633, 5.7551, 5.69388, 5.63265, 5.57143, 5.5102, 5.44898, 5.38776, 5.32653, 5.26531, 5.20408, 5.14286, 5.08163, 5.02041, 4.95918, 4.89796, 4.83673, 4.77551, 4.71429, 4.65306, 4.59184, 4.53061, 4.46939, 4.40816, 4.34694, 4.28571, 4.22449, 4.16327, 4.10204, 4.04082, 3.97959, 3.91837, 3.85714, 3.79592, 3.73469, 3.67347, 3.61224, 3.55102, 3.4898, 3.42857, 3.36735, 3.30612, 3.2449, 3.18367, 3.12245, 3.06122, 3]` (`vec`) |
| `856` | `unique(v:vec) -> vec` | `FUNC` | `v:vec, none` | `-> vec, none` | `{ out := vec() (x in v) { if (!(x in out)) { x into out } } out }` | `unique(vec(2, 4, 6))` | `[2, 4, 6]` (`vec`) |
| `857` | `zip_with(a:vec, b:vec, f:fn) -> vec` | `FUNC` | `a:vec, none`, `b:vec, none`, `f:fn, none` | `-> vec, none` | `{ if (a.len() != b.len()) { error("Vector length mismatch for zip_wit...` | `zip_with(vec(1, 2, 3), vec(10, 20, 30), (a, b) => a + b)` | `[11, 22, 33]` (`vec`) |
| `858` | `median(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ n := v.len() if (n == 0) { error("Cannot compute median of an empty...` | `median(vec(2, 4, 6))` | `4` (`real`) |
| `859` | `sample_variance(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ n := v.len() if (n <= 1) { error("sample_variance() requires at lea...` | `sample_variance(vec(2, 4, 6))` | `4` (`real`) |
| `860` | `sample_stddev(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `√sample_variance(v)` | `sample_stddev(vec(2, 4, 6))` | `2` (`real`) |
| `861` | `cov(x:vec, y:vec)` | `FUNC` | `x:vec, none`, `y:vec, none` | `-> real, none` | `{ n := x.len() if (n != y.len() \|\| n == 0) { error("cov() requires no...` | `cov(vec(2, 4, 6), vec(1, 3, 5))` | `2.666666667` (`real`) |
| `862` | `corr(x:vec, y:vec)` | `FUNC` | `x:vec, none`, `y:vec, none` | `-> real, none` | `cov(x, y) / (stddev(x) * stddev(y))` | `corr(vec(2, 4, 6), vec(1, 3, 5))` | `1` (`real`) |
| `863` | `linreg_slope(x:vec, y:vec)` | `FUNC` | `x:vec, none`, `y:vec, none` | `-> real, none` | `cov(x, y) / variance(x)` | `linreg_slope(vec(2, 4, 6), vec(1, 3, 5))` | `1` (`real`) |
| `864` | `linreg_intercept(x:vec, y:vec)` | `FUNC` | `x:vec, none`, `y:vec, none` | `-> real, none` | `mean(y) - linreg_slope(x, y) * mean(x)` | `linreg_intercept(vec(2, 4, 6), vec(1, 3, 5))` | `-1` (`real`) |
| `865` | `linreg_r2(x:vec, y:vec)` | `FUNC` | `x:vec, none`, `y:vec, none` | `-> real, none` | `corr(x, y) ^ 2` | `linreg_r2(vec(2, 4, 6), vec(1, 3, 5))` | `1` (`real`) |
| `866` | `map()                           -> map` | `FUNC` | `()` (0 params) | `-> map, none` | `@map_new(0)` | `map()` | `{}` (`map`) |
| `867` | `dict()                          -> map` | `FUNC` | `()` (0 params) | `-> map, none` | `@map_new(0)` | `dict()` | `{}` (`map`) |
| `868` | `map_set(m:map, k:T, val:T)      -> map` | `FUNC` | `m:map, none`, `k:T, none`, `val:T, none` | `-> map, none` | `@map_set(m, k, val)` | `map_set(dict("a", 10), "b", 20)` | `{"a": 10, "b": 20}` (`map`) |
| `869` | `put(m:map, k:T, val:T)          -> map` | `FUNC` | `m:map, none`, `k:T, none`, `val:T, none` | `-> map, none` | `@map_set(m, k, val)` | `put(dict("a", 10), "b", 20)` | `{"a": 10, "b": 20}` (`map`) |
| `870` | `set(m:map, k:T, val:T)          -> map` | `FUNC` | `m:map, none`, `k:T, none`, `val:T, none` | `-> map, none` | `@map_set(m, k, val)` | `set(dict("a", 10), "b", 20)` | `{"a": 10, "b": 20}` (`map`) |
| `871` | `map_get(m:map, k:T)             -> T` | `FUNC` | `m:map, none`, `k:T, none` | `-> T, none` | `@map_get(m, k)` | `map_get(dict("a", 10, "b", 20), "a")` | `10` (`real`) |
| `872` | `get(m:map, k:T)                 -> T` | `FUNC` | `m:map, none`, `k:T, none` | `-> T, none` | `@map_get(m, k)` | `get(dict("a", 10, "b", 20), "a")` | `10` (`real`) |
| `873` | `map_get(m:map, k:T, def_val:T)  -> T` | `FUNC` | `m:map, none`, `k:T, none`, `def_val:T, none` | `-> T, none` | `@map_get(m, k, def_val)` | `map_get(dict("a", 10), "z", 99)` | `99` (`real`) |
| `874` | `get_or(m:map, k:T, def_val:T)   -> T` | `FUNC` | `m:map, none`, `k:T, none`, `def_val:T, none` | `-> T, none` | `@map_get(m, k, def_val)` | `get_or(dict("a", 10), "z", 99)` | `99` (`real`) |
| `875` | `map_has(m:map, k:T)` | `FUNC` | `m:map, none`, `k:T, none` | `-> real, none` | `@map_has(m, k)` | `map_has(dict("a", 10, "b", 20), "a")` | `1` (`real`) |
| `876` | `has_key(m:map, k:T)` | `FUNC` | `m:map, none`, `k:T, none` | `-> real, none` | `@map_has(m, k)` | `has_key(dict("a", 10, "b", 20), "a")` | `1` (`real`) |
| `877` | `map_del(m:map, k:T)             -> map` | `FUNC` | `m:map, none`, `k:T, none` | `-> map, none` | `@map_del(m, k)` | `map_del(dict("a", 10, "b", 20), "a")` | `{"b": 20}` (`map`) |
| `878` | `remove(m:map, k:T)              -> map` | `FUNC` | `m:map, none`, `k:T, none` | `-> map, none` | `@map_del(m, k)` | `remove(dict("a", 10, "b", 20), "a")` | `{"b": 20}` (`map`) |
| `879` | `keys(m:map)                     -> vec` | `FUNC` | `m:map, none` | `-> vec, none` | `[k for k in m]` | `keys(dict("a", 10, "b", 20))` | `["a", "b"]` (`vec`) |
| `880` | `vals(m:map)                     -> vec` | `FUNC` | `m:map, none` | `-> vec, none` | `[v for k, v in m]` | `vals(dict("a", 10, "b", 20))` | `[10, 20]` (`vec`) |
| `881` | `values(m:map)                   -> vec` | `FUNC` | `m:map, none` | `-> vec, none` | `[v for k, v in m]` | `values(dict("a", 10, "b", 20))` | `[10, 20]` (`vec`) |
| `882` | `entries(m:map)                  -> vec` | `FUNC` | `m:map, none` | `-> vec, none` | `[k + "=" + (v_) for k, v in m]` | `entries(dict("a", 10, "b", 20))` | `["a=10", "b=20"]` (`vec`) |
| `883` | `len(m:map)` | `FUNC` | `m:map, none` | `-> real, none` | `@vlen(m)` | `len(dict("a", 10, "b", 20))` | `2` (`real`) |
| `884` | `is_empty(m:map)` | `FUNC` | `m:map, none` | `-> real, none` | `@eq(@vlen(m), 0)` | `is_empty(dict("a", 10, "b", 20))` | `0` (`real`) |
| `885` | `clear(m:map)                    -> map` | `FUNC` | `m:map, none` | `-> map, none` | `@vclear(m)` | `clear(dict("a", 10, "b", 20))` | `{}` (`map`) |
| `886` | `(a:vec) "==" (b:vec) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec, none`, `b:vec, none` | `-> real, none` | `{ if (a.len() != b.len()) { return 0 } (i, x in a) { if (x != b[i]) {...` | `vec(2, 4, 6) == vec(1, 3, 5)` | `0` (`real`) |
| `887` | `(a:vec) "!=" (b:vec) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec, none`, `b:vec, none` | `-> real, none` | `!(a == b)` | `vec(2, 4, 6) != vec(1, 3, 5)` | `1` (`real`) |
| `888` | `(a:vec) "~"  (b:vec) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec, none`, `b:vec, none` | `-> real, none` | `{ if (a.len() != b.len()) { return 0 } (i, x in a) { if (!(x ~ b[i]))...` | `vec(2, 4, 6) ~ vec(1, 3, 5)` | `0` (`real`) |
| `889` | `dim_vec(x:T)         -> vec` | `FUNC` | `x:T, none` | `-> vec, none` | `[@unit_name(x, i) for i in 0..<7]` | `dim_vec(6)` | `[0, 0, 0, 0, 0, 0, 0]` (`vec`) |
| `890` | `same_dim(a:T, b:T)` | `FUNC` | `a:T, none`, `b:T, none` | `-> real, none` | `(dim_vec(a) == dim_vec(b))` | `same_dim(6, 3)` | `1` (`real`) |

### Module `[biology]` (`Library/biology.math` — 60 Overloads)

Population genetics (Hardy-Weinberg), molecular biology/PCR, Michaelis-Menten enzyme kinetics, plant water potential, cell morphometrics, human physiology, and ecology.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `891` | `hw_q_from_p(p_dom)` | `FUNC` | `p_dom:real, none` | `-> real, none` | `1 - p_dom` | `hw_q_from_p(0.6)` | `0.4` (`real`) |
| `892` | `hw_p_from_q2(q2_rec_freq)` | `FUNC` | `q2_rec_freq:real, none` | `-> real, none` | `1 - √q2_rec_freq` | `hw_p_from_q2(0.04)` | `0.8` (`real`) |
| `893` | `hw_homo_dom(p_dom)` | `FUNC` | `p_dom:real, none` | `-> real, none` | `p_dom ^ 2` | `hw_homo_dom(0.6)` | `0.36` (`real`) |
| `894` | `hw_hetero(p_dom)` | `FUNC` | `p_dom:real, none` | `-> real, none` | `2 * p_dom * (1 - p_dom)` | `hw_hetero(0.6)` | `0.48` (`real`) |
| `895` | `hw_homo_rec(p_dom)` | `FUNC` | `p_dom:real, none` | `-> real, none` | `(1 - p_dom) ^ 2` | `hw_homo_rec(0.6)` | `0.16` (`real`) |
| `896` | `hw_carrier_count(q2_rec_freq, pop_size)` | `FUNC` | `q2_rec_freq:real, none`, `pop_size:real, none` | `-> real, none` | `{ q_val := √q2_rec_freq p_val := 1 - q_val 2 * p_val * q_val * pop_si...` | `hw_carrier_count(0.04, 1000)` | `320` (`real`) |
| `897` | `recomb_freq(recombinants, total_offspring)` | `FUNC` | `recombinants:real, none`, `total_offspring:real, none` | `-> real, none` | `(recombinants / total_offspring) * 100` | `recomb_freq(6, 3)` | `200` (`real`) |
| `898` | `chi_sq_term(observed, expected)` | `FUNC` | `observed:real, none`, `expected:real, none` | `-> real, none` | `((observed - expected) ^ 2) / expected` | `chi_sq_term(6, 3)` | `3` (`real`) |
| `899` | `chargaff_g_from_a(a_pct)` | `FUNC` | `a_pct:real, none` | `-> real, none` | `50 - a_pct` | `chargaff_g_from_a(6)` | `44` (`real`) |
| `900` | `chargaff_c_from_t(t_pct)` | `FUNC` | `t_pct:real, none` | `-> real, none` | `50 - t_pct` | `chargaff_c_from_t(6)` | `44` (`real`) |
| `901` | `dna_length_nm(base_pairs)` | `FUNC` | `base_pairs:real, none` | `-> real, none` | `base_pairs * 0.34` | `dna_length_nm(6)` | `2.04` (`real`) |
| `902` | `dna_turns(base_pairs)` | `FUNC` | `base_pairs:real, none` | `-> real, none` | `base_pairs / 10` | `dna_turns(6)` | `0.6` (`real`) |
| `903` | `dna_weight_da(base_pairs)` | `FUNC` | `base_pairs:real, none` | `-> real, none` | `base_pairs * 650` | `dna_weight_da(6)` | `3900` (`real`) |
| `904` | `codons_for_protein(amino_acids)` | `FUNC` | `amino_acids:real, none` | `-> real, none` | `amino_acids + 1` | `codons_for_protein(6)` | `7` (`real`) |
| `905` | `mrna_bases_for_protein(amino_acids)` | `FUNC` | `amino_acids:real, none` | `-> real, none` | `3 * (amino_acids + 1)` | `mrna_bases_for_protein(6)` | `21` (`real`) |
| `906` | `pcr_copies(n_cycles)` | `FUNC` | `n_cycles:real, none` | `-> real, none` | `2 ^ n_cycles` | `pcr_copies(6)` | `64` (`real`) |
| `907` | `pcr_yield_from(n0_templates, n_cycles)` | `FUNC` | `n0_templates:real, none`, `n_cycles:real, none` | `-> real, none` | `n0_templates * (2 ^ n_cycles)` | `pcr_yield_from(6, 3)` | `48` (`real`) |
| `908` | `res_digest_linear(cut_sites)` | `FUNC` | `cut_sites:real, none` | `-> real, none` | `cut_sites + 1` | `res_digest_linear(6)` | `7` (`real`) |
| `909` | `res_digest_circular(cut_sites)` | `FUNC` | `cut_sites:real, none` | `-> real, none` | `cut_sites` | `res_digest_circular(6)` | `6` (`real`) |
| `910` | `michaelis_menten(vmax, km, substrate_s)` | `FUNC` | `vmax:real, none`, `km:real, none`, `substrate_s:real, none` | `-> real, none` | `(vmax * substrate_s) / (km + substrate_s)` | `michaelis_menten(6, 3, 3)` | `3` (`real`) |
| `911` | `competitive_inh_v(vmax, km, substrate_s, inh_i, ki)` | `FUNC` | `vmax:real, none`, `km:real, none`, `substrate_s:real, none`, `inh_i:real, none`, `ki:real, none` | `-> real, none` | `(vmax * substrate_s) / (km * (1 + inh_i / ki) + substrate_s)` | `competitive_inh_v(6, 3, 3, 3, 3)` | `2` (`real`) |
| `912` | `noncomp_inh_v(vmax, km, substrate_s, inh_i, ki)` | `FUNC` | `vmax:real, none`, `km:real, none`, `substrate_s:real, none`, `inh_i:real, none`, `ki:real, none` | `-> real, none` | `((vmax / (1 + inh_i / ki)) * substrate_s) / (km + substrate_s)` | `noncomp_inh_v(6, 3, 3, 3, 3)` | `1.5` (`real`) |
| `913` | `enzyme_catalytic_eff(kcat, km)` | `FUNC` | `kcat:real, none`, `km:real, none` | `-> real, none` | `kcat / km` | `enzyme_catalytic_eff(6, 3)` | `2` (`real`) |
| `914` | `q10_rate(r1, t1_c, t2_c, q10_coeff = 2)` | `FUNC` | `r1:real, none`, `t1_c:real, none`, `t2_c:real, none`, `q10_coeff:real, none = 2` | `-> real, none` | `r1 * (q10_coeff ^ ((t2_c - t1_c) / 10))` | `q10_rate(6, 3, 3, 3)` | `6` (`real`) |
| `915` | `q10_rate(r1, t1_c, t2_c) [default wrapper: q10_coeff=2]` | `FUNC` | `r1:real, none`, `t1_c:real, none`, `t2_c:real, none` | `-> real, none` | `Calls q10_rate(...) with q10_coeff=2` | `q10_rate(6, 3, 3)` | `6` (`real`) |
| `916` | `q10_from_rates(r1, r2, t1_c, t2_c)` | `FUNC` | `r1:real, none`, `r2:real, none`, `t1_c:real, none`, `t2_c:real, none` | `-> real, none` | `(r2 / r1) ^ (10 / (t2_c - t1_c))` | `q10_from_rates(10, 20, 20, 30)` | `2` (`real`) |
| `917` | `respiratory_quotient(co2_produced, o2_consumed)` | `FUNC` | `co2_produced:real, none`, `o2_consumed:real, none` | `-> real, none` | `co2_produced / o2_consumed` | `respiratory_quotient(6, 3)` | `2` (`real`) |
| `918` | `aerobic_atp_yield(glucose_moles)` | `FUNC` | `glucose_moles:real, none` | `-> real, none` | `38 * glucose_moles` | `aerobic_atp_yield(6)` | `228` (`real`) |
| `919` | `water_potential(psi_s, psi_p)` | `FUNC` | `psi_s:real, none`, `psi_p:real, none` | `-> real, none` | `psi_s + psi_p` | `water_potential(6, 3)` | `9` (`real`) |
| `920` | `solute_potential(i_ion, molarity_c, temp_k)` | `FUNC` | `i_ion:real, none`, `molarity_c:real, none`, `temp_k:real, none` | `-> real, none` | `-i_ion * molarity_c * 0.0831 * temp_k` | `solute_potential(6, 3, 3)` | `-4.4874` (`real`) |
| `921` | `diffusion_pressure_deficit(osmotic_p, turgor_p)` | `FUNC` | `osmotic_p:real, none`, `turgor_p:real, none` | `-> real, none` | `osmotic_p - turgor_p` | `diffusion_pressure_deficit(6, 3)` | `3` (`real`) |
| `922` | `stomatal_index(stomata_s, epidermal_e)` | `FUNC` | `stomata_s:real, none`, `epidermal_e:real, none` | `-> real, none` | `(stomata_s / (stomata_s + epidermal_e)) * 100` | `stomatal_index(6, 3)` | `66.66666667` (`real`) |
| `923` | `total_magnification(ocular_mag, objective_mag)` | `FUNC` | `ocular_mag:real, none`, `objective_mag:real, none` | `-> real, none` | `ocular_mag * objective_mag` | `total_magnification(6, 3)` | `18` (`real`) |
| `924` | `actual_specimen_size(image_size, magnification)` | `FUNC` | `image_size:real, none`, `magnification:real, none` | `-> real, none` | `image_size / magnification` | `actual_specimen_size(6, 3)` | `2` (`real`) |
| `925` | `fov_diameter(fov_low, mag_low, mag_high)` | `FUNC` | `fov_low:real, none`, `mag_low:real, none`, `mag_high:real, none` | `-> real, none` | `(fov_low * mag_low) / mag_high` | `fov_diameter(6, 3, 3)` | `6` (`real`) |
| `926` | `mitotic_index(dividing_cells, total_cells)` | `FUNC` | `dividing_cells:real, none`, `total_cells:real, none` | `-> real, none` | `(dividing_cells / total_cells) * 100` | `mitotic_index(6, 3)` | `200` (`real`) |
| `927` | `cell_surface_sphere(r)` | `FUNC` | `r:real, none` | `-> real, none` | `4π * (r ^ 2)` | `cell_surface_sphere(6)` | `452.3893421` (`real`) |
| `928` | `cell_volume_sphere(r)` | `FUNC` | `r:real, none` | `-> real, none` | `(4 / 3) * π * (r ^ 3)` | `cell_volume_sphere(6)` | `904.7786842` (`real`) |
| `929` | `sa_to_vol_sphere(r)` | `FUNC` | `r:real, none` | `-> real, none` | `3 / r` | `sa_to_vol_sphere(6)` | `0.5` (`real`) |
| `930` | `stroke_volume(edv_ml, esv_ml)` | `FUNC` | `edv_ml:real, none`, `esv_ml:real, none` | `-> real, none` | `edv_ml - esv_ml` | `stroke_volume(6, 3)` | `3` (`real`) |
| `931` | `ejection_fraction(edv_ml, esv_ml)` | `FUNC` | `edv_ml:real, none`, `esv_ml:real, none` | `-> real, none` | `((edv_ml - esv_ml) / edv_ml) * 100` | `ejection_fraction(6, 3)` | `50` (`real`) |
| `932` | `cardiac_output(hr_bpm, stroke_vol_ml)` | `FUNC` | `hr_bpm:real, none`, `stroke_vol_ml:real, none` | `-> real, none` | `(hr_bpm * stroke_vol_ml) / 1000` | `cardiac_output(6, 3)` | `0.018` (`real`) |
| `933` | `pulse_pressure(systolic_p, diastolic_p)` | `FUNC` | `systolic_p:real, none`, `diastolic_p:real, none` | `-> real, none` | `systolic_p - diastolic_p` | `pulse_pressure(6, 3)` | `3` (`real`) |
| `934` | `mean_arterial_pressure(systolic_p, diastolic_p)` | `FUNC` | `systolic_p:real, none`, `diastolic_p:real, none` | `-> real, none` | `diastolic_p + (systolic_p - diastolic_p) / 3` | `mean_arterial_pressure(6, 3)` | `4` (`real`) |
| `935` | `minute_ventilation(tidal_vol_ml, resp_rate_bpm)` | `FUNC` | `tidal_vol_ml:real, none`, `resp_rate_bpm:real, none` | `-> real, none` | `tidal_vol_ml * resp_rate_bpm` | `minute_ventilation(6, 3)` | `18` (`real`) |
| `936` | `alveolar_ventilation(tidal_vol_ml, dead_space_ml, resp_rate_bpm)` | `FUNC` | `tidal_vol_ml:real, none`, `dead_space_ml:real, none`, `resp_rate_bpm:real, none` | `-> real, none` | `(tidal_vol_ml - dead_space_ml) * resp_rate_bpm` | `alveolar_ventilation(6, 3, 3)` | `9` (`real`) |
| `937` | `vital_capacity(tv_ml, irv_ml, erv_ml)` | `FUNC` | `tv_ml:real, none`, `irv_ml:real, none`, `erv_ml:real, none` | `-> real, none` | `tv_ml + irv_ml + erv_ml` | `vital_capacity(6, 3, 3)` | `12` (`real`) |
| `938` | `total_lung_capacity(vc_ml, rv_ml)` | `FUNC` | `vc_ml:real, none`, `rv_ml:real, none` | `-> real, none` | `vc_ml + rv_ml` | `total_lung_capacity(6, 3)` | `9` (`real`) |
| `939` | `net_filtration_pressure(ghp, bcops, chp)` | `FUNC` | `ghp:real, none`, `bcops:real, none`, `chp:real, none` | `-> real, none` | `ghp - (bcops + chp)` | `net_filtration_pressure(6, 3, 3)` | `0` (`real`) |
| `940` | `renal_clearance(urine_conc_u, urine_flow_v, plasma_conc_p)` | `FUNC` | `urine_conc_u:real, none`, `urine_flow_v:real, none`, `plasma_conc_p:real, none` | `-> real, none` | `(urine_conc_u * urine_flow_v) / plasma_conc_p` | `renal_clearance(6, 3, 3)` | `6` (`real`) |
| `941` | `bmi_calc(mass_kg, height_m)` | `FUNC` | `mass_kg:real, none`, `height_m:real, none` | `-> real, none` | `mass_kg / (height_m ^ 2)` | `bmi_calc(6, 3)` | `0.6666666667` (`real`) |
| `942` | `pop_exp_growth(n0, r_intrinsic, time_t)` | `FUNC` | `n0:real, none`, `r_intrinsic:real, none`, `time_t:real, none` | `-> real, none` | `n0 * exp(r_intrinsic * time_t)` | `pop_exp_growth(6, 3, 3)` | `48618.50357` (`real`) |
| `943` | `pop_doubling_time(r_intrinsic)` | `FUNC` | `r_intrinsic:real, none` | `-> real, none` | `ln(2) / r_intrinsic` | `pop_doubling_time(6)` | `0.1155245301` (`real`) |
| `944` | `pop_logistic_dNdt(r_intrinsic, n_cur, k_carrying)` | `FUNC` | `r_intrinsic:real, none`, `n_cur:real, none`, `k_carrying:real, none` | `-> real, none` | `r_intrinsic * n_cur * ((k_carrying - n_cur) / k_carrying)` | `pop_logistic_dNdt(6, 3, 3)` | `0` (`real`) |
| `945` | `pop_logistic_nt(n0, k_carrying, r_intrinsic, time_t)` | `FUNC` | `n0:real, none`, `k_carrying:real, none`, `r_intrinsic:real, none`, `time_t:real, none` | `-> real, none` | `k_carrying / (1 + ((k_carrying - n0) / n0) * exp(-r_intrinsic * time_t))` | `pop_logistic_nt(6, 3, 3, 3)` | `3.000185126` (`real`) |
| `946` | `lincoln_petersen_pop(marked_m1, total_catch_c, recaptured_m2)` | `FUNC` | `marked_m1:real, none`, `total_catch_c:real, none`, `recaptured_m2:real, none` | `-> real, none` | `(marked_m1 * total_catch_c) / recaptured_m2` | `lincoln_petersen_pop(6, 3, 3)` | `6` (`real`) |
| `947` | `trophic_energy_at(base_energy, trophic_steps)` | `FUNC` | `base_energy:real, none`, `trophic_steps:real, none` | `-> real, none` | `base_energy * (0.1 ^ trophic_steps)` | `trophic_energy_at(6, 3)` | `0.006` (`real`) |
| `948` | `net_primary_prod(gpp, respiration_r)` | `FUNC` | `gpp:real, none`, `respiration_r:real, none` | `-> real, none` | `gpp - respiration_r` | `net_primary_prod(6, 3)` | `3` (`real`) |
| `949` | `simpson_diversity_vec(counts:vec)` | `FUNC` | `counts:vec, none` | `-> real, none` | `{ tot   := counts.sum() numer := 0 (ni in counts) { numer += ni * (ni...` | `simpson_diversity_vec(vec(2, 4, 6))` | `0.6666666667` (`real`) |
| `950` | `shannon_diversity_vec(counts:vec)` | `FUNC` | `counts:vec, none` | `-> real, none` | `{ tot := counts.sum() h   := 0 (ni in counts) { if (ni > 0) { p_i := ...` | `shannon_diversity_vec(vec(2, 4, 6))` | `1.011404265` (`real`) |

### Module `[geom_ext]` (`Library/geom.math` — 38 Overloads)

Flat C-style `$vec2(x, y)`, `$vec3(x, y, z)`, and nested `$circle(center:vec2, radius)` structs, dot/cross products, norms, interpolation, and circle/sphere geometry.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `951` | `"-" (v:vec2)             :80  -> vec2` | `PREFIX` (prec `80`) | `v:vec2, none` | `-> vec2, none` | `vec2(-v.x, -v.y)` | `-vec2(3, 4)` | `vec2(x=-3, y=-4)` (`vec2`) |
| `952` | `(a:vec2) "+"  (b:vec2) :<50 -> vec2` | `INFIX_LEFT` (prec `50`) | `a:vec2, none`, `b:vec2, none` | `-> vec2, none` | `vec2(a.x + b.x, a.y + b.y)` | `vec2(3, 4) + vec2(1, 2)` | `vec2(x=4, y=6)` (`vec2`) |
| `953` | `(a:vec2) "-"  (b:vec2) :<50 -> vec2` | `INFIX_LEFT` (prec `50`) | `a:vec2, none`, `b:vec2, none` | `-> vec2, none` | `vec2(a.x - b.x, a.y - b.y)` | `vec2(3, 4) - vec2(1, 2)` | `vec2(x=2, y=2)` (`vec2`) |
| `954` | `(a:vec2) into (b:vec2) :>55 -> vec2` | `INFIX_RIGHT` (prec `55`) | `a:vec2, none`, `b:vec2, none` | `-> vec2, none` | `vec2(a.x + b.x, a.y + b.y)` | `vec2(3, 4) into vec2(1, 2)` | `vec2(x=4, y=6)` (`vec2`) |
| `955` | `(s)      "*"  (v:vec2) :<60 -> vec2` | `INFIX_LEFT` (prec `60`) | `s:real, none`, `v:vec2, none` | `-> vec2, none` | `vec2(s * v.x, s * v.y)` | `6 * vec2(1, 2)` | `vec2(x=6, y=12)` (`vec2`) |
| `956` | `(v:vec2) "*"  (s)      :<60 -> vec2` | `INFIX_LEFT` (prec `60`) | `v:vec2, none`, `s:real, none` | `-> vec2, none` | `vec2(v.x * s, v.y * s)` | `vec2(3, 4) * 3` | `vec2(x=9, y=12)` (`vec2`) |
| `957` | `(v:vec2) "/"  (s)      :<60 -> vec2` | `INFIX_LEFT` (prec `60`) | `v:vec2, none`, `s:real, none` | `-> vec2, none` | `vec2(v.x / s, v.y / s)` | `vec2(3, 4) / 3` | `vec2(x=1, y=1.33333)` (`vec2`) |
| `958` | `(a:vec2) "==" (b:vec2) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec2, none`, `b:vec2, none` | `-> real, none` | `(a.x == b.x) && (a.y == b.y)` | `vec2(3, 4) == vec2(1, 2)` | `0` (`real`) |
| `959` | `(a:vec2) "!=" (b:vec2) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec2, none`, `b:vec2, none` | `-> real, none` | `(a.x != b.x) \|\| (a.y != b.y)` | `vec2(3, 4) != vec2(1, 2)` | `1` (`real`) |
| `960` | `(a:vec2) "~"  (b:vec2) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec2, none`, `b:vec2, none` | `-> real, none` | `(a.x ~ b.x) && (a.y ~ b.y)` | `vec2(3, 4) ~ vec2(1, 2)` | `0` (`real`) |
| `961` | `(z:real, T) into (v:vec2, T) :>55 -> vec3, T` | `INFIX_RIGHT` (prec `55`) | `z:real, T`, `v:vec2, T` | `-> vec3, T` | `vec3(v.x, v.y, z)` | `6 into vec2(1, 2)` | `vec3(x=1, y=2, z=6)` (`vec3`) |
| `962` | `(v:vec2, T) into (z:real, T) :>55 -> vec3, T` | `INFIX_RIGHT` (prec `55`) | `v:vec2, T`, `z:real, T` | `-> vec3, T` | `vec3(v.x, v.y, z)` | `vec2(3, 4) into 3` | `vec3(x=3, y=4, z=3)` (`vec3`) |
| `963` | `dot(a:vec2, b:vec2)` | `FUNC` | `a:vec2, none`, `b:vec2, none` | `-> real, none` | `a.x * b.x + a.y * b.y` | `dot(vec2(3, 4), vec2(1, 2))` | `11` (`real`) |
| `964` | `cross2(a:vec2, b:vec2)` | `FUNC` | `a:vec2, none`, `b:vec2, none` | `-> real, none` | `a.x * b.y - a.y * b.x` | `cross2(vec2(3, 4), vec2(1, 2))` | `2` (`real`) |
| `965` | `vec2_mag(v:vec2)` | `FUNC` | `v:vec2, none` | `-> real, none` | `hypot(v.x, v.y)` | `vec2_mag(vec2(3, 4))` | `5` (`real`) |
| `966` | `vec2_dist(a:vec2, b:vec2)` | `FUNC` | `a:vec2, none`, `b:vec2, none` | `-> real, none` | `hypot(b.x - a.x, b.y - a.y)` | `vec2_dist(vec2(3, 4), vec2(1, 2))` | `2.828427125` (`real`) |
| `967` | `"-" (v:vec3)             :80  -> vec3` | `PREFIX` (prec `80`) | `v:vec3, none` | `-> vec3, none` | `vec3(-v.x, -v.y, -v.z)` | `-vec3(1, 2, 3)` | `vec3(x=-1, y=-2, z=-3)` (`vec3`) |
| `968` | `(a:vec3) "+"  (b:vec3) :<50 -> vec3` | `INFIX_LEFT` (prec `50`) | `a:vec3, none`, `b:vec3, none` | `-> vec3, none` | `vec3(a.x + b.x, a.y + b.y, a.z + b.z)` | `vec3(1, 2, 3) + vec3(4, 5, 6)` | `vec3(x=5, y=7, z=9)` (`vec3`) |
| `969` | `(a:vec3) "-"  (b:vec3) :<50 -> vec3` | `INFIX_LEFT` (prec `50`) | `a:vec3, none`, `b:vec3, none` | `-> vec3, none` | `vec3(a.x - b.x, a.y - b.y, a.z - b.z)` | `vec3(1, 2, 3) - vec3(4, 5, 6)` | `vec3(x=-3, y=-3, z=-3)` (`vec3`) |
| `970` | `(a:vec3) into (b:vec3) :>55 -> vec3` | `INFIX_RIGHT` (prec `55`) | `a:vec3, none`, `b:vec3, none` | `-> vec3, none` | `vec3(a.x + b.x, a.y + b.y, a.z + b.z)` | `vec3(1, 2, 3) into vec3(4, 5, 6)` | `vec3(x=5, y=7, z=9)` (`vec3`) |
| `971` | `(s)      "*"  (v:vec3) :<60 -> vec3` | `INFIX_LEFT` (prec `60`) | `s:real, none`, `v:vec3, none` | `-> vec3, none` | `vec3(s * v.x, s * v.y, s * v.z)` | `6 * vec3(4, 5, 6)` | `vec3(x=24, y=30, z=36)` (`vec3`) |
| `972` | `(v:vec3) "*"  (s)      :<60 -> vec3` | `INFIX_LEFT` (prec `60`) | `v:vec3, none`, `s:real, none` | `-> vec3, none` | `vec3(v.x * s, v.y * s, v.z * s)` | `vec3(1, 2, 3) * 3` | `vec3(x=3, y=6, z=9)` (`vec3`) |
| `973` | `(v:vec3) "/"  (s)      :<60 -> vec3` | `INFIX_LEFT` (prec `60`) | `v:vec3, none`, `s:real, none` | `-> vec3, none` | `vec3(v.x / s, v.y / s, v.z / s)` | `vec3(1, 2, 3) / 3` | `vec3(x=0.333333, y=0.666667, z=1)` (`vec3`) |
| `974` | `(a:vec3) "**" (b:vec3) :<20` | `INFIX_LEFT` (prec `20`) | `a:vec3, none`, `b:vec3, none` | `-> real, none` | `a.x * b.x + a.y * b.y + a.z * b.z` | `vec3(1, 2, 3) ** vec3(4, 5, 6)` | `32` (`real`) |
| `975` | `(a:vec3) "==" (b:vec3) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec3, none`, `b:vec3, none` | `-> real, none` | `(a.x == b.x) && (a.y == b.y) && (a.z == b.z)` | `vec3(1, 2, 3) == vec3(4, 5, 6)` | `0` (`real`) |
| `976` | `(a:vec3) "!=" (b:vec3) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec3, none`, `b:vec3, none` | `-> real, none` | `(a.x != b.x) \|\| (a.y != b.y) \|\| (a.z != b.z)` | `vec3(1, 2, 3) != vec3(4, 5, 6)` | `1` (`real`) |
| `977` | `(a:vec3) "~"  (b:vec3) :<30` | `INFIX_LEFT` (prec `30`) | `a:vec3, none`, `b:vec3, none` | `-> real, none` | `(a.x ~ b.x) && (a.y ~ b.y) && (a.z ~ b.z)` | `vec3(1, 2, 3) ~ vec3(4, 5, 6)` | `0` (`real`) |
| `978` | `dot3(a:vec3, b:vec3)` | `FUNC` | `a:vec3, none`, `b:vec3, none` | `-> real, none` | `a ** b` | `dot3(vec3(1, 2, 3), vec3(4, 5, 6))` | `32` (`real`) |
| `979` | `cross3(a:vec3, b:vec3) -> vec3` | `FUNC` | `a:vec3, none`, `b:vec3, none` | `-> vec3, none` | `vec3(a.y * b.z - a.z * b.y,` | `cross3(vec3(1, 2, 3), vec3(4, 5, 6))` | `vec3(x=-3, y=6, z=-3)` (`vec3`) |
| `980` | `vec3_mag(v:vec3)` | `FUNC` | `v:vec3, none` | `-> real, none` | `hypot(hypot(v.x, v.y), v.z)` | `vec3_mag(vec3(1, 2, 3))` | `3.741657387` (`real`) |
| `981` | `vec3_dist(a:vec3, b:vec3)` | `FUNC` | `a:vec3, none`, `b:vec3, none` | `-> real, none` | `vec3_mag(b - a)` | `vec3_dist(vec3(1, 2, 3), vec3(4, 5, 6))` | `5.196152423` (`real`) |
| `982` | `lerp(a, b, t)` | `FUNC` | `a:real, none`, `b:real, none`, `t:real, none` | `-> real, none` | `a + (b - a) * t` | `lerp(6, 3, 3)` | `-3` (`real`) |
| `983` | `vec3_path_len(pts:vec)` | `FUNC` | `pts:vec, none` | `-> real, none` | `{ total := 0 idx   := 1 n     := pts.len() (idx < n) ?? { total += ve...` | `vec3_path_len(list(vec3(0, 0, 0), vec3(3, 4, 0), vec3(3, 4, 12)))` | `17` (`real`) |
| `984` | `move_circle(c:circle, dx, dy) -> circle` | `FUNC` | `c:circle, none`, `dx:real, none`, `dy:real, none` | `-> circle, none` | `{ c.center.x += dx c.center.y += dy c }` | `move_circle(circle(vec2(1, 2), 5), 3, 3)` | `circle(center=vec2(x=4, y=5), radius=5)` (`circle`) |
| `985` | `circle_area(c:circle)` | `FUNC` | `c:circle, none` | `-> real, none` | `π * c.radius^2` | `circle_area(circle(vec2(1, 2), 5))` | `78.53981634` (`real`) |
| `986` | `circle_circum(c:circle)` | `FUNC` | `c:circle, none` | `-> real, none` | `2π * c.radius` | `circle_circum(circle(vec2(1, 2), 5))` | `31.41592654` (`real`) |
| `987` | `sphere_vol(r)` | `FUNC` | `r:real, none` | `-> real, none` | `(4 / 3) * π * (r ^ 3)` | `sphere_vol(6)` | `904.7786842` (`real`) |
| `988` | `sphere_area(r)` | `FUNC` | `r:real, none` | `-> real, none` | `4π * (r ^ 2)` | `sphere_area(6)` | `452.3893421` (`real`) |

### Module `[mat]` (`Library/mat.math` — 91 Overloads)

1D–4D `$mat` tensors, reshaping, Hadamard ops, matrix multiplication, `det`/`inv`/`linsolve`, LU, QR, Cholesky, Eigen, SVD, Moore-Penrose `pinv`, `lstsq`, and matrix norms.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `989` | `mat_rank(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `@mrank(m)` | `mat_rank([4, 2 ; 2, 3])` | `2` (`real`) |
| `990` | `mat_len(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `@mlen(m)` | `mat_len([4, 2 ; 2, 3])` | `4` (`real`) |
| `991` | `mat_dim(m:mat, axis)` | `FUNC` | `m:mat, none`, `axis:real, none` | `-> real, none` | `@mdim(m, axis)` | `mat_dim([1, 2, 3 ; 4, 5, 6], 1)` | `3` (`real`) |
| `992` | `mat_rows(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `@mdim(m, 0)` | `mat_rows([4, 2 ; 2, 3])` | `2` (`real`) |
| `993` | `mat_cols(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `(mat_rank(m) >= 2) ? @mdim(m, 1) : @mdim(m, 0)` | `mat_cols([4, 2 ; 2, 3])` | `2` (`real`) |
| `994` | `mat_shape(m:mat)          -> vec` | `FUNC` | `m:mat, none` | `-> vec, none` | `[@mdim(m, i) for i in 0..<mat_rank(m)]` | `mat_shape([4, 2 ; 2, 3])` | `[2, 2]` (`vec`) |
| `995` | `reshape(m:mat, r, c)      -> mat` | `FUNC` | `m:mat, none`, `r:real, none`, `c:real, none` | `-> mat, none` | `@mreshape(m, r, c)` | `reshape([1, 2, 3 ; 4, 5, 6], 3, 2)` | `[[1, 2], [3, 4], [5, 6]]` (`mat`) |
| `996` | `reshape(v:vec, r, c)      -> mat` | `FUNC` | `v:vec, none`, `r:real, none`, `c:real, none` | `-> mat, none` | `@mreshape(v, r, c)` | `reshape(1...6, 2, 3)` | `[[1, 2, 3], [4, 5, 6]]` (`mat`) |
| `997` | `flatten(m:mat)            -> vec` | `FUNC` | `m:mat, none` | `-> vec, none` | `[x for x in m]` | `flatten([4, 2 ; 2, 3])` | `[4, 2, 2, 3]` (`vec`) |
| `998` | `mat_fill(r, c, v:real, T) -> mat, T` | `FUNC` | `r:real, none`, `c:real, none`, `v:real, T` | `-> mat, T` | `{ if (r <= 0 \|\| c <= 0) { error("Matrix dimensions must be positive")...` | `mat_fill(6, 3, 3)` | `[[3, 3, 3], [3, 3, 3], [3, 3, 3], [3, 3, 3], [3, 3, 3], [3, 3, 3]]` (`mat`) |
| `999` | `mat_zeros(r, c)           -> mat` | `FUNC` | `r:real, none`, `c:real, none` | `-> mat, none` | `mat_fill(r, c, 0)` | `mat_zeros(6, 3)` | `[[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]]` (`mat`) |
| `1000` | `mat_ones(r, c)            -> mat` | `FUNC` | `r:real, none`, `c:real, none` | `-> mat, none` | `mat_fill(r, c, 1)` | `mat_ones(6, 3)` | `[[1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1]]` (`mat`) |
| `1001` | `mat_eye(n)                -> mat` | `FUNC` | `n:real, none` | `-> mat, none` | `{ if (n <= 0) { error("Identity matrix dimension must be positive") }...` | `mat_eye(6)` | `[[1, 0, 0, 0, 0, 0], [0, 1, 0, 0, 0, 0], [0, 0, 1, 0, 0, 0], [0, 0, 0, 1, 0, 0], [0, 0, 0, 0, 1, 0], [0, 0, 0, 0, 0, 1]]` (`mat`) |
| `1002` | `map(m:mat, f:fn)          -> mat` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> mat, none` | `@mreshape([f(x) for x in m], m, 0)` | `map([4, 2 ; 2, 3], (x => x^2 + 1))` | `[[17, 5], [5, 10]]` (`mat`) |
| `1003` | `filter(m:mat, f:fn)       -> vec` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> vec, none` | `[x for x in m if f(x)]` | `filter([1, 2 ; 3, 4], x => x >= 3)` | `[3, 4]` (`vec`) |
| `1004` | `reduce(m:mat, f:fn)` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> real, none` | `reduce(flatten(m), f)` | `reduce([1, 2 ; 3, 4], (a, b) => a + b)` | `10` (`real`) |
| `1005` | `reduce(m:mat, init, f:fn)` | `FUNC` | `m:mat, none`, `init:real, none`, `f:fn, none` | `-> real, none` | `reduce(flatten(m), init, f)` | `reduce([1, 2 ; 3, 4], 10, (a, b) => a + b)` | `20` (`real`) |
| `1006` | `fold(m:mat, init, f:fn)` | `FUNC` | `m:mat, none`, `init:real, none`, `f:fn, none` | `-> real, none` | `reduce(flatten(m), init, f)` | `fold([1, 2 ; 3, 4], 10, (a, b) => a + b)` | `20` (`real`) |
| `1007` | `any(m:mat, f:fn)` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> real, none` | `any(flatten(m), f)` | `any([1, 3 ; 4, 5], x => x % 2 == 0)` | `1` (`real`) |
| `1008` | `all(m:mat, f:fn)` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> real, none` | `all(flatten(m), f)` | `all([2, 4 ; 6, 8], x => x % 2 == 0)` | `1` (`real`) |
| `1009` | `find(m:mat, f:fn)` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> real, none` | `find(flatten(m), f)` | `find([2, 5 ; 12, 18], x => x > 10)` | `12` (`real`) |
| `1010` | `count_if(m:mat, f:fn)` | `FUNC` | `m:mat, none`, `f:fn, none` | `-> real, none` | `count_if(flatten(m), f)` | `count_if([1, 5 ; 8, 12], x => x >= 5)` | `3` (`real`) |
| `1011` | `mat_zip(a:mat, b:mat, f:fn) -> mat` | `FUNC` | `a:mat, none`, `b:mat, none`, `f:fn, none` | `-> mat, none` | `{ if (mat_shape(a) != mat_shape(b)) { error("Matrix shape mismatch") ...` | `mat_zip([1, 2 ; 3, 4], [10, 20 ; 30, 40], (a, b) => a + b)` | `[[11, 22], [33, 44]]` (`mat`) |
| `1012` | `"-" (m:mat, T)                 :80  -> mat, T` | `PREFIX` (prec `80`) | `m:mat, T` | `-> mat, T` | `m.map(x => -x)` | `-[4, 2 ; 2, 3]` | `[[-4, -2], [-2, -3]]` (`mat`) |
| `1013` | `(a:mat, T)     "+"  (b:mat, T) :<50 -> mat, T` | `INFIX_LEFT` (prec `50`) | `a:mat, T`, `b:mat, T` | `-> mat, T` | `mat_zip(a, b, (x, y) => x + y)` | `[4, 2 ; 2, 3] + [1, 2 ; 3, 4]` | `[[5, 4], [5, 7]]` (`mat`) |
| `1014` | `(a:mat, T)     "-"  (b:mat, T) :<50 -> mat, T` | `INFIX_LEFT` (prec `50`) | `a:mat, T`, `b:mat, T` | `-> mat, T` | `mat_zip(a, b, (x, y) => x - y)` | `[4, 2 ; 2, 3] - [1, 2 ; 3, 4]` | `[[3, 0], [-1, -1]]` (`mat`) |
| `1015` | `(a:mat, T)     into (b:mat, T) :>55 -> mat, T` | `INFIX_RIGHT` (prec `55`) | `a:mat, T`, `b:mat, T` | `-> mat, T` | `mat_zip(a, b, (x, y) => x + y)` | `[4, 2 ; 2, 3] into [1, 2 ; 3, 4]` | `[[5, 4], [5, 7]]` (`mat`) |
| `1016` | `(a:mat) "*" (b:mat) :<60 -> mat` | `INFIX_LEFT` (prec `60`) | `a:mat, none`, `b:mat, none` | `-> mat, none` | `{ ra := mat_rank(a) rb := mat_rank(b) if (ra == 2 && rb == 2) { M  :=...` | `[4, 2 ; 2, 3] * [1, 2 ; 3, 4]` | `[[10, 16], [11, 16]]` (`mat`) |
| `1017` | `(m:mat) "*" (s:real) :<60 -> mat` | `INFIX_LEFT` (prec `60`) | `m:mat, none`, `s:real, none` | `-> mat, none` | `m.map(x => x * s)` | `[4, 2 ; 2, 3] * 3` | `[[12, 6], [6, 9]]` (`mat`) |
| `1018` | `(s:real) "*" (m:mat) :<60 -> mat` | `INFIX_LEFT` (prec `60`) | `s:real, none`, `m:mat, none` | `-> mat, none` | `m.map(x => s * x)` | `6 * [1, 2 ; 3, 4]` | `[[6, 12], [18, 24]]` (`mat`) |
| `1019` | `(m:mat) "/" (s:real) :<60 -> mat` | `INFIX_LEFT` (prec `60`) | `m:mat, none`, `s:real, none` | `-> mat, none` | `{ if (s == 0) { error("Division by zero in matrix scalar division") }...` | `[4, 2 ; 2, 3] / 3` | `[[1.33333, 0.666667], [0.666667, 1]]` (`mat`) |
| `1020` | `(item:real, T) in (m:mat, T) :<42` | `INFIX_LEFT` (prec `42`) | `item:real, T`, `m:mat, T` | `-> real, none` | `{ u_it := unit_of(item) (x in m) { if (@eq(unit_of(x), u_it)) { if (x...` | `3 in [1, 2 ; 3, 4]` | `1` (`real`) |
| `1021` | `(a:mat)  ".*" (b:mat)  :<60 -> mat` | `INFIX_LEFT` (prec `60`) | `a:mat, none`, `b:mat, none` | `-> mat, none` | `mat_zip(a, b, (x, y) => x * y)` | `[4, 2 ; 2, 3] .* [1, 2 ; 3, 4]` | `[[4, 4], [6, 12]]` (`mat`) |
| `1022` | `(a:mat)  "./" (b:mat)  :<60 -> mat` | `INFIX_LEFT` (prec `60`) | `a:mat, none`, `b:mat, none` | `-> mat, none` | `mat_zip(a, b, (x, y) => x / y)` | `[4, 2 ; 2, 3] ./ [1, 2 ; 3, 4]` | `[[4, 1], [0.666667, 0.75]]` (`mat`) |
| `1023` | `(a:mat)  ".^" (b:real) :>85 -> mat` | `INFIX_RIGHT` (prec `85`) | `a:mat, none`, `b:real, none` | `-> mat, none` | `a.map(x => x ^ b)` | `[4, 2 ; 2, 3] .^ 3` | `[[64, 8], [8, 27]]` (`mat`) |
| `1024` | `(a:real) ".^" (b:mat)  :>85 -> mat` | `INFIX_RIGHT` (prec `85`) | `a:real, none`, `b:mat, none` | `-> mat, none` | `b.map(x => a ^ x)` | `6 .^ [1, 2 ; 3, 4]` | `[[6, 36], [216, 1296]]` (`mat`) |
| `1025` | `(a:mat)  ".^" (b:mat)  :>85 -> mat` | `INFIX_RIGHT` (prec `85`) | `a:mat, none`, `b:mat, none` | `-> mat, none` | `mat_zip(a, b, (x, y) => x ^ y)` | `[4, 2 ; 2, 3] .^ [1, 2 ; 3, 4]` | `[[4, 4], [8, 81]]` (`mat`) |
| `1026` | `transpose(m:mat, T) -> mat, T` | `FUNC` | `m:mat, T` | `-> mat, T` | `{ r := mat_rank(m) if (r == 1) { return @mreshape(m, mat_len(m), 1) }...` | `transpose([4, 2 ; 2, 3])` | `[[4, 2], [2, 3]]` (`mat`) |
| `1027` | `trace(m:mat, T) -> real, T` | `FUNC` | `m:mat, T` | `-> real, T` | `{ r := mat_rows(m) if (mat_rank(m) != 2 \|\| r != mat_cols(m) \|\| r == 0...` | `trace([4, 2 ; 2, 3])` | `7` (`real`) |
| `1028` | `det(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `{ n := mat_rows(m) if (mat_rank(m) != 2 \|\| n != mat_cols(m) \|\| n == 0...` | `det([4, 2 ; 2, 3])` | `8` (`real`) |
| `1029` | `inv(m:mat) -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `{ n := mat_rows(m) if (mat_rank(m) != 2 \|\| n != mat_cols(m) \|\| n == 0...` | `inv([4, 2 ; 2, 3])` | `[[0.375, -0.25], [-0.25, 0.5]]` (`mat`) |
| `1030` | `linsolve(a:mat, b:vec) -> vec` | `FUNC` | `a:mat, none`, `b:vec, none` | `-> vec, none` | `{ n := mat_rows(a) if (mat_rank(a) != 2 \|\| n != mat_cols(a) \|\| n == 0...` | `linsolve([2, 3 ; 5, -1], list(8, 3))` | `[1, 2]` (`vec`) |
| `1031` | `linsolve(a:mat, b:mat) -> mat` | `FUNC` | `a:mat, none`, `b:mat, none` | `-> mat, none` | `{ n  := mat_rows(a) rb := mat_rank(b) if (rb == 1) { if (mat_len(b) !...` | `linsolve([2, 3 ; 5, -1], [8, 3])` | `[1, 2]` (`mat`) |
| `1032` | `diag(v:vec) -> mat` | `FUNC` | `v:vec, none` | `-> mat, none` | `{ n   := v.len() out := mat_zeros(n, n) (i, x in v) { out[i, i] = x }...` | `diag(vec(2, 4, 6))` | `[[2, 0, 0], [0, 4, 0], [0, 0, 6]]` (`mat`) |
| `1033` | `diag(m:mat, T) -> vec` | `FUNC` | `m:mat, T` | `-> vec, none` | `{ n := min(mat_rows(m), mat_cols(m)) [m[i, i] for i in 0..<n] }` | `diag([4, 2 ; 2, 3])` | `[4, 3]` (`vec`) |
| `1034` | `mat_sum(m:mat, T)        -> real, T` | `FUNC` | `m:mat, T` | `-> real, T` | `m.reduce((a, b) => a + b)` | `mat_sum([4, 2 ; 2, 3])` | `11` (`real`) |
| `1035` | `mat_mean(m:mat, T)       -> real, T` | `FUNC` | `m:mat, T` | `-> real, T` | `mat_sum(m) / mat_len(m)` | `mat_mean([4, 2 ; 2, 3])` | `2.75` (`real`) |
| `1036` | `mat_min(m:mat, T)        -> real, T` | `FUNC` | `m:mat, T` | `-> real, T` | `m.reduce((a, b) => a < b ? a : b)` | `mat_min([4, 2 ; 2, 3])` | `2` (`real`) |
| `1037` | `mat_max(m:mat, T)        -> real, T` | `FUNC` | `m:mat, T` | `-> real, T` | `m.reduce((a, b) => a > b ? a : b)` | `mat_max([4, 2 ; 2, 3])` | `4` (`real`) |
| `1038` | `frobenius_norm(m:mat, T) -> real, T` | `FUNC` | `m:mat, T` | `-> real, T` | `√(m.map(x => x^2).reduce((a, b) => a + b))` | `frobenius_norm([4, 2 ; 2, 3])` | `5.744562647` (`real`) |
| `1039` | `mat_pow(m:mat, n:real, none) -> mat` | `FUNC` | `m:mat, none`, `n:real, none (locked)` | `-> mat, none` | `{ r := mat_rows(m) if (r != mat_cols(m)) { error("Matrix power requir...` | `mat_pow([4, 2 ; 2, 3], 3)` | `[[108, 82], [82, 67]]` (`mat`) |
| `1040` | `(m:mat) "^" (n:real, none) :>85 -> mat` | `INFIX_RIGHT` (prec `85`) | `m:mat, none`, `n:real, none (locked)` | `-> mat, none` | `mat_pow(m, n)` | `[4, 2 ; 2, 3] ^ 3` | `[[108, 82], [82, 67]]` (`mat`) |
| `1041` | `(m:mat) "*" (v:vec) :<60 -> vec` | `INFIX_LEFT` (prec `60`) | `m:mat, none`, `v:vec, none` | `-> vec, none` | `{ if (mat_rank(m) != 2) { error("mat * vec requires a 2D matrix on th...` | `[1, 2 ; 3, 4] * list(5, 6)` | `[17, 39]` (`vec`) |
| `1042` | `(v:vec) "*" (m:mat) :<60 -> vec` | `INFIX_LEFT` (prec `60`) | `v:vec, none`, `m:mat, none` | `-> vec, none` | `{ if (mat_rank(m) != 2) { error("vec * mat requires a 2D matrix on th...` | `list(5, 6) * [1, 2 ; 3, 4]` | `[23, 34]` (`vec`) |
| `1043` | `outer(a:vec, b:vec) -> mat` | `FUNC` | `a:vec, none`, `b:vec, none` | `-> mat, none` | `{ na  := a.len() nb  := b.len() out := mat_zeros(na, nb) (i, x in a) ...` | `outer(vec(2, 4, 6), vec(1, 3, 5))` | `[[2, 6, 10], [4, 12, 20], [6, 18, 30]]` (`mat`) |
| `1044` | `vstack(a:mat, b:mat) -> mat` | `FUNC` | `a:mat, none`, `b:mat, none` | `-> mat, none` | `{ ra := (mat_rank(a) == 1) ? 1 : mat_rows(a) ca := (mat_rank(a) == 1)...` | `vstack([4, 2 ; 2, 3], [1, 2 ; 3, 4])` | `[[4, 2], [2, 3], [1, 2], [3, 4]]` (`mat`) |
| `1045` | `hstack(a:mat, b:mat) -> mat` | `FUNC` | `a:mat, none`, `b:mat, none` | `-> mat, none` | `{ ra := (mat_rank(a) == 1) ? 1 : mat_rows(a) ca := (mat_rank(a) == 1)...` | `hstack([4, 2 ; 2, 3], [1, 2 ; 3, 4])` | `[[4, 2, 1, 2], [2, 3, 3, 4]]` (`mat`) |
| `1046` | `kron(a:mat, b:mat)   -> mat` | `FUNC` | `a:mat, none`, `b:mat, none` | `-> mat, none` | `{ ra  := mat_rows(a) ca  := mat_cols(a) rb  := mat_rows(b) cb  := mat...` | `kron([4, 2 ; 2, 3], [1, 2 ; 3, 4])` | `[[4, 8, 2, 4], [12, 16, 6, 8], [2, 4, 3, 6], [6, 8, 9, 12]]` (`mat`) |
| `1047` | `tril(m:mat, k = 0) -> mat` | `FUNC` | `m:mat, none`, `k:real, none = 0` | `-> mat, none` | `{ r   := mat_rows(m) c   := mat_cols(m) out := mat_zeros(r, c) (i in ...` | `tril([4, 2 ; 2, 3], 3)` | `[[4, 2], [2, 3]]` (`mat`) |
| `1048` | `tril(m) [default wrapper: k=0]` | `FUNC` | `m:mat, none` | `-> mat, none` | `Calls tril(...) with k=0` | `tril([4, 2 ; 2, 3])` | `[[4, 0], [2, 3]]` (`mat`) |
| `1049` | `triu(m:mat, k = 0) -> mat` | `FUNC` | `m:mat, none`, `k:real, none = 0` | `-> mat, none` | `{ r   := mat_rows(m) c   := mat_cols(m) out := mat_zeros(r, c) (i in ...` | `triu([4, 2 ; 2, 3], 3)` | `[[0, 0], [0, 0]]` (`mat`) |
| `1050` | `triu(m) [default wrapper: k=0]` | `FUNC` | `m:mat, none` | `-> mat, none` | `Calls triu(...) with k=0` | `triu([4, 2 ; 2, 3])` | `[[4, 2], [0, 3]]` (`mat`) |
| `1051` | `lu_helper(m:mat, mode) -> mat` | `FUNC` | `m:mat, none`, `mode:real, none` | `-> mat, none` | `{ n := mat_rows(m) if (mat_rank(m) != 2 \|\| n != mat_cols(m) \|\| n == 0...` | `lu_helper([4, 3 ; 6, 3], 0)` | `[[1, 0], [0.666667, 1]]` (`mat`) |
| `1052` | `lu_l(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `lu_helper(m, 0)` | `lu_l([4, 2 ; 2, 3])` | `[[1, 0], [0.5, 1]]` (`mat`) |
| `1053` | `lu_u(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `lu_helper(m, 1)` | `lu_u([4, 2 ; 2, 3])` | `[[4, 2], [0, 2]]` (`mat`) |
| `1054` | `lu_p(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `lu_helper(m, 2)` | `lu_p([4, 2 ; 2, 3])` | `[[1, 0], [0, 1]]` (`mat`) |
| `1055` | `qr_q(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `{ r := mat_rows(m) c := mat_cols(m) if (mat_rank(m) != 2 \|\| r < c \|\| ...` | `qr_q([4, 2 ; 2, 3])` | `[[0.894427, -0.447214], [0.447214, 0.894427]]` (`mat`) |
| `1056` | `qr_r(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `transpose(qr_q(m)) * m` | `qr_r([4, 2 ; 2, 3])` | `[[4.47214, 3.1305], [4.44089e-16, 1.78885]]` (`mat`) |
| `1057` | `chol(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `{ n := mat_rows(m) if (mat_rank(m) != 2 \|\| n != mat_cols(m) \|\| n == 0...` | `chol([4, 2 ; 2, 3])` | `[[2, 0], [1, 1.41421]]` (`mat`) |
| `1058` | `jacobi_helper(m:mat, mode) -> mat` | `FUNC` | `m:mat, none`, `mode:real, none` | `-> mat, none` | `{ n := mat_rows(m) if (mat_rank(m) != 2 \|\| n != mat_cols(m) \|\| n == 0...` | `jacobi_helper([4, 2 ; 2, 3], 3)` | `[[0.788205, -0.615412], [0.615412, 0.788205]]` (`mat`) |
| `1059` | `eigvals(m:mat) -> vec` | `FUNC` | `m:mat, none` | `-> vec, none` | `{ if (mat_rank(m) != 2 \|\| mat_rows(m) != mat_cols(m) \|\| mat_rows(m) =...` | `eigvals([4, 2 ; 2, 3])` | `[5.56155, 1.43845]` (`vec`) |
| `1060` | `eigvecs(m:mat) -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `jacobi_helper(m, 1)` | `eigvecs([4, 2 ; 2, 3])` | `[[0.788205, -0.615412], [0.615412, 0.788205]]` (`mat`) |
| `1061` | `svd_s(m:mat) -> vec` | `FUNC` | `m:mat, none` | `-> vec, none` | `{ if (mat_rank(m) != 2) { error("SVD requires a 2D matrix") } M   := ...` | `svd_s([4, 2 ; 2, 3])` | `[5.56155, 1.43845]` (`vec`) |
| `1062` | `svd_v(m:mat) -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `{ if (mat_rank(m) != 2) { error("SVD requires a 2D matrix") } jacobi_...` | `svd_v([4, 2 ; 2, 3])` | `[[0.788205, -0.615412], [0.615412, 0.788205]]` (`mat`) |
| `1063` | `svd_u(m:mat) -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `{ if (mat_rank(m) != 2) { error("SVD requires a 2D matrix") } M   := ...` | `svd_u([4, 2 ; 2, 3])` | `[[0.788205, -0.615412], [0.615412, 0.788205]]` (`mat`) |
| `1064` | `pinv(m:mat)             -> mat` | `FUNC` | `m:mat, none` | `-> mat, none` | `{ s    := svd_s(m) u    := svd_u(m) v    := svd_v(m) tol  := 1e-10 * ...` | `pinv([4, 2 ; 2, 3])` | `[[0.375, -0.25], [-0.25, 0.5]]` (`mat`) |
| `1065` | `lstsq(a:mat, b:vec)     -> vec` | `FUNC` | `a:mat, none`, `b:vec, none` | `-> vec, none` | `pinv(a) * b` | `lstsq([1, 1 ; 1, 2 ; 1, 3], vec(6, 5, 7))` | `[5, 0.5]` (`vec`) |
| `1066` | `lstsq(a:mat, b:mat)     -> mat` | `FUNC` | `a:mat, none`, `b:mat, none` | `-> mat, none` | `pinv(a) * b` | `lstsq([1, 1 ; 1, 2 ; 1, 3], [6, 5, 7])` | `[5, 0.5]` (`mat`) |
| `1067` | `power_vec(m:mat, steps = 40) -> vec` | `FUNC` | `m:mat, none`, `steps:real, none = 40` | `-> vec, none` | `{ n := mat_rows(m) if (n != mat_cols(m) \|\| n == 0) { error("power_vec...` | `power_vec([4, 2 ; 2, 3], 3)` | `[0.786893, 0.61709]` (`vec`) |
| `1068` | `power_vec(m) [default wrapper: steps=40]` | `FUNC` | `m:mat, none` | `-> vec, none` | `Calls power_vec(...) with steps=40` | `power_vec([4, 2 ; 2, 3])` | `[0.788205, 0.615412]` (`vec`) |
| `1069` | `power_iter(m:mat, steps = 40)` | `FUNC` | `m:mat, none`, `steps:real, none = 40` | `-> real, none` | `{ v := power_vec(m, steps) dot(v, m * v) / dot(v, v) }` | `power_iter([4, 2 ; 2, 3], 3)` | `5.561534107` (`real`) |
| `1070` | `power_iter(m) [default wrapper: steps=40]` | `FUNC` | `m:mat, none` | `-> real, none` | `Calls power_iter(...) with steps=40` | `power_iter([4, 2 ; 2, 3])` | `5.561552813` (`real`) |
| `1071` | `matrix_rank(m:mat, tol = 1e-9)` | `FUNC` | `m:mat, none`, `tol:real, none = 1e-9` | `-> real, none` | `{ sv := svd_s(m) sv.count_if(s => s > tol) }` | `matrix_rank([4, 2 ; 2, 3], 3)` | `1` (`real`) |
| `1072` | `matrix_rank(m) [default wrapper: tol=1e-9]` | `FUNC` | `m:mat, none` | `-> real, none` | `Calls matrix_rank(...) with tol=1e-9` | `matrix_rank([4, 2 ; 2, 3])` | `2` (`real`) |
| `1073` | `spectral_norm(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `{ sv := svd_s(m) sv[0] }` | `spectral_norm([4, 2 ; 2, 3])` | `5.561552813` (`real`) |
| `1074` | `cond(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `{ sv   := svd_s(m) smin := sv[sv.len() - 1] if (smin <= 1e-15) { erro...` | `cond([4, 2 ; 2, 3])` | `3.866358711` (`real`) |
| `1075` | `mat_norm1(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `{ r    := mat_rows(m) c    := mat_cols(m) best := 0 (j in 0..<c) { s ...` | `mat_norm1([4, 2 ; 2, 3])` | `6` (`real`) |
| `1076` | `mat_norm_inf(m:mat)` | `FUNC` | `m:mat, none` | `-> real, none` | `{ r    := mat_rows(m) c    := mat_cols(m) best := 0 (i in 0..<r) { s ...` | `mat_norm_inf([4, 2 ; 2, 3])` | `6` (`real`) |
| `1077` | `(a:mat) "==" (b:mat) :<30` | `INFIX_LEFT` (prec `30`) | `a:mat, none`, `b:mat, none` | `-> real, none` | `{ if (mat_shape(a) != mat_shape(b)) { return 0 } flatten(a) == flatte...` | `[4, 2 ; 2, 3] == [1, 2 ; 3, 4]` | `0` (`real`) |
| `1078` | `(a:mat) "!=" (b:mat) :<30` | `INFIX_LEFT` (prec `30`) | `a:mat, none`, `b:mat, none` | `-> real, none` | `!(a == b)` | `[4, 2 ; 2, 3] != [1, 2 ; 3, 4]` | `1` (`real`) |
| `1079` | `(a:mat) "~"  (b:mat) :<30` | `INFIX_LEFT` (prec `30`) | `a:mat, none`, `b:mat, none` | `-> real, none` | `{ if (mat_shape(a) != mat_shape(b)) { return 0 } flatten(a) ~ flatten...` | `[4, 2 ; 2, 3] ~ [1, 2 ; 3, 4]` | `0` (`real`) |

### Module `[algebra]` (`Library/algebra.math` — 28 Overloads)

`$mat2(a, b, c, d)` struct, Cramer's 2x2 solver, real & complex quadratic root solvers, and symbolic/numerical polynomial algebra on coefficient vectors.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `1080` | `(m1:mat2) "+"  (m2:mat2) :<50 -> mat2` | `INFIX_LEFT` (prec `50`) | `m1:mat2, none`, `m2:mat2, none` | `-> mat2, none` | `mat2(m1.a + m2.a, m1.b + m2.b, m1.c + m2.c, m1.d + m2.d)` | `mat2(4, 7, 2, 6) + mat2(1, 2, 3, 4)` | `mat2(a=5, b=9, c=5, d=10)` (`mat2`) |
| `1081` | `(m1:mat2) "-"  (m2:mat2) :<50 -> mat2` | `INFIX_LEFT` (prec `50`) | `m1:mat2, none`, `m2:mat2, none` | `-> mat2, none` | `mat2(m1.a - m2.a, m1.b - m2.b, m1.c - m2.c, m1.d - m2.d)` | `mat2(4, 7, 2, 6) - mat2(1, 2, 3, 4)` | `mat2(a=3, b=5, c=-1, d=2)` (`mat2`) |
| `1082` | `(m1:mat2) into (m2:mat2) :>55 -> mat2` | `INFIX_RIGHT` (prec `55`) | `m1:mat2, none`, `m2:mat2, none` | `-> mat2, none` | `mat2(m1.a + m2.a, m1.b + m2.b, m1.c + m2.c, m1.d + m2.d)` | `mat2(4, 7, 2, 6) into mat2(1, 2, 3, 4)` | `mat2(a=5, b=9, c=5, d=10)` (`mat2`) |
| `1083` | `(m1:mat2) "*"  (m2:mat2) :<60 -> mat2` | `INFIX_LEFT` (prec `60`) | `m1:mat2, none`, `m2:mat2, none` | `-> mat2, none` | `mat2(m1.a * m2.a + m1.b * m2.c, m1.a * m2.b + m1.b * m2.d,` | `mat2(4, 7, 2, 6) * mat2(1, 2, 3, 4)` | `mat2(a=25, b=36, c=20, d=28)` (`mat2`) |
| `1084` | `(m:mat2) "*" (v:vec2) :<60 -> vec2` | `INFIX_LEFT` (prec `60`) | `m:mat2, none`, `v:vec2, none` | `-> vec2, none` | `vec2(m.a * v.x + m.b * v.y, m.c * v.x + m.d * v.y)` | `mat2(4, 7, 2, 6) * vec2(1, 2)` | `vec2(x=18, y=14)` (`vec2`) |
| `1085` | `"-" (m:mat2)             :80  -> mat2` | `PREFIX` (prec `80`) | `m:mat2, none` | `-> mat2, none` | `mat2(-m.a, -m.b, -m.c, -m.d)` | `-mat2(4, 7, 2, 6)` | `mat2(a=-4, b=-7, c=-2, d=-6)` (`mat2`) |
| `1086` | `(s)      "*" (m:mat2) :<60 -> mat2` | `INFIX_LEFT` (prec `60`) | `s:real, none`, `m:mat2, none` | `-> mat2, none` | `mat2(s * m.a, s * m.b, s * m.c, s * m.d)` | `6 * mat2(1, 2, 3, 4)` | `mat2(a=6, b=12, c=18, d=24)` (`mat2`) |
| `1087` | `(m:mat2) "*" (s)      :<60 -> mat2` | `INFIX_LEFT` (prec `60`) | `m:mat2, none`, `s:real, none` | `-> mat2, none` | `mat2(m.a * s, m.b * s, m.c * s, m.d * s)` | `mat2(4, 7, 2, 6) * 3` | `mat2(a=12, b=21, c=6, d=18)` (`mat2`) |
| `1088` | `(m:mat2) "/" (s)      :<60 -> mat2` | `INFIX_LEFT` (prec `60`) | `m:mat2, none`, `s:real, none` | `-> mat2, none` | `mat2(m.a / s, m.b / s, m.c / s, m.d / s)` | `mat2(4, 7, 2, 6) / 3` | `mat2(a=1.33333, b=2.33333, c=0.666667, d=2)` (`mat2`) |
| `1089` | `(m1:mat2) "==" (m2:mat2) :<30` | `INFIX_LEFT` (prec `30`) | `m1:mat2, none`, `m2:mat2, none` | `-> real, none` | `(m1.a == m2.a) && (m1.b == m2.b) && (m1.c == m2.c) && (m1.d == m2.d)` | `mat2(4, 7, 2, 6) == mat2(1, 2, 3, 4)` | `0` (`real`) |
| `1090` | `(m1:mat2) "!=" (m2:mat2) :<30` | `INFIX_LEFT` (prec `30`) | `m1:mat2, none`, `m2:mat2, none` | `-> real, none` | `(m1.a != m2.a) \|\| (m1.b != m2.b) \|\| (m1.c != m2.c) \|\| (m1.d != m2.d)` | `mat2(4, 7, 2, 6) != mat2(1, 2, 3, 4)` | `1` (`real`) |
| `1091` | `(m1:mat2) "~"  (m2:mat2) :<30` | `INFIX_LEFT` (prec `30`) | `m1:mat2, none`, `m2:mat2, none` | `-> real, none` | `(m1.a ~ m2.a) && (m1.b ~ m2.b) && (m1.c ~ m2.c) && (m1.d ~ m2.d)` | `mat2(4, 7, 2, 6) ~ mat2(1, 2, 3, 4)` | `0` (`real`) |
| `1092` | `det(m:mat2)` | `FUNC` | `m:mat2, none` | `-> real, none` | `m.a * m.d - m.b * m.c` | `det(mat2(4, 7, 2, 6))` | `10` (`real`) |
| `1093` | `trace(m:mat2)` | `FUNC` | `m:mat2, none` | `-> real, none` | `m.a + m.d` | `trace(mat2(4, 7, 2, 6))` | `10` (`real`) |
| `1094` | `transpose(m:mat2) -> mat2` | `FUNC` | `m:mat2, none` | `-> mat2, none` | `mat2(m.a, m.c, m.b, m.d)` | `transpose(mat2(4, 7, 2, 6))` | `mat2(a=4, b=2, c=7, d=6)` (`mat2`) |
| `1095` | `inv_mat2(m:mat2) -> mat2` | `FUNC` | `m:mat2, none` | `-> mat2, none` | `{ d := det(m) mat2(m.d / d, -m.b / d, -m.c / d, m.a / d) }` | `inv_mat2(mat2(4, 7, 2, 6))` | `mat2(a=0.6, b=-0.7, c=-0.2, d=0.4)` (`mat2`) |
| `1096` | `solve_2x2(a1, b1, c1, a2, b2, c2) -> vec2` | `FUNC` | `a1:real, none`, `b1:real, none`, `c1:real, none`, `a2:real, none`, `b2:real, none`, `c2:real, none` | `-> vec2, none` | `{ d  := a1 * b2 - b1 * a2 dx := c1 * b2 - b1 * c2 dy := a1 * c2 - c1 ...` | `solve_2x2(2, 3, 8, 5, -1, 3)` | `vec2(x=1, y=2)` (`vec2`) |
| `1097` | `quad_disc(a, b, c)` | `FUNC` | `a:real, none`, `b:real, none`, `c:real, none` | `-> real, none` | `b^2 - 4a * c` | `quad_disc(6, 3, 3)` | `-63` (`real`) |
| `1098` | `quad_roots(a, b, c) -> vec2` | `FUNC` | `a:real, none`, `b:real, none`, `c:real, none` | `-> vec2, none` | `{ sd := √(b^2 - 4a * c) vec2((-b + sd) / (2a), (-b - sd) / (2a)) }` | `quad_roots(1, -5, 6)` | `vec2(x=3, y=2)` (`vec2`) |
| `1099` | `quad_croot(a, b, c) -> complex` | `FUNC` | `a:real, none`, `b:real, none`, `c:real, none` | `-> complex, none` | `{ d := b^2 - 4a * c if (d >= 0) { complex((-b + √d) / (2a), 0) } else...` | `quad_croot(1, 2, 5)` | `-1 + 2i` (`complex`) |
| `1100` | `poly_val(coeffs:vec, x)` | `FUNC` | `coeffs:vec, none`, `x:real, none` | `-> real, none` | `{ n   := coeffs.len() acc := 0 idx := n - 1 (idx >= 0) ?? { acc = acc...` | `poly_val(vec(6, -5, 1), 4)` | `2` (`real`) |
| `1101` | `poly_deriv(coeffs:vec) -> vec` | `FUNC` | `coeffs:vec, none` | `-> vec, none` | `{ n   := coeffs.len() out := vec<real>() if (n <= 1) { 0 into out } e...` | `poly_deriv(vec(6, -5, 1))` | `[-5, 2]` (`vec`) |
| `1102` | `poly_integ(coeffs:vec, c0 = 0) -> vec` | `FUNC` | `coeffs:vec, none`, `c0:real, none = 0` | `-> vec, none` | `{ n   := coeffs.len() out := vec<real>() c0 into out idx := 0 (idx < ...` | `poly_integ(vec(6, -5, 1), 4)` | `[4, 6, -2.5, 0.333333]` (`vec`) |
| `1103` | `poly_integ(coeffs) [default wrapper: c0=0]` | `FUNC` | `coeffs:vec, none` | `-> vec, none` | `Calls poly_integ(...) with c0=0` | `poly_integ(vec(6, -5, 1))` | `[0, 6, -2.5, 0.333333]` (`vec`) |
| `1104` | `poly_add(p:vec, q:vec) -> vec` | `FUNC` | `p:vec, none`, `q:vec, none` | `-> vec, none` | `{ np  := p.len() nq  := q.len() n   := max(np, nq) out := vec<real>()...` | `poly_add(list(-2, 1), list(-3, 2))` | `[-5, 3]` (`vec`) |
| `1105` | `poly_mul(p:vec, q:vec) -> vec` | `FUNC` | `p:vec, none`, `q:vec, none` | `-> vec, none` | `{ np  := p.len() nq  := q.len() deg := np + nq - 1 out := vec<real>(d...` | `poly_mul(list(-2, 1), list(-3, 1))` | `[6, -5, 1]` (`vec`) |
| `1106` | `poly_newton(coeffs:vec, x0, steps = 12)` | `FUNC` | `coeffs:vec, none`, `x0:real, none`, `steps:real, none = 12` | `-> real, none` | `{ dp := poly_deriv(coeffs) x  := x0 k  := 0 (k < steps) ?? { fx  := p...` | `poly_newton(vec(-9, 0, 1), 5, 10)` | `3` (`real`) |
| `1107` | `poly_newton(coeffs, x0) [default wrapper: steps=12]` | `FUNC` | `coeffs:vec, none`, `x0:real, none` | `-> real, none` | `Calls poly_newton(...) with steps=12` | `poly_newton(vec(-9, 0, 1), 5)` | `3` (`real`) |

### Module `[physics]` (`Library/physics.math` — 100 Overloads)

14 CODATA constants, SUVAT kinematics, projectiles, collisions, rotational/SHM mechanics, orbital astrophysics, fluids, thermodynamics, circuits, optics, quantum, and relativity.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `1108` | `suvat_v(u, a, t)` | `FUNC` | `u:real, none`, `a:real, none`, `t:real, none` | `-> real, none` | `u + a * t` | `suvat_v(6, 3, 3)` | `15` (`real`) |
| `1109` | `suvat_s(u, a, t)` | `FUNC` | `u:real, none`, `a:real, none`, `t:real, none` | `-> real, none` | `u * t + 0.5 * a * (t ^ 2)` | `suvat_s(6, 3, 3)` | `31.5` (`real`) |
| `1110` | `suvat_v2(u, a, s)` | `FUNC` | `u:real, none`, `a:real, none`, `s:real, none` | `-> real, none` | `√((u ^ 2) + 2a * s)` | `suvat_v2(6, 3, 3)` | `7.348469228` (`real`) |
| `1111` | `suvat_s_avg(u, v, t)` | `FUNC` | `u:real, none`, `v:real, none`, `t:real, none` | `-> real, none` | `0.5 * (u + v) * t` | `suvat_s_avg(6, 3, 3)` | `13.5` (`real`) |
| `1112` | `suvat_nth_sec(u, a, n)` | `FUNC` | `u:real, none`, `a:real, none`, `n:real, none` | `-> real, none` | `u + 0.5 * a * (2n - 1)` | `suvat_nth_sec(6, 3, 3)` | `13.5` (`real`) |
| `1113` | `ke(mass, vel)` | `FUNC` | `mass:real, none`, `vel:real, none` | `-> real, none` | `0.5 * mass * (vel ^ 2)` | `ke(6, 3)` | `27` (`real`) |
| `1114` | `pe(mass, height)` | `FUNC` | `mass:real, none`, `height:real, none` | `-> real, none` | `mass * g0 * height` | `pe(6, 3)` | `176.5197` (`real`) |
| `1115` | `mech_energy(mass, vel, height)` | `FUNC` | `mass:real, none`, `vel:real, none`, `height:real, none` | `-> real, none` | `ke(mass, vel) + pe(mass, height)` | `mech_energy(6, 3, 3)` | `203.5197` (`real`) |
| `1116` | `proj_range(vel, angle_deg)` | `FUNC` | `vel:real, none`, `angle_deg:real, none` | `-> real, none` | `(vel ^ 2) * sin(2 * deg2rad(angle_deg)) / g0` | `proj_range(6, 3)` | `0.3837217274` (`real`) |
| `1117` | `proj_max_h(vel, angle_deg)` | `FUNC` | `vel:real, none`, `angle_deg:real, none` | `-> real, none` | `((vel * sin(deg2rad(angle_deg))) ^ 2) / (2 * g0)` | `proj_max_h(6, 3)` | `0.005027500898` (`real`) |
| `1118` | `proj_time(vel, angle_deg)` | `FUNC` | `vel:real, none`, `angle_deg:real, none` | `-> real, none` | `2 * vel * sin(deg2rad(angle_deg)) / g0` | `proj_time(6, 3)` | `0.06404138772` (`real`) |
| `1119` | `momentum(mass, vel)` | `FUNC` | `mass:real, none`, `vel:real, none` | `-> real, none` | `mass * vel` | `momentum(6, 3)` | `18` (`real`) |
| `1120` | `impulse(force_f, dt)` | `FUNC` | `force_f:real, none`, `dt:real, none` | `-> real, none` | `force_f * dt` | `impulse(6, 3)` | `18` (`real`) |
| `1121` | `recoil_vel(m_gun, m_bullet, v_bullet)` | `FUNC` | `m_gun:real, none`, `m_bullet:real, none`, `v_bullet:real, none` | `-> real, none` | `-(m_bullet * v_bullet) / m_gun` | `recoil_vel(6, 3, 3)` | `-1.5` (`real`) |
| `1122` | `friction_force(mu, normal_n)` | `FUNC` | `mu:real, none`, `normal_n:real, none` | `-> real, none` | `mu * normal_n` | `friction_force(6, 3)` | `18` (`real`) |
| `1123` | `incline_normal(mass, theta_deg)` | `FUNC` | `mass:real, none`, `theta_deg:real, none` | `-> real, none` | `mass * g0 * cos(deg2rad(theta_deg))` | `incline_normal(6, 3)` | `58.75926196` (`real`) |
| `1124` | `incline_slide_acc(theta_deg, mu_k)` | `FUNC` | `theta_deg:real, none`, `mu_k:real, none` | `-> real, none` | `g0 * (sin(deg2rad(theta_deg)) - mu_k * cos(deg2rad(theta_deg)))` | `incline_slide_acc(6, 3)` | `-28.23371038` (`real`) |
| `1125` | `banking_angle_deg(vel, r)` | `FUNC` | `vel:real, none`, `r:real, none` | `-> real, none` | `rad2deg(atan((vel ^ 2) / (r * g0)))` | `banking_angle_deg(6, 3)` | `50.74359039` (`real`) |
| `1126` | `elastic_v1(m1, u1, m2, u2)` | `FUNC` | `m1:real, none`, `u1:real, none`, `m2:real, none`, `u2:real, none` | `-> real, none` | `((m1 - m2) * u1 + 2 * m2 * u2) / (m1 + m2)` | `elastic_v1(6, 3, 3, 3)` | `3` (`real`) |
| `1127` | `elastic_v2(m1, u1, m2, u2)` | `FUNC` | `m1:real, none`, `u1:real, none`, `m2:real, none`, `u2:real, none` | `-> real, none` | `((m2 - m1) * u2 + 2 * m1 * u1) / (m1 + m2)` | `elastic_v2(6, 3, 3, 3)` | `3` (`real`) |
| `1128` | `inelastic_v(m1, u1, m2, u2)` | `FUNC` | `m1:real, none`, `u1:real, none`, `m2:real, none`, `u2:real, none` | `-> real, none` | `(m1 * u1 + m2 * u2) / (m1 + m2)` | `inelastic_v(6, 3, 3, 3)` | `3` (`real`) |
| `1129` | `centripetal_acc(vel, r)` | `FUNC` | `vel:real, none`, `r:real, none` | `-> real, none` | `(vel ^ 2) / r` | `centripetal_acc(6, 3)` | `12` (`real`) |
| `1130` | `centripetal_force(mass, vel, r)` | `FUNC` | `mass:real, none`, `vel:real, none`, `r:real, none` | `-> real, none` | `mass * (vel ^ 2) / r` | `centripetal_force(6, 3, 3)` | `18` (`real`) |
| `1131` | `torque_val(r, force_f, theta_deg)` | `FUNC` | `r:real, none`, `force_f:real, none`, `theta_deg:real, none` | `-> real, none` | `r * force_f * sin(deg2rad(theta_deg))` | `torque_val(6, 3, 3)` | `0.9420472124` (`real`) |
| `1132` | `ang_momentum(inertia_i, omega)` | `FUNC` | `inertia_i:real, none`, `omega:real, none` | `-> real, none` | `inertia_i * omega` | `ang_momentum(6, 3)` | `18` (`real`) |
| `1133` | `rot_ke(inertia_i, omega)` | `FUNC` | `inertia_i:real, none`, `omega:real, none` | `-> real, none` | `0.5 * inertia_i * (omega ^ 2)` | `rot_ke(6, 3)` | `27` (`real`) |
| `1134` | `hooke_force(k_spring, disp)` | `FUNC` | `k_spring:real, none`, `disp:real, none` | `-> real, none` | `-k_spring * disp` | `hooke_force(6, 3)` | `-18` (`real`) |
| `1135` | `elastic_pe(k_spring, disp)` | `FUNC` | `k_spring:real, none`, `disp:real, none` | `-> real, none` | `0.5 * k_spring * (disp ^ 2)` | `elastic_pe(6, 3)` | `27` (`real`) |
| `1136` | `pendulum_period(length_m)` | `FUNC` | `length_m:real, none` | `-> real, none` | `2π * √(length_m / g0)` | `pendulum_period(6)` | `4.914678982` (`real`) |
| `1137` | `shm_omega(k_spring, mass)` | `FUNC` | `k_spring:real, none`, `mass:real, none` | `-> real, none` | `√(k_spring / mass)` | `shm_omega(6, 3)` | `1.414213562` (`real`) |
| `1138` | `shm_period_spring(mass, k_spring)` | `FUNC` | `mass:real, none`, `k_spring:real, none` | `-> real, none` | `2π * √(mass / k_spring)` | `shm_period_spring(6, 3)` | `8.885765876` (`real`) |
| `1139` | `shm_vel(omega, amp_a, disp_x)` | `FUNC` | `omega:real, none`, `amp_a:real, none`, `disp_x:real, none` | `-> real, none` | `omega * √((amp_a ^ 2) - (disp_x ^ 2))` | `shm_vel(6, 3, 3)` | `0` (`real`) |
| `1140` | `shm_acc(omega, disp_x)` | `FUNC` | `omega:real, none`, `disp_x:real, none` | `-> real, none` | `-(omega ^ 2) * disp_x` | `shm_acc(6, 3)` | `-108` (`real`) |
| `1141` | `grav_force(m1, m2, r)` | `FUNC` | `m1:real, none`, `m2:real, none`, `r:real, none` | `-> real, none` | `G_grav * m1 * m2 / (r ^ 2)` | `grav_force(6, 3, 3)` | `1.33486e-10` (`real`) |
| `1142` | `grav_pe(m1, m2, r)` | `FUNC` | `m1:real, none`, `m2:real, none`, `r:real, none` | `-> real, none` | `-G_grav * m1 * m2 / r` | `grav_pe(6, 3, 3)` | `-4.00458e-10` (`real`) |
| `1143` | `orbital_vel(mass_body, r)` | `FUNC` | `mass_body:real, none`, `r:real, none` | `-> real, none` | `√(G_grav * mass_body / r)` | `orbital_vel(6, 3)` | `1.155361415e-05` (`real`) |
| `1144` | `escape_vel(mass_body, r)` | `FUNC` | `mass_body:real, none`, `r:real, none` | `-> real, none` | `√(2 * G_grav * mass_body / r)` | `escape_vel(6, 3)` | `1.633927783e-05` (`real`) |
| `1145` | `schwarzschild_r(mass_body)` | `FUNC` | `mass_body:real, none` | `-> real, none` | `2 * G_grav * mass_body / (c0 ^ 2)` | `schwarzschild_r(6)` | `8.911392323e-27` (`real`) |
| `1146` | `kepler_period(semi_major_a, mass_body)` | `FUNC` | `semi_major_a:real, none`, `mass_body:real, none` | `-> real, none` | `2π * √((semi_major_a ^ 3) / (G_grav * mass_body))` | `kepler_period(6, 3)` | `6525942.678` (`real`) |
| `1147` | `hydro_pressure(rho, depth_h)` | `FUNC` | `rho:real, none`, `depth_h:real, none` | `-> real, none` | `rho * g0 * depth_h` | `hydro_pressure(6, 3)` | `176.5197` (`real`) |
| `1148` | `buoyant_force(rho_fluid, vol_sub)` | `FUNC` | `rho_fluid:real, none`, `vol_sub:real, none` | `-> real, none` | `rho_fluid * g0 * vol_sub` | `buoyant_force(6, 3)` | `176.5197` (`real`) |
| `1149` | `continuity_v2(a1, v1, a2)` | `FUNC` | `a1:real, none`, `v1:real, none`, `a2:real, none` | `-> real, none` | `(a1 * v1) / a2` | `continuity_v2(6, 3, 3)` | `6` (`real`) |
| `1150` | `stokes_drag(eta, r, vel)` | `FUNC` | `eta:real, none`, `r:real, none`, `vel:real, none` | `-> real, none` | `6π * eta * r * vel` | `stokes_drag(6, 3, 3)` | `1017.87602` (`real`) |
| `1151` | `terminal_vel(r, rho_sphere, rho_fluid, eta)` | `FUNC` | `r:real, none`, `rho_sphere:real, none`, `rho_fluid:real, none`, `eta:real, none` | `-> real, none` | `(2 * (r ^ 2) * (rho_sphere - rho_fluid) * g0) / (9 * eta)` | `terminal_vel(6, 3, 3, 3)` | `0` (`real`) |
| `1152` | `stress_val(force_f, area)` | `FUNC` | `force_f:real, none`, `area:real, none` | `-> real, none` | `force_f / area` | `stress_val(6, 3)` | `2` (`real`) |
| `1153` | `strain_val(dl, l0)` | `FUNC` | `dl:real, none`, `l0:real, none` | `-> real, none` | `dl / l0` | `strain_val(6, 3)` | `2` (`real`) |
| `1154` | `youngs_modulus(force_f, l0, area, dl)` | `FUNC` | `force_f:real, none`, `l0:real, none`, `area:real, none`, `dl:real, none` | `-> real, none` | `(force_f * l0) / (area * dl)` | `youngs_modulus(6, 3, 3, 3)` | `2` (`real`) |
| `1155` | `sensible_heat(mass, c_spec, dt)` | `FUNC` | `mass:real, none`, `c_spec:real, none`, `dt:real, none` | `-> real, none` | `mass * c_spec * dt` | `sensible_heat(6, 3, 3)` | `54` (`real`) |
| `1156` | `latent_heat(mass, l_spec)` | `FUNC` | `mass:real, none`, `l_spec:real, none` | `-> real, none` | `mass * l_spec` | `latent_heat(6, 3)` | `18` (`real`) |
| `1157` | `thermal_expansion_l(l0, alpha, dt)` | `FUNC` | `l0:real, none`, `alpha:real, none`, `dt:real, none` | `-> real, none` | `l0 * (1 + alpha * dt)` | `thermal_expansion_l(6, 3, 3)` | `60` (`real`) |
| `1158` | `thermal_cond_rate(k_cond, area, dt, thickness)` | `FUNC` | `k_cond:real, none`, `area:real, none`, `dt:real, none`, `thickness:real, none` | `-> real, none` | `(k_cond * area * dt) / thickness` | `thermal_cond_rate(6, 3, 3, 3)` | `18` (`real`) |
| `1159` | `wien_peak_lambda(temp_k)` | `FUNC` | `temp_k:real, none` | `-> real, none` | `b_wien / temp_k` | `wien_peak_lambda(6)` | `0.0004829619925` (`real`) |
| `1160` | `stefan_boltzmann_p(area, emissivity, temp_k)` | `FUNC` | `area:real, none`, `emissivity:real, none`, `temp_k:real, none` | `-> real, none` | `area * emissivity * sigma_sb * (temp_k ^ 4)` | `stefan_boltzmann_p(6, 3, 3)` | `8.267405903e-05` (`real`) |
| `1161` | `carnot_eff(t_cold_k, t_hot_k)` | `FUNC` | `t_cold_k:real, none`, `t_hot_k:real, none` | `-> real, none` | `1 - (t_cold_k / t_hot_k)` | `carnot_eff(6, 3)` | `-1` (`real`) |
| `1162` | `coulomb_force(q1, q2, r)` | `FUNC` | `q1:real, none`, `q2:real, none`, `r:real, none` | `-> real, none` | `ke_coulomb * q1 * q2 / (r ^ 2)` | `coulomb_force(6, 3, 3)` | `1.797510358e+10` (`real`) |
| `1163` | `ohm_v(current_i, res_r)` | `FUNC` | `current_i:real, none`, `res_r:real, none` | `-> real, none` | `current_i * res_r` | `ohm_v(6, 3)` | `18` (`real`) |
| `1164` | `ohm_i(volt_v, res_r)` | `FUNC` | `volt_v:real, none`, `res_r:real, none` | `-> real, none` | `volt_v / res_r` | `ohm_i(6, 3)` | `2` (`real`) |
| `1165` | `ohm_r(volt_v, current_i)` | `FUNC` | `volt_v:real, none`, `current_i:real, none` | `-> real, none` | `volt_v / current_i` | `ohm_r(6, 3)` | `2` (`real`) |
| `1166` | `elec_power(volt_v, current_i)` | `FUNC` | `volt_v:real, none`, `current_i:real, none` | `-> real, none` | `volt_v * current_i` | `elec_power(6, 3)` | `18` (`real`) |
| `1167` | `cap_energy(cap_c, volt_v)` | `FUNC` | `cap_c:real, none`, `volt_v:real, none` | `-> real, none` | `0.5 * cap_c * (volt_v ^ 2)` | `cap_energy(6, 3)` | `27` (`real`) |
| `1168` | `ind_energy(ind_l, current_i)` | `FUNC` | `ind_l:real, none`, `current_i:real, none` | `-> real, none` | `0.5 * ind_l * (current_i ^ 2)` | `ind_energy(6, 3)` | `27` (`real`) |
| `1169` | `res_series(r1, r2)` | `FUNC` | `r1:real, none`, `r2:real, none` | `-> real, none` | `r1 + r2` | `res_series(6, 3)` | `9` (`real`) |
| `1170` | `res_parallel(r1, r2)` | `FUNC` | `r1:real, none`, `r2:real, none` | `-> real, none` | `(r1 * r2) / (r1 + r2)` | `res_parallel(6, 3)` | `2` (`real`) |
| `1171` | `cap_series(c1, c2)` | `FUNC` | `c1:real, none`, `c2:real, none` | `-> real, none` | `(c1 * c2) / (c1 + c2)` | `cap_series(6, 3)` | `2` (`real`) |
| `1172` | `cap_parallel(c1, c2)` | `FUNC` | `c1:real, none`, `c2:real, none` | `-> real, none` | `c1 + c2` | `cap_parallel(6, 3)` | `9` (`real`) |
| `1173` | `parallel_plate_cap(area, sep_d)` | `FUNC` | `area:real, none`, `sep_d:real, none` | `-> real, none` | `eps0 * area / sep_d` | `parallel_plate_cap(6, 3)` | `1.770837563e-11` (`real`) |
| `1174` | `rc_tau(res_r, cap_c)` | `FUNC` | `res_r:real, none`, `cap_c:real, none` | `-> real, none` | `res_r * cap_c` | `rc_tau(6, 3)` | `18` (`real`) |
| `1175` | `wheatstone_r4(r1, r2, r3)` | `FUNC` | `r1:real, none`, `r2:real, none`, `r3:real, none` | `-> real, none` | `(r2 * r3) / r1` | `wheatstone_r4(6, 3, 3)` | `1.5` (`real`) |
| `1176` | `drift_velocity(current_i, n_density, area, q_charge)` | `FUNC` | `current_i:real, none`, `n_density:real, none`, `area:real, none`, `q_charge:real, none` | `-> real, none` | `current_i / (n_density * area * q_charge)` | `drift_velocity(6, 3, 3, 3)` | `0.2222222222` (`real`) |
| `1177` | `lorentz_mag_force(q, vel, b_field, theta_deg)` | `FUNC` | `q:real, none`, `vel:real, none`, `b_field:real, none`, `theta_deg:real, none` | `-> real, none` | `q * vel * b_field * sin(deg2rad(theta_deg))` | `lorentz_mag_force(6, 3, 3, 3)` | `2.826141637` (`real`) |
| `1178` | `cyclotron_radius(mass, vel, q, b_field)` | `FUNC` | `mass:real, none`, `vel:real, none`, `q:real, none`, `b_field:real, none` | `-> real, none` | `(mass * vel) / (q * b_field)` | `cyclotron_radius(6, 3, 3, 3)` | `2` (`real`) |
| `1179` | `solenoid_b(n_turns_per_m, current_i)` | `FUNC` | `n_turns_per_m:real, none`, `current_i:real, none` | `-> real, none` | `mu0 * n_turns_per_m * current_i` | `solenoid_b(6, 3)` | `2.261946712e-05` (`real`) |
| `1180` | `inductive_reactance(freq_hz, ind_l)` | `FUNC` | `freq_hz:real, none`, `ind_l:real, none` | `-> real, none` | `2π * freq_hz * ind_l` | `inductive_reactance(6, 3)` | `113.0973355` (`real`) |
| `1181` | `capacitive_reactance(freq_hz, cap_c)` | `FUNC` | `freq_hz:real, none`, `cap_c:real, none` | `-> real, none` | `1 / (2π * freq_hz * cap_c)` | `capacitive_reactance(6, 3)` | `0.008841941283` (`real`) |
| `1182` | `rlc_impedance(res_r, xl, xc)` | `FUNC` | `res_r:real, none`, `xl:real, none`, `xc:real, none` | `-> real, none` | `√((res_r ^ 2) + ((xl - xc) ^ 2))` | `rlc_impedance(6, 3, 3)` | `6` (`real`) |
| `1183` | `rlc_resonant_freq(ind_l, cap_c)` | `FUNC` | `ind_l:real, none`, `cap_c:real, none` | `-> real, none` | `1 / (2π * √(ind_l * cap_c))` | `rlc_resonant_freq(6, 3)` | `0.03751317984` (`real`) |
| `1184` | `transformer_v2(v1, n1, n2)` | `FUNC` | `v1:real, none`, `n1:real, none`, `n2:real, none` | `-> real, none` | `v1 * (n2 / n1)` | `transformer_v2(6, 3, 3)` | `6` (`real`) |
| `1185` | `doppler_freq(f0, v_sound, v_obs, v_src)` | `FUNC` | `f0:real, none`, `v_sound:real, none`, `v_obs:real, none`, `v_src:real, none` | `-> real, none` | `f0 * ((v_sound + v_obs) / (v_sound - v_src))` | `doppler_freq(440, 340, 20, 15)` | `487.3846154` (`real`) |
| `1186` | `beat_freq(f1, f2)` | `FUNC` | `f1:real, none`, `f2:real, none` | `-> real, none` | `@abs(f1 - f2)` | `beat_freq(6, 3)` | `3` (`real`) |
| `1187` | `sound_db(intensity_i)` | `FUNC` | `intensity_i:real, none` | `-> real, none` | `10 * log10(intensity_i / (10 ^ -12))` | `sound_db(6)` | `127.7815125` (`real`) |
| `1188` | `snell_angle2(n1, theta1_deg, n2)` | `FUNC` | `n1:real, none`, `theta1_deg:real, none`, `n2:real, none` | `-> real, none` | `rad2deg(asin(n1 * sin(deg2rad(theta1_deg)) / n2))` | `snell_angle2(1.0, 30, 1.5)` | `19.47122063` (`real`) |
| `1189` | `lens_focal(do_dist, di_dist)` | `FUNC` | `do_dist:real, none`, `di_dist:real, none` | `-> real, none` | `(do_dist * di_dist) / (do_dist + di_dist)` | `lens_focal(6, 3)` | `2` (`real`) |
| `1190` | `mirror_image_v(f_len, u_obj)` | `FUNC` | `f_len:real, none`, `u_obj:real, none` | `-> real, none` | `(f_len * u_obj) / (u_obj - f_len)` | `mirror_image_v(6, 3)` | `-6` (`real`) |
| `1191` | `magnification_lens(v_img, u_obj)` | `FUNC` | `v_img:real, none`, `u_obj:real, none` | `-> real, none` | `v_img / u_obj` | `magnification_lens(6, 3)` | `2` (`real`) |
| `1192` | `lens_power_dioptre(f_meters)` | `FUNC` | `f_meters:real, none` | `-> real, none` | `1 / f_meters` | `lens_power_dioptre(6)` | `0.1666666667` (`real`) |
| `1193` | `young_fringe_width(wavelength_m, screen_d, slit_sep_d)` | `FUNC` | `wavelength_m:real, none`, `screen_d:real, none`, `slit_sep_d:real, none` | `-> real, none` | `(wavelength_m * screen_d) / slit_sep_d` | `young_fringe_width(6, 3, 3)` | `6` (`real`) |
| `1194` | `malus_law(i0, theta_deg)` | `FUNC` | `i0:real, none`, `theta_deg:real, none` | `-> real, none` | `i0 * (cos(deg2rad(theta_deg)) ^ 2)` | `malus_law(6, 3)` | `5.983565686` (`real`) |
| `1195` | `brewster_angle(n1, n2)` | `FUNC` | `n1:real, none`, `n2:real, none` | `-> real, none` | `rad2deg(atan(n2 / n1))` | `brewster_angle(6, 3)` | `26.56505118` (`real`) |
| `1196` | `photon_energy(freq_hz)` | `FUNC` | `freq_hz:real, none` | `-> real, none` | `h_planck * freq_hz` | `photon_energy(6)` | `3.97564209e-33` (`real`) |
| `1197` | `photon_wavelength(wavelength_m)` | `FUNC` | `wavelength_m:real, none` | `-> real, none` | `h_planck * c0 / wavelength_m` | `photon_wavelength(6)` | `3.310743095e-26` (`real`) |
| `1198` | `photoelectric_ke_max(freq_hz, work_fn_j)` | `FUNC` | `freq_hz:real, none`, `work_fn_j:real, none` | `-> real, none` | `h_planck * freq_hz - work_fn_j` | `photoelectric_ke_max(6, 3)` | `-3` (`real`) |
| `1199` | `de_broglie(mass, vel)` | `FUNC` | `mass:real, none`, `vel:real, none` | `-> real, none` | `h_planck / (mass * vel)` | `de_broglie(6, 3)` | `3.681150083e-35` (`real`) |
| `1200` | `bohr_radius_n(n_orbit)` | `FUNC` | `n_orbit:real, none` | `-> real, none` | `5.29177210903 * (10 ^ -11) * (n_orbit ^ 2)` | `bohr_radius_n(6)` | `1.905037959e-09` (`real`) |
| `1201` | `bohr_energy_ev(n_orbit, z_atomic = 1)` | `FUNC` | `n_orbit:real, none`, `z_atomic:real, none = 1` | `-> real, none` | `-13.6 * ((z_atomic ^ 2) / (n_orbit ^ 2))` | `bohr_energy_ev(6, 3)` | `-3.4` (`real`) |
| `1202` | `bohr_energy_ev(n_orbit) [default wrapper: z_atomic=1]` | `FUNC` | `n_orbit:real, none` | `-> real, none` | `Calls bohr_energy_ev(...) with z_atomic=1` | `bohr_energy_ev(6)` | `-0.3777777778` (`real`) |
| `1203` | `rydberg_wavelength(n1, n2)` | `FUNC` | `n1:real, none`, `n2:real, none` | `-> real, none` | `1 / (rydberg_R * ((1 / (n1 ^ 2)) - (1 / (n2 ^ 2))))` | `rydberg_wavelength(2, 3)` | `6.561122764e-07` (`real`) |
| `1204` | `lorentz_gamma(vel)` | `FUNC` | `vel:real, none` | `-> real, none` | `1 / √(1 - (vel / c0) ^ 2)` | `lorentz_gamma(0.6 * c0)` | `1.25` (`real`) |
| `1205` | `rel_energy(mass, vel)` | `FUNC` | `mass:real, none`, `vel:real, none` | `-> real, none` | `lorentz_gamma(vel) * mass * (c0 ^ 2)` | `rel_energy(1, 0.6 * c0)` | `1.123443973e+17` (`real`) |
| `1206` | `time_dilation(t_proper, vel)` | `FUNC` | `t_proper:real, none`, `vel:real, none` | `-> real, none` | `t_proper * lorentz_gamma(vel)` | `time_dilation(1, 0.6 * c0)` | `1.25` (`real`) |
| `1207` | `length_contraction(l_proper, vel)` | `FUNC` | `l_proper:real, none`, `vel:real, none` | `-> real, none` | `l_proper / lorentz_gamma(vel)` | `length_contraction(1, 0.6 * c0)` | `0.8` (`real`) |

### Module `[string]` (`Library/string.math` — 57 Overloads)

String concatenation, slicing, search, case conversion, trimming, padding, splitting/joining, `fmt`/`format` formatting, hex/bin formatters, and backtracking regular expressions.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `1208` | `(a:str) "+"  (b:str) :<50 -> str` | `INFIX_LEFT` (prec `50`) | `a:str, none`, `b:str, none` | `-> str, none` | `@add(a, b)` | `"MEP" + "Math"` | `"MEPMath"` (`str`) |
| `1209` | `(a:str) into (b:str) :>55 -> str` | `INFIX_RIGHT` (prec `55`) | `a:str, none`, `b:str, none` | `-> str, none` | `@add(a, b)` | `"MEP" into "Math"` | `"MEPMath"` (`str`) |
| `1210` | `(a:str) "==" (b:str) :<30` | `INFIX_LEFT` (prec `30`) | `a:str, none`, `b:str, none` | `-> real, none` | `@eq(a, b)` | `"MEP" == "Math"` | `0` (`real`) |
| `1211` | `(a:str) "!=" (b:str) :<30` | `INFIX_LEFT` (prec `30`) | `a:str, none`, `b:str, none` | `-> real, none` | `@ne(a, b)` | `"MEP" != "Math"` | `1` (`real`) |
| `1212` | `(a:str) "<"  (b:str) :<30` | `INFIX_LEFT` (prec `30`) | `a:str, none`, `b:str, none` | `-> real, none` | `@lt(a, b)` | `"MEP" < "Math"` | `1` (`real`) |
| `1213` | `(a:str) "<=" (b:str) :<30` | `INFIX_LEFT` (prec `30`) | `a:str, none`, `b:str, none` | `-> real, none` | `@le(a, b)` | `"MEP" <= "Math"` | `1` (`real`) |
| `1214` | `(a:str) ">"  (b:str) :<30` | `INFIX_LEFT` (prec `30`) | `a:str, none`, `b:str, none` | `-> real, none` | `@gt(a, b)` | `"MEP" > "Math"` | `0` (`real`) |
| `1215` | `(a:str) ">=" (b:str) :<30` | `INFIX_LEFT` (prec `30`) | `a:str, none`, `b:str, none` | `-> real, none` | `@ge(a, b)` | `"MEP" >= "Math"` | `0` (`real`) |
| `1216` | `len(s:str)` | `FUNC` | `s:str, none` | `-> real, none` | `@vlen(s)` | `len("MEP")` | `3` (`real`) |
| `1217` | `at(s:str, idx)     -> str` | `FUNC` | `s:str, none`, `idx:real, none` | `-> str, none` | `@vget(s, idx)` | `at("Planck", 0)` | `"P"` (`str`) |
| `1218` | `at(s:str, rng:vec) -> str` | `FUNC` | `s:str, none`, `rng:vec, none` | `-> str, none` | `{ n   := len(s) out := "" (i in rng) { k := (i < 0) ? (n + i) : i if ...` | `at("abcdef", 1...4)` | `"bcde"` (`str`) |
| `1219` | `is_empty(s:str)` | `FUNC` | `s:str, none` | `-> real, none` | `(len(s) == 0)` | `is_empty("MEP")` | `0` (`real`) |
| `1220` | `substr(s:str, start, stop) -> str` | `FUNC` | `s:str, none`, `start:real, none`, `stop:real, none` | `-> str, none` | `{ n := len(s) if (n == 0) { return("") } a := start < 0 ? n + start :...` | `substr("MEP", 3, 3)` | `""` (`str`) |
| `1221` | `str_repeat(s:str, n) -> str` | `FUNC` | `s:str, none`, `n:real, none` | `-> str, none` | `{ out := "" k   := 0 (k < n) ?? { out = out + s k  += 1 } out }` | `str_repeat("MEP", 3)` | `"MEPMEPMEP"` (`str`) |
| `1222` | `str_reverse(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `{ n := len(s) if (n <= 1) { return s } s[(n - 1)...0] }` | `str_reverse("MEP")` | `"PEM"` (`str`) |
| `1223` | `starts_with(s:str, prefix:str)` | `FUNC` | `s:str, none`, `prefix:str, none` | `-> real, none` | `{ np := len(prefix) if (np > len(s)) { return 0 } if (np == 0) { retu...` | `starts_with("MEP", "Math")` | `0` (`real`) |
| `1224` | `ends_with(s:str, suffix:str)` | `FUNC` | `s:str, none`, `suffix:str, none` | `-> real, none` | `{ ns := len(suffix) n  := len(s) if (ns > n) { return 0 } if (ns == 0...` | `ends_with("MEP", "Math")` | `0` (`real`) |
| `1225` | `chars(s:str) -> vec` | `FUNC` | `s:str, none` | `-> vec, none` | `[c for c in s]` | `chars("MEP")` | `["M", "E", "P"]` (`vec`) |
| `1226` | `str_join(v:vec, sep:str = "") -> str` | `FUNC` | `v:vec, none`, `sep:str, none = ""` | `-> str, none` | `{ out := "" (i, item in v) { if (i > 0) { out = out + sep } out = out...` | `str_join(vec(2, 4, 6), "Math")` | `"2Math4Math6"` (`str`) |
| `1227` | `str_join(v) [default wrapper: sep=""]` | `FUNC` | `v:vec, none` | `-> str, none` | `Calls str_join(...) with sep=""` | `str_join(vec(2, 4, 6))` | `"246"` (`str`) |
| `1228` | `fmt(f:str, a:T)                     -> str` | `FUNC` | `f:str, none`, `a:T, none` | `-> str, none` | `@str_fmt(f, a)` | `fmt("{}", 10)` | `"10"` (`str`) |
| `1229` | `fmt(f:str, a:T, b:T)                -> str` | `FUNC` | `f:str, none`, `a:T, none`, `b:T, none` | `-> str, none` | `@str_fmt(f, a, b)` | `fmt("{} + {}", 10, 20)` | `"10 + 20"` (`str`) |
| `1230` | `fmt(f:str, a:T, b:T, c:T)           -> str` | `FUNC` | `f:str, none`, `a:T, none`, `b:T, none`, `c:T, none` | `-> str, none` | `@str_fmt(@str_fmt(f, a, b), c)` | `fmt("{} + {} + {}", 10, 20, 30)` | `"10 + 20 + 30"` (`str`) |
| `1231` | `fmt(f:str, a:T, b:T, c:T, d:T)      -> str` | `FUNC` | `f:str, none`, `a:T, none`, `b:T, none`, `c:T, none`, `d:T, none` | `-> str, none` | `@str_fmt(@str_fmt(f, a, b), c, d)` | `fmt("{} + {} + {} + {}", 10, 20, 30, 40)` | `"10 + 20 + 30 + 40"` (`str`) |
| `1232` | `format(f:str, a:T)                  -> str` | `FUNC` | `f:str, none`, `a:T, none` | `-> str, none` | `@str_fmt(f, a)` | `format("%g", 10)` | `"10"` (`str`) |
| `1233` | `format(f:str, a:T, b:T)             -> str` | `FUNC` | `f:str, none`, `a:T, none`, `b:T, none` | `-> str, none` | `@str_fmt(f, a, b)` | `format("%g-%g", 10, 20)` | `"10-20"` (`str`) |
| `1234` | `format(f:str, a:T, b:T, c:T)        -> str` | `FUNC` | `f:str, none`, `a:T, none`, `b:T, none`, `c:T, none` | `-> str, none` | `@str_fmt(@str_fmt(f, a, b), c)` | `format("%g-%g-%g", 10, 20, 30)` | `"10-20-30"` (`str`) |
| `1235` | `format(f:str, a:T, b:T, c:T, d:T)   -> str` | `FUNC` | `f:str, none`, `a:T, none`, `b:T, none`, `c:T, none`, `d:T, none` | `-> str, none` | `@str_fmt(@str_fmt(f, a, b), c, d)` | `format("%g-%g-%g-%g", 10, 20, 30, 40)` | `"10-20-30-40"` (`str`) |
| `1236` | `to_hex(n:real, none) -> str` | `FUNC` | `n:real, none (locked)` | `-> str, none` | `{ if (n != floor(n)) { error("Base formatting requires an exact integ...` | `to_hex(6)` | `"0x6"` (`str`) |
| `1237` | `to_bin(n:real, none) -> str` | `FUNC` | `n:real, none (locked)` | `-> str, none` | `{ if (n != floor(n)) { error("Base formatting requires an exact integ...` | `to_bin(6)` | `"0b110"` (`str`) |
| `1238` | `index_of(s:str, sub:str, start = 0)` | `FUNC` | `s:str, none`, `sub:str, none`, `start:real, none = 0` | `-> real, none` | `{ n := len(s) m := len(sub) if (start < 0) { start = 0 } if (m == 0) ...` | `index_of("MEP", "Math", 3)` | `-1` (`real`) |
| `1239` | `index_of(s, sub) [default wrapper: start=0]` | `FUNC` | `s:str, none`, `sub:str, none` | `-> real, none` | `Calls index_of(...) with start=0` | `index_of("MEP", "Math")` | `-1` (`real`) |
| `1240` | `contains(s:str, sub:str)` | `FUNC` | `s:str, none`, `sub:str, none` | `-> real, none` | `(index_of(s, sub, 0) >= 0)` | `contains("MEP", "Math")` | `0` (`real`) |
| `1241` | `(sub:str) in (s:str) :<42` | `INFIX_LEFT` (prec `42`) | `sub:str, none`, `s:str, none` | `-> real, none` | `contains(s, sub)` | `"MEP" in "Hello MEP"` | `1` (`real`) |
| `1242` | `upper(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `{ lo  := "abcdefghijklmnopqrstuvwxyz" hi  := "ABCDEFGHIJKLMNOPQRSTUVW...` | `upper("MEP")` | `"MEP"` (`str`) |
| `1243` | `to_upper(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `upper(s)` | `to_upper("MEP")` | `"MEP"` (`str`) |
| `1244` | `lower(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `{ lo  := "abcdefghijklmnopqrstuvwxyz" hi  := "ABCDEFGHIJKLMNOPQRSTUVW...` | `lower("MEP")` | `"mep"` (`str`) |
| `1245` | `to_lower(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `lower(s)` | `to_lower("MEP")` | `"mep"` (`str`) |
| `1246` | `ltrim(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `@regex_replace(s, "^\s+", "")` | `ltrim("MEP")` | `"MEP"` (`str`) |
| `1247` | `rtrim(s:str) -> str` | `FUNC` | `s:str, none` | `-> str, none` | `@regex_replace(s, "\s+$", "")` | `rtrim("MEP")` | `"MEP"` (`str`) |
| `1248` | `trim(s:str)  -> str` | `FUNC` | `s:str, none` | `-> str, none` | `rtrim(ltrim(s))` | `trim("MEP")` | `"MEP"` (`str`) |
| `1249` | `pad_left(s:str, width, ch:str = " ") -> str` | `FUNC` | `s:str, none`, `width:real, none`, `ch:str, none = " "` | `-> str, none` | `{ n := len(s) if (n >= width \|\| len(ch) == 0) { return s } str_repeat...` | `pad_left("MEP", 3, "Math")` | `"MEP"` (`str`) |
| `1250` | `pad_left(s, width) [default wrapper: ch=" "]` | `FUNC` | `s:str, none`, `width:real, none` | `-> str, none` | `Calls pad_left(...) with ch=" "` | `pad_left("MEP", 3)` | `"MEP"` (`str`) |
| `1251` | `pad_right(s:str, width, ch:str = " ") -> str` | `FUNC` | `s:str, none`, `width:real, none`, `ch:str, none = " "` | `-> str, none` | `{ n := len(s) if (n >= width \|\| len(ch) == 0) { return s } s + str_re...` | `pad_right("MEP", 3, "Math")` | `"MEP"` (`str`) |
| `1252` | `pad_right(s, width) [default wrapper: ch=" "]` | `FUNC` | `s:str, none`, `width:real, none` | `-> str, none` | `Calls pad_right(...) with ch=" "` | `pad_right("MEP", 3)` | `"MEP"` (`str`) |
| `1253` | `split(s:str, sep:str = " ") -> vec` | `FUNC` | `s:str, none`, `sep:str, none = " "` | `-> vec, none` | `{ out := vec<str>() n   := len(s) m   := len(sep) if (m == 0) { (ch i...` | `split("MEP", "Math")` | `["MEP"]` (`vec`) |
| `1254` | `split(s) [default wrapper: sep=" "]` | `FUNC` | `s:str, none` | `-> vec, none` | `Calls split(...) with sep=" "` | `split("MEP")` | `["MEP"]` (`vec`) |
| `1255` | `replace(s:str, old_s:str, new_s:str) -> str` | `FUNC` | `s:str, none`, `old_s:str, none`, `new_s:str, none` | `-> str, none` | `{ if (len(old_s) == 0) { return s } str_join(split(s, old_s), new_s) }` | `replace("MEP", "Math", "Math")` | `"MEP"` (`str`) |
| `1256` | `replace_first(s:str, old_s:str, new_s:str) -> str` | `FUNC` | `s:str, none`, `old_s:str, none`, `new_s:str, none` | `-> str, none` | `{ idx := index_of(s, old_s, 0) if (idx < 0) { return s } ol  := len(o...` | `replace_first("MEP", "Math", "Math")` | `"MEP"` (`str`) |
| `1257` | `str_count(s:str, sub:str)` | `FUNC` | `s:str, none`, `sub:str, none` | `-> real, none` | `{ if (len(sub) == 0) { return 0 } parts := split(s, sub) parts.len() ...` | `str_count("MEP", "Math")` | `0` (`real`) |
| `1258` | `lines(s:str)                        -> vec` | `FUNC` | `s:str, none` | `-> vec, none` | `split(s, " ")` | `lines("MEP")` | `["MEP"]` (`vec`) |
| `1259` | `words(s:str)                        -> vec` | `FUNC` | `s:str, none` | `-> vec, none` | `@regex_find_all(s, "\S+")` | `words("MEP")` | `["MEP"]` (`vec`) |
| `1260` | `regex_match(s:str, pat:str)` | `FUNC` | `s:str, none`, `pat:str, none` | `-> real, none` | `@regex_match(s, pat)` | `regex_match("MEP", "\d+")` | `0` (`real`) |
| `1261` | `regex_find(s:str, pat:str)            -> str` | `FUNC` | `s:str, none`, `pat:str, none` | `-> str, none` | `@regex_find(s, pat)` | `regex_find("MEP", "\d+")` | `""` (`str`) |
| `1262` | `regex_find_all(s:str, pat:str)        -> vec` | `FUNC` | `s:str, none`, `pat:str, none` | `-> vec, none` | `@regex_find_all(s, pat)` | `regex_find_all("MEP", "\d+")` | `[]` (`vec`) |
| `1263` | `regex_replace(s:str, pat:str, rep:str)-> str` | `FUNC` | `s:str, none`, `pat:str, none`, `rep:str, none` | `-> str, none` | `@regex_replace(s, pat, rep)` | `regex_replace("MEP", "\d+", "Math")` | `"MEP"` (`str`) |
| `1264` | `regex_split(s:str, pat:str)           -> vec` | `FUNC` | `s:str, none`, `pat:str, none` | `-> vec, none` | `@regex_split(s, pat)` | `regex_split("MEP", "\d+")` | `["MEP"]` (`vec`) |

### Module `[calculus]` (`Library/calculus.math` — 76 Overloads)

Unit-preserving numerical derivatives (`diff`, `diff2`, `diff_n`), Simpson integration (`integrate`), Richardson limits (`lim`), series (`sigma`, `prod`), root/extrema solvers (`solve`, `fmin`, `fmax`), multivariable calculus (`grad2`, `grad3`, `hessian2`, `laplacian2`), and RK4 ODE solvers.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `1265` | `calc_snap(res, tol)` | `FUNC` | `res:real, none`, `tol:real, none` | `-> real, none` | `{ r := raw_val(res) n := @floor(r + 0.5) s := (@abs(r - n) < tol) ? n...` | `calc_snap(3.0000000001, 1e-8)` | `3` (`real`) |
| `1266` | `diff(var_x, expr, x0)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none` | `-> real, none` | `{ scale := (@abs(raw_val(x0)) > 1) ? @abs(raw_val(x0)) : 1 h     := w...` | `diff(x, x^3 + 2*x, 3)` | `29` (`real`) |
| `1267` | `diff2(var_x, expr, x0)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none` | `-> real, none` | `{ scale := (@abs(raw_val(x0)) > 1) ? @abs(raw_val(x0)) : 1 h     := w...` | `diff2(x, x^4, 2)` | `48` (`real`) |
| `1268` | `diff_n(var_x, expr, x0, order)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none`, `order:real, none` | `-> real, none` | `{ scale := (@abs(raw_val(x0)) > 1) ? @abs(raw_val(x0)) : 1 if (order ...` | `diff_n(x, x^4, 2, 3)` | `48` (`real`) |
| `1269` | `calc_integ_fn(f:fn, a, b, steps = 200)` | `FUNC` | `f:fn, none`, `a:real, none`, `b:real, none`, `steps:real, none = 200` | `-> real, none` | `{ n := @floor(steps) if (n < 2) { n = 2 } if (n % 2 != 0) { n += 1 } ...` | `calc_integ_fn(x => 3 * x^2, 0, 2, 100)` | `8` (`real`) |
| `1270` | `calc_integ_fn(f, a, b) [default wrapper: steps=200]` | `FUNC` | `f:fn, none`, `a:real, none`, `b:real, none` | `-> real, none` | `Calls calc_integ_fn(...) with steps=200` | `calc_integ_fn(x => 3 * x^2, 0, 2)` | `8` (`real`) |
| `1271` | `integrate(var_x, expr, a, b, steps = 200)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `a:real, none`, `b:real, none`, `steps:real, none = 200` | `-> real, none` | `calc_integ_fn(expr, a, b, steps)` | `integrate(x, 3 * x^2, 0, 2, 100)` | `8` (`real`) |
| `1272` | `integrate(var_x, expr, a, b) [default wrapper: steps=200]` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `a:real, none`, `b:real, none` | `-> real, none` | `Calls integrate(...) with steps=200` | `integrate(x, 3 * x^2, 0, 2)` | `8` (`real`) |
| `1273` | `ntg(var_x, expr, a, b)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `a:real, none`, `b:real, none` | `-> real, none` | `calc_integ_fn(expr, a, b, 200)` | `ntg(x, 3 * x^2, 0, 2)` | `8` (`real`) |
| `1274` | `lim_right(var_x, expr, x0)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none` | `-> real, none` | `{ scale := (@abs(raw_val(x0)) > 1) ? @abs(raw_val(x0)) : 1 h1    := w...` | `lim_right(x, (x^2 - 4) / (x - 2), 2)` | `4` (`real`) |
| `1275` | `lim_left(var_x, expr, x0)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none` | `-> real, none` | `{ scale := (@abs(raw_val(x0)) > 1) ? @abs(raw_val(x0)) : 1 h1    := w...` | `lim_left(x, (x^2 - 4) / (x - 2), 2)` | `4` (`real`) |
| `1276` | `lim(var_x, expr, x0)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none` | `-> real, none` | `{ scale := (@abs(raw_val(x0)) > 1) ? @abs(raw_val(x0)) : 1 h1    := w...` | `lim(x, (x^2 - 4) / (x - 2), 2)` | `4` (`real`) |
| `1277` | `sigma(var_k, expr, lo, hi)` | `FUNC` | `var_k:real, none`, `expr:fn, none`, `lo:real, none`, `hi:real, none` | `-> real, none` | `{ k_lo := @floor(lo + 0.5) k_hi := @floor(hi + 0.5) if (k_lo > k_hi) ...` | `sigma(k, k, 1, 5)` | `15` (`real`) |
| `1278` | `prod(var_k, expr, lo, hi)` | `FUNC` | `var_k:real, none`, `expr:fn, none`, `lo:real, none`, `hi:real, none` | `-> real, none` | `{ k_lo := @floor(lo + 0.5) k_hi := @floor(hi + 0.5) if (k_lo > k_hi) ...` | `prod(k, k, 1, 5)` | `120` (`real`) |
| `1279` | `solve_impl(f:fn, x_init)` | `FUNC` | `f:fn, none`, `x_init:real, none` | `-> real, none` | `{ xr   := raw_val(x_init) iter := 0 (iter < 80) ?? { fx := raw_val(f(...` | `solve_impl(x => x^2 - 9, 2)` | `3` (`real`) |
| `1280` | `solve(var_x, eq, guess = 1)` | `FUNC` | `var_x:real, none`, `eq:fn, none`, `guess:real, none = 1` | `-> real, none` | `solve_impl(eq, guess)` | `solve(x, x^2 == 9, 2)` | `3` (`real`) |
| `1281` | `solve(var_x, eq) [default wrapper: guess=1]` | `FUNC` | `var_x:real, none`, `eq:fn, none` | `-> real, none` | `Calls solve(...) with guess=1` | `solve(x, 2 * x + 5 == 17)` | `6` (`real`) |
| `1282` | `solve(var_x, eq, a, b)` | `FUNC` | `var_x:real, none`, `eq:fn, none`, `a:real, none`, `b:real, none` | `-> real, none` | `solve_impl(eq, 0.5 * (a + b))` | `solve(x, x^2 == 9, 1, 5)` | `3` (`real`) |
| `1283` | `calc_extremum(f:fn, a_in, b_in, is_min)` | `FUNC` | `f:fn, none`, `a_in:real, none`, `b_in:real, none`, `is_min:real, none` | `-> real, none` | `{ u_x := b_in - a_in a   := raw_val(a_in) b   := raw_val(b_in) if (a ...` | `calc_extremum(x => (x - 3)^2 + 5, 0, 10, 1)` | `3` (`real`) |
| `1284` | `fmin(var_x, expr, a, b)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `a:real, none`, `b:real, none` | `-> real, none` | `calc_extremum(expr, a, b, 1)` | `fmin(x, (x - 3)^2 + 5, 0, 10)` | `3` (`real`) |
| `1285` | `fmax(var_x, expr, a, b)` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `a:real, none`, `b:real, none` | `-> real, none` | `calc_extremum(expr, a, b, 0)` | `fmax(x, -(x - 4)^2 + 12, 0, 10)` | `4` (`real`) |
| `1286` | `ode_rk4(var_t, var_y, expr, t0, y0, t1, steps = 100)` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `expr:fn, none`, `t0:real, none`, `y0:real, none`, `t1:real, none`, `steps:real, none = 100` | `-> real, none` | `{ n := @floor(steps) if (n < 1) { n = 1 } if (n > 4096) { n = 4096 } ...` | `ode_rk4(t, y, 2 * t, 0, 0, 3, 20)` | `9` (`real`) |
| `1287` | `ode_rk4(var_t, var_y, expr, t0, y0, t1) [default wrapper: steps=100]` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `expr:fn, none`, `t0:real, none`, `y0:real, none`, `t1:real, none` | `-> real, none` | `Calls ode_rk4(...) with steps=100` | `ode_rk4(t, y, 2 * t, 0, 0, 3, 20)` | `9` (`real`) |
| `1288` | `ode_rk4_vec(var_t, var_y, expr, t0, y0, t1, steps = 10) -> vec` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `expr:fn, none`, `t0:real, none`, `y0:real, none`, `t1:real, none`, `steps:real, none = 10` | `-> vec, none` | `{ n := @floor(steps) if (n < 1) { n = 1 } if (n > 4096) { n = 4096 } ...` | `ode_rk4_vec(t, y, 2 * t, 0, 0, 3, 20)` | `[0, 0.0225, 0.09, 0.2025, 0.36, 0.5625, 0.81, 1.1025, 1.44, 1.8225, 2.25, 2.7225, 3.24, 3.8025, 4.41, 5.0625, 5.76, 6.5025, 7.29, 8.1225, 9]` (`vec`) |
| `1289` | `ode_rk4_vec(var_t, var_y, expr, t0, y0, t1) [default wrapper: steps=10]` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `expr:fn, none`, `t0:real, none`, `y0:real, none`, `t1:real, none` | `-> vec, none` | `Calls ode_rk4_vec(...) with steps=10` | `ode_rk4_vec(t, y, 2 * t, 0, 0, 3, 20)` | `[0, 0.0225, 0.09, 0.2025, 0.36, 0.5625, 0.81, 1.1025, 1.44, 1.8225, 2.25, 2.7225, 3.24, 3.8025, 4.41, 5.0625, 5.76, 6.5025, 7.29, 8.1225, 9]` (`vec`) |
| `1290` | `ode_coupled2(var_t, var_x, var_y, dx, dy, t0, x0, y0, t1, steps = 100) -> vec` | `FUNC` | `var_t:real, none`, `var_x:real, none`, `var_y:real, none`, `dx:fn, none`, `dy:fn, none`, `t0:real, none`, `x0:real, none`, `y0:real, none`, `t1:real, none`, `steps:real, none = 100` | `-> vec, none` | `{ n := @floor(steps) if (n < 1) { n = 1 } if (n > 4096) { n = 4096 } ...` | `ode_coupled2(t, x, y, y, -x, 0, 1, 0, π, 100)` | `[-1, -2.54927e-08]` (`vec`) |
| `1291` | `ode_coupled2(var_t, var_x, var_y, dx, dy, t0, x0, y0, t1) [default wrapper: steps=100]` | `FUNC` | `var_t:real, none`, `var_x:real, none`, `var_y:real, none`, `dx:fn, none`, `dy:fn, none`, `t0:real, none`, `x0:real, none`, `y0:real, none`, `t1:real, none` | `-> vec, none` | `Calls ode_coupled2(...) with steps=100` | `ode_coupled2(t, x, y, y, -x, 0, 1, 0, π, 100)` | `[-1, -2.54927e-08]` (`vec`) |
| `1292` | `ode_coupled3(var_t, var_x, var_y, var_z, dx, dy, dz, t0, x0, y0, z0, t1, steps = 100) -> vec` | `FUNC` | `var_t:real, none`, `var_x:real, none`, `var_y:real, none`, `var_z:real, none`, `dx:fn, none`, `dy:fn, none`, `dz:fn, none`, `t0:real, none`, `x0:real, none`, `y0:real, none`, `z0:real, none`, `t1:real, none`, `steps:real, none = 100` | `-> vec, none` | `{ n := @floor(steps) if (n < 1) { n = 1 } if (n > 4096) { n = 4096 } ...` | `ode_coupled3(t, x, y, z, y, -x, -z, 0, 1, 0, 1, 1, 50)` | `[0.540302, -0.841471, 0.367879]` (`vec`) |
| `1293` | `ode_coupled3(var_t, var_x, var_y, var_z, dx, dy, dz, t0, x0, y0, z0, t1) [default wrapper: steps=100]` | `FUNC` | `var_t:real, none`, `var_x:real, none`, `var_y:real, none`, `var_z:real, none`, `dx:fn, none`, `dy:fn, none`, `dz:fn, none`, `t0:real, none`, `x0:real, none`, `y0:real, none`, `z0:real, none`, `t1:real, none` | `-> vec, none` | `Calls ode_coupled3(...) with steps=100` | `ode_coupled3(t, x, y, z, y, -x, -z, 0, 1, 0, 1, 1, 50)` | `[0.540302, -0.841471, 0.367879]` (`vec`) |
| `1294` | `poly_eval(c:vec, x)` | `FUNC` | `c:vec, none`, `x:real, none` | `-> real, none` | `{ n   := c.len() acc := 0 k   := n - 1 (k >= 0) ?? { acc = acc * x + ...` | `poly_eval(vec(6, -5, 1), 4)` | `2` (`real`) |
| `1295` | `poly_diff(c:vec) -> vec` | `FUNC` | `c:vec, none` | `-> vec, none` | `{ n   := c.len() out := vec<real>() if (n <= 1) { 0 into out } else {...` | `poly_diff(vec(6, -5, 1))` | `[-5, 2]` (`vec`) |
| `1296` | `poly_diff_at(c:vec, x0)` | `FUNC` | `c:vec, none`, `x0:real, none` | `-> real, none` | `{ dc := poly_diff(c) poly_eval(dc, x0) }` | `poly_diff_at(vec(6, -5, 1), 4)` | `3` (`real`) |
| `1297` | `poly_diff2_at(c:vec, x0)` | `FUNC` | `c:vec, none`, `x0:real, none` | `-> real, none` | `{ d2c := poly_diff(poly_diff(c)) poly_eval(d2c, x0) }` | `poly_diff2_at(vec(6, -5, 1), 4)` | `2` (`real`) |
| `1298` | `poly_def_integ(c:vec, a, b)` | `FUNC` | `c:vec, none`, `a:real, none`, `b:real, none` | `-> real, none` | `{ ic := poly_integ(c, 0) poly_eval(ic, b) - poly_eval(ic, a) }` | `poly_def_integ(vec(1, -3, 2), 0, 3)` | `7.5` (`real`) |
| `1299` | `tangent_slope(c:vec, x0)` | `FUNC` | `c:vec, none`, `x0:real, none` | `-> real, none` | `poly_diff_at(c, x0)` | `tangent_slope(vec(0, 0, 1), 3)` | `6` (`real`) |
| `1300` | `normal_slope(c:vec, x0)` | `FUNC` | `c:vec, none`, `x0:real, none` | `-> real, none` | `-1 / poly_diff_at(c, x0)` | `normal_slope(vec(0, 0, 1), 3)` | `-0.1666666667` (`real`) |
| `1301` | `tangent_y(c:vec, x0, x)` | `FUNC` | `c:vec, none`, `x0:real, none`, `x:real, none` | `-> real, none` | `poly_eval(c, x0) + poly_diff_at(c, x0) * (x - x0)` | `tangent_y(vec(0, 0, 1), 3, 4)` | `15` (`real`) |
| `1302` | `normal_y(c:vec, x0, x)` | `FUNC` | `c:vec, none`, `x0:real, none`, `x:real, none` | `-> real, none` | `poly_eval(c, x0) + (-1 / poly_diff_at(c, x0)) * (x - x0)` | `normal_y(vec(0, 0, 1), 3, 4)` | `8.833333333` (`real`) |
| `1303` | `poly_curvature(c:vec, x0)` | `FUNC` | `c:vec, none`, `x0:real, none` | `-> real, none` | `{ dy  := poly_diff_at(c, x0) d2y := poly_diff2_at(c, x0) @abs(d2y) / ...` | `poly_curvature(vec(6, -5, 1), 4)` | `0.0632455532` (`real`) |
| `1304` | `quad_critical_x(a, b, c_term = 0)` | `FUNC` | `a:real, none`, `b:real, none`, `c_term:real, none = 0` | `-> real, none` | `-b / (2a) + 0 * c_term` | `quad_critical_x(6, 3, 3)` | `-0.25` (`real`) |
| `1305` | `quad_critical_x(a, b) [default wrapper: c_term=0]` | `FUNC` | `a:real, none`, `b:real, none` | `-> real, none` | `Calls quad_critical_x(...) with c_term=0` | `quad_critical_x(6, 3)` | `-0.25` (`real`) |
| `1306` | `quad_extremum_y(a, b, c_term)` | `FUNC` | `a:real, none`, `b:real, none`, `c_term:real, none` | `-> real, none` | `c_term - (b^2) / (4a)` | `quad_extremum_y(6, 3, 3)` | `2.625` (`real`) |
| `1307` | `poly_newton_root(c:vec, x_init)` | `FUNC` | `c:vec, none`, `x_init:real, none` | `-> real, none` | `{ xr   := x_init iter := 0 (iter < 35) ?? { fx  := poly_eval(c, xr) d...` | `poly_newton_root(vec(-9, 0, 1), 5)` | `3` (`real`) |
| `1308` | `poly_avg_val(c:vec, a, b)` | `FUNC` | `c:vec, none`, `a:real, none`, `b:real, none` | `-> real, none` | `poly_def_integ(c, a, b) / (b - a)` | `poly_avg_val(vec(1, -3, 2), 0, 3)` | `2.5` (`real`) |
| `1309` | `poly_arc_length(c:vec, a, b)` | `FUNC` | `c:vec, none`, `a:real, none`, `b:real, none` | `-> real, none` | `integrate(t_var, √(1 + (poly_diff_at(c, t_var) ^ 2)), a, b)` | `poly_arc_length(vec(1, -3, 2), 0, 3)` | `11.96214871` (`real`) |
| `1310` | `poly_vol_disk(c:vec, a, b)` | `FUNC` | `c:vec, none`, `a:real, none`, `b:real, none` | `-> real, none` | `π * integrate(t_var, poly_eval(c, t_var) ^ 2, a, b)` | `poly_vol_disk(vec(1, -3, 2), 0, 3)` | `139.4867141` (`real`) |
| `1311` | `poly_vol_shell(c:vec, a, b)` | `FUNC` | `c:vec, none`, `a:real, none`, `b:real, none` | `-> real, none` | `2π * integrate(t_var, t_var * @abs(poly_eval(c, t_var)), a, b)` | `poly_vol_shell(vec(1, -3, 2), 0, 3)` | `113.4900283` (`real`) |
| `1312` | `trapz_vec(y:vec, dx)` | `FUNC` | `y:vec, none`, `dx:real, none` | `-> real, none` | `{ n   := y.len() acc := 0.5 * (y[0] + y[n - 1]) k   := 1 (k < n - 1) ...` | `trapz_vec(vec(2, 4, 6), 3)` | `24` (`real`) |
| `1313` | `simpson_vec(y:vec, dx)` | `FUNC` | `y:vec, none`, `dx:real, none` | `-> real, none` | `{ n   := y.len() acc := y[0] + y[n - 1] k   := 1 (k < n - 1) ?? { w  ...` | `simpson_vec(vec(2, 4, 6), 3)` | `24` (`real`) |
| `1314` | `taylor_exp(x, n_terms = 12)` | `FUNC` | `x:real, none`, `n_terms:real, none = 12` | `-> real, none` | `sigma(k_idx, (x ^ k_idx) / (k_idx!), 0, n_terms - 1)` | `taylor_exp(6, 3)` | `25` (`real`) |
| `1315` | `taylor_exp(x) [default wrapper: n_terms=12]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls taylor_exp(...) with n_terms=12` | `taylor_exp(6)` | `395.3231169` (`real`) |
| `1316` | `taylor_sin(x, n_terms = 10)` | `FUNC` | `x:real, none`, `n_terms:real, none = 10` | `-> real, none` | `sigma(k_idx, ((-1) ^ k_idx) * (x ^ (2k_idx + 1)) / ((2k_idx + 1)!), 0...` | `taylor_sin(6, 3)` | `34.8` (`real`) |
| `1317` | `taylor_sin(x) [default wrapper: n_terms=10]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls taylor_sin(...) with n_terms=10` | `taylor_sin(6)` | `-0.2798160636` (`real`) |
| `1318` | `taylor_cos(x, n_terms = 10)` | `FUNC` | `x:real, none`, `n_terms:real, none = 10` | `-> real, none` | `sigma(k_idx, ((-1) ^ k_idx) * (x ^ (2k_idx)) / ((2k_idx)!), 0, n_term...` | `taylor_cos(6, 3)` | `37` (`real`) |
| `1319` | `taylor_cos(x) [default wrapper: n_terms=10]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls taylor_cos(...) with n_terms=10` | `taylor_cos(6)` | `0.9587773571` (`real`) |
| `1320` | `pdiff_x(f:fn, x0, y0)` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none` | `-> real, none` | `{ h := 1e-5 * (1 + @abs(x0)) (-f(x0 + 2h, y0) + 8 * f(x0 + h, y0) - 8...` | `pdiff_x((x, y) => x^2 * y + 3 * y^2, 2, 3)` | `12` (`real`) |
| `1321` | `pdiff_y(f:fn, x0, y0)` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none` | `-> real, none` | `{ h := 1e-5 * (1 + @abs(y0)) (-f(x0, y0 + 2h) + 8 * f(x0, y0 + h) - 8...` | `pdiff_y((x, y) => x^2 * y + 3 * y^2, 2, 3)` | `22` (`real`) |
| `1322` | `grad2(f:fn, x0, y0) -> vec2` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none` | `-> vec2, none` | `vec2(pdiff_x(f, x0, y0), pdiff_y(f, x0, y0))` | `grad2((x, y) => x^2 * y + 3 * y^2, 2, 3)` | `vec2(x=12, y=22)` (`vec2`) |
| `1323` | `grad3(f:fn, x0, y0, z0) -> vec3` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none`, `z0:real, none` | `-> vec3, none` | `{ hx := 1e-5 * (1 + @abs(x0)) hy := 1e-5 * (1 + @abs(y0)) hz := 1e-5 ...` | `grad3((x, y, z) => x^2 + 2 * y^2 + 3 * z^2, 1, 2, 3)` | `vec3(x=2, y=8, z=18)` (`vec3`) |
| `1324` | `grad3(f:fn, p:vec3) -> vec3` | `FUNC` | `f:fn, none`, `p:vec3, none` | `-> vec3, none` | `{ hx := 1e-5 * (1 + @abs(p.x)) hy := 1e-5 * (1 + @abs(p.y)) hz := 1e-...` | `grad3(v => v.x^2 + 2 * v.y^2 + 3 * v.z^2, vec3(1, 2, 3))` | `vec3(x=2, y=8, z=18)` (`vec3`) |
| `1325` | `laplacian2(f:fn, x0, y0)` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none` | `-> real, none` | `{ hx  := 1e-4 * (1 + @abs(x0)) hy  := 1e-4 * (1 + @abs(y0)) f0  := f(...` | `laplacian2((x, y) => x^2 * y + 3 * y^2, 2, 3)` | `12.00000027` (`real`) |
| `1326` | `hessian2(f:fn, x0, y0) -> mat2` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none` | `-> mat2, none` | `{ hx  := 1e-4 * (1 + @abs(x0)) hy  := 1e-4 * (1 + @abs(y0)) f0  := f(...` | `hessian2((x, y) => x^2 * y + 3 * y^2, 2, 3)` | `mat2(a=6, b=4, c=4, d=6)` (`mat2`) |
| `1327` | `dir_deriv2(f:fn, x0, y0, u:vec2)` | `FUNC` | `f:fn, none`, `x0:real, none`, `y0:real, none`, `u:vec2, none` | `-> real, none` | `{ g := grad2(f, x0, y0) dot(g, u) / vec2_mag(u) }` | `dir_deriv2((x, y) => x^2 + y^2, 3, 4, vec2(3, 4))` | `10` (`real`) |
| `1328` | `grad_descent_1d(f:fn, x0, lr = 0.1, steps = 100)` | `FUNC` | `f:fn, none`, `x0:real, none`, `lr:real, none = 0.1`, `steps:real, none = 100` | `-> real, none` | `{ x := x0 k := 0 (k < steps) ?? { h  := 1e-5 * (1 + @abs(x)) dx := (f...` | `round(grad_descent_1d(x => (x - 4)^2 + 1, 0, 0.2, 60))` | `4` (`real`) |
| `1329` | `grad_descent_1d(f, x0) [default wrapper: lr=0.1, steps=100]` | `FUNC` | `f:fn, none`, `x0:real, none` | `-> real, none` | `Calls grad_descent_1d(...) with lr=0.1, steps=100` | `round(grad_descent_1d(x => (x - 4)^2 + 1, 0))` | `4` (`real`) |
| `1330` | `grad_descent_1d(f, x0, lr) [default wrapper: steps=100]` | `FUNC` | `f:fn, none`, `x0:real, none`, `lr:real, none` | `-> real, none` | `Calls grad_descent_1d(...) with steps=100` | `round(grad_descent_1d(x => (x - 4)^2 + 1, 0, 0.2))` | `4` (`real`) |
| `1331` | `grad_descent_2d(f:fn, p0:vec2, lr = 0.1, steps = 100) -> vec2` | `FUNC` | `f:fn, none`, `p0:vec2, none`, `lr:real, none = 0.1`, `steps:real, none = 100` | `-> vec2, none` | `{ p := p0 k := 0 (k < steps) ?? { g := grad2(f, p.x, p.y) p  = p - lr...` | `grad_descent_2d((x, y) => (x - 3)^2 + (y + 2)^2, vec2(0, 0), 0.2, 60)` | `vec2(x=3, y=-2)` (`vec2`) |
| `1332` | `grad_descent_2d(f, p0) [default wrapper: lr=0.1, steps=100]` | `FUNC` | `f:fn, none`, `p0:vec2, none` | `-> vec2, none` | `Calls grad_descent_2d(...) with lr=0.1, steps=100` | `grad_descent_2d((x, y) => (x - 3)^2 + (y + 2)^2, vec2(0, 0))` | `vec2(x=3, y=-2)` (`vec2`) |
| `1333` | `grad_descent_2d(f, p0, lr) [default wrapper: steps=100]` | `FUNC` | `f:fn, none`, `p0:vec2, none`, `lr:real, none` | `-> vec2, none` | `Calls grad_descent_2d(...) with steps=100` | `grad_descent_2d((x, y) => (x - 3)^2 + (y + 2)^2, vec2(0, 0), 0.2)` | `vec2(x=3, y=-2)` (`vec2`) |
| `1334` | `ode_rk4_sys(f:fn, t0, y0:T, t1, steps = 100) -> vec` | `FUNC` | `f:fn, none`, `t0:real, none`, `y0:T, none`, `t1:real, none`, `steps:real, none = 100` | `-> vec, none` | `{ if (steps <= 0) { error("ode_rk4_sys() steps must be positive") } h...` | `ode_rk4_sys((t, y) => [y[1], -y[0]], 0, list(1, 0), π, 100)` | `[-1, -2.54927e-08]` (`vec`) |
| `1335` | `ode_rk4_sys(f, t0, y0, t1) [default wrapper: steps=100]` | `FUNC` | `f:fn, none`, `t0:real, none`, `y0:T, none`, `t1:real, none` | `-> vec, none` | `Calls ode_rk4_sys(...) with steps=100` | `ode_rk4_sys((t, y) => [y[1], -y[0]], 0, list(1, 0), π)` | `[-1, -2.54927e-08]` (`vec`) |
| `1336` | `ode_rk4_sys(var_t, var_y, rhs_vec, t0, y0:T, t1, steps = 100) -> vec` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `rhs_vec:fn, none`, `t0:real, none`, `y0:T, none`, `t1:real, none`, `steps:real, none = 100` | `-> vec, none` | `ode_rk4_sys(rhs_vec, t0, y0, t1, steps)` | `ode_rk4_sys(t, y, [y[1], -y[0]], 0, list(1, 0), π, 100)` | `[-1, -2.54927e-08]` (`vec`) |
| `1337` | `ode_rk4_sys(var_t, var_y, rhs_vec, t0, y0, t1) [default wrapper: steps=100]` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `rhs_vec:fn, none`, `t0:real, none`, `y0:T, none`, `t1:real, none` | `-> vec, none` | `Calls ode_rk4_sys(...) with steps=100` | `ode_rk4_sys(t, y, [y[1], -y[0]], 0, list(1, 0), π, 100)` | `[-1, -2.54927e-08]` (`vec`) |
| `1338` | `ode_rk4_sys_traj(var_t, var_y, rhs_vec, t0, y0:T, t1, steps = 20) -> mat` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `rhs_vec:fn, none`, `t0:real, none`, `y0:T, none`, `t1:real, none`, `steps:real, none = 20` | `-> mat, none` | `{ st := @floor(steps) if (st < 1) { st = 1 } h   := (t1 - t0) / st tc...` | `mat_rows(ode_rk4_sys_traj(t, y, [y[1], -y[0]], 0, [1, 0], 1, 10))` | `11` (`real`) |
| `1339` | `ode_rk4_sys_traj(var_t, var_y, rhs_vec, t0, y0, t1) [default wrapper: steps=20]` | `FUNC` | `var_t:real, none`, `var_y:real, none`, `rhs_vec:fn, none`, `t0:real, none`, `y0:T, none`, `t1:real, none` | `-> mat, none` | `Calls ode_rk4_sys_traj(...) with steps=20` | `mat_rows(ode_rk4_sys_traj(t, y, [y[1], -y[0]], 0, [1, 0], 1, 10))` | `11` (`real`) |
| `1340` | `sym_taylor(var_x, expr, x0, order) -> str` | `FUNC` | `var_x:real, none`, `expr:fn, none`, `x0:real, none`, `order:real, none` | `-> str, none` | `{ vname := sym_diff(var_x, expr, -1) d_str := sym_diff(var_x, expr, 0...` | `sym_taylor(x, exp(x), 0, 3)` | `"1 + x + 0.5 * x ^ 2 + 0.1666666667 * x ^ 3"` (`str`) |

### Module `[user_physics]` (`Test/user_physics.math` — 44 Overloads)

Aerospace engineering, Mach/planet classifiers, Collatz tagged-scope state machine, host `@extern` FX/KV/Periodic-Table bindings, custom `data_size`/`bandwidth` units, and pattern-matched `switch`.

| # | Symbol / Overload Signature | Fixity & Prec | Parameter Annotations (`name:type, unit = default`) | Return (`-> type, unit`) | Implementation / Semantics | Tested Example | Engine Output |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `1341` | `rocket_dv(isp_s, m0_kg, mf_kg)` | `FUNC` | `isp_s:real, none`, `m0_kg:real, none`, `mf_kg:real, none` | `-> real, none` | `isp_s * g0 * ln(m0_kg / mf_kg)` | `round(rocket_dv(300, 1000, 500), 2)` | `2039.24` (`real`) |
| `1342` | `fspl_ratio(dist_m, lambda_m)` | `FUNC` | `dist_m:real, none`, `lambda_m:real, none` | `-> real, none` | `((4π * dist_m) / lambda_m) ^ 2` | `fspl_ratio(6, 3)` | `631.6546817` (`real`) |
| `1343` | `classify_mach(m) -> str` | `FUNC` | `m:real, none` | `-> str, none` | `{ if (m < 0.8) { "subsonic" } or (m < 1.2) { "transonic" } or (m < 5....` | `classify_mach(6)` | `"hypersonic"` (`str`) |
| `1344` | `planet_g(id)` | `FUNC` | `id:real, none` | `-> real, none` | `switch (id) { case 1 : { 3.7 } case 2 : { 8.87 } case 3 : { 9.80665 }...` | `planet_g(6)` | `0` (`real`) |
| `1345` | `collatz_steps(n)` | `FUNC` | `n:real, none` | `-> real, none` | `{ steps := 0 jump(Loop) Tick : { steps += 1 } Loop : { if (n <= 1) { ...` | `collatz_steps(6)` | `8` (`real`) |
| `1346` | `fx_rate(from:str, to:str)` | `FUNC` | `from:str, none`, `to:str, none` | `-> real, none` | `@fx_rate(from, to)` | `fx_rate("USD", "INR")` | `84` (`real`) |
| `1347` | `fx_set_rate(ccy:str, per_usd)` | `FUNC` | `ccy:str, none`, `per_usd:real, none` | `-> real, none` | `@fx_set_rate(ccy, per_usd)` | `fx_set_rate("USD", 3)` | `3` (`real`) |
| `1348` | `convert_fx(amount, from:str, to:str)` | `FUNC` | `amount:real, none`, `from:str, none`, `to:str, none` | `-> real, none` | `{ assert(amount >= 0, "Currency amount cannot be negative") amount * ...` | `convert_fx(6, "INR", "INR")` | `6` (`real`) |
| `1349` | `fx_with_fee(amount, from:str, to:str, fee_pct = 1.5)` | `FUNC` | `amount:real, none`, `from:str, none`, `to:str, none`, `fee_pct:real, none = 1.5` | `-> real, none` | `{ assert(amount >= 0, "Currency amount cannot be negative") assert(fe...` | `fx_with_fee(6, "INR", "INR", 3)` | `5.82` (`real`) |
| `1350` | `fx_with_fee(amount, from, to) [default wrapper: fee_pct=1.5]` | `FUNC` | `amount:real, none`, `from:str, none`, `to:str, none` | `-> real, none` | `Calls fx_with_fee(...) with fee_pct=1.5` | `fx_with_fee(6, "INR", "INR")` | `5.91` (`real`) |
| `1351` | `(usd) to_inr :90` | `POSTFIX` (prec `90`) | `usd:real, none` | `-> real, none` | `usd * @fx_rate("USD", "INR")` | `(6) to_inr` | `504` (`real`) |
| `1352` | `(eur) to_usd :90` | `POSTFIX` (prec `90`) | `eur:real, none` | `-> real, none` | `eur * @fx_rate("EUR", "USD")` | `(6) to_usd` | `6.52173913` (`real`) |
| `1353` | `vec_sum_fx(prices_usd:vec, target_ccy:str)` | `FUNC` | `prices_usd:vec, none`, `target_ccy:str, none` | `-> real, none` | `prices_usd.sum() * @fx_rate("USD", target_ccy)` | `vec_sum_fx(vec(2, 4, 6), "INR")` | `1008` (`real`) |
| `1354` | `fx_history(from:str, to:str, days) -> vec` | `FUNC` | `from:str, none`, `to:str, none`, `days:real, none` | `-> vec, none` | `@fx_history(from, to, days)` | `fx_history("USD", "INR", 3)` | `[82, 82.5, 83]` (`vec`) |
| `1355` | `fx_sma(from:str, to:str, days, window)` | `FUNC` | `from:str, none`, `to:str, none`, `days:real, none`, `window:real, none` | `-> real, none` | `sma(@fx_history(from, to, days), window)` | `fx_sma("USD", "INR", 3, 3)` | `82.5` (`real`) |
| `1356` | `save_var(key:str, val)` | `FUNC` | `key:str, none`, `val:real, none` | `-> real, none` | `@kv_set(key, val)` | `save_var("MEP", 3)` | `3` (`real`) |
| `1357` | `load_var(key:str)` | `FUNC` | `key:str, none` | `-> real, none` | `@kv_get(key)` | `load_var("MEP")` | `3` (`real`) |
| `1358` | `elem(sym:str)` | `FUNC` | `sym:str, none` | `-> real, none` | `@elem_mass(sym)` | `elem("O")` | `15.999` (`real`) |
| `1359` | `molar_mass_h2o()` | `FUNC` | `()` (0 params) | `-> real, none` | `2 * @elem_mass("H")  + @elem_mass("O")` | `molar_mass_h2o()` | `18.015` (`real`) |
| `1360` | `molar_mass_co2()` | `FUNC` | `()` (0 params) | `-> real, none` | `@elem_mass("C")      + 2 * @elem_mass("O")` | `molar_mass_co2()` | `44.009` (`real`) |
| `1361` | `molar_mass_nacl()` | `FUNC` | `()` (0 params) | `-> real, none` | `@elem_mass("Na")     + @elem_mass("Cl")` | `molar_mass_nacl()` | `58.44` (`real`) |
| `1362` | `molar_mass_glucose()` | `FUNC` | `()` (0 params) | `-> real, none` | `6 * @elem_mass("C")  + 12 * @elem_mass("H") + 6 * @elem_mass("O")` | `molar_mass_glucose()` | `180.156` (`real`) |
| `1363` | `(x:real, distance) "+" (y:real, distance) :<50 -> real, distance` | `INFIX_LEFT` (prec `50`) | `x:real, distance`, `y:real, distance` | `-> real, distance` | `@add(x, y)` | `(100 m) + (100 m)` | `200 [distance]` (`real`) |
| `1364` | `(x:real, distance) "-" (y:real, distance) :<50 -> real, distance` | `INFIX_LEFT` (prec `50`) | `x:real, distance`, `y:real, distance` | `-> real, distance` | `@sub(x, y)` | `(100 m) - (100 m)` | `0 [distance]` (`real`) |
| `1365` | `(d:real, distance) "/" (t:real, time) :<60 -> real, speed` | `INFIX_LEFT` (prec `60`) | `d:real, distance`, `t:real, time` | `-> real, speed` | `@div(d, t)` | `(100 m) / (5 sec)` | `20 [speed]` (`real`) |
| `1366` | `(v:real, speed)    "*" (t:real, time) :<60 -> real, distance` | `INFIX_LEFT` (prec `60`) | `v:real, speed`, `t:real, time` | `-> real, distance` | `@mul(v, t)` | `(20 mps) * (5 sec)` | `100 [distance]` (`real`) |
| `1367` | `avg_speed(d:real, distance, t:real, time) -> real, speed` | `FUNC` | `d:real, distance`, `t:real, time` | `-> real, speed` | `d / t` | `avg_speed((100 m), (5 sec))` | `20 [speed]` (`real`) |
| `1368` | `scale_unit(x:real, T, k)      -> real, T` | `FUNC` | `x:real, T`, `k:real, none` | `-> real, T` | `x * k` | `scale_unit(6, 3)` | `18` (`real`) |
| `1369` | `unit_span(a:real, T, b:real, T) -> real, T` | `FUNC` | `a:real, T`, `b:real, T` | `-> real, T` | `@abs(a - b)` | `unit_span(6, 3)` | `3` (`real`) |
| `1370` | `(x) B  :95 -> real, data_size` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, data_size` | `x` | `(6) B` | `6 [data_size]` (`real`) |
| `1371` | `(x) KB :95 -> real, data_size` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, data_size` | `x * 1024` | `(6) KB` | `6144 [data_size]` (`real`) |
| `1372` | `(x) MB :95 -> real, data_size` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, data_size` | `x * (1024 ^ 2)` | `(6) MB` | `6291456 [data_size]` (`real`) |
| `1373` | `(x) GB :95 -> real, data_size` | `POSTFIX` (prec `95`) | `x:real, none` | `-> real, data_size` | `x * (1024 ^ 3)` | `(6) GB` | `6442450944 [data_size]` (`real`) |
| `1374` | `(sz:real, data_size) "/" (t:real, time) :<60 -> real, bandwidth` | `INFIX_LEFT` (prec `60`) | `sz:real, data_size`, `t:real, time` | `-> real, bandwidth` | `@div(sz, t)` | `(2 MB) / (5 sec)` | `419430.4 [bandwidth]` (`real`) |
| `1375` | `(bw:real, bandwidth) "*" (t:real, time) :<60 -> real, data_size` | `INFIX_LEFT` (prec `60`) | `bw:real, bandwidth`, `t:real, time` | `-> real, data_size` | `@mul(bw, t)` | `((10 MB) / (2 sec)) * (5 sec)` | `26214400 [data_size]` (`real`) |
| `1376` | `hyp_3d(x, y, z = 0)` | `FUNC` | `x:real, none`, `y:real, none`, `z:real, none = 0` | `-> real, none` | `√(x^2 + y^2 + z^2)` | `hyp_3d(6, 3, 3)` | `7.348469228` (`real`) |
| `1377` | `hyp_3d(x, y) [default wrapper: z=0]` | `FUNC` | `x:real, none`, `y:real, none` | `-> real, none` | `Calls hyp_3d(...) with z=0` | `hyp_3d(6, 3)` | `6.708203932` (`real`) |
| `1378` | `log_base(x, base = 10)` | `FUNC` | `x:real, none`, `base:real, none = 10` | `-> real, none` | `ln(x) / ln(base)` | `log_base(6, 3)` | `1.630929754` (`real`) |
| `1379` | `log_base(x) [default wrapper: base=10]` | `FUNC` | `x:real, none` | `-> real, none` | `Calls log_base(...) with base=10` | `log_base(6)` | `0.7781512504` (`real`) |
| `1380` | `sum_squares_to(n)` | `FUNC` | `n:real, none` | `-> real, none` | `{ s := 0 (i in 1...n) { s += i^2 } s }` | `sum_squares_to(6)` | `91` (`real`) |
| `1381` | `vec_dot_self(v:vec)` | `FUNC` | `v:vec, none` | `-> real, none` | `{ s := 0 (x in v) { s += x^2 } s }` | `vec_dot_self(vec(2, 4, 6))` | `56` (`real`) |
| `1382` | `safe_sqrt_ret(x)` | `FUNC` | `x:real, none` | `-> real, none` | `{ if (x < 0) { return -1 } if (x == 0) { return 0 } res := √x return ...` | `safe_sqrt_ret(6)` | `24.49489743` (`real`) |
| `1383` | `describe_vec2(p:vec2) -> str` | `FUNC` | `p:vec2, none` | `-> str, none` | `switch (p) { case vec2(0, 0)           : { "origin" } case vec2(0, y)...` | `describe_vec2(vec2(3, 4))` | `"general"` (`str`) |
| `1384` | `grade_band(score) -> str` | `FUNC` | `score:real, none` | `-> str, none` | `switch (score) { case < 0, > 100 : { "invalid" } case >= 90      : { ...` | `grade_band(6)` | `"F"` (`str`) |
