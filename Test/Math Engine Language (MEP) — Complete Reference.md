# Math Engine Language (MEP) — Complete Reference & Tutorial Guidebook

*Verified against the engine source and a live sanitizer build (`gcc -std=c11 -Wall -Wextra -Wconversion -Werror -fsanitize=address,undefined`), 22 October 2026 (529 / 529 automated tests, 19 modules, 12 types, 35 unit groups, 28 constants, 1,359 operator & function overloads). This single reference & tutorial guidebook in `Test/` merges the complete language specification, chapter-by-chapter tutorial walkthroughs, and the exhaustive overload-by-overload reference.*

## 1. Overview & Architecture

MEP is an extension-driven expression language and C11 engine for scientific and everyday math. A small C core provides memory, parsing, linking, and evaluation; almost everything else (operators, units, types, functions) is defined in text extensions (`.math`) that are compiled into the engine at start-up.

Key properties:

- **Embeddable.** The engine is plain C11, makes zero internal heap allocations during evaluation, never touches the filesystem, and runs from fixed-size arenas (`sizeof(MemArena) ≈ 267.8 KB`, `sizeof(SymTable) ≈ 84.7 KB`, `sizeof(Engine) ≈ 788.6 KB` including its isolated 256 KB `.mathc` serialization buffer and 8 parallel session slots; `sizeof(Value) == 8`, `sizeof(AstNode) == 8`). It targets the Android NDK but builds anywhere.
- **Extensible & Order-Independent.** Operators, precedences, types, unit groups (`@dim`), constants, and functions are all declared in `.math` source and linked topologically via `<[dep]` declarations, not hard-coded in C.
- **Unit-aware (7D SI Algebra).** Every value carries an entity type (`real`, `iota`, `complex`, `vec2`…) and an orthogonal unit tag (`distance`, `angle`, `mass`, `temperature`, `substance`, `luminous_intensity`…). Mixing incompatible units is an error, while multiplying/dividing/exponentiating SI units automatically resolves derived 7D SI unit groups.
- **Stateless or Stateful Session Evaluations.** Each call to `eng_eval` is a fresh stateless script where variables do not survive to the next call (section 4.4), while `eng_session_eval` runs inside an isolated parallel in-memory session that persists variables, type/unit locks, containers, and closures and emits an order-tagged raw-text snapshot per evaluation. Loaded extensions and host callbacks persist across the engine's lifetime.

### 1.1 Repository Layout & Modular Separation of Concerns

- **`Core/Code/` & `Core/Headers/` (Core Pipeline)**: Arena (`mem_arena`), symbol table (`sym_table`), pattern-driven lexer (`lexer`), Pratt parser (`parser`), AST evaluator (`evaluator`), and top-level engine wrapper (`math_engine`).
- **`Core/compile/` (Compilation, Topological Linking, `.mathc` Binary & User Loader)**:
  - `compiler.h` / `compiler.c`: `[module]` unit staging (`ext_compile_unit`), duplicate module rejection, and declaration compiler (`ext_compile_slice`).
  - `linker.h` / `linker.c`: `<[dep]` dependency DAG resolution, missing/circular dependency detection via Kahn's topological sort, and transactional linking (`ext_link_units`, `ext_compile_and_link`, `ext_load_str`).
  - `mathc_bin.h` / `mathc_bin.c`: 40-byte `MathcHeader`, FNV-1a checksum, pre-copy structural validator, and O(1) `.mathc` serializer/loader (`ext_save_mathc_mem`, `ext_load_mathc_mem`).
  - `user_loader.h` / `user_loader.c`: Host-facing user extension compile, link, and `.mathc` load API (`user_build_create`, `user_build_destroy`, `user_build_init`, `user_compile_unit`, `user_link_ext`, `user_link_ext_buf`, `user_compile_and_link`, `user_compile_ext`, `user_load_mathc`).
  - `ext_loader.h`: Umbrella header including `compiler.h`, `linker.h`, and `mathc_bin.h`.
- **`Core/session/` (Parallel In-Memory Sessions & Order-Tagged Snapshots)**:
  - `session.h` / `session.c`: Isolated parallel session lifecycle (`eng_session_open`, `eng_session_close`, `eng_session_reset`) and stateful evaluation (`eng_session_eval`).
  - `snapshot.h` / `snapshot.c`: Canonical value expression serializer, order-tagged 32-bit FNV-1a integrity hash-chained raw-text snapshot builder (`MSP1|seq:...|prev:...|hash:...`), iterative snapshot reader (`eng_session_snapshot_next`), and full-history session continuation (`eng_session_restore`).
- **`Core/lib/delegate/` (`delegate.h` / `delegate.c` — C `@` Intrinsics)**: Contains the `@<intrinsic>` registry and all C implementations behind `@` delegates (`@add`, `@sin`, `@vpush`, `@vshift`, `@tostr`, `@fromstr`, `@mmul`, `@mlinsolve`, `@mhadamard`, `@calc_integ`, `@calc_ode`…). Every section in `delegate.h` and `delegate.c` is annotated with `/*Below section is the C's native ...*/` and `/*Construct ...*/` banners (section 16).
- **`Core/lib/hardcode/` (`hardcode.h` / `hardcode.c` — Hardcoded Grammar & Ergonomic Rules)**: Isolates the grammar rules fixed in C (newline statement separators, reserved words, implicit multiplication, undefined iterator variables `i in 1...5 {}` and `i, x in v {}`, dual unit/variable `"in"` disambiguation `25m in km`, parameterized containers `vec<T>()` / `list<T>()` / `stack<T>()` / `queue<T>()`, comprehensions, lambdas, and pattern-matched `switch`). Every section in `hardcode.h` and `hardcode.c` is annotated with `/*code section below manages hardcoding ...*/` and `/*Construct ...*/` banners (section 16).
- **`Library/*.math` (Standard Library — 15 files, 17 module units)**: Order-independent `.math` modules (no numerical prefixes) compiled and linked by `Library/mathc.c` into `Core/Headers/stdlib_precompiled.h` (`Library/stdlib.mathc`) and loaded in one `memcpy` by `eng_init`.
- **`Test/`**: Contains the host harness, CLI (`-e`/`--eval`, `--repl`, `--bench`), & 529-test verification suite (`Test/main.c`), sample user extension (`Test/user_physics.math`), and this canonical guidebook (`Test/Math Engine Language (MEP) — Complete Reference.md`).

## 2. Quick Start & Tutorial Tour

### 2.1 Embedding in C (`math_engine.h`)

```c
#include "math_engine.h"
#include <stdio.h>

/* Engine (~683.7 KB) holds isolated arenas + 256 KB .mathc buffer: use eng_create() or static */

int main(void) {
    Engine *eng = eng_create();
    if (!eng) return 1;
    Value out;
    if (eng_eval(eng, "ke(2 kg, 36 kph)", &out)) {
        char buf[128];
        eng_format_value(eng, out, buf, sizeof buf);
        printf("%s\n", buf);              /* 100 */
    } else {
        fprintf(stderr, "%s\n", eng_last_error(eng));
    }
    eng_destroy(eng);
    return 0;
}
```

### 2.2 First Tour of the Language

Every result below is verified live by the engine:

```text
1 + 2 * 3                              # 7
2 sin(π / 2) + 2 √16 + (2π / π)        # 12   (implicit multiplication)
100 km                                 # 100000  [distance]  (stored in SI metres)
25km in m                              # 25000   [distance]  (bare unit conversion)
(2.5 km) in (1 m)                      # 2500                (dimensionless ratio)
42_                                    # "42"    (postfix _ converts any value to $str)
_"3 + 4i"                              # 3 + 4i  (prefix _ parses $str into best type)
5 into 5i                              # 5 + 5i  (into operator composes values)
v := vec<real>() ; 10 into v ; 20 into v ; v   # [10, 20]
q := queue<real>(10, 20) ; q.enqueue(30) ; q.dequeue()   # 10 (FIFO queue)
s := stack<real>(10, 20) ; s.push(30) ; s.pop()          # 30 (LIFO stack)
0...1 by 0.25                          # [0, 0.25, 0.5, 0.75, 1]  (stepped range)
[i * 10 + x for i, x in vec(5, 6, 7)]  # [5, 16, 27]              (indexed comprehension)
5 |> (x => x + 3) |> (x => x^2)        # 64                       (pipe operator |>)
1_000_000 + 2.5e3                      # 1002500                  (digit separators & sci notation)
"Hello, " + "MEP!"                     # "Hello, MEP!"            (string concatenation)
((2 mol) * (8.314462618 JpmolK) * (300 K)) / (50 L) # 99773.55142 [pressure] (7D SI unit algebra)
linsolve([2, 1 ; 5, 3], [4, 11])       # [1, 2]                   (NxN linear solver)
ode_rk4(t, y, y, 0, 1, 1)              # 2.718281828              (RK4 ODE solver)
quad_roots(1, -5, 6)                   # vec2(x=3, y=2)
diff(x, x^3 + 2*x, 3)                  # 29
```

### 2.3 Building the Project

Build with the provided `makefile` (`make run`, `make run release`, `make stdlib`), `CMakeLists.txt`, or directly with `gcc`/`clang`:

```bash
gcc -std=c11 -Wall -Wextra -Wconversion -Werror -g -Og -fsanitize=address,undefined \
    -ICore/Headers -ICore/compile -ICore/session -ICore/lib/delegate -ICore/lib/hardcode \
    Core/Code/*.c Core/compile/*.c Core/session/*.c \
    Core/lib/delegate/delegate.c Core/lib/hardcode/hardcode.c \
    Test/main.c -lm -o myapp && ./myapp
```

## 3. Lexical Structure

**Source text** is UTF-8. Symbols such as `π`, `φ`, `ϕ`, `°`, and `√` are ordinary tokens.

**Comments** start with `#` and run to the end of the line.

**Numbers** are decimal, **hexadecimal** (`0xFF`, `0x1A_2B`), or **binary** (`0b1010_0101`) with optional **scientific notation** (`1e-9`, `6.022e23`, `2.5E+4`) and **underscore digit separators** between digits (`1_000_000`, `2_500.75_5`). A trailing `_` after a number (`42_`) is still the postfix stringifier (`"42"`), and `2e` without a following digit or `+/-` is still implicit multiplication `2 * e` (`5.436563657`). Negative numbers are the unary minus operator applied to a literal. Results print with 10 significant digits (`1/3` prints `0.3333333333`; very large values use `1.23456789e+11`).

**Strings** are double-quoted literals (`"USD"`). They support concatenation (`"Hello, " + "MEP!"`), `len(s)` (`s.len()`), character indexing (`s[0]`, `s[-1]`), substring & stepped slicing (`s[0...4]`, `s[3...0]`, `s[0...5 by 2]`), substring search (`"lo" in "Hello"` -> `1`), character iteration (`(c in "abc") { ... }`, `(i, c in "abc") { ... }`), pure MEP string helpers in `[string]` (`is_empty`, `substr`, `str_repeat`, `str_reverse`, `starts_with`, `ends_with`, `chars`, `str_join`), `==`, `!=`, `type_of`, host callbacks, and converting to/from typed values via the `"_"` operators (section 5.1).

**Constants** `π` and `pi` (`3.141592654`), `e` (`2.718281828`), and `phi`, `φ`, and `ϕ` (golden ratio `1.618033989`) are predefined in `[arith]`. Additional physical and chemical constants are predefined in `[physics]` and `[chemistry]` (section 10.0.2).

**Statements** are separated by a newline or by `;`:

```text
x := 10
y := 20
x + y            # 30   (newline acts as ';')

x := 10 ; y := 20 ; x + y     # on one line, ';' is compulsory
x := 10 y := 20               # ERROR: Unexpected trailing token 'y'
```

A line ending in an infix operator continues onto the next line. A line starting with a prefix-capable operator (`-x`, `!c`, `_"42"`) starts a new statement. Implicit multiplication never crosses a newline. A trailing `;` before `}`, `)`, `]` or end of input is ignored.

**Reserved words** cannot be variable, parameter, or tag names: `if or then else default switch case jump run break continue return`. (`loop` is not a reserved keyword and can be used freely as a user scope tag `loop : { ... }` or variable name.)

## 4. Types, Units and Variables

### 4.1 Entity Types

| Type | Module | Kind & Arena Footprint | Meaning | Example |
| :-- | :-- | :-- | :-- | :-- |
| `real` | `[boot_types]` | Consuming scalar (inline 8B NaN-box or 1 slot) | 64-bit IEEE-754 float | `42`, `3.14`, `1_000_000`, `1.5e-3` |
| `iota` | `[boot_types]` | Consuming scalar (1 slot) | Pure imaginary scalar | `5i`, `(3 + 4)i` |
| `complex` | `[complex_ext]` | Struct (`re`, `im` — 2 slots) | Complex number | `1 + 2i`, `5 into 5i`, `complex(3, 4)` |
| `str` | `[boot_types]` | Consuming string (interned `StrSlice`) | UTF-8 string slice | `"Euler"`, `42_` |
| `vec` | `[boot_types]` | Dynamic container (4-slot header + payload) | Growable homogeneous container (`vec<T>`, `list<T>`, `stack<T>`, `queue<T>`) | `vec<real>(1, 2, 3)` |
| `mat` | `[boot_types]` | Dense tensor (8-slot header + payload) | 1D–4D dense matrix/tensor | `[1, 2 ; 3, 4]` |
| `fn` | `[boot_types]` | Closure (4-slot header) | First-class lambda closure (0–16 params, defaults, recursion) | `(x, k = 2) => x * k` |
| `map` | `[boot_types]` | Dictionary (4-slot header + key/val table) | Ordered string-keyed map/dictionary (`$map`) | `dict("a", 10, "b", 20)` |
| `vec2` | `[geom_ext]` | Struct (`x`, `y` — 2 slots) | 2D Cartesian vector | `vec2(3, 4)` |
| `vec3` | `[geom_ext]` | Struct (`x`, `y`, `z` — 3 slots) | 3D Cartesian vector | `vec3(1, 2, 3)` |
| `circle` | `[geom_ext]` | Nested struct (`center:vec2`, `radius` — 3 slots) | 2D circle | `circle(vec2(1, 2), 5)` |
| `mat2` | `[algebra]` | Struct (`a`, `b`, `c`, `d` — 4 slots) | 2×2 row-major matrix | `mat2(4, 7, 2, 6)` |

`type_of(x)` returns the entity type name as a string; `unit_of(x)` returns the unit group name (`"distance"`, or `"none"`).

### 4.2 Units

Postfix unit operators bind at precedence 95 and normalise to SI base units, tagging the value with its unit group: `100 km` is `100000 [distance]`, `90 deg` is `1.5708 [angle]`.
- **Addition and subtraction (`+`, `-`)** require matching unit groups and never form new compound units: `50m + 20cm` gives `50.2 [distance]`, while `100cm + 60°` fails with `Unit mismatch for '+'`.
- **Multiplication, division, powers, and roots (`*`, `/`, `^`, `√`/`sqrt`, `cbrt`)** automatically perform **full 7D SI dimensional exponent algebra** $(\text{m}^L \cdot \text{kg}^M \cdot \text{s}^T \cdot \text{A}^I \cdot \text{K}^\Theta \cdot \text{mol}^N \cdot \text{cd}^J)$ across all 7 SI base units (declared with `@dim` in `Library/units.math`) and **compound unit cancellation**:
  - **Kinematics & Mechanics (`m`, `kg`, `s`)**: `(100 m) / (10 sec)` $\to$ `10 [speed]`, `(36 kph) * (10 sec)` $\to$ `100 [distance]`, `(20 mps) / (4 sec)` $\to$ `5 [acceleration]`, `(10 kg) * (9.8 mps2)` $\to$ `98 [force]` (`N`), `(50 N) * (4 m)` $\to$ `200 [energy]` (`J`), `(200 J) / (10 sec)` $\to$ `20 [power]` (`W`), `(100 N) / (2 m2)` $\to$ `50 [pressure]` (`Pa`).
  - **Electromagnetism (`A`)**: `(2 A) * (5 sec)` $\to$ `10 [charge]` (`C`), `(12 V) * (2 A)` $\to$ `24 [power]` (`W`), `(12 V) / (4 ohm)` $\to$ `3 [current]` (`A`), `(2 F_cap) * (5 V)` $\to$ `10 [charge]` (`C`).
  - **Thermodynamics (`K`)**: `(500 J) / (25 K)` $\to$ `20 [heat_capacity]` (`J/K`), `(4184 J) / ((2 kg) * (5 K))` $\to$ `418.4 [specific_heat]` (`J/(kg*K)`), and calorimetry `(2 kg) * (4184 JpkgK) * (10 K)` $\to$ `83680 [energy]` (`J`).
  - **Chemistry & Stoichiometry (`mol`)**: `(36 gram) / (18 gpmol)` $\to$ `2 [substance]` (`mol`), `((0.5 mol) / (250 mL)) in M_conc` $\to$ `2 [molarity]` (`mol/L`), `(100 J) / (2 mol)` $\to$ `50 [molar_energy]` (`J/mol`), `(0.5 mol) / (10 sec)` $\to$ `0.05 [catalytic_activity]` (`kat`), and Ideal Gas Law `((2 mol) * (8.314462618 JpmolK) * (300 K)) / (50 L)` $\to$ `99773.55142 [pressure]` (`Pa`).
  - **Photometry (`cd`, `lm`, `lx`, `klx`)**: `(800 lm) / (4 m2)` $\to$ `200 [illuminance]` (`lx`), `(200 lx) * (4 m2)` $\to$ `800 [luminous_intensity]` (`cd`/`lm`).
  - **Powers, Roots & Cancellation**: `(5 m)^2` $\to$ `25 [area]`, `√(25 m2)` $\to$ `5 [distance]`, `(2 m)^3` $\to$ `8 [volume]`, `cbrt(27 m3)` $\to$ `3 [distance]`, `1 / (0.02 sec)` $\to$ `50 [frequency]` (`Hz`), `(2 km) / (500 m)` $\to$ `4 [none]`.
Groups are declared in `.math` files, so you can also invent custom non-SI unit groups (section 12).

Because postfix operators only fire after a complete operand, `m := 5 ; m + 10` treats `m` as a variable, while `50m` is fifty metres. `50im`, `(50m)i`, and `50 m i` give `50i [distance]`, whereas `50mi` is 50 miles (`80467.2 [distance]`) because maximal munch matches the 2-character `mi` (miles) suffix.

### 4.3 Declaring and Assigning Variables

`:=` declares, `=` assigns to an existing variable. Compound forms are `+= -= *= /= %= ^=`.

| Declaration | Entity lock | Unit lock | Effect |
| :-- | :-- | :-- | :-- |
| `x := 5` | dynamic | dynamic | may become complex or change unit |
| `x : real := 5` | `real` | dynamic | `x += 5i` is rejected |
| `x : distance := 50m` | dynamic | `distance` | `x += 50im` promotes to `50 + 50i [distance]`; `x += 60°` is rejected |
| `x : real, distance := 50m` | `real` | `distance` | `x += 10cm` gives `50.1`; unitless assignment is rejected |
| `v : vec<vec2> := vec<vec2>()` | `vec` (elem `vec2`) | dynamic | locks variable to `$vec` |

Multiple variables: `x, y := 5` broadcasts a scalar. Destructuring unpacks a struct, `vec` or `mat` in order: `r1, r2 := quad_roots(1, -5, 6)` gives `r1 = 3`, `r2 = 2`; `a, b, c := vec(10, 20, 30)`.

### 4.4 Scope and Lifetime

Both `( ... )` and `{ ... }` can contain multiple `;`- or newline-separated statements and evaluate to their last expression, but they differ in **lexical scoping**, **implicit multiplication**, and **block grammar**:
- **`{ ... }` (Lexical Scope & Control Block)**: Opens a **new child lexical scope**. Variables declared with `:=` inside `{ ... }` are local to that block (shadowing outer variables without leaking out), whereas `=` mutates an existing outer variable. `{ ... }` also delimits control/loop bodies (`if`, `switch`, `for-in`, `while`/`do-while`), never triggers implicit multiplication after `}`, and allows empty `{}` (`<void>`).
- **`( ... )` (Same-Scope Grouping Expression)**: Groups one or more statements **in the current scope** without creating a child lexical scope—so variables declared with `:=` inside `( ... )` remain visible after `)`. Parenthesized expressions also participate in implicit multiplication (`(a + b)(c + d)`).

```text
x := 1 ; { x := 99 } ; x                # 1  ({} opens a child lexical scope)
x := 1 ; { x = 99 } ; x                 # 99 (= mutates outer x)
{ a := 3 ; b := 4 ; a^2 + b^2 }         # 25 (multi-statement block)
(a := 3 ; b := 4 ; a + b) * 10 + a      # 73 (() runs multiple statements in current scope; a stays visible)
```

In stateless `eng_eval` calls, variables live only for that single call (`eng_eval("x := 10")` followed by `eng_eval("x")` fails with `Undefined symbol or constant 'x'`). In session calls (`eng_session_eval`), top-level variables (along with their concrete type/unit locks, containers, structs, and lambda closures) persist across evaluations within the session and are captured in each evaluation's order-tagged raw-text snapshot.

## 5. Operators and Precedence

Higher numbers bind tighter. Everything here is declared in the library, so extensions can add to it.

| Prec | Kind | Operators |
| --: | :-- | :-- |
| 95 | postfix / member | `.` field or UFCS call, `v[i]`, `f(x)`, `i` (imaginary), `_` (`@tostr` stringifier), unit suffixes (`km`, `deg`, `°`, `kg`, `hr`…) |
| 90 | postfix | `!` factorial, `to_<unit>` conversions, `++ +++ -- ---` |
| 85 | right infix, prefix | `^`, `.^` (elementwise power); prefix `++ +++ -- ---`, prefix `_` (`@fromstr` string-to-type parser) |
| 80 | prefix | `-`, `+`, `√`, `!` (boolean NOT) |
| 60 | left infix | `*`, `/`, `%`, `.*`, `./` (Hadamard elementwise), and implicit multiplication (`60 -> 65`) |
| 55 | right infix | `into` (composite constructor & container insert) |
| 50 | left infix | `+`, `-` (including string concatenation & `vec + vec`, `vec - vec`) |
| 45 / 44 / 42 | left infix | `..`, `...`, `..<`, `<..`, `plus`, `minus` (45); `by` stepped range modifier (`a...b by step` at 44); `in` membership/substring & unit conversion (`:<42`) |
| 35 / 34 | left infix | `&`, `\|` (distributed combinators) |
| 30 | left infix | `==  !=  <  <=  >  >=`, `~` (approximately equal) |
| 25 | prefix | `not` (boolean negation binding below relational comparisons) |
| 20 | left infix | `&&`, `and`, `nand`, and `**` (vec3 dot product) |
| 18 / 15 | left infix | `xor`, `^^` (18) / `\|\|`, `or`, `nor` (15) |
| 12 | left infix | `\|>` pipe operator (`x \|> f`, `x \|> f(a, b)`, `x \|> (a => ...)`) |
| 10 | right infix | `cond ? a : b` |
| 8 | right infix | `=>` lambda |
| 5 | right infix | `:=  =  +=  -=  *=  /=  %=  ^=` |
| 3 | left infix | `??` (`(cond) ?? { body }` while-loop; `{ body } ?? (cond)` do-while loop), `?!?` (`(body) ?!? (cond)` do-while loop) |
| 1 | left infix | `;` and newline |

### 5.1 Plain String Conversion Operators (`x_` and `_s`)

- **Postfix `"_"` (`(x:T) "_" :95 -> str = @tostr(x)`)**:
  Converts any runtime value (`real`, `iota`, `complex`, `vec2`, `vec3`, `circle`, `mat2`, `vec`, `mat`, `fn`, `str`) into its plain string (`$str`) representation:
  ```text
  42_                  # "42"
  (3 + 4i)_            # "3 + 4i"
  vec2(3, 4)_          # "vec2(x=3, y=4)"
  vec(10, 20, 30)_     # "[10, 20, 30]"
  [1, 2 ; 3, 4]_       # "[[1, 2], [3, 4]]"
  ```
- **Prefix `"_"` (`"_"(s:str) :85 -> T = @fromstr(s)`)**:
  Parses and evaluates a plain string `$str` into its most appropriate non-string runtime type (`real`, `iota`, `complex`, structs like `vec2`/`vec3`/`circle`/`mat2`, 1D dynamic `vec`, multidimensional `mat`, or unit-tagged quantities like `"100 km"`). If the string cannot be converted into any valid non-string type, it raises `No compatible type found for '...'`:
  ```text
  _"42" + 8            # 50          (real)
  _"3 + 4i"            # 3 + 4i      (complex)
  _"vec2(3, 4)"        # vec2(x=3, y=4)
  _"[10, 20, 30]"      # [10, 20, 30] (vec)
  _"[[1, 2], [3, 4]]"  # [[1, 2], [3, 4]] (mat)
  unit_of(_"100 km")   # "distance"
  _(vec3(1, 2, 3)_)    # vec3(x=1, y=2, z=3) (lossless round-trip)
  _"not_a_type"        # ERROR: No compatible type found for 'not_a_type'
  ```

### 5.2 The `"into"` Insert & Composition Operator

`"into"` is a right-associative infix operator (`:>55`) that inserts a component or element into another value and returns the resulting composite type or container:

| Expression | Result Type | Behavior / Result |
| :-- | :-- | :-- |
| `5 into 5`, `50m into 20cm`, `5i into 3i`, `(1+2i) into (3+4i)`, `vec2 into vec2`, `vec3 into vec3`, `mat2 into mat2`, `mat into mat`, `str into str` | same entity & unit | **Likewise entities & units return their sum / concatenation**: `5 into 5` is `10`, `50m into 20cm` is `50.2 [distance]`, `5i into 3i` is `8i`, `"a" into "b"` is `"ab"` |
| `5 into 5i`, `5i into 5` | `complex` | Cross-entity `real` + `iota` combines into `5 + 5i` (`complex(5, 5)`) |
| `2 into (3 + 4i)` | `complex` | Adds real component into complex: `5 + 4i` |
| `3i into (3 + 4i)` | `complex` | Adds imaginary component into complex: `3 + 7i` |
| `7 into vec2(3, 4)` or `vec2(3, 4) into 7` | `vec3` | Promotes 2D `vec2(x, y)` + `z` into 3D `vec3(x=3, y=4, z=7)` |
| `item into v` (`v` is `vec`/`list`/`stack`/`queue`) | `vec` | Appends `item` into `v` (`@vpush(v, item)`), acting as an infix alternative to `v.push(item)` |

Because `"into"` is right-associative, `10 into 20 into vec<real>()` chains naturally (`20` is pushed first, then `10`, yielding `[20, 10]`), and `v := vec<real>() ; 10 into v ; 20 into v` yields `[10, 20]`.

### 5.3 Implicit Multiplication, Booleans, Approximate Equality & Complex Numbers

- **Implicit multiplication** inserts `*` when a value, variable, or `)` is directly followed by an identifier, `(`, or `√`: `x := 4 ; 2x^2 + 3x + 1` is `45` (the `^` binds first), `3(x+1)(x-1)` works, and `2 3` is a syntax error.
- **Booleans** are reals: `0` is false, anything non-zero is true, and comparisons return `1` or `0`. Both prefix `!` (precedence 80) and prefix `not` (precedence 25) strictly flip boolean `0` and `1`; `!5` raises `Operand 5 out of range for boolean negation '!': expected 0 or 1`. Because `not` binds at precedence 25 (below comparisons at 30), `not x == 5` and `!(x == 5)` both negate the boolean result of `x == 5`.
- **Approximate equality** `a ~ b` is true when `|a - b| <= 0.001`: `5.001 ~ 5` is `1`, `5.01 ~ 5` is `0`. Works on `real`, `iota`, `complex`, `vec2`, `vec3`, and `mat2`.
- **Distributed comparisons** let one comparison fan out over `&`, `|`, and `!`: `x == 1 | 3` means `(x == 1) | (x == 3)`, `(x & y == 1 | 2)` means `((x == 1) | (x == 2)) & ((y == 1) | (y == 2))`, and `x == !(1 | 2)` means `!((x == 1) | (x == 2))`.
- **Complex numbers.** The postfix `i` makes an imaginary scalar and is typed by multiple dispatch: `1 + 1i` (`1 + 1i`), `(3 + 4)i` (`7i`), `((3 + 4)i)i` (`-7`), `(1 + 2i)i` (`-2 + 1i`), `(3 + 4i) / (1 - 2i)` (`-1 + 2i`).
- **Four Range Operators, Stepped Ranges (`by`), and `range(start, stop, step = 1)`.**
  - `a..b` (2-dot): **exclusive–exclusive** `(a, b)` — `1..5` builds `[2, 3, 4]`.
  - `a...b` (3-dot): **inclusive–inclusive** `[a, b]` — `1...5` builds `[1, 2, 3, 4, 5]`.
  - `a..<b`: **inclusive–exclusive** `[a, b)` — `1..<5` builds `[1, 2, 3, 4]`.
  - `a<..b`: **exclusive–inclusive** `(a, b]` — `1<..5` builds `[2, 3, 4, 5]`.
  - **Stepped Ranges (`by` & `range`)**: Any of the 4 range operators can be followed by `by step` (supporting integer, fractional, and negative steps), or constructed with `range(start, stop, step = 1)`:
    - `1...10 by 2` $\to$ `[1, 3, 5, 7, 9]`, `0...1 by 0.25` $\to$ `[0, 0.25, 0.5, 0.75, 1]`, `10...2 by -3` $\to$ `[10, 7, 4]`, `range(1, 10, 3)` $\to$ `[1, 4, 7, 10]`.
    - Stepped ranges iterate in **O(1) memory** inside `for-in` loops and comprehensions (`(x in 0...10 by 2) { ... }`), support O(1) `in` membership (`7 in (1...10 by 2)` is `1`, `6 in (1...10 by 2)` is `0`), and work as strided slices on vectors and strings (`v[0...5 by 2]`, `"abcdef"[0...5 by 2]` $\to$ `"ace"`).
- **Pipe Operator (`|>`, precedence 12).** Passes the left-hand value into a function (`x |> f` $\equiv$ `f(x)`), prepends it as the first argument of a call (`x |> f(a, b)` $\equiv$ `f(x, a, b)`), or invokes a lambda (`5 |> (x => x + 3) |> (x => x^2)` $\to$ `64`).

## 6. Control Flow

Every construct is an expression with a value.

**Conditionals.** `if` takes `or` (or `then`) for else-if branches and `else` for the fallback; a 3-argument function form `if(cond, then_val, else_val)` also exists.

```text
x := 7
if (x < 5) { "small" } or (x < 10) { "medium" } else { "large" }   # "medium"
if(1, 10, 20)                      # 10
x == 7 ? "yes" : "no"              # ternary
```

**`switch`** matches exact values, inclusive/exclusive ranges, relational guards, and destructuring patterns; patterns combine with `,` and take an optional `if` guard. Cases end with `else` (or `default`).

```text
switch (5) { case 1, 2 : { "low" }  case 3...10 : { "mid" }  case > 100 : { "high" }  else : { "other" } }   # "mid"

p := vec2(0, 7)
switch (p) {
    case vec2(0, 0)           : { 0 }
    case vec2(0, y)           : { y * 10 }       # binds y
    case vec2(x, y) if x == y : { x * 100 }
    else                      : { -1 }
}                                                # 70
```

**Loops (`for-in`, Pre-Tested `while`, and Post-Tested `do-while`).**
- **Single-Variable & 2-Variable Indexed `for-in` Loops**: Either a parenthesized header `(var in iterable) { ... }` / `(i, val in iterable) { ... }` or an **unparenthesized** header `var in iterable { ... }` / `i, val in iterable { ... }` (with optional `??`) automatically declares the loop variable(s) and iterates over any range (including `by` stepped ranges in O(1) memory), `vec`, `mat`, or `str` (yielding 1-character strings). In the 2-variable form `i, val in iterable`, `i` receives the 0-based index (`0, 1, 2, ...`) and `val` receives the element—while standalone `undef_var in 1...10` without a loop body still fails with `Undefined symbol or constant 'undef_var'`.
- **Pre-Tested `while` Loop (`(cond) ?? { body }`)**: Putting the condition on the left of `??` and the `{ body }` block on the right tests `cond` before each iteration. Use `1 ?? { ... }` for an unconditional loop exited with `break`.
- **Post-Tested `do-while` Loop (`{ body } ?? (cond)` and `(body) ?!? (cond)`)**: Putting the `{ body }` brace block on the **left** of `??` and `(cond)` on the **right** executes `{ body }` once before testing `(cond)` at the bottom of each iteration (`continue` jumps straight to the bottom `(cond)` check). For single-expression bodies without braces, use the dedicated `?!?` operator (`(body) ?!? (cond)`).

```text
sum := 0 ; (i in 1...5) { sum += i } ; sum                  # 15 (parenthesized for-in)
sum := 0 ; i in 1...5 { sum += i } ; sum                    # 15 (unparenthesized for-in)
sum := 0 ; (i, x in vec(10, 20, 30)) { sum += i * x } ; sum # 80 (2-var indexed for-in)
sum := 0 ; k in 1..<4 ?? { sum += k * 10 } ; sum            # 60 (for-in with optional ??)
n := 0 ; (n < 5) ?? { n += 2 } ; n                          # 6  (pre-tested while)
k := 10 ; { k += 1 } ?? (k < 5) ; k                         # 11 (post-tested do-while: runs once)
k := 0 ; s := 0 ; { k += 1 ; s += k } ?? (k < 5) ; s        # 15 (post-tested do-while: 1..5)
k := 10 ; (k += 5) ?!? (k < 5) ; k                          # 15 (explicit ?!? do-while operator)
k := 0 ; 1 ?? { k += 1 ; if (k >= 7) { break } } ; k        # 7  (unconditional loop with break)
```

**Jumps & Exiting `switch` Cases (`break` vs `return`).**
- In `switch`, cases **never fall through**—when a matching `case ... : { ... }` block finishes, its last expression automatically becomes the value of the `switch` without needing `break`.
- To exit a `switch` case **early** (e.g. inside an `if` guard mid-block):
  - `break`, `break expr`, or `break(expr)` exits the **`switch` block** early and makes the `switch` evaluate to `expr` (or the last statement evaluated before bare `break`).
  - `break(Tag)` or `break(Tag, expr)` exits an enclosing tagged loop or scope `Tag : ...` with `expr`.
  - `return`, `return expr`, or `return(expr)` exits the **entire enclosing function** (or top-level script) immediately with `expr`.
- Loops also evaluate to the value of `break expr` / `break(expr)`:

```text
found := (x in 1...20) { if (x^2 > 50) { break x } }   # 8
switch (2) { case 2 : { x := 25 ; if (x > 20) { break(x * 2) } ; 999 } else : { 0 } }   # 50
```

**Tagged scopes.** `Tag : loop-or-block` names a scope so inner code can target it with `break(Tag)`, `break(Tag, value)`, `continue(Tag)`, `jump(Tag)` and `run(Tag)` (or `Tag()`):

```text
res := Outer : (i in 1...5) {
    (j in 1...5) { if (i * j == 12) { break(Outer, i * 100 + j) } }
}                                   # 304
```

## 7. Collections (`vec<T>`, `list<T>`, `stack<T>`, `queue<T>`), Matrices, Comprehensions & Lambdas

### 7.1 Parameterized Dynamic Vectors, Lists & Sublists (`stack<T>`, `queue<T>`)

All dynamic homogeneous 1D containers share the `$vec` runtime representation and support explicit `<T>` (and `<T, unit>`) type-parameterized nomenclature as well as untyped inference:

| Container Form | Variations Supported | Description |
| :-- | :-- | :-- |
| `vec<T>(...)` / `vec(...)` | `vec<T>()`, `vec<T>(cap)`, `vec<T>(count, init)`, `vec<T>(a, b, c, ...)`, plus 1- or 2-arg element constructors when arg 1 is non-`real` or unit-tagged (`vec("a")`, `vec(10m, 20m)`) | Growable homogeneous vector; locks element type to `T` (and optional unit in `vec<real, distance>(...)`) even when created empty |
| `list<T>(...)` / `list(...)` | `list<T>()`, `list<T>(a)`, `list<T>(a, b, ...)` | Element-wise list constructor (1 or 2 args are always elements, never capacity/fill) |
| `stack<T>(...)` / `stack(...)` | `stack<T>()`, `stack<T>(a, b, ...)` | LIFO stack sublist supporting `.push(x)` / `x into s`, `.pop()`, `.peek()` / `.top()` / `.back()`, `.is_empty()` |
| `queue<T>(...)` / `queue(...)` | `queue<T>()`, `queue<T>(a, b, ...)` | FIFO queue sublist supporting `.enqueue(x)` / `.push(x)` / `x into q`, `.dequeue()` / `.shift()` / `.pop_front()`, `.front()`, `.is_empty()` |

All elements in a container must share the locked element type `T` (and unit lock, if specified):
- `v := vec<vec2>() ; 10 into v` raises `Vector type mismatch: cannot push 'real' into vec<vec2>`.
- `q := queue<real, distance>() ; q.enqueue(60 deg)` raises `Vector unit mismatch`.

| Call / Operator | Result |
| :-- | :-- |
| `push(v, x)`, `v.push(x)`, `x into v` | append to back (mutates `v`) |
| `pop(v)`, `v.pop()` | remove and return last element (LIFO pop) |
| `peek(v)`, `top(v)`, `back(v)` | inspect last element without removing (`v[-1]`) |
| `enqueue(q, x)`, `q.enqueue(x)` | append to back of queue (`@vpush(q, x)`) |
| `dequeue(q)`, `shift(q)`, `pop_front(q)` | remove and return first element (`@vshift(q)`, FIFO dequeue) |
| `front(q)`, `q.front()` | inspect first element without removing (`q[0]`) |
| `is_empty(v)`, `v.is_empty()` | returns `1` if `len(v) == 0`, else `0` |
| `at(v, i)`, `set(v, i, x)`, `v[i]` | read / write; negative `i` counts from the end |
| `len`, `cap`, `resize(v, n)`, `shrink`, `clear` | size and capacity management |
| `vec_sum`, `sum`, `mean`, `variance`, `stddev`, `sma(v, w)` | statistical reductions (`variance` is population variance) |

Slicing returns a new vector and also assigns: `v := vec(10, 20, 30, 40, 50) ; v[1...3]` is `[20, 30, 40]`, `v[0..4]` is `[20, 30, 40]`, `v[-3...-1]` is `[30, 40, 50]`, `v[3...1]` reverses, and `v[1...3] = 99` broadcasts.

### 7.2 Matrices (`mat`)

Literals use `[...]`: `[1, 2, 3]` (1D), `[[1, 2], [3, 4]]` or `[1, 2 ; 3, 4]` (2D), and nested forms up to four dimensions. Index with `M[r, c]` or `M[r][c]`, and slice with ranges: `M[0...1, 1...2]` gives a submatrix, `M[2, 0...1]` a row.

Arithmetic: `A + B`, `A - B`, `A into B`, `A * B` (matrix product), `s * A`, `A * s`, `A / s`, `-A`, `item in A`, plus **Hadamard elementwise operators** `A .* B`, `A ./ B`, `A .^ B`, `A .^ s`, `s .^ A` (which also work on `vec` alongside `v1 + v2` and `v1 - v2`). Functions: `linsolve(A, b)` (general $N \times N$ Gaussian elimination with partial pivoting for 1D `vec`/`mat` or 2D multi-RHS `B`), `det`, `trace`, `transpose`, `inv` (Gauss–Jordan), `mat_zeros(r, c)`, `mat_ones(r, c)`, `mat_eye(n)`, `mat_fill(r, c, v)`, `mat_rows`, `mat_cols`, `mat_rank`, `mat_len`, `mat_shape`. Example: `[[1,2],[3,4]] * [[5,6],[7,8]]` is `[[19, 22], [43, 50]]`, and `linsolve([2, 1 ; 5, 3], [4, 11])` is `[1, 2]`.

### 7.3 Comprehensions

`[expr for x in iterable if cond]` and the 2-variable indexed form `[expr for i, x in iterable if cond]` build a vector from a range (including `by` stepped ranges), `vec`, `mat`, or `str`:

```text
[x^2 for x in 1...5]                      # [1, 4, 9, 16, 25]
[x for x in 0...10 by 3]                  # [0, 3, 6, 9]
[i * 100 + x for i, x in vec(10, 20, 30) if i != 1]  # [10, 230]
pts := list<vec2>(vec2(1, 2), vec2(3, 4), vec2(5, 6))
[p.x + p.y for p in pts if p.x > 1]       # [7, 11]
```

### 7.4 Lambdas and Higher-Order Functions

Lambdas are first-class closures: `x => x^2`, `(a, b) => a + b`, `() => 42`, or with a block body `(x) => { y := x * 2 ; y + 1 }`. They capture variables from the enclosing scope. Call with `f(5)`, `call(f, 5)`, or immediately: `((x) => x * 3)(7)` is `21`.

```text
k := 10 ; scale := x => x * k ; scale(7)       # 70
```

Higher-order methods work on both `vec` and `mat`: `map`, `filter`, `reduce(f)`, `reduce(init, f)` / `fold(init, f)`, `any`, `all`, `find`, `count_if`.

```text
(1...5).map(x => x^2).filter(x => x > 5).sum()     # 50
(1...4).fold(10, (acc, x) => acc + x)              # 20
vec(3,1,2).count_if(x => x >= 2)                   # 2
[1,2;3,4].map(x => x * 10)                         # [[10, 20], [30, 40]]
```

**UFCS.** `obj.fn(a, b)` calls `fn(obj, a, b)` whenever `fn` is not a field of `obj`, so calls chain: `v.push(4).push(5).len()`, `[4, 7 ; 2, 6].transpose().det()` (`10`).

## 8. Structs and Math Types

| Type | Fields | Operators & Functions |
| :-- | :-- | :-- |
| `complex` | `re`, `im` | `-z`, `+`, `-`, `*`, `/` (with `complex`, `real`, `iota`), `==`, `!=`, `~`, `into`; `conj`, `cabs`, `carg`, `cexp_i`, `polar` |
| `vec2` | `x`, `y` | `-v`, `+`, `-`, `s * v`, `v * s`, `v / s`, `==`, `!=`, `~`, `into` (`vec2 + vec2` or `z into vec2` $\to$ `vec3`); `dot`, `cross2`, `vec2_mag`, `vec2_dist` |
| `vec3` | `x`, `y`, `z` | `-v`, `+`, `-`, `s * v`, `v * s`, `v / s`, `a ** b` (dot product), `==`, `!=`, `~`, `into`; `dot3`, `cross3`, `vec3_mag`, `vec3_dist`, `lerp`, `vec3_path_len` |
| `circle` | `center:vec2`, `radius` | `circle_area`, `circle_circum`, `move_circle` |
| `mat2` | `a, b, c, d` | `-m`, `+`, `-`, `m1 * m2`, `m * vec2`, `s * m`, `m * s`, `m / s`, `==`, `!=`, `~`, `into`; `det`, `trace`, `transpose`, `inv_mat2` |

Structs are built by calling the type name (`vec2(3, 4)`, `circle(vec2(3, 7), 5)`) with strict field-count and field-type validation, and support field access (`v.x`) and C-style indexing for reading and writing, including nested access:

```text
v := vec3(2, 4, 6) ; v[0] = 10 ; v[1] += 5 ; v[2] *= 3 ; v     # vec3(x=10, y=9, z=18)
c := circle(vec2(3, 7), 5) ; c[0][1] = 99 ; c[1] += 10 ; c     # circle(center=vec2(x=3, y=99), radius=15)
```

## 9. Unit Conversions & Dual Variable/Unit Disambiguation

There are three ways to convert within a unit group; converting across groups is always an error.

1. **The `in` operator** supports both bare unit names (`25km in m` $\to$ `25000 [distance]`, preserving the unit group) and parenthesised unit quantities (`(2.5 km) in (1 m)` $\to$ `2500 [none]`).
   - **Dual Variable/Unit Disambiguation (`hardcode_should_use_unit_for_in_rhs`)**: What if `m` or `km` is *also* declared as a local variable or vector?
     - If `m := 5` (scalar), `25km in m` still converts `25km` to metres (`25000 [distance]`), and `(25km in m) * m` evaluates to `125000 [distance]`.
     - If `km := vec(10, 25, 30)` (unitless vector) and the left side is `25m` (`distance`), `25m in km` sees that the vector's element unit (`none`) does not match `distance`, so it automatically resolves `km` as the unit (`0.025 [distance]`), whereas `25 in km` checks vector membership (`1`).
     - If `km := vec(10m, 25m, 50m)` (a vector of distances!), `25m in km` checks vector membership (`1`), while writing `25m in 1km` explicitly forces unit conversion (`0.025`), and writing `25m in (km)` explicitly forces vector membership lookup.
2. **Postfix `to_<unit>`** (precedence 90) converts and returns a plain number (`real [none]`): `5 km to_m` is `5000`, `100 cm to_m` is `1`, `90 deg to_rad` is `1.570796327`, `10 kph to_mps` is `2.777777778`.
3. **Pairwise functions** `<from>_to_<to>(x)`: `km_to_m`, `deg_to_rad`, `kg_to_g`, `hr_to_sec`, `atm_to_Pa`, `kWh_to_J` and 100+ more (all listed in Section 10.13 & 10.14).

Temperature supports both unit suffixes (`K`, `kelvin`, `mK`, `degC`, `degF`, stored in SI Kelvin so `100 degC` is `373.15 [temperature]`) and scalar helper functions (`c_to_k`, `k_to_c`, `c_to_f`, `f_to_c`, `f_to_k`, `k_to_f`).

**All 35 Unit Groups across the 7 SI base dimensions** $(\text{m}, \text{kg}, \text{s}, \text{A}, \text{K}, \text{mol}, \text{cd})$ and custom groups (all suffixes precedence 95, normalised to SI):

| Group (SI Base) | `@dim` $(\text{m}, \text{kg}, \text{s}, \text{A}, \text{K}, \text{mol}, \text{cd})$ | Suffixes (`:95`) | Postfix Extractors (`:90`) |
| :-- | :-- | :-- | :-- |
| `angle` (`rad`) | non-SI angle | `deg`, `°`, `rad`, `arcmin`, `arcsec` | `to_rad`, `to_deg`, `to_arcmin`, `to_arcsec` |
| `distance` (`m`) | $(1, 0, 0, 0, 0, 0, 0)$ | `m`, `km`, `cm`, `mm`, `um`, `nm`, `mi`, `nmi`, `yd`, `ft`, `inch`, `im`, `ikm`, `icm`, `imm` (`m`/`km`/`cm`/`mm` also overloaded on `iota`) | `to_km`, `to_m`, `to_cm`, `to_mm`, `to_um`, `to_nm`, `to_mi`, `to_nmi`, `to_yd`, `to_ft`, `to_inch` |
| `area` (`m^2`) | $(2, 0, 0, 0, 0, 0, 0)$ | `m2`, `km2`, `cm2`, `mm2`, `ha`, `acre`, `yd2`, `ft2`, `in2` | `to_m2`, `to_km2`, `to_cm2`, `to_mm2`, `to_ha`, `to_acre`, `to_ft2`, `to_in2` |
| `volume` (`m^3`) | $(3, 0, 0, 0, 0, 0, 0)$ | `m3`, `cm3`, `mm3`, `L`, `liter`, `mL`, `gal`, `ft3`, `in3` | `to_m3`, `to_cm3`, `to_L`, `to_mL`, `to_gal`, `to_ft3` |
| `mass` (`kg`) | $(0, 1, 0, 0, 0, 0, 0)$ | `kg`, `gram`, `mg`, `ton`, `lb`, `oz`, `amu` | `to_kg`, `to_gram`, `to_mg`, `to_ton`, `to_lb`, `to_oz` |
| `density` (`kg/m^3`) | $(-3, 1, 0, 0, 0, 0, 0)$ | `kgm3`, `gcm3` | `to_kgm3`, `to_gcm3` |
| `time` (`s`) / `time2` (`s^2`) | $(0, 0, 1, 0, 0, 0, 0)$ / $(0, 0, 2, 0, 0, 0, 0)$ | `ms`, `us`, `ns`, `sec`, `min_t`, `hr`, `day`, `sec2` | `to_sec`, `to_ms`, `to_us`, `to_ns`, `to_min`, `to_hr`, `to_day` |
| `frequency` (`Hz` = `s^-1`) | $(0, 0, -1, 0, 0, 0, 0)$ | `Hz`, `kHz`, `MHz`, `GHz`, `rpm` | `to_Hz`, `to_kHz`, `to_MHz`, `to_GHz`, `to_rpm` |
| `speed` (`m/s`) | $(1, 0, -1, 0, 0, 0, 0)$ | `mps`, `kph`, `mph`, `knots` | `to_mps`, `to_kph`, `to_mph`, `to_knots` |
| `acceleration` (`m/s^2`) | $(1, 0, -2, 0, 0, 0, 0)$ | `mps2`, `g_acc`, `cmps2`, `fps2` | `to_mps2`, `to_g_acc`, `to_fps2` |
| `specific_energy` (`J/kg`) | $(2, 0, -2, 0, 0, 0, 0)$ | `Jpkg` | — |
| `momentum` (`N*s` = `kg*m/s`) | $(1, 1, -1, 0, 0, 0, 0)$ | `Ns`, `kgmps` | `to_Ns`, `to_kgmps` |
| `force` (`N` = `kg*m/s^2`) | $(1, 1, -2, 0, 0, 0, 0)$ | `N`, `kN`, `MN`, `mN`, `dyn`, `lbf`, `kgf` | `to_N`, `to_kN`, `to_MN`, `to_mN`, `to_dyn`, `to_lbf`, `to_kgf` |
| `pressure` (`Pa` = `N/m^2`) | $(-1, 1, -2, 0, 0, 0, 0)$ | `Pa`, `kPa`, `MPa`, `bar`, `atm`, `psi`, `torr` | `to_Pa`, `to_kPa`, `to_MPa`, `to_bar`, `to_atm`, `to_psi`, `to_torr` |
| `energy` (`J` = `N*m`) | $(2, 1, -2, 0, 0, 0, 0)$ | `J`, `kJ`, `MJ`, `cal`, `kcal`, `Wh`, `kWh`, `eV` | `to_J`, `to_kJ`, `to_MJ`, `to_cal`, `to_kcal`, `to_Wh`, `to_kWh`, `to_eV` |
| `power` (`W` = `J/s`) | $(2, 1, -3, 0, 0, 0, 0)$ | `W`, `kW`, `MW`, `hp` | `to_W`, `to_kW`, `to_MW`, `to_hp` |
| `current` (`A`) | $(0, 0, 0, 1, 0, 0, 0)$ | `A`, `mA`, `uA`, `kA` | `to_A`, `to_mA` |
| `charge` (`C` = `A*s`) | $(0, 0, 1, 1, 0, 0, 0)$ | `C`, `mC`, `uC`, `Ah`, `mAh` | `to_C`, `to_mAh` |
| `voltage` (`V` = `W/A`) | $(2, 1, -3, -1, 0, 0, 0)$ | `V`, `mV`, `kV` | `to_V`, `to_mV`, `to_kV` |
| `resistance` (`ohm` = `V/A`) | $(2, 1, -3, -2, 0, 0, 0)$ | `ohm`, `kohm`, `Mohm` | `to_ohm`, `to_kohm` |
| `capacitance` (`F` = `C/V`) | $(-2, -1, 4, 2, 0, 0, 0)$ | `F_cap`, `mF`, `uF`, `nF`, `pF` | — |
| `temperature` (`K`) | $(0, 0, 0, 0, 1, 0, 0)$ | `K`, `kelvin`, `mK`, `degC`, `degF` | `to_K`, `to_mK`, `to_degC`, `to_degF` |
| `heat_capacity` (`J/K`) | $(2, 1, -2, 0, -1, 0, 0)$ | `JpK` | `to_JpK` |
| `specific_heat` (`J/(kg*K)`) | $(2, 0, -2, 0, -1, 0, 0)$ | `JpkgK` | `to_JpkgK` |
| `substance` (`mol`) | $(0, 0, 0, 0, 0, 1, 0)$ | `mol`, `mmol`, `umol`, `nmol`, `kmol` | `to_mol`, `to_mmol`, `to_umol`, `to_kmol` |
| `molar_mass` (`kg/mol`) | $(0, 1, 0, 0, 0, -1, 0)$ | `kgpmol`, `gpmol` | `to_kgpmol`, `to_gpmol` |
| `molarity` (`mol/m^3`) | $(-3, 0, 0, 0, 0, 1, 0)$ | `molm3`, `M_conc`, `mM_conc` | `to_molm3`, `to_M_conc`, `to_mM_conc` |
| `molar_energy` (`J/mol`) | $(2, 1, -2, 0, 0, -1, 0)$ | `Jpmol`, `kJpmol` | `to_Jpmol`, `to_kJpmol` |
| `molar_heat_capacity` (`J/(mol*K)`) | $(2, 1, -2, 0, -1, -1, 0)$ | `JpmolK` | `to_JpmolK` |
| `catalytic_activity` (`kat` = `mol/s`) | $(0, 0, -1, 0, 0, 1, 0)$ | `kat`, `mkat`, `ukat` | `to_kat` |
| `luminous_intensity` (`cd` / `lm`) | $(0, 0, 0, 0, 0, 0, 1)$ | `cd`, `lm` | `to_cd`, `to_lm` |
| `illuminance` (`lx` = `lm/m^2`) | $(-2, 0, 0, 0, 0, 0, 1)$ | `lx`, `klx` | `to_lx`, `to_klx` |
| `data_size` / `bandwidth` (`[user_physics]`) | custom non-SI groups | `B`, `KB`, `MB`, `GB` (`data_size` / `time` $\to$ `bandwidth`) | — |
| unitless ratios | $(0, 0, 0, 0, 0, 0, 0)$ | `pct` (x 0.01), `dB` (amplitude ratio), `dBm` (watts) | `to_inr`, `to_usd` (`[user_physics]`) |

## 10. Standard Library & Extension Catalogue — Complete Construct, Type, Constant, Unit, Operator & Function Overload Reference

`eng_init` loads 17 named modules plus an implicit `default` module (where headerless user extensions land) from the 15 `.math` files in `Library/`. Loading `Test/user_physics.math` adds `[user_physics]` for 19 modules total. Each module can be switched off and on in O(1) with `eng_set_module`; disabling `units` makes `100 km` fail with `Undefined symbol or constant 'km'`.

### 10.0.1 Module Summary Table

| # | Module | Source File | Overloads | Constants | Types / `@dim` Groups |
| --: | :-- | :-- | --: | --: | :-- |
| 0 | `[default]` | implicit fallback | 0 | 0 | Headerless user extensions land here |
| 1 | `[boot_syntax]` | `Library/bootstrap.math` | 7 | 0 | Syntax delimiters `( ) { } [ ] ,` |
| 2 | `[boot_types]` | `Library/bootstrap.math` | 0 | 0 | 6 core types: `$real`, `$iota`, `$str`, `$vec`, `$mat`, `$fn` |
| 3 | `[boot_control]` | `Library/bootstrap.math` | 66 | 0 | Assignment, control flow, loops, `=>`, `\|>`, `call` (0..4 args), calculus/ODE (`ode_rk4_vec`), `_`, introspection |
| 4 | `[arith]` | `Library/arith.math` | 132 | 6 | Scalar math, bitwise ops (`~`, `<<`, `>>`, `bit_*`), probability PMFs/CDFs, number theory, commercial math |
| 5 | `[vec]` | `Library/vec.math` | 81 | 0 | `vec<T>/list<T>/stack<T>/queue<T>`, sorting, `linspace`, `unique`, `zip_with`, stats & linear regression |
| 6 | `[string]` | `Library/string.math` | 19 | 0 | `$str` operators (`+`, `into`, `==`, `!=`, `in`, `len`, `at`), `to_hex`, `to_bin` & pure MEP string helpers |
| 7 | `[mat]` | `Library/mat.math` | 50 | 0 | `$mat` arithmetic, `reshape`, `flatten`, `diag`, `mat_pow`/`M^n`, `frobenius_norm`, `linsolve`, `inv`, `det`, HOFs |
| 8 | `[trig]` | `Library/trig.math` | 35 | 0 | Trig, hyperbolic, log, exp, root, `erf`, `erfc`, `gamma`, `lgamma`, `beta_fn`, `norm_pdf`, `norm_cdf` |
| 9 | `[complex_ext]` | `Library/complex.math` | 73 | 0 | `$complex(re, im)`, postfix `i`, mixed `*`/`/`/`^`, `√z`/`sqrt`, `exp`/`ln`/trig/hyperbolic, `re`/`im` |
| 10 | `[geom_ext]` | `Library/geom.math` | 38 | 0 | `$vec2(x, y)`, `$vec3(x, y, z)`, `$circle(center:vec2, radius)`, vector/circle/sphere math |
| 11 | `[algebra]` | `Library/algebra.math` | 28 | 0 | `$mat2(a, b, c, d)`, `solve_2x2`, quadratics (`quad_*`), polynomial algebra (`poly_*`) |
| 12 | `[calculus]` | `Library/calculus.math` | 38 | 0 | Polynomial calculus, `pdiff_x/y`, `grad2/3`, `hessian2`, `laplacian2`, `grad_descent_1d/2d`, Taylor series |
| 13 | `[units]` | `Library/units.math` | 169 | 0 | 32 SI `@dim` groups + `angle`, 151 unit suffixes, 18 unit helper functions |
| 14 | `[conversions]` | `Library/conversions.math` | 239 | 0 | `in` operator, 128 postfix `to_<unit>` extractors, 110 `<u1>_to_<u2>` functions |
| 15 | `[physics]` | `Library/physics.math` | 100 | 14 | Mechanics, SHM, gravitation, fluids, thermo, EM, AC, optics, quantum, relativity |
| 16 | `[chemistry]` | `Library/chemistry.math` | 61 | 7 | Stoichiometry, solutions, gas laws, pH/buffers, kinetics, nuclear, thermo, electrochem |
| 17 | `[biology]` | `Library/biology.math` | 60 | 0 | Hardy-Weinberg, DNA/PCR, enzyme kinetics, water potential, physiology, ecology |
| 18 | `[user_physics]` | `Test/user_physics.math` | 44 | 1 | Sample host extension: `@extern` FX/KV/element callbacks, `data_size`/`bandwidth` units |

### 10.0.2 Complete Constants Catalogue (All 28 Predefined Constants)

| Constant | Module | Type & Unit | `.math` Definition | Value (`eng_format_value`) | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `π` | `[arith]` | `real [none]` | `3.14159265358979323846` | `3.141592654` | `sin(π / 2)` | `1` |
| `pi` | `[arith]` | `real [none]` | `3.14159265358979323846` | `3.141592654` | `cos(pi)` | `-1` |
| `e` | `[arith]` | `real [none]` | `2.71828182845904523536` | `2.718281828` | `ln(e ^ 3)` | `3` |
| `phi` | `[arith]` | `real [none]` | `1.61803398874989484820` | `1.618033989` | `phi^2 - phi` | `1` |
| `φ` | `[arith]` | `real [none]` | `1.61803398874989484820` | `1.618033989` | `φ^2 - φ` | `1` |
| `ϕ` | `[arith]` | `real [none]` | `1.61803398874989484820` | `1.618033989` | `ϕ^2 - ϕ` | `1` |
| `g0` | `[physics]` | `real [none]` | `9.80665` (standard gravity, $\text{m/s}^2$) | `9.80665` | `pe(2, 10)` | `196.133` |
| `c0` | `[physics]` | `real [none]` | `299792458` (speed of light in vacuum, $\text{m/s}$) | `299792458` | `lorentz_gamma(0.6 * c0)` | `1.25` |
| `G_grav` | `[physics]` | `real [none]` | `6.67430 * (10 ^ -11)` (gravitational constant) | `6.6743e-11` | `grav_force(1e10, 1e10, 100)` | `667430` |
| `h_planck` | `[physics]` | `real [none]` | `6.62607015 * (10 ^ -34)` (Planck constant, $\text{J}\cdot\text{s}$) | `6.62607015e-34` | `photon_energy(1e14)` | `6.62607015e-20` |
| `k_B` | `[physics]` | `real [none]` | `1.380649 * (10 ^ -23)` (Boltzmann constant, $\text{J/K}$) | `1.380649e-23` | `k_B * 300` | `4.141947e-21` |
| `ke_coulomb` | `[physics]` | `real [none]` | `8987551792.3` (Coulomb constant $k_e$, $\text{N}\cdot\text{m}^2/\text{C}^2$) | `8987551792` | `coulomb_force(1e-6, 1e-6, 1)` | `0.008987551792` |
| `eps0` | `[physics]` | `real [none]` | `8.8541878128 * (10 ^ -12)` (vacuum permittivity $\varepsilon_0$) | `8.854187813e-12` | `parallel_plate_cap(1, 0.001)` | `8.854187813e-09` |
| `mu0` | `[physics]` | `real [none]` | `4 * π * (10 ^ -7)` (vacuum permeability $\mu_0$) | `1.256637062e-06` | `solenoid_b(1000, 2)` | `0.002513274123` |
| `q_e` | `[physics]` | `real [none]` | `1.602176634 * (10 ^ -19)` (elementary charge $e$, $\text{C}$) | `1.602176634e-19` | `q_e * 1e19` | `1.602176634` |
| `m_e` | `[physics]` | `real [none]` | `9.1093837015 * (10 ^ -31)` (electron mass, $\text{kg}$) | `9.109383702e-31` | `m_e * 1e31` | `9.109383702` |
| `m_p` | `[physics]` | `real [none]` | `1.67262192369 * (10 ^ -27)` (proton mass, $\text{kg}$) | `1.672621924e-27` | `m_p / m_e` | `1836.152674` |
| `sigma_sb` | `[physics]` | `real [none]` | `5.670374419 * (10 ^ -8)` (Stefan-Boltzmann $\sigma$) | `5.670374419e-08` | `stefan_boltzmann_p(1, 1, 100)` | `5.670374419` |
| `b_wien` | `[physics]` | `real [none]` | `0.002897771955` (Wien displacement constant, $\text{m}\cdot\text{K}$) | `0.002897771955` | `wien_peak_lambda(5778)` | `5.015181646e-07` |
| `rydberg_R` | `[physics]` | `real [none]` | `10973731.56816` (Rydberg constant $R_\infty$, $\text{m}^{-1}$) | `10973731.57` | `rydberg_wavelength(2, 3)` | `6.561122764e-07` |
| `N_A` | `[chemistry]` | `real [none]` | `6.02214076 * (10 ^ 23)` (Avogadro constant, $\text{mol}^{-1}$) | `6.02214076e+23` | `particles_from_moles(2)` | `1.204428152e+24` |
| `R_gas` | `[chemistry]` | `real [none]` | `8.314462618` (universal gas constant, $\text{J}/(\text{mol}\cdot\text{K})$) | `8.314462618` | `ideal_gas_p(1, 300, 1)` | `2494.338785` |
| `R_atm` | `[chemistry]` | `real [none]` | `0.082057366` (gas constant, $\text{L}\cdot\text{atm}/(\text{mol}\cdot\text{K})$) | `0.082057366` | `osmotic_pressure(1, 300)` | `24.6172098` |
| `F_faraday` | `[chemistry]` | `real [none]` | `96485.33212` (Faraday constant, $\text{C/mol}$) | `96485.33212` | `gibbs_electro(2, 1.1)` | `-212267.7307` |
| `T_stp` | `[chemistry]` | `real [none]` | `273.15` (standard temperature, $\text{K}$) | `273.15` | `T_stp + 25` | `298.15` |
| `P_stp` | `[chemistry]` | `real [none]` | `101325` (standard pressure, $\text{Pa}$) | `101325` | `P_stp / 1000` | `101.325` |
| `kw_water` | `[chemistry]` | `real [none]` | `10 ^ -14` (water auto-ionization constant $K_w$ at $25^\circ\text{C}$) | `1e-14` | `-log10(kw_water)` | `14` |
| `tau_const` | `[user_physics]` | `real [none]` | `2π` (circle constant $\tau = 2\pi$) | `6.283185307` | `cos(tau_const)` | `1` |


### 10.1 Module `[boot_syntax]` (`Library/bootstrap.math` — 7 overloads)

Core expression grouping & block/matrix delimiters (`( )`, `{ }`, `[ ]`, `,`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `"("` | `"("` | `syntax` | `void` | `@lparen` | `(2 + 3) * 4` | `20` |
| `")"` | `")"` | `syntax` | `void` | `@rparen` | `(2 + 3) * 4` | `20` |
| `"{"` | `"{"` | `syntax` | `void` | `@lbrace` | `{ a := 3 ; a^2 }` | `9` |
| `"}"` | `"}"` | `syntax` | `void` | `@rbrace` | `{ a := 3 ; a^2 }` | `9` |
| `"["` | `"["` | `syntax` | `void` | `@lbrack` | `[1, 2 ; 3, 4]` | `[[1, 2], [3, 4]]` |
| `"]"` | `"]"` | `syntax` | `void` | `@rbrack` | `[1, 2 ; 3, 4]` | `[[1, 2], [3, 4]]` |
| `","` | `","` | `syntax` | `void` | `@comma` | `max(10, 25)` | `25` |

### 10.2 Module `[boot_types]` (`Library/bootstrap.math` — 0 overloads)

Core lexical consuming types (`$real`, `$iota`, `$str`) and container/closure entity types (`$vec`, `$mat`, `$fn`).

| Type Declaration | Kind & Layout | Description | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- |
| `$real ~ \d+ ("_" \d+)* ("." \d+ ("_" \d+)*)? (("e" \| "E") ("+" \| "-")? \d+ ("_" \d+)*)? => @f64` | Consuming scalar (1 slot) | 64-bit IEEE-754 float with optional `_` digit separators & scientific notation | `1_000_000 + 2.5e3` | `1002500` |
| `$iota ~ \d+ ("." \d+)? "i" => @f64_trim1` | Consuming scalar (1 slot) | Pure imaginary literal (`5i`, `3.5i`) | `3i + 4i` | `7i` |
| `$str ~ "\"" [^"]* "\"" => @str_slice` | Consuming string (1 slot) | Double-quoted UTF-8 string slice | `"Hello, " + "MEP!"` | `"Hello, MEP!"` |
| `$vec ~ "vec" => @vec_type` | Dynamic 1D container (4-slot header) | Homogeneous growable vector (`vec<T>`, `list<T>`, `stack<T>`, `queue<T>`) | `vec<real>(10, 20, 30)` | `[10, 20, 30]` |
| `$mat ~ "mat" => @mat_type` | Dense 1D–4D tensor (8-slot header) | Multidimensional matrix/tensor (`[1, 2 ; 3, 4]`) | `[1, 2 ; 3, 4]` | `[[1, 2], [3, 4]]` |
| `$fn ~ "fn" => @fn_type` | First-class closure (4-slot header) | Lambda function value (`x => expr`, `(a, b) => expr`) | `(x => x^2)(9)` | `81` |

### 10.3 Module `[boot_control]` (`Library/bootstrap.math` — 61 overloads)

Assignment (`:=`, `=`, `+=`..`^=`), lambdas (`=>`), pipe (`|>`), loops (`??`, `?!?`), ternary (`? :`), strict boolean `!` & distributed `&`/`|`, `if`/`or`/`then`/`else`, `switch`/`case`/`default`, jumps (`break`, `continue`, `return`, `jump`, `run`), lazy calculus & RK4 ODE intrinsics (`diff`..`ode_rk4`), string conversion (`_`), and introspection (`type_of`, `unit_of`, `error`, `assert`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `(real) . (real)` | `(s) "." (f) :<95` | `infix_L :95` | `real` | `@field(s, f)` | `vec2(3, 4).x` | `3` |
| `(real) ; (real)` | `(a) ";" (b) :<1` | `infix_L :1` | `real` | `@seq(a, b)` | `a := 4 ; a + 6` | `10` |
| `(real) := (real)` | `(target) ":=" (val) :>5` | `infix_R :5` | `real` | `@store(target, val)` | `x := 42` | `42` |
| `(real) = (real)` | `(target) "="  (val) :>5` | `infix_R :5` | `real` | `@store(target, val)` | `x := 10 ; x = 25 ; x` | `25` |
| `(real) += (real)` | `(target) "+=" (val) :>5` | `infix_R :5` | `real` | `@store(target, @add(target, val))` | `x := 10 ; x += 5 ; x` | `15` |
| `(real) -= (real)` | `(target) "-=" (val) :>5` | `infix_R :5` | `real` | `@store(target, @sub(target, val))` | `x := 10 ; x -= 3 ; x` | `7` |
| `(real) *= (real)` | `(target) "*=" (val) :>5` | `infix_R :5` | `real` | `@store(target, @mul(target, val))` | `x := 10 ; x *= 4 ; x` | `40` |
| `(real) /= (real)` | `(target) "/=" (val) :>5` | `infix_R :5` | `real` | `@store(target, @div(target, val))` | `x := 10 ; x /= 2 ; x` | `5` |
| `(real) %= (real)` | `(target) "%=" (val) :>5` | `infix_R :5` | `real` | `@store(target, @mod(target, val))` | `x := 10 ; x %= 3 ; x` | `1` |
| `(real) ^= (real)` | `(target) "^=" (val) :>5` | `infix_R :5` | `real` | `@store(target, @pow(target, val))` | `x := 3 ; x ^= 4 ; x` | `81` |
| `(real) => (real)` | `(params) "=>" (body) :>8  -> fn` | `infix_R :8` | `fn` | `@lambda(params, body)` | `sq := x => x^2 ; sq(6)` | `36` |
| `(T) \|> (T)` | `(x:T)    "\|>" (f:T)  :<12 -> T` | `infix_L :12` | `T` | `@pipe(x, f)` | `5 \|> (x => x + 3) \|> (x => x^2)` | `64` |
| `call(fn, T)` | `call(f:fn, a:T)           -> T` | `func` | `T` | `@fcall(f, a, 0)` | `call(x => x^2, 7)` | `49` |
| `call(fn, T, T)` | `call(f:fn, a:T, b:T)      -> T` | `func` | `T` | `@fcall(f, a, b)` | `call((a, b) => a + b, 12, 8)` | `20` |
| `(real) ?? (real)` | `(cond) "??"  (body) :<3` | `infix_L :3` | `real` | `@while(cond, body)` | `n := 0 ; (n < 5) ?? { n += 2 } ; n` | `6` |
| `(real) ?!? (real)` | `(body) "?!?" (cond) :<3` | `infix_L :3` | `real` | `@do_while(body, cond)` | `k := 10 ; (k += 5) ?!? (k < 5) ; k` | `15` |
| `(real) ? (real)` | `(cond) "?" (branches) :>10` | `infix_R :10` | `real` | `@ternary(cond, branches)` | `1 ? 10 : 20` | `10` |
| `(real) : (real)` | `(a)    ":" (b)        :>10` | `infix_R :10` | `real` | `@colon(a, b)` | `0 ? 10 : 20` | `20` |
| `!(real)` | `"!" (a:real, none) :80` | `prefix :80` | `real` | `@cnot(a)` | `!0` | `1` |
| `(real) & (real)` | `(a) "&" (b)        :<35` | `infix_L :35` | `real` | `@cand(a, b)` | `x := 2 ; x & 2 == 1 \| 2` | `1` |
| `(real) \| (real)` | `(a) "\|" (b)        :<34` | `infix_L :34` | `real` | `@cor(a, b)` | `x := 3 ; x == 1 \| 3` | `1` |
| `if(real, real, real)` | `if(cond, then_val, else_val)` | `func` | `real` | `@if(cond, then_val, else_val)` | `if(1, 10, 20)` | `10` |
| `or(real, real)` | `or(cond, body)` | `func` | `real` | `@or_branch(cond, body)` | `if (0) { "a" } or (1) { "b" } else { "c" }` | `"b"` |
| `then(real, real)` | `then(cond, body)` | `func` | `real` | `@or_branch(cond, body)` | `if (0) { "a" } then (1) { "b" } else { "c" }` | `"b"` |
| `else(real)` | `else(body)` | `func` | `real` | `@else_branch(body)` | `if (0) { 1 } else { 99 }` | `99` |
| `default(real)` | `default(body)` | `func` | `real` | `@else_branch(body)` | `switch (9) { case 1 : { 10 } default : { 99 } }` | `99` |
| `switch(real, real)` | `switch(val, body)` | `func` | `real` | `@switch_block(val, body)` | `switch (2) { case 1 : { "one" } case 2 : { "two" } else : { "other" } }` | `"two"` |
| `case(real, real)` | `case(val, body)` | `func` | `real` | `@case_branch(val, body)` | `switch (5) { case 1...10 : { "in_range" } else : { "out" } }` | `"in_range"` |
| `jump(real)` | `jump(tag)` | `func` | `real` | `@jump_tag(tag)` | `i := 0 ; Top : { i += 1 ; if (i < 3) { jump(Top) } ; i }` | `3` |
| `run(real)` | `run(tag)` | `func` | `real` | `@run_tag(tag)` | `Helper : { 21 * 2 } ; run(Helper)` | `42` |
| `break()` | `break()` | `func` | `real` | `@break_loop(0)` | `k := 0 ; 1 ?? { k += 1 ; if (k == 4) { break } } ; k` | `4` |
| `break(real)` | `break(tag)` | `func` | `real` | `@break_loop(tag)` | `(x in 1...10) { if (x^2 > 20) { break(x) } }` | `5` |
| `break(real, T)` | `break(tag, val:T)       -> T` | `func` | `T` | `@break_loop(tag, val)` | `Outer : (i in 1...5) { (j in 1...5) { if (i*j == 6) { break(Outer, i*10+j) } } }` | `23` |
| `continue()` | `continue()` | `func` | `real` | `@continue_loop(0)` | `s := 0 ; (i in 1...5) { if (i == 3) { continue } ; s += i } ; s` | `12` |
| `continue(real)` | `continue(tag)` | `func` | `real` | `@continue_loop(tag)` | `s := 0 ; Outer : (i in 1...3) { if (i == 2) { continue(Outer) } ; s += i } ; s` | `4` |
| `return()` | `return()` | `func` | `real` | `@return_fn(0)` | `{ x := 10 ; return() ; 99 }` | `0` |
| `return(T)` | `return(val:T)           -> T` | `func` | `T` | `@return_fn(val)` | `{ x := 10 ; if (x > 5) { return(x * 3) } ; 0 }` | `30` |
| `diff(real, real, real)` | `diff(var_x, expr, x0)` | `func` | `real` | `@calc_diff(var_x, expr, x0)` | `diff(x, x^3 + 2*x, 3)` | `29` |
| `diff2(real, real, real)` | `diff2(var_x, expr, x0)` | `func` | `real` | `@calc_diff2(var_x, expr, x0)` | `diff2(x, x^4, 2)` | `48` |
| `diff_n(real, real, real, real)` | `diff_n(var_x, expr, x0, order)` | `func` | `real` | `@calc_diffn(var_x, expr, x0)` | `diff_n(x, x^4, 2, 3)` | `48` |
| `integrate(real, real, real, real)` | `integrate(var_x, expr, a, b)` | `func` | `real` | `@calc_integ(var_x, expr, a)` | `integrate(x, 3 * x^2, 0, 2)` | `8` |
| `integrate(real, real, real, real, real)` | `integrate(var_x, expr, a, b, steps)` | `func` | `real` | `@calc_integ(var_x, expr, a)` | `integrate(x, 3 * x^2, 0, 2, 200)` | `8` |
| `ntg(real, real, real, real)` | `ntg(var_x, expr, a, b)` | `func` | `real` | `@calc_integ(var_x, expr, a)` | `ntg(x, 2 * x, 0, 3)` | `9` |
| `lim(real, real, real)` | `lim(var_x, expr, x0)` | `func` | `real` | `@calc_lim(var_x, expr, x0)` | `lim(x, sin(x) / x, 0)` | `1` |
| `lim_left(real, real, real)` | `lim_left(var_x, expr, x0)` | `func` | `real` | `@calc_lim_l(var_x, expr, x0)` | `lim_left(x, abs(x) / x, 0)` | `-1` |
| `lim_right(real, real, real)` | `lim_right(var_x, expr, x0)` | `func` | `real` | `@calc_lim_r(var_x, expr, x0)` | `lim_right(x, abs(x) / x, 0)` | `1` |
| `sigma(real, real, real, real)` | `sigma(var_k, expr, lo, hi)` | `func` | `real` | `@calc_sigma(var_k, expr, lo)` | `sigma(k, k^2, 1, 5)` | `55` |
| `prod(real, real, real, real)` | `prod(var_k, expr, lo, hi)` | `func` | `real` | `@calc_prod(var_k, expr, lo)` | `prod(k, k, 1, 5)` | `120` |
| `solve(real, real)` | `solve(var_x, eq)` | `func` | `real` | `@calc_solve(var_x, eq)` | `solve(x, x^2 - 9)` | `3` |
| `solve(real, real, real)` | `solve(var_x, eq, guess)` | `func` | `real` | `@calc_solve(var_x, eq, guess)` | `solve(x, x^2 == 16, 3)` | `4` |
| `solve(real, real, real, real)` | `solve(var_x, eq, a, b)` | `func` | `real` | `@calc_solve(var_x, eq, a)` | `solve(x, x^3 - 2*x - 5, 1, 3)` | `2.094551482` |
| `fmin(real, real, real, real)` | `fmin(var_x, expr, a, b)` | `func` | `real` | `@calc_fmin(var_x, expr, a)` | `fmin(x, (x - 3)^2 + 1, 0, 6)` | `3` |
| `fmax(real, real, real, real)` | `fmax(var_x, expr, a, b)` | `func` | `real` | `@calc_fmax(var_x, expr, a)` | `fmax(x, -(x - 2)^2 + 5, 0, 5)` | `2` |
| `ode_rk4(real, real, real, real, real, real)` | `ode_rk4(var_t, var_y, expr, t0, y0, t1)` | `func` | `real` | `@calc_ode(var_t, var_y, expr)` | `ode_rk4(t, y, y, 0, 1, 1)` | `2.718281828` |
| `ode_rk4(real, real, real, real, real, real, real)` | `ode_rk4(var_t, var_y, expr, t0, y0, t1, steps)` | `func` | `real` | `@calc_ode(var_t, var_y, expr)` | `ode_rk4(t, y, 2 * t, 0, 0, 3, 100)` | `9` |
| `type_of(T)` | `type_of(x:T) -> str` | `func` | `str` | `@type_name(x)` | `type_of(3 + 4i)` | `"complex"` |
| `unit_of(T)` | `unit_of(x:T) -> str` | `func` | `str` | `@unit_name(x)` | `unit_of(100 km)` | `"distance"` |
| `(T) _` | `(x:T) "_"   :95 -> str` | `postfix :95` | `str` | `@tostr(x)` | `vec2(3, 4)_` | `"vec2(x=3, y=4)"` |
| `_(str)` | `"_" (s:str) :85 -> T` | `prefix :85` | `T` | `@fromstr(s)` | `_"3 + 4i"` | `3 + 4i` |
| `error(str)` | `error(msg:str)` | `func` | `real` | `@err(msg)` | `if(0, error("boom"), 1)` | `1` |
| `assert(real, str)` | `assert(cond, msg:str)` | `func` | `real` | `if(cond, 1, error(msg))` | `assert(2 + 2 == 4, "math ok")` | `1` |

### 10.4 Module `[arith]` (`Library/arith.math` — 116 overloads)

Scalar arithmetic (`+`, `-`, `*`, `/`, `%`, `^`, `into`, `plus`, `minus`), comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`, `~`), boolean logic (`not`, `&&`/`and`, `||`/`or`, `xor`/`^^`, `nand`, `nor`), unary/postfix operators (`-`, `+`, `√`, `++`, `+++`, `--`, `---`, `!`), rounding, number theory (`gcd`, `lcm`, `npr`, `ncr`, `fib`, `is_prime`, `mod_pow`), constants (`π`, `pi`, `e`, `phi`, `φ`, `ϕ`), and commercial/aptitude mathematics.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `(real) + (real)` | `(a:real, T) "+"    (b:real, T) :<50 -> real, T` | `infix_L :50` | `real` | `@add(a, b)` | `6 + 3` | `9` |
| `(real) - (real)` | `(a:real, T) "-"    (b:real, T) :<50 -> real, T` | `infix_L :50` | `real` | `@sub(a, b)` | `6 - 3` | `3` |
| `(real) into (real)` | `(a:real, T) into   (b:real, T) :>55 -> real, T` | `infix_R :55` | `real` | `@add(a, b)` | `6 into 3` | `9` |
| `(real) * (real)` | `(a)         "*"    (b)         :<60` | `infix_L :60` | `real` | `@mul(a, b)` | `6 * 3` | `18` |
| `(real) / (real)` | `(a)         "/"    (b)         :<60` | `infix_L :60` | `real` | `@div(a, b)` | `6 / 3` | `2` |
| `(real) % (real)` | `(a)         "%"    (b)         :<60` | `infix_L :60` | `real` | `@mod(a, b)` | `6 % 3` | `0` |
| `(real) plus (real)` | `(a) plus  (b) :<45` | `infix_L :45` | `real` | `a + b` | `6 plus 3` | `9` |
| `(real) minus (real)` | `(a) minus (b) :<45` | `infix_L :45` | `real` | `a - b` | `6 minus 3` | `3` |
| `(real) ^ (real)` | `(a) "^" (b) :>85` | `infix_R :85` | `real` | `@pow(a, b)` | `6 ^ 3` | `216` |
| `(real) == (real)` | `(a)         "==" (b)         :<30` | `infix_L :30` | `real` | `@eq(a, b)` | `6 == 3` | `0` |
| `(real) != (real)` | `(a)         "!=" (b)         :<30` | `infix_L :30` | `real` | `@ne(a, b)` | `6 != 3` | `1` |
| `(real) < (real)` | `(a)         "<"  (b)         :<30` | `infix_L :30` | `real` | `@lt(a, b)` | `6 < 3` | `0` |
| `(real) <= (real)` | `(a)         "<=" (b)         :<30` | `infix_L :30` | `real` | `@le(a, b)` | `6 <= 3` | `0` |
| `(real) > (real)` | `(a)         ">"  (b)         :<30` | `infix_L :30` | `real` | `@gt(a, b)` | `6 > 3` | `1` |
| `(real) >= (real)` | `(a)         ">=" (b)         :<30` | `infix_L :30` | `real` | `@ge(a, b)` | `6 >= 3` | `1` |
| `(real) ~ (real)` | `(a:real, T) "~"  (b:real, T) :<30` | `infix_L :30` | `real` | `@approx(a, b)` | `6 ~ 3` | `0` |
| `not(real)` | `not(x) :25` | `prefix :25` | `real` | `@lnot(x)` | `not 0` | `1` |
| `(real) && (real)` | `(a) "&&" (b) :<20` | `infix_L :20` | `real` | `@land(a, b)` | `6 && 3` | `1` |
| `(real) and (real)` | `(a) and  (b) :<20` | `infix_L :20` | `real` | `@land(a, b)` | `6 and 3` | `1` |
| `(real) nand (real)` | `(a) nand (b) :<20` | `infix_L :20` | `real` | `@lnand(a, b)` | `6 nand 3` | `0` |
| `(real) xor (real)` | `(a) xor  (b) :<18` | `infix_L :18` | `real` | `@lxor(a, b)` | `6 xor 3` | `0` |
| `(real) ^^ (real)` | `(a) "^^" (b) :<18` | `infix_L :18` | `real` | `@lxor(a, b)` | `6 ^^ 3` | `0` |
| `(real) \|\| (real)` | `(a) "\|\|" (b) :<15` | `infix_L :15` | `real` | `@lor(a, b)` | `6 \|\| 3` | `1` |
| `(real) or (real)` | `(a) or   (b) :<15` | `infix_L :15` | `real` | `@lor(a, b)` | `6 or 3` | `1` |
| `(real) nor (real)` | `(a) nor  (b) :<15` | `infix_L :15` | `real` | `@lnor(a, b)` | `6 nor 3` | `0` |
| `-(real)` | `"-" (x) :80` | `prefix :80` | `real` | `@neg(x)` | `-6` | `-6` |
| `+(real)` | `"+" (x) :80` | `prefix :80` | `real` | `x` | `+6` | `6` |
| `√(real)` | `"√" (x) :80` | `prefix :80` | `real` | `@sqrt(x)` | `√6` | `2.449489743` |
| `++(real)` | `"++"  (x) :85` | `prefix :85` | `real` | `x + 1` | `++6` | `7` |
| `+++(real)` | `"+++" (x) :85` | `prefix :85` | `real` | `x + 2` | `+++6` | `8` |
| `--(real)` | `"--"  (x) :85` | `prefix :85` | `real` | `x - 1` | `--6` | `5` |
| `---(real)` | `"---" (x) :85` | `prefix :85` | `real` | `x - 2` | `---6` | `4` |
| `(real) ++` | `(x) "++"  :90` | `postfix :90` | `real` | `x + 1` | `6++` | `7` |
| `(real) +++` | `(x) "+++" :90` | `postfix :90` | `real` | `x + 2` | `6+++` | `8` |
| `(real) --` | `(x) "--"  :90` | `postfix :90` | `real` | `x - 1` | `6--` | `5` |
| `(real) ---` | `(x) "---" :90` | `postfix :90` | `real` | `x - 2` | `6---` | `4` |
| `(real) !` | `(n) "!" :90` | `postfix :90` | `real` | `{ acc := 1 k   := n (k > 1) ?? { acc *= k k   -= 1 } acc }` | `6!` | `720` |
| `abs(real)` | `abs(x)` | `func` | `real` | `@abs(x)` | `abs(6)` | `6` |
| `floor(real)` | `floor(x)` | `func` | `real` | `@floor(x)` | `floor(6)` | `6` |
| `ceil(real)` | `ceil(x)` | `func` | `real` | `@ceil(x)` | `ceil(6)` | `6` |
| `round(real, real)` | `round(x, digits = 0)` | `func` | `real` | `{ s := 10 ^ digits ((x >= 0) ? @floor(x * s + 0.5) : @ceil(x * s - 0.5)) / s }` | `round(6, 3)` | `6` |
| `round(real)` | `round(x, digits = 0) [default-arg overload]` | `func` | `real` | `{ s := 10 ^ digits ((x >= 0) ? @floor(x * s + 0.5) : @ceil(x * s - 0.5)) / s }` | `round(6)` | `6` |
| `trunc(real)` | `trunc(x)` | `func` | `real` | `(x >= 0) ? @floor(x) : @ceil(x)` | `trunc(6)` | `6` |
| `fract(real)` | `fract(x)` | `func` | `real` | `x - @floor(x)` | `fract(6)` | `0` |
| `sign(real)` | `sign(x)` | `func` | `real` | `(x > 0) ? 1 : (x < 0) ? -1 : 0` | `sign(6)` | `1` |
| `min(real, real)` | `min(a, b)` | `func` | `real` | `(a < b) ? a : b` | `min(6, 3)` | `3` |
| `max(real, real)` | `max(a, b)` | `func` | `real` | `(a > b) ? a : b` | `max(6, 3)` | `6` |
| `clamp(real, real, real)` | `clamp(x, lo, hi)` | `func` | `real` | `min(max(x, lo), hi)` | `clamp(6, 3, 2)` | `2` |
| `abs_diff(real, real)` | `abs_diff(a, b)` | `func` | `real` | `(a < b) ? (b - a) : (a - b)` | `abs_diff(6, 3)` | `3` |
| `is_even(real)` | `is_even(n)` | `func` | `real` | `(n % 2 == 0)` | `is_even(6)` | `1` |
| `is_odd(real)` | `is_odd(n)` | `func` | `real` | `(n % 2 != 0)` | `is_odd(6)` | `0` |
| `is_close(real, real, real)` | `is_close(a, b, tol = 0.000001)` | `func` | `real` | `(@abs(a - b) <= tol)` | `is_close(6, 3, 2)` | `0` |
| `is_close(real, real)` | `is_close(a, b, tol = 0.000001) [default-arg overload]` | `func` | `real` | `(@abs(a - b) <= tol)` | `is_close(6, 3)` | `0` |
| `gcd(real, real)` | `gcd(a, b)` | `func` | `real` | `{ x := @abs(a) y := @abs(b) (y > 0) ?? { t := x % y x = y y = t } x }` | `gcd(6, 3)` | `3` |
| `lcm(real, real)` | `lcm(a, b)` | `func` | `real` | `(a == 0 \|\| b == 0) ? 0 : @abs(a * b) / gcd(a, b)` | `lcm(6, 3)` | `6` |
| `npr(real, real)` | `npr(n, r)` | `func` | `real` | `(r < 0 \|\| r > n) ? 0 : (n!) / ((n - r)!)` | `npr(6, 3)` | `120` |
| `ncr(real, real)` | `ncr(n, r)` | `func` | `real` | `(r < 0 \|\| r > n) ? 0 : (n!) / ((r!) * ((n - r)!))` | `ncr(6, 3)` | `20` |
| `fib(real)` | `fib(n)` | `func` | `real` | `{ a := 0 b := 1 k := 0 (k < n) ?? { nxt := a + b a = b b = nxt k += 1 } a }` | `fib(6)` | `8` |
| `is_prime(real)` | `is_prime(n)` | `func` | `real` | `{ if (n < 2) { return 0 } d := 2 (d^2 <= n) ?? { if (n % d == 0) { return 0 }...` | `is_prime(6)` | `0` |
| `mod_pow(real, real, real)` | `mod_pow(base, exp_n, m)` | `func` | `real` | `{ res := 1 b   := base % m e_k := exp_n (e_k > 0) ?? { if (e_k % 2 == 1) { re...` | `mod_pow(6, 3, 2)` | `0` |
| `pct_of(real, real)` | `pct_of(part, whole)` | `func` | `real` | `(part / whole) * 100` | `pct_of(6, 3)` | `200` |
| `pct_val(real, real)` | `pct_val(pct, total)` | `func` | `real` | `(pct / 100) * total` | `pct_val(6, 3)` | `0.18` |
| `pct_change(real, real)` | `pct_change(old_v, new_v)` | `func` | `real` | `((new_v - old_v) / old_v) * 100` | `pct_change(6, 3)` | `-50` |
| `pct_inc(real, real)` | `pct_inc(val, pct)` | `func` | `real` | `val * (1 + pct / 100)` | `pct_inc(6, 3)` | `6.18` |
| `pct_dec(real, real)` | `pct_dec(val, pct)` | `func` | `real` | `val * (1 - pct / 100)` | `pct_dec(6, 3)` | `5.82` |
| `pct_error(real, real)` | `pct_error(measured, exact)` | `func` | `real` | `(@abs(measured - exact) / @abs(exact)) * 100` | `pct_error(6, 3)` | `100` |
| `successive_pct(real, real)` | `successive_pct(p1, p2)` | `func` | `real` | `p1 + p2 + (p1 * p2) / 100` | `successive_pct(6, 3)` | `9.18` |
| `profit(real, real)` | `profit(cp, sp)` | `func` | `real` | `sp - cp` | `profit(6, 3)` | `-3` |
| `loss(real, real)` | `loss(cp, sp)` | `func` | `real` | `cp - sp` | `loss(6, 3)` | `3` |
| `profit_pct(real, real)` | `profit_pct(cp, sp)` | `func` | `real` | `((sp - cp) / cp) * 100` | `profit_pct(6, 3)` | `-50` |
| `loss_pct(real, real)` | `loss_pct(cp, sp)` | `func` | `real` | `((cp - sp) / cp) * 100` | `loss_pct(6, 3)` | `50` |
| `sp_from_profit_pct(real, real)` | `sp_from_profit_pct(cp, p_pct)` | `func` | `real` | `cp * (1 + p_pct / 100)` | `sp_from_profit_pct(6, 3)` | `6.18` |
| `sp_from_loss_pct(real, real)` | `sp_from_loss_pct(cp, l_pct)` | `func` | `real` | `cp * (1 - l_pct / 100)` | `sp_from_loss_pct(6, 3)` | `5.82` |
| `cp_from_profit_pct(real, real)` | `cp_from_profit_pct(sp, p_pct)` | `func` | `real` | `sp / (1 + p_pct / 100)` | `cp_from_profit_pct(6, 3)` | `5.825242718` |
| `cp_from_loss_pct(real, real)` | `cp_from_loss_pct(sp, l_pct)` | `func` | `real` | `sp / (1 - l_pct / 100)` | `cp_from_loss_pct(6, 3)` | `6.18556701` |
| `discount_amt(real, real)` | `discount_amt(marked_p, d_pct)` | `func` | `real` | `marked_p * (d_pct / 100)` | `discount_amt(6, 3)` | `0.18` |
| `sp_after_discount(real, real)` | `sp_after_discount(marked_p, d_pct)` | `func` | `real` | `marked_p * (1 - d_pct / 100)` | `sp_after_discount(6, 3)` | `5.82` |
| `successive_discount(real, real)` | `successive_discount(d1, d2)` | `func` | `real` | `d1 + d2 - (d1 * d2) / 100` | `successive_discount(6, 3)` | `8.82` |
| `simple_interest(real, real, real)` | `simple_interest(p, r_pct, t_yr)` | `func` | `real` | `(p * r_pct * t_yr) / 100` | `simple_interest(6, 3, 2)` | `0.36` |
| `si_amount(real, real, real)` | `si_amount(p, r_pct, t_yr)` | `func` | `real` | `p + simple_interest(p, r_pct, t_yr)` | `si_amount(6, 3, 2)` | `6.36` |
| `compound_amount(real, real, real)` | `compound_amount(p, r_pct, t_yr)` | `func` | `real` | `p * ((1 + r_pct / 100) ^ t_yr)` | `compound_amount(6, 3, 2)` | `6.3654` |
| `compound_interest(real, real, real)` | `compound_interest(p, r_pct, t_yr)` | `func` | `real` | `compound_amount(p, r_pct, t_yr) - p` | `compound_interest(6, 3, 2)` | `0.3654` |
| `ci_amount_n(real, real, real, real)` | `ci_amount_n(p, r_pct, t_yr, n_per_yr)` | `func` | `real` | `p * ((1 + (r_pct / 100) / n_per_yr) ^ (n_per_yr * t_yr))` | `ci_amount_n(6, 3, 2, 4)` | `6.369593087` |
| `ci_si_diff_2yr(real, real)` | `ci_si_diff_2yr(p, r_pct)` | `func` | `real` | `p * ((r_pct / 100) ^ 2)` | `ci_si_diff_2yr(6, 3)` | `0.0054` |
| `depreciated_val(real, real, real)` | `depreciated_val(p0, r_pct, t_yr)` | `func` | `real` | `p0 * ((1 - r_pct / 100) ^ t_yr)` | `depreciated_val(6, 3, 2)` | `5.6454` |
| `emi(real, real, real)` | `emi(principal, annual_rate_pct, months)` | `func` | `real` | `{ r_m := (annual_rate_pct / 12) / 100 f   := (1 + r_m) ^ months (principal * ...` | `emi(6, 3, 2)` | `3.011254682` |
| `work_rate(real)` | `work_rate(days)` | `func` | `real` | `1 / days` | `work_rate(6)` | `0.1666666667` |
| `work_together_2(real, real)` | `work_together_2(d1, d2)` | `func` | `real` | `(d1 * d2) / (d1 + d2)` | `work_together_2(6, 3)` | `2` |
| `work_together_3(real, real, real)` | `work_together_3(d1, d2, d3)` | `func` | `real` | `(d1 * d2 * d3) / (d1 * d2 + d2 * d3 + d3 * d1)` | `work_together_3(6, 3, 2)` | `1` |
| `work_other_person(real, real)` | `work_other_person(d_both, d_one)` | `func` | `real` | `(d_both * d_one) / (d_one - d_both)` | `work_other_person(6, 3)` | `-6` |
| `work_rem_after(real, real)` | `work_rem_after(d_total, days_worked)` | `func` | `real` | `1 - (days_worked / d_total)` | `work_rem_after(6, 3)` | `0.5` |
| `pipe_fill_with_leak(real, real)` | `pipe_fill_with_leak(t_fill, t_empty)` | `func` | `real` | `(t_fill * t_empty) / (t_empty - t_fill)` | `pipe_fill_with_leak(6, 3)` | `-6` |
| `man_days_solve(real, real, real, real, real)` | `man_days_solve(m1, d1, w1, m2, w2)` | `func` | `real` | `(m1 * d1 * w2) / (w1 * m2)` | `man_days_solve(6, 3, 2, 4, 5)` | `11.25` |
| `ap_nth(real, real, real)` | `ap_nth(a, d, n)` | `func` | `real` | `a + (n - 1) * d` | `ap_nth(6, 3, 2)` | `9` |
| `ap_sum(real, real, real)` | `ap_sum(a, d, n)` | `func` | `real` | `(n / 2) * (2a + (n - 1) * d)` | `ap_sum(6, 3, 2)` | `15` |
| `gp_nth(real, real, real)` | `gp_nth(a, r, n)` | `func` | `real` | `a * (r ^ (n - 1))` | `gp_nth(6, 3, 2)` | `18` |
| `gp_sum(real, real, real)` | `gp_sum(a, r, n)` | `func` | `real` | `(r == 1) ? (a * n) : a * ((r ^ n) - 1) / (r - 1)` | `gp_sum(6, 3, 2)` | `24` |
| `gp_inf_sum(real, real)` | `gp_inf_sum(a, r)` | `func` | `real` | `a / (1 - r)` | `gp_inf_sum(10, 0.5)` | `20` |
| `hp_nth(real, real, real)` | `hp_nth(a, d, n)` | `func` | `real` | `1 / (a + (n - 1) * d)` | `hp_nth(6, 3, 2)` | `0.1111111111` |
| `arith_mean(real, real)` | `arith_mean(a, b)` | `func` | `real` | `(a + b) / 2` | `arith_mean(6, 3)` | `4.5` |
| `geom_mean(real, real)` | `geom_mean(a, b)` | `func` | `real` | `√(a * b)` | `geom_mean(6, 3)` | `4.242640687` |
| `harm_mean(real, real)` | `harm_mean(a, b)` | `func` | `real` | `2a * b / (a + b)` | `harm_mean(6, 3)` | `4` |
| `fourth_proportional(real, real, real)` | `fourth_proportional(a, b, c)` | `func` | `real` | `(b * c) / a` | `fourth_proportional(6, 3, 2)` | `1` |
| `third_proportional(real, real)` | `third_proportional(a, b)` | `func` | `real` | `(b^2) / a` | `third_proportional(6, 3)` | `1.5` |
| `mean_proportional(real, real)` | `mean_proportional(a, b)` | `func` | `real` | `√(a * b)` | `mean_proportional(6, 3)` | `4.242640687` |
| `alligation_ratio(real, real, real)` | `alligation_ratio(cheaper, dearer, mean_p)` | `func` | `real` | `(dearer - mean_p) / (mean_p - cheaper)` | `alligation_ratio(20, 50, 30)` | `2` |
| `avg_speed_2(real, real)` | `avg_speed_2(s1, s2)` | `func` | `real` | `2s1 * s2 / (s1 + s2)` | `avg_speed_2(6, 3)` | `4` |
| `rel_speed_same(real, real)` | `rel_speed_same(s1, s2)` | `func` | `real` | `@abs(s1 - s2)` | `rel_speed_same(6, 3)` | `3` |
| `rel_speed_opp(real, real)` | `rel_speed_opp(s1, s2)` | `func` | `real` | `s1 + s2` | `rel_speed_opp(6, 3)` | `9` |
| `boat_downstream(real, real)` | `boat_downstream(u_boat, v_stream)` | `func` | `real` | `u_boat + v_stream` | `boat_downstream(6, 3)` | `9` |
| `boat_upstream(real, real)` | `boat_upstream(u_boat, v_stream)` | `func` | `real` | `u_boat - v_stream` | `boat_upstream(6, 3)` | `3` |
| `boat_still_speed(real, real)` | `boat_still_speed(d_spd, u_spd)` | `func` | `real` | `(d_spd + u_spd) / 2` | `boat_still_speed(6, 3)` | `4.5` |
| `stream_speed(real, real)` | `stream_speed(d_spd, u_spd)` | `func` | `real` | `(d_spd - u_spd) / 2` | `stream_speed(6, 3)` | `1.5` |
| `sum_n(real)` | `sum_n(n)` | `func` | `real` | `n * (n + 1) / 2` | `sum_n(6)` | `21` |
| `sum_sq_n(real)` | `sum_sq_n(n)` | `func` | `real` | `n * (n + 1) * (2n + 1) / 6` | `sum_sq_n(6)` | `91` |
| `sum_cube_n(real)` | `sum_cube_n(n)` | `func` | `real` | `(n * (n + 1) / 2) ^ 2` | `sum_cube_n(6)` | `441` |

### 10.5 Module `[vec]` (`Library/vec.math` — 58 overloads)

Dynamic homogeneous 1D containers (`vec<T>`, `list<T>`, `stack<T>`, `queue<T>`), LIFO/FIFO operations (`push`, `pop`, `enqueue`, `dequeue`, `shift`, `pop_front`, `peek`, `top`, `back`, `front`, `is_empty`, `at`, `set`, `len`, `cap`, `resize`, `shrink`, `clear`), ranges (`..`, `...`, `..<`, `<..`, `by`, pure MEP `range`), membership/insert (`in`, `into`), higher-order pipelines (`map`, `filter`, `reduce`, `fold`, `any`, `all`, `find`, `count_if`), elementwise Hadamard operators (`+`, `-`, `.*`, `./`, `.^`), and statistics (`vec_sum`, `sum`, `mean`, `variance`, `stddev`, `sma`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `vec()` | `vec()                         -> vec` | `func` | `vec` | `@vnew(0)` | `vec()` | `[]` |
| `vec(real)` | `vec(cap:real, none)           -> vec` | `func` | `vec` | `@vnew(cap)` | `vec(6)` | `[]` |
| `vec(real, T)` | `vec(count:real, none, init:T) -> vec` | `func` | `vec` | `@vfill(count, init)` | `vec(6, 20)` | `[20, 20, 20, 20, 20, 20]` |
| `list()` | `list()                  -> vec` | `func` | `vec` | `@vnew(0)` | `list()` | `[]` |
| `list(T)` | `list(item:T)            -> vec` | `func` | `vec` | `@vfill(1, item)` | `list(10)` | `[10]` |
| `stack()` | `stack()                 -> vec` | `func` | `vec` | `@vnew(0)` | `stack()` | `[]` |
| `stack(T)` | `stack(item:T)           -> vec` | `func` | `vec` | `@vfill(1, item)` | `stack(10)` | `[10]` |
| `queue()` | `queue()                 -> vec` | `func` | `vec` | `@vnew(0)` | `queue()` | `[]` |
| `queue(T)` | `queue(item:T)           -> vec` | `func` | `vec` | `@vfill(1, item)` | `queue(10)` | `[10]` |
| `push(vec, T)` | `push(v:vec, item:T)     -> vec` | `func` | `vec` | `@vpush(v, item)` | `push(vec(2, 4, 6), 20)` | `[2, 4, 6, 20]` |
| `pop(vec)` | `pop(v:vec)              -> T` | `func` | `T` | `@vpop(v)` | `pop(vec(2, 4, 6))` | `6` |
| `enqueue(vec, T)` | `enqueue(q:vec, item:T)  -> vec` | `func` | `vec` | `@vpush(q, item)` | `enqueue(vec(2, 4, 6), 20)` | `[2, 4, 6, 20]` |
| `dequeue(vec)` | `dequeue(q:vec)          -> T` | `func` | `T` | `@vshift(q)` | `dequeue(vec(2, 4, 6))` | `2` |
| `shift(vec)` | `shift(q:vec)            -> T` | `func` | `T` | `@vshift(q)` | `shift(vec(2, 4, 6))` | `2` |
| `pop_front(vec)` | `pop_front(q:vec)        -> T` | `func` | `T` | `@vshift(q)` | `pop_front(vec(2, 4, 6))` | `2` |
| `peek(vec)` | `peek(s:vec)             -> T` | `func` | `T` | `s[-1]` | `peek(vec(2, 4, 6))` | `6` |
| `top(vec)` | `top(s:vec)              -> T` | `func` | `T` | `s[-1]` | `top(vec(2, 4, 6))` | `6` |
| `back(vec)` | `back(s:vec)             -> T` | `func` | `T` | `s[-1]` | `back(vec(2, 4, 6))` | `6` |
| `front(vec)` | `front(q:vec)            -> T` | `func` | `T` | `q[0]` | `front(vec(2, 4, 6))` | `2` |
| `is_empty(vec)` | `is_empty(v:vec)` | `func` | `real` | `@eq(@vlen(v), 0)` | `is_empty(vec(2, 4, 6))` | `0` |
| `at(vec, real)` | `at(v:vec, idx)          -> T` | `func` | `T` | `@vget(v, idx)` | `at(vec(2, 4, 6), 1)` | `4` |
| `set(vec, real, T)` | `set(v:vec, idx, val:T)  -> vec` | `func` | `vec` | `@vset(v, idx, val)` | `set(vec(2, 4, 6), 1, 99)` | `[2, 99, 6]` |
| `len(vec)` | `len(v:vec)` | `func` | `real` | `@vlen(v)` | `len(vec(2, 4, 6))` | `3` |
| `cap(vec)` | `cap(v:vec)` | `func` | `real` | `@vcap(v)` | `cap(vec(2, 4, 6))` | `4` |
| `resize(vec, real)` | `resize(v:vec, new_len)  -> vec` | `func` | `vec` | `@vresize(v, new_len)` | `resize(vec(2, 4, 6), 3)` | `[2, 4, 6]` |
| `shrink(vec)` | `shrink(v:vec)           -> vec` | `func` | `vec` | `@vshrink(v)` | `shrink(vec(2, 4, 6))` | `[2, 4, 6]` |
| `clear(vec)` | `clear(v:vec)            -> vec` | `func` | `vec` | `@vclear(v)` | `clear(vec(2, 4, 6))` | `[]` |
| `(real) ... (real)` | `(a:real, T) "..." (b:real, T) :<45 -> vec` | `infix_L :45` | `vec` | `@vrange(a, b, 0)` | `1...5` | `[1, 2, 3, 4, 5]` |
| `(real) ..< (real)` | `(a:real, T) "..<" (b:real, T) :<45 -> vec` | `infix_L :45` | `vec` | `@vrange(a, b, 1)` | `1..<5` | `[1, 2, 3, 4]` |
| `(real) <.. (real)` | `(a:real, T) "<.." (b:real, T) :<45 -> vec` | `infix_L :45` | `vec` | `@vrange(a, b, 2)` | `1<..5` | `[2, 3, 4, 5]` |
| `(real) .. (real)` | `(a:real, T) ".."  (b:real, T) :<45 -> vec` | `infix_L :45` | `vec` | `@vrange(a, b, 3)` | `1..5` | `[2, 3, 4]` |
| `(vec) by (real)` | `(r:vec)     by    (step:real) :<44 -> vec` | `infix_L :44` | `vec` | `@vrange_step(r, step, 0)` | `(1...9) by 2` | `[1, 3, 5, 7, 9]` |
| `(T) in (vec)` | `(item:T)    in    (v:vec)     :<42` | `infix_L :42` | `real` | `@vin(item, v)` | `10 in vec(1, 2, 3)` | `0` |
| `(T) into (vec)` | `(item:T)    into  (v:vec)     :>55 -> vec` | `infix_R :55` | `vec` | `@vpush(v, item)` | `40 into vec(10, 20, 30)` | `[10, 20, 30, 40]` |
| `range(real, real, real)` | `range(start:real, T, stop:real, T, step = 1) -> vec` | `func` | `vec` | `{ if (step == 0) { error("Range step cannot be zero") } out := vec<real, T>()...` | `range(6, 3, 2)` | `[]` |
| `range(real, real)` | `range(start:real, T, stop:real, T, step = 1) -> vec [default-arg overload]` | `func` | `vec` | `{ if (step == 0) { error("Range step cannot be zero") } out := vec<real, T>()...` | `range(6, 3)` | `[]` |
| `map(vec, fn)` | `map(v:vec, f:fn)            -> vec` | `func` | `vec` | `@vmap(v, f, 0)` | `map(vec(2, 4, 6), (x => x * 2))` | `[4, 8, 12]` |
| `filter(vec, fn)` | `filter(v:vec, f:fn)         -> vec` | `func` | `vec` | `@vfilter(v, f, 0)` | `filter(vec(2, 4, 6), (x => x * 2))` | `[2, 4, 6]` |
| `reduce(vec, fn)` | `reduce(v:vec, f:fn)         -> T` | `func` | `T` | `@vreduce(v, f, 0)` | `vec(1, 2, 3, 4).reduce((a, b) => a + b)` | `10` |
| `reduce(vec, T, fn)` | `reduce(v:vec, init:T, f:fn) -> T` | `func` | `T` | `@vreduce(v, init, f)` | `vec(1, 2, 3, 4).reduce(10, (a, b) => a + b)` | `20` |
| `fold(vec, T, fn)` | `fold(v:vec, init:T, f:fn)   -> T` | `func` | `T` | `@vreduce(v, init, f)` | `vec(1, 2, 3, 4).fold(10, (a, b) => a + b)` | `20` |
| `any(vec, fn)` | `any(v:vec, f:fn)` | `func` | `real` | `@vany(v, f, 0)` | `any(vec(2, 4, 6), (x => x * 2))` | `1` |
| `all(vec, fn)` | `all(v:vec, f:fn)` | `func` | `real` | `@vall(v, f, 0)` | `all(vec(2, 4, 6), (x => x * 2))` | `1` |
| `find(vec, fn)` | `find(v:vec, f:fn)           -> T` | `func` | `T` | `@vfind(v, f, 0)` | `find(vec(2, 4, 6), (x => x * 2))` | `2` |
| `count_if(vec, fn)` | `count_if(v:vec, f:fn)` | `func` | `real` | `@vcount(v, f, 0)` | `count_if(vec(2, 4, 6), (x => x * 2))` | `3` |
| `(vec) + (vec)` | `(a:vec) "+" (b:vec) :<50 -> vec` | `infix_L :50` | `vec` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '+'") } [x + b[...` | `vec(2, 4, 6) + vec(1, 2, 3)` | `[3, 6, 9]` |
| `(vec) - (vec)` | `(a:vec) "-" (b:vec) :<50 -> vec` | `infix_L :50` | `vec` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '-'") } [x - b[...` | `vec(2, 4, 6) - vec(1, 2, 3)` | `[1, 2, 3]` |
| `(vec) .* (vec)` | `(a:vec) ".*" (b:vec) :<60 -> vec` | `infix_L :60` | `vec` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '.*'") } [x * b...` | `vec(2, 4, 6) .* vec(1, 2, 3)` | `[2, 8, 18]` |
| `(vec) ./ (vec)` | `(a:vec) "./" (b:vec) :<60 -> vec` | `infix_L :60` | `vec` | `{ if (a.len() != b.len()) { error("Vector length mismatch for './'") } [x / b...` | `vec(2, 4, 6) ./ vec(1, 2, 3)` | `[2, 2, 2]` |
| `(vec) .^ (vec)` | `(a:vec) ".^" (b:vec) :>85 -> vec` | `infix_R :85` | `vec` | `{ if (a.len() != b.len()) { error("Vector length mismatch for '.^'") } [x ^ b...` | `vec(2, 4, 6) .^ vec(1, 2, 3)` | `[2, 16, 216]` |
| `(vec) .^ (real)` | `(a:vec)  ".^" (p:real) :>85 -> vec` | `infix_R :85` | `vec` | `[x ^ p for x in a]` | `vec(2, 4, 6) .^ 3` | `[8, 64, 216]` |
| `(real) .^ (vec)` | `(s:real) ".^" (a:vec)  :>85 -> vec` | `infix_R :85` | `vec` | `[s ^ x for x in a]` | `6 .^ vec(1, 2, 3)` | `[6, 36, 216]` |
| `vec_sum(vec)` | `vec_sum(v:vec)` | `func` | `real` | `{ s := 0 (x in v) { s += x } s }` | `vec_sum(vec(2, 4, 6))` | `12` |
| `sum(vec)` | `sum(v:vec)` | `func` | `real` | `vec_sum(v)` | `sum(vec(2, 4, 6))` | `12` |
| `mean(vec)` | `mean(v:vec)` | `func` | `real` | `vec_sum(v) / v.len()` | `mean(vec(2, 4, 6))` | `4` |
| `variance(vec)` | `variance(v:vec)` | `func` | `real` | `{ mu  := mean(v) acc := 0 (x in v) { d := x - mu acc += d^2 } acc / v.len() }` | `variance(vec(2, 4, 6))` | `2.666666667` |
| `stddev(vec)` | `stddev(v:vec)` | `func` | `real` | `√variance(v)` | `stddev(vec(2, 4, 6))` | `1.632993162` |
| `sma(vec, real)` | `sma(v:vec, window)` | `func` | `real` | `{ n     := v.len() if (n == 0) { error("Cannot compute sma() of an empty vect...` | `sma(vec(2, 4, 6), 3)` | `4` |

### 10.6 Module `[string]` (`Library/string.math` — 17 overloads)

First-class UTF-8 string (`$str`) operators (`+`, `into`, `==`, `!=`, `in`), indexing/slicing (`len`, `at`), and pure MEP string manipulation functions (`is_empty`, `substr`, `str_repeat`, `str_reverse`, `starts_with`, `ends_with`, `chars`, `str_join`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `(str) + (str)` | `(a:str)   "+"  (b:str) :<50 -> str` | `infix_L :50` | `str` | `@add(a, b)` | `"Hello" + "lo"` | `"Hellolo"` |
| `(str) into (str)` | `(a:str)   into (b:str) :>55 -> str` | `infix_R :55` | `str` | `@add(a, b)` | `"Hello" into "lo"` | `"Hellolo"` |
| `(str) == (str)` | `(a:str)   "==" (b:str) :<30` | `infix_L :30` | `real` | `@eq(a, b)` | `"Hello" == "lo"` | `0` |
| `(str) != (str)` | `(a:str)   "!=" (b:str) :<30` | `infix_L :30` | `real` | `@ne(a, b)` | `"Hello" != "lo"` | `1` |
| `(str) in (str)` | `(sub:str) in   (s:str) :<42` | `infix_L :42` | `real` | `@vin(sub, s)` | `"Hello" in "lo"` | `0` |
| `len(str)` | `len(s:str)` | `func` | `real` | `@vlen(s)` | `len("Hello")` | `5` |
| `at(str, real)` | `at(s:str, idx)     -> str` | `func` | `str` | `@vget(s, idx)` | `at("Planck", 0)` | `"P"` |
| `at(str, vec)` | `at(s:str, rng:vec) -> str` | `func` | `str` | `@vget(s, rng)` | `at("Planck", 0)` | `"P"` |
| `is_empty(str)` | `is_empty(s:str)` | `func` | `real` | `(len(s) == 0)` | `is_empty("Hello")` | `0` |
| `substr(str, real, real)` | `substr(s:str, start, stop) -> str` | `func` | `str` | `{ n := len(s) if (n == 0) { return("") } a := start < 0 ? n + start : start b...` | `substr("Hello", 3, 2)` | `""` |
| `str_repeat(str, real)` | `str_repeat(s:str, n) -> str` | `func` | `str` | `{ out := "" k   := 0 (k < n) ?? { out = out + s k  += 1 } out }` | `str_repeat("Hello", 3)` | `"HelloHelloHello"` |
| `str_reverse(str)` | `str_reverse(s:str) -> str` | `func` | `str` | `{ n := len(s) if (n <= 1) { return s } s[(n - 1)...0] }` | `str_reverse("Hello")` | `"olleH"` |
| `starts_with(str, str)` | `starts_with(s:str, prefix:str)` | `func` | `real` | `{ np := len(prefix) if (np > len(s)) { return 0 } if (np == 0) { return 1 } s...` | `starts_with("Hello", "lo")` | `0` |
| `ends_with(str, str)` | `ends_with(s:str, suffix:str)` | `func` | `real` | `{ ns := len(suffix) n  := len(s) if (ns > n) { return 0 } if (ns == 0) { retu...` | `ends_with("Hello", "lo")` | `1` |
| `chars(str)` | `chars(s:str) -> vec` | `func` | `vec` | `[c for c in s]` | `chars("Hello")` | `["H", "e", "l", "l", "o"]` |
| `str_join(vec, str)` | `str_join(v:vec, sep:str = "") -> str` | `func` | `str` | `{ out := "" (i, item in v) { if (i > 0) { out = out + sep } out = out + (item...` | `str_join(list("M", "E", "P"), "-")` | `"M-E-P"` |
| `str_join(vec)` | `str_join(v:vec, sep:str = "") -> str [default-arg overload]` | `func` | `str` | `{ out := "" (i, item in v) { if (i > 0) { out = out + sep } out = out + (item...` | `str_join(list("M", "E", "P"))` | `"MEP"` |

### 10.7 Module `[mat]` (`Library/mat.math` — 38 overloads)

1D–4D dense matrices & tensors (`$mat`), matrix arithmetic (`-`, `+`, `-`, `into`, `*`, `/`, `in`), Hadamard elementwise operators (`.*`, `./`, `.^`), constructors & shape queries (`mat_fill`, `mat_zeros`, `mat_ones`, `mat_eye`, `mat_rows`, `mat_cols`, `mat_rank`, `mat_len`, `mat_shape`), linear algebra (`transpose`, `trace`, `det`, `inv`, `linsolve`), and tensor higher-order functions (`map`, `filter`, `reduce`, `fold`, `any`, `all`, `find`, `count_if`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `-(mat)` | `"-" (m:mat, T)                 :80  -> mat, T` | `prefix :80` | `mat` | `@mscal(m, -1)` | `-[2, 1 ; 5, 3]` | `[[-2, -1], [-5, -3]]` |
| `(mat) + (mat)` | `(a:mat, T)     "+"  (b:mat, T) :<50 -> mat, T` | `infix_L :50` | `mat` | `@madd(a, b)` | `[2, 1 ; 5, 3] + [1, 0 ; 0, 1]` | `[[3, 1], [5, 4]]` |
| `(mat) - (mat)` | `(a:mat, T)     "-"  (b:mat, T) :<50 -> mat, T` | `infix_L :50` | `mat` | `@msub(a, b)` | `[2, 1 ; 5, 3] - [1, 0 ; 0, 1]` | `[[1, 1], [5, 2]]` |
| `(mat) into (mat)` | `(a:mat, T)     into (b:mat, T) :>55 -> mat, T` | `infix_R :55` | `mat` | `@madd(a, b)` | `[2, 1 ; 5, 3] into [1, 0 ; 0, 1]` | `[[3, 1], [5, 4]]` |
| `(mat) * (mat)` | `(a:mat)        "*"  (b:mat)    :<60 -> mat` | `infix_L :60` | `mat` | `@mmul(a, b)` | `[2, 1 ; 5, 3] * [1, 0 ; 0, 1]` | `[[2, 1], [5, 3]]` |
| `(mat) * (real)` | `(m:mat, T)     "*"  (s)        :<60 -> mat, T` | `infix_L :60` | `mat` | `@mscal(m, s)` | `[2, 1 ; 5, 3] * 3` | `[[6, 3], [15, 9]]` |
| `(real) * (mat)` | `(s)            "*"  (m:mat, T) :<60 -> mat, T` | `infix_L :60` | `mat` | `@mscal(m, s)` | `6 * [1, 0 ; 0, 1]` | `[[6, 0], [0, 6]]` |
| `(mat) / (real)` | `(m:mat, T)     "/"  (s)        :<60 -> mat, T` | `infix_L :60` | `mat` | `@mscal(m, 1 / s)` | `[2, 1 ; 5, 3] / 3` | `[[0.666667, 0.333333], [1.66667, 1]]` |
| `(real) in (mat)` | `(item:real, T) in   (m:mat, T) :<42` | `infix_L :42` | `real` | `@vin(item, m)` | `6 in [1, 0 ; 0, 1]` | `0` |
| `(mat) .* (mat)` | `(a:mat)    ".*" (b:mat)  :<60 -> mat` | `infix_L :60` | `mat` | `@mhadamard(a, b, 0)` | `[2, 1 ; 5, 3] .* [1, 0 ; 0, 1]` | `[[2, 0], [0, 3]]` |
| `(mat) ./ (mat)` | `(a:mat)    "./" (b:mat)  :<60 -> mat` | `infix_L :60` | `mat` | `@mhadamard(a, b, 1)` | `[4, 6 ; 8, 10] ./ [2, 3 ; 4, 5]` | `[[2, 2], [2, 2]]` |
| `(mat) .^ (real)` | `(a:mat, T) ".^" (b:real) :>85 -> mat, T` | `infix_R :85` | `mat` | `@mhadamard(a, b, 2)` | `[2, 1 ; 5, 3] .^ 3` | `[[8, 1], [125, 27]]` |
| `(real) .^ (mat)` | `(a:real)   ".^" (b:mat)  :>85 -> mat` | `infix_R :85` | `mat` | `b.map(x => a ^ x)` | `6 .^ [1, 0 ; 0, 1]` | `[[6, 1], [1, 6]]` |
| `(mat) .^ (mat)` | `(a:mat)    ".^" (b:mat)  :>85 -> mat` | `infix_R :85` | `mat` | `@mhadamard(a, b, 2)` | `[2, 1 ; 5, 3] .^ [1, 0 ; 0, 1]` | `[[2, 1], [1, 3]]` |
| `mat_fill(real, real, real)` | `mat_fill(r, c, v:real, T) -> mat, T` | `func` | `mat` | `@mfill(r, c, v)` | `mat_fill(6, 3, 2)` | `[[2, 2, 2], [2, 2, 2], [2, 2, 2], [2, 2, 2], [2, 2, 2], [2, 2, 2]]` |
| `mat_zeros(real, real)` | `mat_zeros(r, c)           -> mat` | `func` | `mat` | `@mzero(r, c)` | `mat_zeros(6, 3)` | `[[0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0], [0, 0, 0]]` |
| `mat_ones(real, real)` | `mat_ones(r, c)            -> mat` | `func` | `mat` | `mat_fill(r, c, 1)` | `mat_ones(6, 3)` | `[[1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1], [1, 1, 1]]` |
| `mat_eye(real)` | `mat_eye(n)                -> mat` | `func` | `mat` | `@meye(n)` | `mat_eye(6)` | `[[1, 0, 0, 0, 0, 0], [0, 1, 0, 0, 0, 0], [0, 0, 1, 0, 0, 0], [0, 0, 0, 1, 0, 0], [0, 0, 0, 0, 1, 0], [0, 0, 0, 0, 0, 1]]` |
| `mat_rows(mat)` | `mat_rows(m:mat)` | `func` | `real` | `@mrows(m)` | `mat_rows([2, 1 ; 5, 3])` | `2` |
| `mat_cols(mat)` | `mat_cols(m:mat)` | `func` | `real` | `@mcols(m)` | `mat_cols([2, 1 ; 5, 3])` | `2` |
| `mat_rank(mat)` | `mat_rank(m:mat)` | `func` | `real` | `@mrank(m)` | `mat_rank([2, 1 ; 5, 3])` | `2` |
| `mat_len(mat)` | `mat_len(m:mat)` | `func` | `real` | `@mlen(m)` | `mat_len([2, 1 ; 5, 3])` | `4` |
| `mat_shape(mat)` | `mat_shape(m:mat)          -> vec` | `func` | `vec` | `@mshape(m)` | `mat_shape([2, 1 ; 5, 3])` | `[2, 2]` |
| `transpose(mat)` | `transpose(m:mat, T)       -> mat, T` | `func` | `mat` | `@mtrans(m)` | `transpose([2, 1 ; 5, 3])` | `[[2, 5], [1, 3]]` |
| `trace(mat)` | `trace(m:mat, T)           -> real, T` | `func` | `real` | `@mtrace(m)` | `trace([2, 1 ; 5, 3])` | `5` |
| `det(mat)` | `det(m:mat)` | `func` | `real` | `@mdet(m)` | `det([2, 1 ; 5, 3])` | `1` |
| `inv(mat)` | `inv(m:mat)                -> mat` | `func` | `mat` | `@minv(m)` | `inv([2, 1 ; 5, 3])` | `[[3, -1], [-5, 2]]` |
| `linsolve(mat, mat)` | `linsolve(a:mat, b:mat)    -> mat` | `func` | `mat` | `@mlinsolve(a, b)` | `linsolve([2, 1 ; 5, 3], [4, 11])` | `[1, 2]` |
| `linsolve(mat, vec)` | `linsolve(a:mat, b:vec)    -> vec` | `func` | `vec` | `@mlinsolve(a, b)` | `linsolve([2, 1 ; 5, 3], [4, 11])` | `[1, 2]` |
| `map(mat, fn)` | `map(m:mat, f:fn)          -> mat` | `func` | `mat` | `@vmap(m, f, 0)` | `map([2, 1 ; 5, 3], (x => x * 2))` | `[[4, 2], [10, 6]]` |
| `filter(mat, fn)` | `filter(m:mat, f:fn)       -> vec` | `func` | `vec` | `@vfilter(m, f, 0)` | `filter([2, 1 ; 5, 3], (x => x * 2))` | `[2, 1, 5, 3]` |
| `reduce(mat, fn)` | `reduce(m:mat, f:fn)` | `func` | `real` | `@vreduce(m, f, 0)` | `[1, 2 ; 3, 4].reduce((a, b) => a + b)` | `10` |
| `reduce(mat, real, fn)` | `reduce(m:mat, init, f:fn)` | `func` | `real` | `@vreduce(m, init, f)` | `[1, 2 ; 3, 4].reduce(10, (a, b) => a + b)` | `20` |
| `fold(mat, real, fn)` | `fold(m:mat, init, f:fn)` | `func` | `real` | `@vreduce(m, init, f)` | `[1, 2 ; 3, 4].fold(10, (a, b) => a + b)` | `20` |
| `any(mat, fn)` | `any(m:mat, f:fn)` | `func` | `real` | `@vany(m, f, 0)` | `any([2, 1 ; 5, 3], (x => x * 2))` | `1` |
| `all(mat, fn)` | `all(m:mat, f:fn)` | `func` | `real` | `@vall(m, f, 0)` | `all([2, 1 ; 5, 3], (x => x * 2))` | `1` |
| `find(mat, fn)` | `find(m:mat, f:fn)` | `func` | `real` | `@vfind(m, f, 0)` | `find([2, 1 ; 5, 3], (x => x * 2))` | `2` |
| `count_if(mat, fn)` | `count_if(m:mat, f:fn)` | `func` | `real` | `@vcount(m, f, 0)` | `count_if([2, 1 ; 5, 3], (x => x * 2))` | `4` |

### 10.8 Module `[trig]` (`Library/trig.math` — 26 overloads)

Trigonometric, reciprocal trig, inverse trig, hyperbolic, inverse hyperbolic, logarithmic, exponential, root, hypotenuse, and logistic sigmoid functions.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `sin(real)` | `sin(x)` | `func` | `real` | `@sin(x)` | `sin(6)` | `-0.2794154982` |
| `cos(real)` | `cos(x)` | `func` | `real` | `@cos(x)` | `cos(6)` | `0.9601702867` |
| `tan(real)` | `tan(x)` | `func` | `real` | `@tan(x)` | `tan(6)` | `-0.2910061914` |
| `sec(real)` | `sec(x)` | `func` | `real` | `1 / @cos(x)` | `sec(6)` | `1.041481927` |
| `csc(real)` | `csc(x)` | `func` | `real` | `1 / @sin(x)` | `csc(6)` | `-3.578899547` |
| `cot(real)` | `cot(x)` | `func` | `real` | `@cos(x) / @sin(x)` | `cot(6)` | `-3.436353004` |
| `asin(real)` | `asin(x)` | `func` | `real` | `@asin(x)` | `asin(0.5)` | `0.5235987756` |
| `acos(real)` | `acos(x)` | `func` | `real` | `@acos(x)` | `acos(0.5)` | `1.047197551` |
| `atan(real)` | `atan(x)` | `func` | `real` | `@atan(x)` | `atan(6)` | `1.405647649` |
| `atan2(real, real)` | `atan2(y, x)` | `func` | `real` | `@atan2(y, x)` | `atan2(6, 3)` | `1.107148718` |
| `sinh(real)` | `sinh(x)` | `func` | `real` | `@sinh(x)` | `sinh(6)` | `201.7131574` |
| `cosh(real)` | `cosh(x)` | `func` | `real` | `@cosh(x)` | `cosh(6)` | `201.7156361` |
| `tanh(real)` | `tanh(x)` | `func` | `real` | `@tanh(x)` | `tanh(6)` | `0.9999877117` |
| `asinh(real)` | `asinh(x)` | `func` | `real` | `@ln(x + √(x^2 + 1))` | `asinh(6)` | `2.491779853` |
| `acosh(real)` | `acosh(x)` | `func` | `real` | `@ln(x + √(x^2 - 1))` | `acosh(2)` | `1.316957897` |
| `atanh(real)` | `atanh(x)` | `func` | `real` | `0.5 * @ln((1 + x) / (1 - x))` | `atanh(0.5)` | `0.5493061443` |
| `ln(real)` | `ln(x)` | `func` | `real` | `@ln(x)` | `ln(6)` | `1.791759469` |
| `log10(real)` | `log10(x)` | `func` | `real` | `@log10(x)` | `log10(6)` | `0.7781512504` |
| `log2(real)` | `log2(x)` | `func` | `real` | `@ln(x) / @ln(2)` | `log2(6)` | `2.584962501` |
| `logb(real, real)` | `logb(x, base)` | `func` | `real` | `@ln(x) / @ln(base)` | `logb(6, 3)` | `1.630929754` |
| `exp(real)` | `exp(x)` | `func` | `real` | `@exp(x)` | `exp(6)` | `403.4287935` |
| `sqrt(real)` | `sqrt(x)` | `func` | `real` | `@sqrt(x)` | `sqrt(6)` | `2.449489743` |
| `cbrt(real)` | `cbrt(x)` | `func` | `real` | `@cbrt(x)` | `cbrt(6)` | `1.817120593` |
| `root(real, real)` | `root(x, n)` | `func` | `real` | `x ^ (1 / n)` | `root(6, 3)` | `1.817120593` |
| `hypot(real, real)` | `hypot(a, b)` | `func` | `real` | `√(a^2 + b^2)` | `hypot(6, 3)` | `6.708203932` |
| `sigmoid(real)` | `sigmoid(x)` | `func` | `real` | `1 / (1 + @exp(-x))` | `sigmoid(6)` | `0.9975273768` |

### 10.9 Module `[complex_ext]` (`Library/complex.math` — 49 overloads)

Non-consuming `$complex(re, im)` struct, postfix `i` operator on `real`/`iota`/`complex` (preserving unit tag `T`), full `iota` & `complex` arithmetic (`-`, `+`, `-`, `*`, `/`), `into` composition, equality (`==`, `!=`, `~`), and complex analysis helpers (`conj`, `cabs`, `carg`, `cexp_i`, `polar`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `(real) i` | `(x:real, T)    i :95 -> iota, T` | `postfix :95` | `iota` | `@raw(x)` | `6 i` | `6i` |
| `(iota) i` | `(x:iota, T)    i :95 -> real, T` | `postfix :95` | `real` | `-@raw(x)` | `4i i` | `-4` |
| `(complex) i` | `(z:complex, T) i :95 -> complex, T` | `postfix :95` | `complex` | `complex(-z.im, z.re)` | `(3 + 4i) i` | `-4 + 3i` |
| `-(iota)` | `"-" (a:iota, T)             :80  -> iota, T` | `prefix :80` | `iota` | `@neg(a)` | `-4i` | `-4i` |
| `(iota) + (iota)` | `(a:iota, T) "+"  (b:iota, T):<50 -> iota, T` | `infix_L :50` | `iota` | `@add(a, b)` | `4i + 2i` | `6i` |
| `(iota) - (iota)` | `(a:iota, T) "-"  (b:iota, T):<50 -> iota, T` | `infix_L :50` | `iota` | `@sub(a, b)` | `4i - 2i` | `2i` |
| `(iota) into (iota)` | `(a:iota, T) into (b:iota, T):>55 -> iota, T` | `infix_R :55` | `iota` | `@add(a, b)` | `4i into 2i` | `6i` |
| `(iota) * (iota)` | `(a:iota)    "*"  (b:iota)   :<60` | `infix_L :60` | `real` | `-(@mul(a, b))` | `4i * 2i` | `-8` |
| `(real) * (iota)` | `(a)         "*"  (b:iota)   :<60 -> iota` | `infix_L :60` | `iota` | `@mul(a, b)` | `6 * 2i` | `12i` |
| `(iota) * (real)` | `(a:iota)    "*"  (b)        :<60 -> iota` | `infix_L :60` | `iota` | `@mul(a, b)` | `4i * 3` | `12i` |
| `(iota) / (iota)` | `(a:iota)    "/"  (b:iota)   :<60` | `infix_L :60` | `real` | `@div(a, b)` | `4i / 2i` | `2` |
| `(iota) / (real)` | `(a:iota, T) "/"  (b:real)   :<60 -> iota, T` | `infix_L :60` | `iota` | `@div(a, b)` | `4i / 3` | `1.33333i` |
| `(real) / (iota)` | `(a:real, T) "/"  (b:iota)   :<60 -> iota, T` | `infix_L :60` | `iota` | `-(@div(a, b))` | `6 / 2i` | `-3i` |
| `(iota) == (iota)` | `(a:iota, T) "==" (b:iota, T):<30` | `infix_L :30` | `real` | `@eq(a, b)` | `4i == 2i` | `0` |
| `(iota) != (iota)` | `(a:iota, T) "!=" (b:iota, T):<30` | `infix_L :30` | `real` | `@ne(a, b)` | `4i != 2i` | `1` |
| `(iota) ~ (iota)` | `(a:iota, T) "~"  (b:iota, T):<30` | `infix_L :30` | `real` | `@approx(a, b)` | `4i ~ 2i` | `0` |
| `(real) + (iota)` | `(a:real, T)    "+" (b:iota, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(a, @raw(b))` | `6 + 2i` | `6 + 2i` |
| `(real) - (iota)` | `(a:real, T)    "-" (b:iota, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(a, -@raw(b))` | `6 - 2i` | `6 - 2i` |
| `(iota) + (real)` | `(a:iota, T)    "+" (b:real, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(b, @raw(a))` | `4i + 3` | `3 + 4i` |
| `(iota) - (real)` | `(a:iota, T)    "-" (b:real, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(-b, @raw(a))` | `4i - 3` | `-3 + 4i` |
| `(complex) + (iota)` | `(z:complex, T) "+" (b:iota, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(z.re, z.im + @raw(b))` | `(3 + 4i) + 2i` | `3 + 6i` |
| `(complex) - (iota)` | `(z:complex, T) "-" (b:iota, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(z.re, z.im - @raw(b))` | `(3 + 4i) - 2i` | `3 + 2i` |
| `(iota) + (complex)` | `(a:iota, T)    "+" (z:complex, T) :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(z.re, @raw(a) + z.im)` | `4i + (1 + 2i)` | `1 + 6i` |
| `(iota) - (complex)` | `(a:iota, T)    "-" (z:complex, T) :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(-z.re, @raw(a) - z.im)` | `4i - (1 + 2i)` | `-1 + 2i` |
| `(complex) + (real)` | `(z:complex, T) "+" (b:real, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(z.re + b, z.im)` | `(3 + 4i) + 3` | `6 + 4i` |
| `(complex) - (real)` | `(z:complex, T) "-" (b:real, T)    :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(z.re - b, z.im)` | `(3 + 4i) - 3` | `0 + 4i` |
| `(real) + (complex)` | `(a:real, T)    "+" (z:complex, T) :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(a + z.re, z.im)` | `6 + (1 + 2i)` | `7 + 2i` |
| `(real) - (complex)` | `(a:real, T)    "-" (z:complex, T) :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(a - z.re, -z.im)` | `6 - (1 + 2i)` | `5 - 2i` |
| `(real) into (iota)` | `(a:real, T)    into (b:iota, T)    :>55 -> complex, T` | `infix_R :55` | `complex` | `complex(a, @raw(b))` | `6 into 2i` | `6 + 2i` |
| `(iota) into (real)` | `(a:iota, T)    into (b:real, T)    :>55 -> complex, T` | `infix_R :55` | `complex` | `complex(b, @raw(a))` | `4i into 3` | `3 + 4i` |
| `(real) into (complex)` | `(a:real, T)    into (z:complex, T) :>55 -> complex, T` | `infix_R :55` | `complex` | `complex(z.re + a, z.im)` | `6 into (1 + 2i)` | `7 + 2i` |
| `(iota) into (complex)` | `(a:iota, T)    into (z:complex, T) :>55 -> complex, T` | `infix_R :55` | `complex` | `complex(z.re, z.im + @raw(a))` | `4i into (1 + 2i)` | `1 + 6i` |
| `(complex) into (complex)` | `(a:complex, T) into (b:complex, T) :>55 -> complex, T` | `infix_R :55` | `complex` | `complex(a.re + b.re, a.im + b.im)` | `(3 + 4i) into (1 + 2i)` | `4 + 6i` |
| `-(complex)` | `"-" (z:complex, T)                 :80  -> complex, T` | `prefix :80` | `complex` | `complex(-z.re, -z.im)` | `-(3 + 4i)` | `-3 - 4i` |
| `(complex) + (complex)` | `(a:complex, T) "+" (b:complex, T) :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(a.re + b.re, a.im + b.im)` | `(3 + 4i) + (1 + 2i)` | `4 + 6i` |
| `(complex) - (complex)` | `(a:complex, T) "-" (b:complex, T) :<50 -> complex, T` | `infix_L :50` | `complex` | `complex(a.re - b.re, a.im - b.im)` | `(3 + 4i) - (1 + 2i)` | `2 + 2i` |
| `(real) * (complex)` | `(s:real)       "*" (z:complex, T) :<60 -> complex, T` | `infix_L :60` | `complex` | `complex(s * z.re, s * z.im)` | `6 * (1 + 2i)` | `6 + 12i` |
| `(complex) * (real)` | `(z:complex, T) "*" (s:real)       :<60 -> complex, T` | `infix_L :60` | `complex` | `complex(z.re * s, z.im * s)` | `(3 + 4i) * 3` | `9 + 12i` |
| `(complex) / (real)` | `(z:complex, T) "/" (s:real)       :<60 -> complex, T` | `infix_L :60` | `complex` | `complex(z.re / s, z.im / s)` | `(3 + 4i) / 3` | `1 + 1.33333i` |
| `(complex) * (complex)` | `(a:complex)    "*" (b:complex)    :<60 -> complex` | `infix_L :60` | `complex` | `complex(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re)` | `(3 + 4i) * (1 + 2i)` | `-5 + 10i` |
| `(complex) / (complex)` | `(a:complex)    "/" (b:complex)    :<60 -> complex` | `infix_L :60` | `complex` | `{ denom := b.re^2 + b.im^2 complex((a.re * b.re + a.im * b.im) / denom, (a.im...` | `(3 + 4i) / (1 + 2i)` | `2.2 - 0.4i` |
| `conj(complex)` | `conj(z:complex, T)      -> complex, T` | `func` | `complex` | `complex(z.re, -z.im)` | `conj((3 + 4i))` | `3 - 4i` |
| `cabs(complex)` | `cabs(z:complex, T)      -> real, T` | `func` | `real` | `√(z.re^2 + z.im^2)` | `cabs((3 + 4i))` | `5` |
| `carg(complex)` | `carg(z:complex)` | `func` | `real` | `atan2(z.im, z.re)` | `carg((3 + 4i))` | `0.927295218` |
| `cexp_i(real)` | `cexp_i(theta)           -> complex` | `func` | `complex` | `complex(cos(theta), sin(theta))` | `cexp_i(6)` | `0.96017 - 0.279415i` |
| `polar(real, real)` | `polar(r:real, T, theta) -> complex, T` | `func` | `complex` | `complex(r * cos(theta), r * sin(theta))` | `polar(6, 3)` | `-5.93995 + 0.84672i` |
| `(complex) == (complex)` | `(a:iota, T) "==" (b:iota, T):<30 [default-arg overload]` | `infix_L :30` | `real` | `@eq(a, b)` | `(3 + 4i) == (1 + 2i)` | `0` |
| `(complex) != (complex)` | `(a:iota, T) "!=" (b:iota, T):<30 [default-arg overload]` | `infix_L :30` | `real` | `@ne(a, b)` | `(3 + 4i) != (1 + 2i)` | `1` |
| `(complex) ~ (complex)` | `(a:complex, T) "~"  (b:complex, T) :<30` | `infix_L :30` | `real` | `(a.re ~ b.re) && (a.im ~ b.im)` | `(3 + 4i) ~ (1 + 2i)` | `0` |

### 10.10 Module `[geom_ext]` (`Library/geom.math` — 38 overloads)

2D/3D vector and circle structs (`$vec2(x, y)`, `$vec3(x, y, z)`, `$circle(center:vec2, radius)`), vector arithmetic (`-`, `+`, `-`, `*`, `/`, `**` dot product), `==`/`!=`/`~`, `into` promotion (`vec2` + `z` -> `vec3`), products & norms (`dot`, `cross2`, `dot3`, `cross3`, `vec2_mag`, `vec3_mag`, `vec2_dist`, `vec3_dist`, `lerp`, `vec3_path_len`), and circle/sphere geometry.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `-(vec2)` | `"-" (v:vec2)             :80  -> vec2` | `prefix :80` | `vec2` | `vec2(-v.x, -v.y)` | `-vec2(3, 4)` | `vec2(x=-3, y=-4)` |
| `(vec2) + (vec2)` | `(a:vec2) "+"  (b:vec2) :<50 -> vec2` | `infix_L :50` | `vec2` | `vec2(a.x + b.x, a.y + b.y)` | `vec2(3, 4) + vec2(1, 2)` | `vec2(x=4, y=6)` |
| `(vec2) - (vec2)` | `(a:vec2) "-"  (b:vec2) :<50 -> vec2` | `infix_L :50` | `vec2` | `vec2(a.x - b.x, a.y - b.y)` | `vec2(3, 4) - vec2(1, 2)` | `vec2(x=2, y=2)` |
| `(vec2) into (vec2)` | `(a:vec2) into (b:vec2) :>55 -> vec2` | `infix_R :55` | `vec2` | `vec2(a.x + b.x, a.y + b.y)` | `vec2(3, 4) into vec2(1, 2)` | `vec2(x=4, y=6)` |
| `(real) * (vec2)` | `(s)      "*"  (v:vec2) :<60 -> vec2` | `infix_L :60` | `vec2` | `vec2(s * v.x, s * v.y)` | `6 * vec2(1, 2)` | `vec2(x=6, y=12)` |
| `(vec2) * (real)` | `(v:vec2) "*"  (s)      :<60 -> vec2` | `infix_L :60` | `vec2` | `vec2(v.x * s, v.y * s)` | `vec2(3, 4) * 3` | `vec2(x=9, y=12)` |
| `(vec2) / (real)` | `(v:vec2) "/"  (s)      :<60 -> vec2` | `infix_L :60` | `vec2` | `vec2(v.x / s, v.y / s)` | `vec2(3, 4) / 3` | `vec2(x=1, y=1.33333)` |
| `(vec2) == (vec2)` | `(a:vec2) "==" (b:vec2) :<30` | `infix_L :30` | `real` | `(a.x == b.x) && (a.y == b.y)` | `vec2(3, 4) == vec2(1, 2)` | `0` |
| `(vec2) != (vec2)` | `(a:vec2) "!=" (b:vec2) :<30` | `infix_L :30` | `real` | `(a.x != b.x) \|\| (a.y != b.y)` | `vec2(3, 4) != vec2(1, 2)` | `1` |
| `(vec2) ~ (vec2)` | `(a:vec2) "~"  (b:vec2) :<30` | `infix_L :30` | `real` | `(a.x ~ b.x) && (a.y ~ b.y)` | `vec2(3, 4) ~ vec2(1, 2)` | `0` |
| `(real) into (vec2)` | `(z:real, T) into (v:vec2, T) :>55 -> vec3, T` | `infix_R :55` | `vec3` | `vec3(v.x, v.y, z)` | `6 into vec2(1, 2)` | `vec3(x=1, y=2, z=6)` |
| `(vec2) into (real)` | `(v:vec2, T) into (z:real, T) :>55 -> vec3, T` | `infix_R :55` | `vec3` | `vec3(v.x, v.y, z)` | `vec2(3, 4) into 3` | `vec3(x=3, y=4, z=3)` |
| `dot(vec2, vec2)` | `dot(a:vec2, b:vec2)` | `func` | `real` | `a.x * b.x + a.y * b.y` | `dot(vec2(3, 4), vec2(1, 2))` | `11` |
| `cross2(vec2, vec2)` | `cross2(a:vec2, b:vec2)` | `func` | `real` | `a.x * b.y - a.y * b.x` | `cross2(vec2(3, 4), vec2(1, 2))` | `2` |
| `vec2_mag(vec2)` | `vec2_mag(v:vec2)` | `func` | `real` | `hypot(v.x, v.y)` | `vec2_mag(vec2(3, 4))` | `5` |
| `vec2_dist(vec2, vec2)` | `vec2_dist(a:vec2, b:vec2)` | `func` | `real` | `hypot(b.x - a.x, b.y - a.y)` | `vec2_dist(vec2(3, 4), vec2(1, 2))` | `2.828427125` |
| `-(vec3)` | `"-" (v:vec3)             :80  -> vec3` | `prefix :80` | `vec3` | `vec3(-v.x, -v.y, -v.z)` | `-vec3(1, 2, 2)` | `vec3(x=-1, y=-2, z=-2)` |
| `(vec3) + (vec3)` | `(a:vec3) "+"  (b:vec3) :<50 -> vec3` | `infix_L :50` | `vec3` | `vec3(a.x + b.x, a.y + b.y, a.z + b.z)` | `vec3(1, 2, 2) + vec3(3, 4, 0)` | `vec3(x=4, y=6, z=2)` |
| `(vec3) - (vec3)` | `(a:vec3) "-"  (b:vec3) :<50 -> vec3` | `infix_L :50` | `vec3` | `vec3(a.x - b.x, a.y - b.y, a.z - b.z)` | `vec3(1, 2, 2) - vec3(3, 4, 0)` | `vec3(x=-2, y=-2, z=2)` |
| `(vec3) into (vec3)` | `(a:vec3) into (b:vec3) :>55 -> vec3` | `infix_R :55` | `vec3` | `vec3(a.x + b.x, a.y + b.y, a.z + b.z)` | `vec3(1, 2, 2) into vec3(3, 4, 0)` | `vec3(x=4, y=6, z=2)` |
| `(real) * (vec3)` | `(s)      "*"  (v:vec3) :<60 -> vec3` | `infix_L :60` | `vec3` | `vec3(s * v.x, s * v.y, s * v.z)` | `6 * vec3(3, 4, 0)` | `vec3(x=18, y=24, z=0)` |
| `(vec3) * (real)` | `(v:vec3) "*"  (s)      :<60 -> vec3` | `infix_L :60` | `vec3` | `vec3(v.x * s, v.y * s, v.z * s)` | `vec3(1, 2, 2) * 3` | `vec3(x=3, y=6, z=6)` |
| `(vec3) / (real)` | `(v:vec3) "/"  (s)      :<60 -> vec3` | `infix_L :60` | `vec3` | `vec3(v.x / s, v.y / s, v.z / s)` | `vec3(1, 2, 2) / 3` | `vec3(x=0.333333, y=0.666667, z=0.666667)` |
| `(vec3) ** (vec3)` | `(a:vec3) "**" (b:vec3) :<20` | `infix_L :20` | `real` | `a.x * b.x + a.y * b.y + a.z * b.z` | `vec3(1, 2, 2) ** vec3(3, 4, 0)` | `11` |
| `(vec3) == (vec3)` | `(a:vec2) "==" (b:vec2) :<30 [default-arg overload]` | `infix_L :30` | `real` | `(a.x == b.x) && (a.y == b.y)` | `vec3(1, 2, 2) == vec3(3, 4, 0)` | `0` |
| `(vec3) != (vec3)` | `(a:vec2) "!=" (b:vec2) :<30 [default-arg overload]` | `infix_L :30` | `real` | `(a.x != b.x) \|\| (a.y != b.y)` | `vec3(1, 2, 2) != vec3(3, 4, 0)` | `1` |
| `(vec3) ~ (vec3)` | `(a:vec3) "~"  (b:vec3) :<30` | `infix_L :30` | `real` | `(a.x ~ b.x) && (a.y ~ b.y) && (a.z ~ b.z)` | `vec3(1, 2, 2) ~ vec3(3, 4, 0)` | `0` |
| `dot3(vec3, vec3)` | `dot3(a:vec3, b:vec3)` | `func` | `real` | `a ** b` | `dot3(vec3(1, 2, 2), vec3(3, 4, 0))` | `11` |
| `cross3(vec3, vec3)` | `cross3(a:vec3, b:vec3) -> vec3` | `func` | `vec3` | `vec3(a.y * b.z - a.z * b.y,` | `cross3(vec3(1, 2, 2), vec3(3, 4, 0))` | `vec3(x=-8, y=6, z=-2)` |
| `vec3_mag(vec3)` | `vec3_mag(v:vec3)` | `func` | `real` | `hypot(hypot(v.x, v.y), v.z)` | `vec3_mag(vec3(1, 2, 2))` | `3` |
| `vec3_dist(vec3, vec3)` | `vec3_dist(a:vec3, b:vec3)` | `func` | `real` | `vec3_mag(b - a)` | `vec3_dist(vec3(1, 2, 2), vec3(3, 4, 0))` | `3.464101615` |
| `lerp(real, real, real)` | `lerp(a, b, t)` | `func` | `real` | `a + (b - a) * t` | `lerp(6, 3, 2)` | `0` |
| `vec3_path_len(vec)` | `vec3_path_len(pts:vec)` | `func` | `real` | `{ total := 0 idx   := 1 n     := pts.len() (idx < n) ?? { total += vec3_dist(...` | `vec3_path_len(list(vec3(0,0,0), vec3(3,4,0), vec3(3,4,12)))` | `17` |
| `move_circle(circle, real, real)` | `move_circle(c:circle, dx, dy) -> circle` | `func` | `circle` | `{ c.center.x += dx c.center.y += dy c }` | `move_circle(circle(vec2(1, 2), 5), 3, 2)` | `circle(center=vec2(x=4, y=4), radius=5)` |
| `circle_area(circle)` | `circle_area(c:circle)` | `func` | `real` | `π * c.radius^2` | `circle_area(circle(vec2(1, 2), 5))` | `78.53981634` |
| `circle_circum(circle)` | `circle_circum(c:circle)` | `func` | `real` | `2π * c.radius` | `circle_circum(circle(vec2(1, 2), 5))` | `31.41592654` |
| `sphere_vol(real)` | `sphere_vol(r)` | `func` | `real` | `(4 / 3) * π * (r ^ 3)` | `sphere_vol(6)` | `904.7786842` |
| `sphere_area(real)` | `sphere_area(r)` | `func` | `real` | `4π * (r ^ 2)` | `sphere_area(6)` | `452.3893421` |

### 10.11 Module `[algebra]` (`Library/algebra.math` — 28 overloads)

2x2 matrix struct (`$mat2(a, b, c, d)`), `mat2` operators (`-`, `+`, `-`, `into`, `*`, `/`, `==`, `!=`, `~`), `det`/`trace`/`transpose`/`inv_mat2`, 2x2 Cramer's rule solver (`solve_2x2`), quadratic solvers (`quad_disc`, `quad_roots`, `quad_croot`), and coefficient-vector polynomial algebra (`poly_val`, `poly_deriv`, `poly_integ`, `poly_add`, `poly_mul`, `poly_newton`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `-(mat2)` | `(m1:mat2) "-"  (m2:mat2) :<50 -> mat2` | `prefix :80` | `mat2` | `mat2(m1.a - m2.a, m1.b - m2.b, m1.c - m2.c, m1.d - m2.d)` | `-mat2(4, 7, 2, 6)` | `mat2(a=-4, b=-7, c=-2, d=-6)` |
| `(mat2) + (mat2)` | `(m1:mat2) "+"  (m2:mat2) :<50 -> mat2` | `infix_L :50` | `mat2` | `mat2(m1.a + m2.a, m1.b + m2.b, m1.c + m2.c, m1.d + m2.d)` | `mat2(4, 7, 2, 6) + mat2(1, 0, 0, 1)` | `mat2(a=5, b=7, c=2, d=7)` |
| `(mat2) - (mat2)` | `"-" (m:mat2)             :80  -> mat2` | `infix_L :50` | `mat2` | `mat2(-m.a, -m.b, -m.c, -m.d)` | `mat2(4, 7, 2, 6) - mat2(1, 0, 0, 1)` | `mat2(a=3, b=7, c=2, d=5)` |
| `(mat2) into (mat2)` | `(m1:mat2) into (m2:mat2) :>55 -> mat2` | `infix_R :55` | `mat2` | `mat2(m1.a + m2.a, m1.b + m2.b, m1.c + m2.c, m1.d + m2.d)` | `mat2(4, 7, 2, 6) into mat2(1, 0, 0, 1)` | `mat2(a=5, b=7, c=2, d=7)` |
| `(mat2) * (mat2)` | `(m1:mat2) "*"  (m2:mat2) :<60 -> mat2` | `infix_L :60` | `mat2` | `mat2(m1.a * m2.a + m1.b * m2.c, m1.a * m2.b + m1.b * m2.d,` | `mat2(4, 7, 2, 6) * mat2(1, 0, 0, 1)` | `mat2(a=4, b=7, c=2, d=6)` |
| `(mat2) * (vec2)` | `(m:mat2) "*" (v:vec2) :<60 -> vec2` | `infix_L :60` | `vec2` | `vec2(m.a * v.x + m.b * v.y, m.c * v.x + m.d * v.y)` | `mat2(4, 7, 2, 6) * vec2(1, 2)` | `vec2(x=18, y=14)` |
| `(real) * (mat2)` | `(s)      "*" (m:mat2) :<60 -> mat2` | `infix_L :60` | `mat2` | `mat2(s * m.a, s * m.b, s * m.c, s * m.d)` | `6 * mat2(1, 0, 0, 1)` | `mat2(a=6, b=0, c=0, d=6)` |
| `(mat2) * (real)` | `(m:mat2) "*" (s)      :<60 -> mat2` | `infix_L :60` | `mat2` | `mat2(m.a * s, m.b * s, m.c * s, m.d * s)` | `mat2(4, 7, 2, 6) * 3` | `mat2(a=12, b=21, c=6, d=18)` |
| `(mat2) / (real)` | `(m:mat2) "/" (s)      :<60 -> mat2` | `infix_L :60` | `mat2` | `mat2(m.a / s, m.b / s, m.c / s, m.d / s)` | `mat2(4, 7, 2, 6) / 3` | `mat2(a=1.33333, b=2.33333, c=0.666667, d=2)` |
| `(mat2) == (mat2)` | `(m1:mat2) "==" (m2:mat2) :<30` | `infix_L :30` | `real` | `(m1.a == m2.a) && (m1.b == m2.b) && (m1.c == m2.c) && (m1.d == m2.d)` | `mat2(4, 7, 2, 6) == mat2(1, 0, 0, 1)` | `0` |
| `(mat2) != (mat2)` | `(m1:mat2) "!=" (m2:mat2) :<30` | `infix_L :30` | `real` | `(m1.a != m2.a) \|\| (m1.b != m2.b) \|\| (m1.c != m2.c) \|\| (m1.d != m2.d)` | `mat2(4, 7, 2, 6) != mat2(1, 0, 0, 1)` | `1` |
| `(mat2) ~ (mat2)` | `(m1:mat2) "~"  (m2:mat2) :<30` | `infix_L :30` | `real` | `(m1.a ~ m2.a) && (m1.b ~ m2.b) && (m1.c ~ m2.c) && (m1.d ~ m2.d)` | `mat2(4, 7, 2, 6) ~ mat2(1, 0, 0, 1)` | `0` |
| `det(mat2)` | `det(m:mat2)` | `func` | `real` | `m.a * m.d - m.b * m.c` | `det(mat2(4, 7, 2, 6))` | `10` |
| `trace(mat2)` | `trace(m:mat2)` | `func` | `real` | `m.a + m.d` | `trace(mat2(4, 7, 2, 6))` | `10` |
| `transpose(mat2)` | `transpose(m:mat2) -> mat2` | `func` | `mat2` | `mat2(m.a, m.c, m.b, m.d)` | `transpose(mat2(4, 7, 2, 6))` | `mat2(a=4, b=2, c=7, d=6)` |
| `inv_mat2(mat2)` | `inv_mat2(m:mat2) -> mat2` | `func` | `mat2` | `{ d := det(m) mat2(m.d / d, -m.b / d, -m.c / d, m.a / d) }` | `inv_mat2(mat2(4, 7, 2, 6))` | `mat2(a=0.6, b=-0.7, c=-0.2, d=0.4)` |
| `solve_2x2(real, real, real, real, real, real)` | `solve_2x2(a1, b1, c1, a2, b2, c2) -> vec2` | `func` | `vec2` | `{ d  := a1 * b2 - b1 * a2 dx := c1 * b2 - b1 * c2 dy := a1 * c2 - c1 * a2 vec...` | `solve_2x2(6, 3, 2, 4, 5, 1)` | `vec2(x=0.388889, y=-0.111111)` |
| `quad_disc(real, real, real)` | `quad_disc(a, b, c)` | `func` | `real` | `b^2 - 4a * c` | `quad_disc(6, 3, 2)` | `-39` |
| `quad_roots(real, real, real)` | `quad_roots(a, b, c) -> vec2` | `func` | `vec2` | `{ sd := √(b^2 - 4a * c) vec2((-b + sd) / (2a), (-b - sd) / (2a)) }` | `quad_roots(1, -5, 6)` | `vec2(x=3, y=2)` |
| `quad_croot(real, real, real)` | `quad_croot(a, b, c) -> complex` | `func` | `complex` | `{ d := b^2 - 4a * c if (d >= 0) { complex((-b + √d) / (2a), 0) } else { compl...` | `quad_croot(1, 2, 5)` | `-1 + 2i` |
| `poly_val(vec, real)` | `poly_val(coeffs:vec, x)` | `func` | `real` | `{ n   := coeffs.len() acc := 0 idx := n - 1 (idx >= 0) ?? { acc = acc * x + c...` | `poly_val(vec(2, 4, 6), 3)` | `68` |
| `poly_deriv(vec)` | `poly_deriv(coeffs:vec) -> vec` | `func` | `vec` | `{ n   := coeffs.len() out := vec<real>() if (n <= 1) { 0 into out } else { id...` | `poly_deriv(vec(2, 4, 6))` | `[4, 12]` |
| `poly_integ(vec, real)` | `poly_integ(coeffs:vec, c0 = 0) -> vec` | `func` | `vec` | `{ n   := coeffs.len() out := vec<real>() c0 into out idx := 0 (idx < n) ?? { ...` | `poly_integ(vec(2, 4, 6), 3)` | `[3, 2, 2, 2]` |
| `poly_integ(vec)` | `poly_integ(coeffs:vec, c0 = 0) -> vec [default-arg overload]` | `func` | `vec` | `{ n   := coeffs.len() out := vec<real>() c0 into out idx := 0 (idx < n) ?? { ...` | `poly_integ(vec(2, 4, 6))` | `[0, 2, 2, 2]` |
| `poly_add(vec, vec)` | `poly_add(p:vec, q:vec) -> vec` | `func` | `vec` | `{ np  := p.len() nq  := q.len() n   := max(np, nq) out := vec<real>() idx := ...` | `poly_add(vec(2, 4, 6), vec(1, 2, 3))` | `[3, 6, 9]` |
| `poly_mul(vec, vec)` | `poly_mul(p:vec, q:vec) -> vec` | `func` | `vec` | `{ np  := p.len() nq  := q.len() deg := np + nq - 1 out := vec<real>(deg, 0) i...` | `poly_mul(vec(2, 4, 6), vec(1, 2, 3))` | `[2, 8, 20, 24, 18]` |
| `poly_newton(vec, real, real)` | `poly_newton(coeffs:vec, x0, steps = 12)` | `func` | `real` | `{ dp := poly_deriv(coeffs) x  := x0 k  := 0 (k < steps) ?? { fx  := poly_val(...` | `poly_newton(vec(-4, 0, 1), 3, 20)` | `2` |
| `poly_newton(vec, real)` | `poly_newton(coeffs:vec, x0, steps = 12) [default-arg overload]` | `func` | `real` | `{ dp := poly_deriv(coeffs) x  := x0 k  := 0 (k < steps) ?? { fx  := poly_val(...` | `poly_newton(vec(-4, 0, 1), 3)` | `2` |

### 10.12 Module `[calculus]` (`Library/calculus.math` — 26 overloads)

Exact polynomial calculus (`poly_eval`, `poly_diff`, `poly_diff_at`, `poly_diff2_at`, `poly_def_integ`), analytic geometry (`tangent_slope`, `normal_slope`, `tangent_y`, `normal_y`, `poly_curvature`, `quad_critical_x`, `quad_extremum_y`, `poly_newton_root`), integral applications (`poly_avg_val`, `poly_arc_length`, `poly_vol_disk`, `poly_vol_shell`), discrete dataset quadrature (`trapz_vec`, `simpson_vec`), and Maclaurin series (`taylor_exp`, `taylor_sin`, `taylor_cos`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `poly_eval(vec, real)` | `poly_eval(c:vec, x)` | `func` | `real` | `{ n   := c.len() acc := 0 k   := n - 1 (k >= 0) ?? { acc = acc * x + c[k] k  ...` | `poly_eval(vec(2, 4, 6), 3)` | `68` |
| `poly_diff(vec)` | `poly_diff(c:vec) -> vec` | `func` | `vec` | `{ n   := c.len() out := vec<real>() if (n <= 1) { 0 into out } else { k := 1 ...` | `poly_diff(vec(2, 4, 6))` | `[4, 12]` |
| `poly_diff_at(vec, real)` | `poly_diff_at(c:vec, x0)` | `func` | `real` | `{ dc := poly_diff(c) poly_eval(dc, x0) }` | `poly_diff_at(vec(2, 4, 6), 3)` | `40` |
| `poly_diff2_at(vec, real)` | `poly_diff2_at(c:vec, x0)` | `func` | `real` | `{ d2c := poly_diff(poly_diff(c)) poly_eval(d2c, x0) }` | `poly_diff2_at(vec(2, 4, 6), 3)` | `12` |
| `poly_def_integ(vec, real, real)` | `poly_def_integ(c:vec, a, b)` | `func` | `real` | `{ ic := poly_integ(c, 0) poly_eval(ic, b) - poly_eval(ic, a) }` | `poly_def_integ(vec(2, 4, 6), 3, 2)` | `-50` |
| `tangent_slope(vec, real)` | `tangent_slope(c:vec, x0)` | `func` | `real` | `poly_diff_at(c, x0)` | `tangent_slope(vec(2, 4, 6), 3)` | `40` |
| `normal_slope(vec, real)` | `normal_slope(c:vec, x0)` | `func` | `real` | `-1 / poly_diff_at(c, x0)` | `normal_slope(vec(2, 4, 6), 3)` | `-0.025` |
| `tangent_y(vec, real, real)` | `tangent_y(c:vec, x0, x)` | `func` | `real` | `poly_eval(c, x0) + poly_diff_at(c, x0) * (x - x0)` | `tangent_y(vec(2, 4, 6), 3, 2)` | `28` |
| `normal_y(vec, real, real)` | `normal_y(c:vec, x0, x)` | `func` | `real` | `poly_eval(c, x0) + (-1 / poly_diff_at(c, x0)) * (x - x0)` | `normal_y(vec(2, 4, 6), 3, 2)` | `68.025` |
| `poly_curvature(vec, real)` | `poly_curvature(c:vec, x0)` | `func` | `real` | `{ dy  := poly_diff_at(c, x0) d2y := poly_diff2_at(c, x0) @abs(d2y) / ((1 + dy...` | `poly_curvature(vec(2, 4, 6), 3)` | `0.000187324356` |
| `quad_critical_x(real, real, real)` | `quad_critical_x(a, b, c_term = 0)` | `func` | `real` | `-b / (2a) + 0 * c_term` | `quad_critical_x(6, 3, 2)` | `-0.25` |
| `quad_critical_x(real, real)` | `quad_critical_x(a, b, c_term = 0) [default-arg overload]` | `func` | `real` | `-b / (2a) + 0 * c_term` | `quad_critical_x(6, 3)` | `-0.25` |
| `quad_extremum_y(real, real, real)` | `quad_extremum_y(a, b, c_term)` | `func` | `real` | `c_term - (b^2) / (4a)` | `quad_extremum_y(6, 3, 2)` | `1.625` |
| `poly_newton_root(vec, real)` | `poly_newton_root(c:vec, x_init)` | `func` | `real` | `{ xr   := x_init iter := 0 (iter < 35) ?? { fx  := poly_eval(c, xr) dfx := po...` | `poly_newton_root(vec(-4, 0, 1), 3)` | `2` |
| `poly_avg_val(vec, real, real)` | `poly_avg_val(c:vec, a, b)` | `func` | `real` | `poly_def_integ(c, a, b) / (b - a)` | `poly_avg_val(vec(2, 4, 6), 3, 2)` | `50` |
| `poly_arc_length(vec, real, real)` | `poly_arc_length(c:vec, a, b)` | `func` | `real` | `integrate(t_var, √(1 + (poly_diff_at(c, t_var) ^ 2)), a, b)` | `poly_arc_length(vec(2, 4, 6), 3, 2)` | `-34.01485807` |
| `poly_vol_disk(vec, real, real)` | `poly_vol_disk(c:vec, a, b)` | `func` | `real` | `π * integrate(t_var, poly_eval(c, t_var) ^ 2, a, b)` | `poly_vol_disk(vec(2, 4, 6), 3, 2)` | `-8157.250045` |
| `poly_vol_shell(vec, real, real)` | `poly_vol_shell(c:vec, a, b)` | `func` | `real` | `2π * integrate(t_var, t_var * @abs(poly_eval(c, t_var)), a, b)` | `poly_vol_shell(vec(2, 4, 6), 3, 2)` | `-803.2005218` |
| `trapz_vec(vec, real)` | `trapz_vec(y:vec, dx)` | `func` | `real` | `{ n   := y.len() acc := 0.5 * (y[0] + y[n - 1]) k   := 1 (k < n - 1) ?? { acc...` | `trapz_vec(vec(2, 4, 6), 3)` | `24` |
| `simpson_vec(vec, real)` | `simpson_vec(y:vec, dx)` | `func` | `real` | `{ n   := y.len() acc := y[0] + y[n - 1] k   := 1 (k < n - 1) ?? { w   := (k %...` | `simpson_vec(vec(0, 1, 4, 9, 16), 1)` | `21.33333333` |
| `taylor_exp(real, real)` | `taylor_exp(x, n_terms = 12)` | `func` | `real` | `sigma(k_idx, (x ^ k_idx) / (k_idx!), 0, n_terms - 1)` | `taylor_exp(6, 3)` | `25` |
| `taylor_exp(real)` | `taylor_exp(x, n_terms = 12) [default-arg overload]` | `func` | `real` | `sigma(k_idx, (x ^ k_idx) / (k_idx!), 0, n_terms - 1)` | `taylor_exp(6)` | `395.3231169` |
| `taylor_sin(real, real)` | `taylor_sin(x, n_terms = 10)` | `func` | `real` | `sigma(k_idx, ((-1) ^ k_idx) * (x ^ (2k_idx + 1)) / ((2k_idx + 1)!), 0, n_term...` | `taylor_sin(6, 3)` | `34.8` |
| `taylor_sin(real)` | `taylor_sin(x, n_terms = 10) [default-arg overload]` | `func` | `real` | `sigma(k_idx, ((-1) ^ k_idx) * (x ^ (2k_idx + 1)) / ((2k_idx + 1)!), 0, n_term...` | `taylor_sin(6)` | `-0.2798160636` |
| `taylor_cos(real, real)` | `taylor_cos(x, n_terms = 10)` | `func` | `real` | `sigma(k_idx, ((-1) ^ k_idx) * (x ^ (2k_idx)) / ((2k_idx)!), 0, n_terms - 1)` | `taylor_cos(6, 3)` | `37` |
| `taylor_cos(real)` | `taylor_cos(x, n_terms = 10) [default-arg overload]` | `func` | `real` | `sigma(k_idx, ((-1) ^ k_idx) * (x ^ (2k_idx)) / ((2k_idx)!), 0, n_terms - 1)` | `taylor_cos(6)` | `0.9587773571` |

### 10.13 Module `[units]` (`Library/units.math` — 169 overloads)

32 SI 7D dimensional `@dim` declarations, 1 non-SI `angle` group, 159 postfix unit suffixes (precedence 95, normalizing to SI base units), and unit helper functions (`deg2rad`, `rad2deg`, `m_to_km`, `m_to_ft`, `m_to_mi`, `ft_to_m`, `in_to_cm`, `cm_to_in`, `kg_to_lb`, `lb_to_kg`, `mps_to_kph`, `mps_to_mph`, `c_to_k`, `k_to_c`, `c_to_f`, `f_to_c`, `f_to_k`, `k_to_f`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `(real) deg` | `(d) deg    :95 -> real, angle` | `postfix :95` | `real[angle]` | `d * π / 180` | `6 deg` | `0.1047197551 [angle]` |
| `(real) °` | `(d) "°"    :95 -> real, angle` | `postfix :95` | `real[angle]` | `d * π / 180` | `6°` | `0.1047197551 [angle]` |
| `(real) rad` | `(r) rad    :95 -> real, angle` | `postfix :95` | `real[angle]` | `r` | `6 rad` | `6 [angle]` |
| `(real) arcmin` | `(m) arcmin :95 -> real, angle` | `postfix :95` | `real[angle]` | `m * π / (180 * 60)` | `6 arcmin` | `0.001745329252 [angle]` |
| `(real) arcsec` | `(s) arcsec :95 -> real, angle` | `postfix :95` | `real[angle]` | `s * π / (180 * 3600)` | `6 arcsec` | `2.908882087e-05 [angle]` |
| `(real) pct` | `(x) pct    :95` | `postfix :95` | `real` | `x * 0.01` | `6 pct` | `0.06` |
| `(real) dB` | `(x) dB     :95` | `postfix :95` | `real` | `10 ^ (x / 20)` | `6 dB` | `1.995262315` |
| `(real) dBm` | `(x) dBm    :95` | `postfix :95` | `real` | `0.001 * (10 ^ (x / 10))` | `6 dBm` | `0.003981071706` |
| `deg2rad(real)` | `deg2rad(deg) -> real, angle` | `func` | `real[angle]` | `deg * π / 180` | `deg2rad(6)` | `0.1047197551 [angle]` |
| `rad2deg(real)` | `rad2deg(rad)` | `func` | `real` | `rad * 180 / π` | `rad2deg(6)` | `343.7746771` |
| `(real) m` | `(x)      m    :95 -> real, distance` | `postfix :95` | `real[distance]` | `x` | `6 m` | `6 [distance]` |
| `(iota) m` | `(x:iota) m    :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@raw(x)` | `4i m` | `4i [distance]` |
| `(real) km` | `(x)      km   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 1000` | `6 km` | `6000 [distance]` |
| `(iota) km` | `(x:iota) km   :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@mul(x, 1000)` | `4i km` | `4000i [distance]` |
| `(real) cm` | `(x)      cm   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 0.01` | `6 cm` | `0.06 [distance]` |
| `(iota) cm` | `(x:iota) cm   :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@mul(x, 0.01)` | `4i cm` | `0.04i [distance]` |
| `(real) mm` | `(x)      mm   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 0.001` | `6 mm` | `0.006 [distance]` |
| `(iota) mm` | `(x:iota) mm   :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@mul(x, 0.001)` | `4i mm` | `0.004i [distance]` |
| `(real) im` | `(x)      im   :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@raw(x)` | `6 im` | `6i [distance]` |
| `(real) ikm` | `(x)      ikm  :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@mul(x, 1000)` | `6 ikm` | `6000i [distance]` |
| `(real) icm` | `(x)      icm  :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@mul(x, 0.01)` | `6 icm` | `0.06i [distance]` |
| `(real) imm` | `(x)      imm  :95 -> iota, distance` | `postfix :95` | `iota[distance]` | `@mul(x, 0.001)` | `6 imm` | `0.006i [distance]` |
| `(real) um` | `(x)      um   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * (10 ^ -6)` | `6 um` | `6e-06 [distance]` |
| `(real) nm` | `(x)      nm   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * (10 ^ -9)` | `6 nm` | `6e-09 [distance]` |
| `(real) mi` | `(x)      mi   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 1609.344` | `6 mi` | `9656.064 [distance]` |
| `(real) nmi` | `(x)      nmi  :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 1852` | `6 nmi` | `11112 [distance]` |
| `(real) yd` | `(x)      yd   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 0.9144` | `6 yd` | `5.4864 [distance]` |
| `(real) ft` | `(x)      ft   :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 0.3048` | `6 ft` | `1.8288 [distance]` |
| `(real) inch` | `(x)      inch :95 -> real, distance` | `postfix :95` | `real[distance]` | `x * 0.0254` | `6 inch` | `0.1524 [distance]` |
| `m_to_km(real)` | `m_to_km(m)` | `func` | `real` | `m / 1000` | `m_to_km(6)` | `0.006` |
| `m_to_ft(real)` | `m_to_ft(m)` | `func` | `real` | `m / 0.3048` | `m_to_ft(6)` | `19.68503937` |
| `m_to_mi(real)` | `m_to_mi(m)` | `func` | `real` | `m / 1609.344` | `m_to_mi(6)` | `0.003728227153` |
| `ft_to_m(real)` | `ft_to_m(ft_v) -> real, distance` | `func` | `real[distance]` | `ft_v * 0.3048` | `ft_to_m(6)` | `1.8288 [distance]` |
| `in_to_cm(real)` | `in_to_cm(in_v)` | `func` | `real` | `in_v * 2.54` | `in_to_cm(6)` | `15.24` |
| `cm_to_in(real)` | `cm_to_in(cm_v)` | `func` | `real` | `cm_v / 2.54` | `cm_to_in(6)` | `2.362204724` |
| `(real) m2` | `(x) m2   :95 -> real, area` | `postfix :95` | `real[area]` | `x` | `6 m2` | `6 [area]` |
| `(real) km2` | `(x) km2  :95 -> real, area` | `postfix :95` | `real[area]` | `x * 1000000` | `6 km2` | `6000000 [area]` |
| `(real) cm2` | `(x) cm2  :95 -> real, area` | `postfix :95` | `real[area]` | `x * 0.0001` | `6 cm2` | `0.0006 [area]` |
| `(real) mm2` | `(x) mm2  :95 -> real, area` | `postfix :95` | `real[area]` | `x * (10 ^ -6)` | `6 mm2` | `6e-06 [area]` |
| `(real) ha` | `(x) ha   :95 -> real, area` | `postfix :95` | `real[area]` | `x * 10000` | `6 ha` | `60000 [area]` |
| `(real) acre` | `(x) acre :95 -> real, area` | `postfix :95` | `real[area]` | `x * 4046.8564224` | `6 acre` | `24281.13853 [area]` |
| `(real) yd2` | `(x) yd2  :95 -> real, area` | `postfix :95` | `real[area]` | `x * 0.83612736` | `6 yd2` | `5.01676416 [area]` |
| `(real) ft2` | `(x) ft2  :95 -> real, area` | `postfix :95` | `real[area]` | `x * 0.09290304` | `6 ft2` | `0.55741824 [area]` |
| `(real) in2` | `(x) in2  :95 -> real, area` | `postfix :95` | `real[area]` | `x * 0.00064516` | `6 in2` | `0.00387096 [area]` |
| `(real) m3` | `(x) m3    :95 -> real, volume` | `postfix :95` | `real[volume]` | `x` | `6 m3` | `6 [volume]` |
| `(real) cm3` | `(x) cm3   :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * (10 ^ -6)` | `6 cm3` | `6e-06 [volume]` |
| `(real) mm3` | `(x) mm3   :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * (10 ^ -9)` | `6 mm3` | `6e-09 [volume]` |
| `(real) L` | `(x) L     :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * 0.001` | `6 L` | `0.006 [volume]` |
| `(real) liter` | `(x) liter :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * 0.001` | `6 liter` | `0.006 [volume]` |
| `(real) mL` | `(x) mL    :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * (10 ^ -6)` | `6 mL` | `6e-06 [volume]` |
| `(real) gal` | `(x) gal   :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * 0.003785411784` | `6 gal` | `0.0227124707 [volume]` |
| `(real) ft3` | `(x) ft3   :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * 0.028316846592` | `6 ft3` | `0.1699010796 [volume]` |
| `(real) in3` | `(x) in3   :95 -> real, volume` | `postfix :95` | `real[volume]` | `x * 0.000016387064` | `6 in3` | `9.8322384e-05 [volume]` |
| `(real) kg` | `(x) kg   :95 -> real, mass` | `postfix :95` | `real[mass]` | `x` | `6 kg` | `6 [mass]` |
| `(real) gram` | `(x) gram :95 -> real, mass` | `postfix :95` | `real[mass]` | `x * 0.001` | `6 gram` | `0.006 [mass]` |
| `(real) mg` | `(x) mg   :95 -> real, mass` | `postfix :95` | `real[mass]` | `x * (10 ^ -6)` | `6 mg` | `6e-06 [mass]` |
| `(real) ton` | `(x) ton  :95 -> real, mass` | `postfix :95` | `real[mass]` | `x * 1000` | `6 ton` | `6000 [mass]` |
| `(real) lb` | `(x) lb   :95 -> real, mass` | `postfix :95` | `real[mass]` | `x * 0.45359237` | `6 lb` | `2.72155422 [mass]` |
| `(real) oz` | `(x) oz   :95 -> real, mass` | `postfix :95` | `real[mass]` | `x * 0.028349523125` | `6 oz` | `0.1700971388 [mass]` |
| `(real) amu` | `(x) amu  :95 -> real, mass` | `postfix :95` | `real[mass]` | `x * 1.66053906660 * (10 ^ -27)` | `6 amu` | `9.9632344e-27 [mass]` |
| `(real) kgm3` | `(x) kgm3 :95 -> real, density` | `postfix :95` | `real[density]` | `x` | `6 kgm3` | `6 [density]` |
| `(real) gcm3` | `(x) gcm3 :95 -> real, density` | `postfix :95` | `real[density]` | `x * 1000` | `6 gcm3` | `6000 [density]` |
| `kg_to_lb(real)` | `kg_to_lb(kg_v)` | `func` | `real` | `kg_v / 0.45359237` | `kg_to_lb(6)` | `13.22773573` |
| `lb_to_kg(real)` | `lb_to_kg(lb_v) -> real, mass` | `func` | `real[mass]` | `lb_v * 0.45359237` | `lb_to_kg(6)` | `2.72155422 [mass]` |
| `(real) ms` | `(x) ms    :95 -> real, time` | `postfix :95` | `real[time]` | `x * 0.001` | `6 ms` | `0.006 [time]` |
| `(real) us` | `(x) us    :95 -> real, time` | `postfix :95` | `real[time]` | `x * (10 ^ -6)` | `6 us` | `6e-06 [time]` |
| `(real) ns` | `(x) ns    :95 -> real, time` | `postfix :95` | `real[time]` | `x * (10 ^ -9)` | `6 ns` | `6e-09 [time]` |
| `(real) sec` | `(x) sec   :95 -> real, time` | `postfix :95` | `real[time]` | `x` | `6 sec` | `6 [time]` |
| `(real) min_t` | `(x) min_t :95 -> real, time` | `postfix :95` | `real[time]` | `x * 60` | `6 min_t` | `360 [time]` |
| `(real) hr` | `(x) hr    :95 -> real, time` | `postfix :95` | `real[time]` | `x * 3600` | `6 hr` | `21600 [time]` |
| `(real) day` | `(x) day   :95 -> real, time` | `postfix :95` | `real[time]` | `x * 86400` | `6 day` | `518400 [time]` |
| `(real) sec2` | `(x) sec2  :95 -> real, time2` | `postfix :95` | `real[time2]` | `x` | `6 sec2` | `6 [time2]` |
| `(real) Hz` | `(x) Hz    :95 -> real, frequency` | `postfix :95` | `real[frequency]` | `x` | `6 Hz` | `6 [frequency]` |
| `(real) kHz` | `(x) kHz   :95 -> real, frequency` | `postfix :95` | `real[frequency]` | `x * 1000` | `6 kHz` | `6000 [frequency]` |
| `(real) MHz` | `(x) MHz   :95 -> real, frequency` | `postfix :95` | `real[frequency]` | `x * 1000000` | `6 MHz` | `6000000 [frequency]` |
| `(real) GHz` | `(x) GHz   :95 -> real, frequency` | `postfix :95` | `real[frequency]` | `x * (10 ^ 9)` | `6 GHz` | `6000000000 [frequency]` |
| `(real) rpm` | `(x) rpm   :95 -> real, frequency` | `postfix :95` | `real[frequency]` | `x / 60` | `6 rpm` | `0.1 [frequency]` |
| `(real) mps` | `(x) mps   :95 -> real, speed` | `postfix :95` | `real[speed]` | `x` | `6 mps` | `6 [speed]` |
| `(real) kph` | `(x) kph   :95 -> real, speed` | `postfix :95` | `real[speed]` | `x / 3.6` | `6 kph` | `1.666666667 [speed]` |
| `(real) mph` | `(x) mph   :95 -> real, speed` | `postfix :95` | `real[speed]` | `x * 0.44704` | `6 mph` | `2.68224 [speed]` |
| `(real) knots` | `(x) knots :95 -> real, speed` | `postfix :95` | `real[speed]` | `x * (1852 / 3600)` | `6 knots` | `3.086666667 [speed]` |
| `(real) mps2` | `(x) mps2  :95 -> real, acceleration` | `postfix :95` | `real[acceleration]` | `x` | `6 mps2` | `6 [acceleration]` |
| `(real) g_acc` | `(x) g_acc :95 -> real, acceleration` | `postfix :95` | `real[acceleration]` | `x * 9.80665` | `6 g_acc` | `58.8399 [acceleration]` |
| `(real) cmps2` | `(x) cmps2 :95 -> real, acceleration` | `postfix :95` | `real[acceleration]` | `x * 0.01` | `6 cmps2` | `0.06 [acceleration]` |
| `(real) fps2` | `(x) fps2  :95 -> real, acceleration` | `postfix :95` | `real[acceleration]` | `x * 0.3048` | `6 fps2` | `1.8288 [acceleration]` |
| `(real) Jpkg` | `(x) Jpkg  :95 -> real, specific_energy` | `postfix :95` | `real[specific_energy]` | `x` | `6 Jpkg` | `6 [specific_energy]` |
| `mps_to_kph(real)` | `mps_to_kph(v)` | `func` | `real` | `v * 3.6` | `mps_to_kph(6)` | `21.6` |
| `mps_to_mph(real)` | `mps_to_mph(v)` | `func` | `real` | `v / 0.44704` | `mps_to_mph(6)` | `13.42161775` |
| `(real) N` | `(x) N     :95 -> real, force` | `postfix :95` | `real[force]` | `x` | `6 N` | `6 [force]` |
| `(real) kN` | `(x) kN    :95 -> real, force` | `postfix :95` | `real[force]` | `x * 1000` | `6 kN` | `6000 [force]` |
| `(real) MN` | `(x) MN    :95 -> real, force` | `postfix :95` | `real[force]` | `x * 1000000` | `6 MN` | `6000000 [force]` |
| `(real) mN` | `(x) mN    :95 -> real, force` | `postfix :95` | `real[force]` | `x * 0.001` | `6 mN` | `0.006 [force]` |
| `(real) dyn` | `(x) dyn   :95 -> real, force` | `postfix :95` | `real[force]` | `x * 0.00001` | `6 dyn` | `6e-05 [force]` |
| `(real) lbf` | `(x) lbf   :95 -> real, force` | `postfix :95` | `real[force]` | `x * 4.4482216152605` | `6 lbf` | `26.68932969 [force]` |
| `(real) kgf` | `(x) kgf   :95 -> real, force` | `postfix :95` | `real[force]` | `x * 9.80665` | `6 kgf` | `58.8399 [force]` |
| `(real) Ns` | `(x) Ns    :95 -> real, momentum` | `postfix :95` | `real[momentum]` | `x` | `6 Ns` | `6 [momentum]` |
| `(real) kgmps` | `(x) kgmps :95 -> real, momentum` | `postfix :95` | `real[momentum]` | `x` | `6 kgmps` | `6 [momentum]` |
| `(real) Pa` | `(x) Pa   :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x` | `6 Pa` | `6 [pressure]` |
| `(real) kPa` | `(x) kPa  :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x * 1000` | `6 kPa` | `6000 [pressure]` |
| `(real) MPa` | `(x) MPa  :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x * 1000000` | `6 MPa` | `6000000 [pressure]` |
| `(real) bar` | `(x) bar  :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x * 100000` | `6 bar` | `600000 [pressure]` |
| `(real) atm` | `(x) atm  :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x * 101325` | `6 atm` | `607950 [pressure]` |
| `(real) psi` | `(x) psi  :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x * 6894.757293168` | `6 psi` | `41368.54376 [pressure]` |
| `(real) torr` | `(x) torr :95 -> real, pressure` | `postfix :95` | `real[pressure]` | `x * (101325 / 760)` | `6 torr` | `799.9342105 [pressure]` |
| `(real) J` | `(x) J    :95 -> real, energy` | `postfix :95` | `real[energy]` | `x` | `6 J` | `6 [energy]` |
| `(real) kJ` | `(x) kJ   :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 1000` | `6 kJ` | `6000 [energy]` |
| `(real) MJ` | `(x) MJ   :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 1000000` | `6 MJ` | `6000000 [energy]` |
| `(real) cal` | `(x) cal  :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 4.184` | `6 cal` | `25.104 [energy]` |
| `(real) kcal` | `(x) kcal :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 4184` | `6 kcal` | `25104 [energy]` |
| `(real) Wh` | `(x) Wh   :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 3600` | `6 Wh` | `21600 [energy]` |
| `(real) kWh` | `(x) kWh  :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 3600000` | `6 kWh` | `21600000 [energy]` |
| `(real) eV` | `(x) eV   :95 -> real, energy` | `postfix :95` | `real[energy]` | `x * 1.602176634 * (10 ^ -19)` | `6 eV` | `9.613059804e-19 [energy]` |
| `(real) W` | `(x) W    :95 -> real, power` | `postfix :95` | `real[power]` | `x` | `6 W` | `6 [power]` |
| `(real) kW` | `(x) kW   :95 -> real, power` | `postfix :95` | `real[power]` | `x * 1000` | `6 kW` | `6000 [power]` |
| `(real) MW` | `(x) MW   :95 -> real, power` | `postfix :95` | `real[power]` | `x * 1000000` | `6 MW` | `6000000 [power]` |
| `(real) hp` | `(x) hp   :95 -> real, power` | `postfix :95` | `real[power]` | `x * 745.699872` | `6 hp` | `4474.199232 [power]` |
| `(real) A` | `(x) A     :95 -> real, current` | `postfix :95` | `real[current]` | `x` | `6 A` | `6 [current]` |
| `(real) mA` | `(x) mA    :95 -> real, current` | `postfix :95` | `real[current]` | `x * 0.001` | `6 mA` | `0.006 [current]` |
| `(real) uA` | `(x) uA    :95 -> real, current` | `postfix :95` | `real[current]` | `x * (10 ^ -6)` | `6 uA` | `6e-06 [current]` |
| `(real) kA` | `(x) kA    :95 -> real, current` | `postfix :95` | `real[current]` | `x * 1000` | `6 kA` | `6000 [current]` |
| `(real) C` | `(x) C     :95 -> real, charge` | `postfix :95` | `real[charge]` | `x` | `6 C` | `6 [charge]` |
| `(real) mC` | `(x) mC    :95 -> real, charge` | `postfix :95` | `real[charge]` | `x * 0.001` | `6 mC` | `0.006 [charge]` |
| `(real) uC` | `(x) uC    :95 -> real, charge` | `postfix :95` | `real[charge]` | `x * (10 ^ -6)` | `6 uC` | `6e-06 [charge]` |
| `(real) Ah` | `(x) Ah    :95 -> real, charge` | `postfix :95` | `real[charge]` | `x * 3600` | `6 Ah` | `21600 [charge]` |
| `(real) mAh` | `(x) mAh   :95 -> real, charge` | `postfix :95` | `real[charge]` | `x * 3.6` | `6 mAh` | `21.6 [charge]` |
| `(real) V` | `(x) V     :95 -> real, voltage` | `postfix :95` | `real[voltage]` | `x` | `6 V` | `6 [voltage]` |
| `(real) mV` | `(x) mV    :95 -> real, voltage` | `postfix :95` | `real[voltage]` | `x * 0.001` | `6 mV` | `0.006 [voltage]` |
| `(real) kV` | `(x) kV    :95 -> real, voltage` | `postfix :95` | `real[voltage]` | `x * 1000` | `6 kV` | `6000 [voltage]` |
| `(real) ohm` | `(x) ohm   :95 -> real, resistance` | `postfix :95` | `real[resistance]` | `x` | `6 ohm` | `6 [resistance]` |
| `(real) kohm` | `(x) kohm  :95 -> real, resistance` | `postfix :95` | `real[resistance]` | `x * 1000` | `6 kohm` | `6000 [resistance]` |
| `(real) Mohm` | `(x) Mohm  :95 -> real, resistance` | `postfix :95` | `real[resistance]` | `x * 1000000` | `6 Mohm` | `6000000 [resistance]` |
| `(real) F_cap` | `(x) F_cap :95 -> real, capacitance` | `postfix :95` | `real[capacitance]` | `x` | `6 F_cap` | `6 [capacitance]` |
| `(real) mF` | `(x) mF    :95 -> real, capacitance` | `postfix :95` | `real[capacitance]` | `x * 0.001` | `6 mF` | `0.006 [capacitance]` |
| `(real) uF` | `(x) uF    :95 -> real, capacitance` | `postfix :95` | `real[capacitance]` | `x * (10 ^ -6)` | `6 uF` | `6e-06 [capacitance]` |
| `(real) nF` | `(x) nF    :95 -> real, capacitance` | `postfix :95` | `real[capacitance]` | `x * (10 ^ -9)` | `6 nF` | `6e-09 [capacitance]` |
| `(real) pF` | `(x) pF    :95 -> real, capacitance` | `postfix :95` | `real[capacitance]` | `x * (10 ^ -12)` | `6 pF` | `6e-12 [capacitance]` |
| `(real) K` | `(x) K      :95 -> real, temperature` | `postfix :95` | `real[temperature]` | `x` | `6 K` | `6 [temperature]` |
| `(real) kelvin` | `(x) kelvin :95 -> real, temperature` | `postfix :95` | `real[temperature]` | `x` | `6 kelvin` | `6 [temperature]` |
| `(real) mK` | `(x) mK     :95 -> real, temperature` | `postfix :95` | `real[temperature]` | `x * 0.001` | `6 mK` | `0.006 [temperature]` |
| `(real) degC` | `(x) degC   :95 -> real, temperature` | `postfix :95` | `real[temperature]` | `x + 273.15` | `6 degC` | `279.15 [temperature]` |
| `(real) degF` | `(x) degF   :95 -> real, temperature` | `postfix :95` | `real[temperature]` | `(x - 32) / 1.8 + 273.15` | `6 degF` | `258.7055556 [temperature]` |
| `(real) JpK` | `(x) JpK    :95 -> real, heat_capacity` | `postfix :95` | `real[heat_capacity]` | `x` | `6 JpK` | `6 [heat_capacity]` |
| `(real) JpkgK` | `(x) JpkgK  :95 -> real, specific_heat` | `postfix :95` | `real[specific_heat]` | `x` | `6 JpkgK` | `6 [specific_heat]` |
| `c_to_k(real)` | `c_to_k(c)` | `func` | `real` | `c + 273.15` | `c_to_k(6)` | `279.15` |
| `k_to_c(real)` | `k_to_c(k)` | `func` | `real` | `k - 273.15` | `k_to_c(6)` | `-267.15` |
| `c_to_f(real)` | `c_to_f(c)` | `func` | `real` | `c * 1.8 + 32` | `c_to_f(6)` | `42.8` |
| `f_to_c(real)` | `f_to_c(f)` | `func` | `real` | `(f - 32) / 1.8` | `f_to_c(6)` | `-14.44444444` |
| `f_to_k(real)` | `f_to_k(f)` | `func` | `real` | `(f - 32) / 1.8 + 273.15` | `f_to_k(6)` | `258.7055556` |
| `k_to_f(real)` | `k_to_f(k)` | `func` | `real` | `(k - 273.15) * 1.8 + 32` | `k_to_f(6)` | `-448.87` |
| `(real) mol` | `(x) mol     :95 -> real, substance` | `postfix :95` | `real[substance]` | `x` | `6 mol` | `6 [substance]` |
| `(real) mmol` | `(x) mmol    :95 -> real, substance` | `postfix :95` | `real[substance]` | `x * 0.001` | `6 mmol` | `0.006 [substance]` |
| `(real) umol` | `(x) umol    :95 -> real, substance` | `postfix :95` | `real[substance]` | `x * 1e-6` | `6 umol` | `6e-06 [substance]` |
| `(real) nmol` | `(x) nmol    :95 -> real, substance` | `postfix :95` | `real[substance]` | `x * 1e-9` | `6 nmol` | `6e-09 [substance]` |
| `(real) kmol` | `(x) kmol    :95 -> real, substance` | `postfix :95` | `real[substance]` | `x * 1000` | `6 kmol` | `6000 [substance]` |
| `(real) kgpmol` | `(x) kgpmol  :95 -> real, molar_mass` | `postfix :95` | `real[molar_mass]` | `x` | `6 kgpmol` | `6 [molar_mass]` |
| `(real) gpmol` | `(x) gpmol   :95 -> real, molar_mass` | `postfix :95` | `real[molar_mass]` | `x * 0.001` | `6 gpmol` | `0.006 [molar_mass]` |
| `(real) molm3` | `(x) molm3   :95 -> real, molarity` | `postfix :95` | `real[molarity]` | `x` | `6 molm3` | `6 [molarity]` |
| `(real) M_conc` | `(x) M_conc  :95 -> real, molarity` | `postfix :95` | `real[molarity]` | `x * 1000` | `6 M_conc` | `6000 [molarity]` |
| `(real) mM_conc` | `(x) mM_conc :95 -> real, molarity` | `postfix :95` | `real[molarity]` | `x` | `6 mM_conc` | `6 [molarity]` |
| `(real) Jpmol` | `(x) Jpmol   :95 -> real, molar_energy` | `postfix :95` | `real[molar_energy]` | `x` | `6 Jpmol` | `6 [molar_energy]` |
| `(real) kJpmol` | `(x) kJpmol  :95 -> real, molar_energy` | `postfix :95` | `real[molar_energy]` | `x * 1000` | `6 kJpmol` | `6000 [molar_energy]` |
| `(real) JpmolK` | `(x) JpmolK  :95 -> real, molar_heat_capacity` | `postfix :95` | `real[molar_heat_capacity]` | `x` | `6 JpmolK` | `6 [molar_heat_capacity]` |
| `(real) kat` | `(x) kat     :95 -> real, catalytic_activity` | `postfix :95` | `real[catalytic_activity]` | `x` | `6 kat` | `6 [catalytic_activity]` |
| `(real) mkat` | `(x) mkat    :95 -> real, catalytic_activity` | `postfix :95` | `real[catalytic_activity]` | `x * 0.001` | `6 mkat` | `0.006 [catalytic_activity]` |
| `(real) ukat` | `(x) ukat    :95 -> real, catalytic_activity` | `postfix :95` | `real[catalytic_activity]` | `x * 1e-6` | `6 ukat` | `6e-06 [catalytic_activity]` |
| `(real) cd` | `(x) cd      :95 -> real, luminous_intensity` | `postfix :95` | `real[luminous_intensity]` | `x` | `6 cd` | `6 [luminous_intensity]` |
| `(real) lm` | `(x) lm      :95 -> real, luminous_intensity` | `postfix :95` | `real[luminous_intensity]` | `x` | `6 lm` | `6 [luminous_intensity]` |
| `(real) lx` | `(x) lx      :95 -> real, illuminance` | `postfix :95` | `real[illuminance]` | `x` | `6 lx` | `6 [illuminance]` |
| `(real) klx` | `(x) klx     :95 -> real, illuminance` | `postfix :95` | `real[illuminance]` | `x * 1000` | `6 klx` | `6000 [illuminance]` |

### 10.14 Module `[conversions]` (`Library/conversions.math` — 239 overloads)

Generic unit ratio/conversion operator `in` (`:<42`), 128 postfix `to_<unit>` extractors (`:90`, returning unitless `real[none]`), and 110 pairwise `<u1>_to_<u2>(x)` scalar conversion functions.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `(real) in (real)` | `(val:real, T) in (unit:real, T) :<42 -> real, T` | `infix_L :42` | `real` | `@div(val, unit)` | `25km in m` | `25000 [distance]` |
| `(real[distance]) to_km` | `(x:real, distance) to_km   :90 -> real, none` | `postfix :90` | `real` | `x / 1000` | `(100 m) to_km` | `0.1` |
| `(real[distance]) to_m` | `(x:real, distance) to_m    :90 -> real, none` | `postfix :90` | `real` | `x` | `(100 m) to_m` | `100` |
| `(real[distance]) to_cm` | `(x:real, distance) to_cm   :90 -> real, none` | `postfix :90` | `real` | `x * 100` | `(100 m) to_cm` | `10000` |
| `(real[distance]) to_mm` | `(x:real, distance) to_mm   :90 -> real, none` | `postfix :90` | `real` | `x * 1000` | `(100 m) to_mm` | `100000` |
| `(real[distance]) to_um` | `(x:real, distance) to_um   :90 -> real, none` | `postfix :90` | `real` | `x * (10 ^ 6)` | `(100 m) to_um` | `100000000` |
| `(real[distance]) to_nm` | `(x:real, distance) to_nm   :90 -> real, none` | `postfix :90` | `real` | `x * (10 ^ 9)` | `(100 m) to_nm` | `1e+11` |
| `(real[distance]) to_mi` | `(x:real, distance) to_mi   :90 -> real, none` | `postfix :90` | `real` | `x / 1609.344` | `(100 m) to_mi` | `0.06213711922` |
| `(real[distance]) to_nmi` | `(x:real, distance) to_nmi  :90 -> real, none` | `postfix :90` | `real` | `x / 1852` | `(100 m) to_nmi` | `0.05399568035` |
| `(real[distance]) to_yd` | `(x:real, distance) to_yd   :90 -> real, none` | `postfix :90` | `real` | `x / 0.9144` | `(100 m) to_yd` | `109.3613298` |
| `(real[distance]) to_ft` | `(x:real, distance) to_ft   :90 -> real, none` | `postfix :90` | `real` | `x / 0.3048` | `(100 m) to_ft` | `328.0839895` |
| `(real[distance]) to_inch` | `(x:real, distance) to_inch :90 -> real, none` | `postfix :90` | `real` | `x / 0.0254` | `(100 m) to_inch` | `3937.007874` |
| `km_to_m(real)` | `km_to_m(x)` | `func` | `real` | `x * 1000` | `km_to_m(6)` | `6000` |
| `m_to_cm(real)` | `m_to_cm(x)` | `func` | `real` | `x * 100` | `m_to_cm(6)` | `600` |
| `cm_to_m(real)` | `cm_to_m(x)` | `func` | `real` | `x * 0.01` | `cm_to_m(6)` | `0.06` |
| `m_to_mm(real)` | `m_to_mm(x)` | `func` | `real` | `x * 1000` | `m_to_mm(6)` | `6000` |
| `mm_to_m(real)` | `mm_to_m(x)` | `func` | `real` | `x * 0.001` | `mm_to_m(6)` | `0.006` |
| `m_to_um(real)` | `m_to_um(x)` | `func` | `real` | `x * (10 ^ 6)` | `m_to_um(6)` | `6000000` |
| `um_to_m(real)` | `um_to_m(x)` | `func` | `real` | `x * (10 ^ -6)` | `um_to_m(6)` | `6e-06` |
| `m_to_nm(real)` | `m_to_nm(x)` | `func` | `real` | `x * (10 ^ 9)` | `m_to_nm(6)` | `6000000000` |
| `nm_to_m(real)` | `nm_to_m(x)` | `func` | `real` | `x * (10 ^ -9)` | `nm_to_m(6)` | `6e-09` |
| `km_to_cm(real)` | `km_to_cm(x)` | `func` | `real` | `x * 100000` | `km_to_cm(6)` | `600000` |
| `cm_to_km(real)` | `cm_to_km(x)` | `func` | `real` | `x / 100000` | `cm_to_km(6)` | `6e-05` |
| `km_to_mm(real)` | `km_to_mm(x)` | `func` | `real` | `x * (10 ^ 6)` | `km_to_mm(6)` | `6000000` |
| `mm_to_km(real)` | `mm_to_km(x)` | `func` | `real` | `x * (10 ^ -6)` | `mm_to_km(6)` | `6e-06` |
| `km_to_nm(real)` | `km_to_nm(x)` | `func` | `real` | `x * (10 ^ 12)` | `km_to_nm(6)` | `6e+12` |
| `nm_to_km(real)` | `nm_to_km(x)` | `func` | `real` | `x * (10 ^ -12)` | `nm_to_km(6)` | `6e-12` |
| `cm_to_mm(real)` | `cm_to_mm(x)` | `func` | `real` | `x * 10` | `cm_to_mm(6)` | `60` |
| `mm_to_cm(real)` | `mm_to_cm(x)` | `func` | `real` | `x * 0.1` | `mm_to_cm(6)` | `0.6` |
| `mm_to_um(real)` | `mm_to_um(x)` | `func` | `real` | `x * 1000` | `mm_to_um(6)` | `6000` |
| `um_to_mm(real)` | `um_to_mm(x)` | `func` | `real` | `x * 0.001` | `um_to_mm(6)` | `0.006` |
| `um_to_nm(real)` | `um_to_nm(x)` | `func` | `real` | `x * 1000` | `um_to_nm(6)` | `6000` |
| `nm_to_um(real)` | `nm_to_um(x)` | `func` | `real` | `x * 0.001` | `nm_to_um(6)` | `0.006` |
| `mi_to_m(real)` | `mi_to_m(x)` | `func` | `real` | `x * 1609.344` | `mi_to_m(6)` | `9656.064` |
| `mi_to_km(real)` | `mi_to_km(x)` | `func` | `real` | `x * 1.609344` | `mi_to_km(6)` | `9.656064` |
| `km_to_mi(real)` | `km_to_mi(x)` | `func` | `real` | `x / 1.609344` | `km_to_mi(6)` | `3.728227153` |
| `yd_to_m(real)` | `yd_to_m(x)` | `func` | `real` | `x * 0.9144` | `yd_to_m(6)` | `5.4864` |
| `m_to_yd(real)` | `m_to_yd(x)` | `func` | `real` | `x / 0.9144` | `m_to_yd(6)` | `6.56167979` |
| `ft_to_in(real)` | `ft_to_in(x)` | `func` | `real` | `x * 12` | `ft_to_in(6)` | `72` |
| `in_to_ft(real)` | `in_to_ft(x)` | `func` | `real` | `x / 12` | `in_to_ft(6)` | `0.5` |
| `m_to_in(real)` | `m_to_in(x)` | `func` | `real` | `x / 0.0254` | `m_to_in(6)` | `236.2204724` |
| `in_to_m(real)` | `in_to_m(x)` | `func` | `real` | `x * 0.0254` | `in_to_m(6)` | `0.1524` |
| `(real[angle]) to_rad` | `(a:real, angle) to_rad    :90 -> real, none` | `postfix :90` | `real` | `a` | `(90 deg) to_rad` | `1.570796327` |
| `(real[angle]) to_deg` | `(a:real, angle) to_deg    :90 -> real, none` | `postfix :90` | `real` | `a * 180 / π` | `(90 deg) to_deg` | `90` |
| `(real[angle]) to_arcmin` | `(a:real, angle) to_arcmin :90 -> real, none` | `postfix :90` | `real` | `a * (180 * 60) / π` | `(90 deg) to_arcmin` | `5400` |
| `(real[angle]) to_arcsec` | `(a:real, angle) to_arcsec :90 -> real, none` | `postfix :90` | `real` | `a * (180 * 3600) / π` | `(90 deg) to_arcsec` | `324000` |
| `deg_to_rad(real)` | `deg_to_rad(d)` | `func` | `real` | `d * π / 180` | `deg_to_rad(6)` | `0.1047197551` |
| `rad_to_deg(real)` | `rad_to_deg(r)` | `func` | `real` | `r * 180 / π` | `rad_to_deg(6)` | `343.7746771` |
| `deg_to_arcmin(real)` | `deg_to_arcmin(d)` | `func` | `real` | `d * 60` | `deg_to_arcmin(6)` | `360` |
| `arcmin_to_deg(real)` | `arcmin_to_deg(m)` | `func` | `real` | `m / 60` | `arcmin_to_deg(6)` | `0.1` |
| `deg_to_arcsec(real)` | `deg_to_arcsec(d)` | `func` | `real` | `d * 3600` | `deg_to_arcsec(6)` | `21600` |
| `arcsec_to_deg(real)` | `arcsec_to_deg(s)` | `func` | `real` | `s / 3600` | `arcsec_to_deg(6)` | `0.001666666667` |
| `rad_to_arcsec(real)` | `rad_to_arcsec(r)` | `func` | `real` | `r * (180 * 3600) / π` | `rad_to_arcsec(6)` | `1237588.837` |
| `arcsec_to_rad(real)` | `arcsec_to_rad(s)` | `func` | `real` | `s * π / (180 * 3600)` | `arcsec_to_rad(6)` | `2.908882087e-05` |
| `(real[mass]) to_kg` | `(m:real, mass) to_kg   :90 -> real, none` | `postfix :90` | `real` | `m` | `(10 kg) to_kg` | `10` |
| `(real[mass]) to_gram` | `(m:real, mass) to_gram :90 -> real, none` | `postfix :90` | `real` | `m * 1000` | `(10 kg) to_gram` | `10000` |
| `(real[mass]) to_mg` | `(m:real, mass) to_mg   :90 -> real, none` | `postfix :90` | `real` | `m * (10 ^ 6)` | `(10 kg) to_mg` | `10000000` |
| `(real[mass]) to_ton` | `(m:real, mass) to_ton  :90 -> real, none` | `postfix :90` | `real` | `m / 1000` | `(10 kg) to_ton` | `0.01` |
| `(real[mass]) to_lb` | `(m:real, mass) to_lb   :90 -> real, none` | `postfix :90` | `real` | `m / 0.45359237` | `(10 kg) to_lb` | `22.04622622` |
| `(real[mass]) to_oz` | `(m:real, mass) to_oz   :90 -> real, none` | `postfix :90` | `real` | `m / 0.028349523125` | `(10 kg) to_oz` | `352.7396195` |
| `kg_to_g(real)` | `kg_to_g(x)` | `func` | `real` | `x * 1000` | `kg_to_g(6)` | `6000` |
| `g_to_kg(real)` | `g_to_kg(x)` | `func` | `real` | `x * 0.001` | `g_to_kg(6)` | `0.006` |
| `kg_to_mg(real)` | `kg_to_mg(x)` | `func` | `real` | `x * (10 ^ 6)` | `kg_to_mg(6)` | `6000000` |
| `mg_to_kg(real)` | `mg_to_kg(x)` | `func` | `real` | `x * (10 ^ -6)` | `mg_to_kg(6)` | `6e-06` |
| `g_to_mg(real)` | `g_to_mg(x)` | `func` | `real` | `x * 1000` | `g_to_mg(6)` | `6000` |
| `mg_to_g(real)` | `mg_to_g(x)` | `func` | `real` | `x * 0.001` | `mg_to_g(6)` | `0.006` |
| `ton_to_kg(real)` | `ton_to_kg(x)` | `func` | `real` | `x * 1000` | `ton_to_kg(6)` | `6000` |
| `kg_to_ton(real)` | `kg_to_ton(x)` | `func` | `real` | `x * 0.001` | `kg_to_ton(6)` | `0.006` |
| `lb_to_oz(real)` | `lb_to_oz(x)` | `func` | `real` | `x * 16` | `lb_to_oz(6)` | `96` |
| `oz_to_lb(real)` | `oz_to_lb(x)` | `func` | `real` | `x / 16` | `oz_to_lb(6)` | `0.375` |
| `g_to_oz(real)` | `g_to_oz(x)` | `func` | `real` | `x / 28.349523125` | `g_to_oz(6)` | `0.2116437717` |
| `oz_to_g(real)` | `oz_to_g(x)` | `func` | `real` | `x * 28.349523125` | `oz_to_g(6)` | `170.0971387` |
| `(real[time]) to_sec` | `(t:real, time) to_sec :90 -> real, none` | `postfix :90` | `real` | `t` | `(5 sec) to_sec` | `5` |
| `(real[time]) to_ms` | `(t:real, time) to_ms  :90 -> real, none` | `postfix :90` | `real` | `t * 1000` | `(5 sec) to_ms` | `5000` |
| `(real[time]) to_us` | `(t:real, time) to_us  :90 -> real, none` | `postfix :90` | `real` | `t * (10 ^ 6)` | `(5 sec) to_us` | `5000000` |
| `(real[time]) to_ns` | `(t:real, time) to_ns  :90 -> real, none` | `postfix :90` | `real` | `t * (10 ^ 9)` | `(5 sec) to_ns` | `5000000000` |
| `(real[time]) to_min` | `(t:real, time) to_min :90 -> real, none` | `postfix :90` | `real` | `t / 60` | `(5 sec) to_min` | `0.08333333333` |
| `(real[time]) to_hr` | `(t:real, time) to_hr  :90 -> real, none` | `postfix :90` | `real` | `t / 3600` | `(5 sec) to_hr` | `0.001388888889` |
| `(real[time]) to_day` | `(t:real, time) to_day :90 -> real, none` | `postfix :90` | `real` | `t / 86400` | `(5 sec) to_day` | `5.787037037e-05` |
| `hr_to_min(real)` | `hr_to_min(x)` | `func` | `real` | `x * 60` | `hr_to_min(6)` | `360` |
| `min_to_hr(real)` | `min_to_hr(x)` | `func` | `real` | `x / 60` | `min_to_hr(6)` | `0.1` |
| `hr_to_sec(real)` | `hr_to_sec(x)` | `func` | `real` | `x * 3600` | `hr_to_sec(6)` | `21600` |
| `sec_to_hr(real)` | `sec_to_hr(x)` | `func` | `real` | `x / 3600` | `sec_to_hr(6)` | `0.001666666667` |
| `min_to_sec(real)` | `min_to_sec(x)` | `func` | `real` | `x * 60` | `min_to_sec(6)` | `360` |
| `sec_to_min(real)` | `sec_to_min(x)` | `func` | `real` | `x / 60` | `sec_to_min(6)` | `0.1` |
| `sec_to_ms(real)` | `sec_to_ms(x)` | `func` | `real` | `x * 1000` | `sec_to_ms(6)` | `6000` |
| `ms_to_sec(real)` | `ms_to_sec(x)` | `func` | `real` | `x * 0.001` | `ms_to_sec(6)` | `0.006` |
| `ms_to_us(real)` | `ms_to_us(x)` | `func` | `real` | `x * 1000` | `ms_to_us(6)` | `6000` |
| `us_to_ms(real)` | `us_to_ms(x)` | `func` | `real` | `x * 0.001` | `us_to_ms(6)` | `0.006` |
| `us_to_ns(real)` | `us_to_ns(x)` | `func` | `real` | `x * 1000` | `us_to_ns(6)` | `6000` |
| `ns_to_us(real)` | `ns_to_us(x)` | `func` | `real` | `x * 0.001` | `ns_to_us(6)` | `0.006` |
| `sec_to_ns(real)` | `sec_to_ns(x)` | `func` | `real` | `x * (10 ^ 9)` | `sec_to_ns(6)` | `6000000000` |
| `ns_to_sec(real)` | `ns_to_sec(x)` | `func` | `real` | `x * (10 ^ -9)` | `ns_to_sec(6)` | `6e-09` |
| `day_to_hr(real)` | `day_to_hr(x)` | `func` | `real` | `x * 24` | `day_to_hr(6)` | `144` |
| `hr_to_day(real)` | `hr_to_day(x)` | `func` | `real` | `x / 24` | `hr_to_day(6)` | `0.25` |
| `day_to_sec(real)` | `day_to_sec(x)` | `func` | `real` | `x * 86400` | `day_to_sec(6)` | `518400` |
| `sec_to_day(real)` | `sec_to_day(x)` | `func` | `real` | `x / 86400` | `sec_to_day(6)` | `6.944444444e-05` |
| `(real[speed]) to_mps` | `(v:real, speed) to_mps   :90 -> real, none` | `postfix :90` | `real` | `v` | `(20 mps) to_mps` | `20` |
| `(real[speed]) to_kph` | `(v:real, speed) to_kph   :90 -> real, none` | `postfix :90` | `real` | `v * 3.6` | `(20 mps) to_kph` | `72` |
| `(real[speed]) to_mph` | `(v:real, speed) to_mph   :90 -> real, none` | `postfix :90` | `real` | `v / 0.44704` | `(20 mps) to_mph` | `44.73872584` |
| `(real[speed]) to_knots` | `(v:real, speed) to_knots :90 -> real, none` | `postfix :90` | `real` | `v * (3600 / 1852)` | `(20 mps) to_knots` | `38.87688985` |
| `kph_to_mps(real)` | `kph_to_mps(x)` | `func` | `real` | `x / 3.6` | `kph_to_mps(6)` | `1.666666667` |
| `mph_to_mps(real)` | `mph_to_mps(x)` | `func` | `real` | `x * 0.44704` | `mph_to_mps(6)` | `2.68224` |
| `kph_to_mph(real)` | `kph_to_mph(x)` | `func` | `real` | `x / 1.609344` | `kph_to_mph(6)` | `3.728227153` |
| `mph_to_kph(real)` | `mph_to_kph(x)` | `func` | `real` | `x * 1.609344` | `mph_to_kph(6)` | `9.656064` |
| `knots_to_mps(real)` | `knots_to_mps(x)` | `func` | `real` | `x * (1852 / 3600)` | `knots_to_mps(6)` | `3.086666667` |
| `mps_to_knots(real)` | `mps_to_knots(x)` | `func` | `real` | `x * (3600 / 1852)` | `mps_to_knots(6)` | `11.66306695` |
| `knots_to_kph(real)` | `knots_to_kph(x)` | `func` | `real` | `x * 1.852` | `knots_to_kph(6)` | `11.112` |
| `kph_to_knots(real)` | `kph_to_knots(x)` | `func` | `real` | `x / 1.852` | `kph_to_knots(6)` | `3.239740821` |
| `(real[pressure]) to_Pa` | `(p:real, pressure) to_Pa   :90 -> real, none` | `postfix :90` | `real` | `p` | `(101325 Pa) to_Pa` | `101325` |
| `(real[pressure]) to_kPa` | `(p:real, pressure) to_kPa  :90 -> real, none` | `postfix :90` | `real` | `p / 1000` | `(101325 Pa) to_kPa` | `101.325` |
| `(real[pressure]) to_MPa` | `(p:real, pressure) to_MPa  :90 -> real, none` | `postfix :90` | `real` | `p / 1000000` | `(101325 Pa) to_MPa` | `0.101325` |
| `(real[pressure]) to_bar` | `(p:real, pressure) to_bar  :90 -> real, none` | `postfix :90` | `real` | `p / 100000` | `(101325 Pa) to_bar` | `1.01325` |
| `(real[pressure]) to_atm` | `(p:real, pressure) to_atm  :90 -> real, none` | `postfix :90` | `real` | `p / 101325` | `(101325 Pa) to_atm` | `1` |
| `(real[pressure]) to_psi` | `(p:real, pressure) to_psi  :90 -> real, none` | `postfix :90` | `real` | `p / 6894.757293168` | `(101325 Pa) to_psi` | `14.69594878` |
| `(real[pressure]) to_torr` | `(p:real, pressure) to_torr :90 -> real, none` | `postfix :90` | `real` | `p * (760 / 101325)` | `(101325 Pa) to_torr` | `760` |
| `atm_to_Pa(real)` | `atm_to_Pa(x)` | `func` | `real` | `x * 101325` | `atm_to_Pa(6)` | `607950` |
| `Pa_to_atm(real)` | `Pa_to_atm(x)` | `func` | `real` | `x / 101325` | `Pa_to_atm(6)` | `5.9215396e-05` |
| `bar_to_Pa(real)` | `bar_to_Pa(x)` | `func` | `real` | `x * 100000` | `bar_to_Pa(6)` | `600000` |
| `Pa_to_bar(real)` | `Pa_to_bar(x)` | `func` | `real` | `x / 100000` | `Pa_to_bar(6)` | `6e-05` |
| `kPa_to_Pa(real)` | `kPa_to_Pa(x)` | `func` | `real` | `x * 1000` | `kPa_to_Pa(6)` | `6000` |
| `Pa_to_kPa(real)` | `Pa_to_kPa(x)` | `func` | `real` | `x / 1000` | `Pa_to_kPa(6)` | `0.006` |
| `psi_to_Pa(real)` | `psi_to_Pa(x)` | `func` | `real` | `x * 6894.757293168` | `psi_to_Pa(6)` | `41368.54376` |
| `Pa_to_psi(real)` | `Pa_to_psi(x)` | `func` | `real` | `x / 6894.757293168` | `Pa_to_psi(6)` | `0.0008702264264` |
| `atm_to_psi(real)` | `atm_to_psi(x)` | `func` | `real` | `x * (101325 / 6894.757293168)` | `atm_to_psi(6)` | `88.17569265` |
| `psi_to_atm(real)` | `psi_to_atm(x)` | `func` | `real` | `x * (6894.757293168 / 101325)` | `psi_to_atm(6)` | `0.4082757835` |
| `atm_to_bar(real)` | `atm_to_bar(x)` | `func` | `real` | `x * 1.01325` | `atm_to_bar(6)` | `6.0795` |
| `bar_to_atm(real)` | `bar_to_atm(x)` | `func` | `real` | `x / 1.01325` | `bar_to_atm(6)` | `5.9215396` |
| `atm_to_torr(real)` | `atm_to_torr(x)` | `func` | `real` | `x * 760` | `atm_to_torr(6)` | `4560` |
| `torr_to_atm(real)` | `torr_to_atm(x)` | `func` | `real` | `x / 760` | `torr_to_atm(6)` | `0.007894736842` |
| `(real[energy]) to_J` | `(e_v:real, energy) to_J    :90 -> real, none` | `postfix :90` | `real` | `e_v` | `(1000 J) to_J` | `1000` |
| `(real[energy]) to_kJ` | `(e_v:real, energy) to_kJ   :90 -> real, none` | `postfix :90` | `real` | `e_v / 1000` | `(1000 J) to_kJ` | `1` |
| `(real[energy]) to_MJ` | `(e_v:real, energy) to_MJ   :90 -> real, none` | `postfix :90` | `real` | `e_v / 1000000` | `(1000 J) to_MJ` | `0.001` |
| `(real[energy]) to_cal` | `(e_v:real, energy) to_cal  :90 -> real, none` | `postfix :90` | `real` | `e_v / 4.184` | `(1000 J) to_cal` | `239.0057361` |
| `(real[energy]) to_kcal` | `(e_v:real, energy) to_kcal :90 -> real, none` | `postfix :90` | `real` | `e_v / 4184` | `(1000 J) to_kcal` | `0.2390057361` |
| `(real[energy]) to_Wh` | `(e_v:real, energy) to_Wh   :90 -> real, none` | `postfix :90` | `real` | `e_v / 3600` | `(1000 J) to_Wh` | `0.2777777778` |
| `(real[energy]) to_kWh` | `(e_v:real, energy) to_kWh  :90 -> real, none` | `postfix :90` | `real` | `e_v / 3600000` | `(1000 J) to_kWh` | `0.0002777777778` |
| `(real[energy]) to_eV` | `(e_v:real, energy) to_eV   :90 -> real, none` | `postfix :90` | `real` | `e_v / (1.602176634 * (10 ^ -19))` | `(1000 J) to_eV` | `6.241509074e+21` |
| `(real[power]) to_W` | `(p_v:real, power)  to_W    :90 -> real, none` | `postfix :90` | `real` | `p_v` | `(500 W) to_W` | `500` |
| `(real[power]) to_kW` | `(p_v:real, power)  to_kW   :90 -> real, none` | `postfix :90` | `real` | `p_v / 1000` | `(500 W) to_kW` | `0.5` |
| `(real[power]) to_MW` | `(p_v:real, power)  to_MW   :90 -> real, none` | `postfix :90` | `real` | `p_v / 1000000` | `(500 W) to_MW` | `0.0005` |
| `(real[power]) to_hp` | `(p_v:real, power)  to_hp   :90 -> real, none` | `postfix :90` | `real` | `p_v / 745.699872` | `(500 W) to_hp` | `0.6705110444` |
| `kJ_to_J(real)` | `kJ_to_J(x)` | `func` | `real` | `x * 1000` | `kJ_to_J(6)` | `6000` |
| `J_to_kJ(real)` | `J_to_kJ(x)` | `func` | `real` | `x / 1000` | `J_to_kJ(6)` | `0.006` |
| `kcal_to_J(real)` | `kcal_to_J(x)` | `func` | `real` | `x * 4184` | `kcal_to_J(6)` | `25104` |
| `J_to_kcal(real)` | `J_to_kcal(x)` | `func` | `real` | `x / 4184` | `J_to_kcal(6)` | `0.001434034417` |
| `kcal_to_kJ(real)` | `kcal_to_kJ(x)` | `func` | `real` | `x * 4.184` | `kcal_to_kJ(6)` | `25.104` |
| `kJ_to_kcal(real)` | `kJ_to_kcal(x)` | `func` | `real` | `x / 4.184` | `kJ_to_kcal(6)` | `1.434034417` |
| `cal_to_J(real)` | `cal_to_J(x)` | `func` | `real` | `x * 4.184` | `cal_to_J(6)` | `25.104` |
| `J_to_cal(real)` | `J_to_cal(x)` | `func` | `real` | `x / 4.184` | `J_to_cal(6)` | `1.434034417` |
| `kWh_to_J(real)` | `kWh_to_J(x)` | `func` | `real` | `x * 3600000` | `kWh_to_J(6)` | `21600000` |
| `J_to_kWh(real)` | `J_to_kWh(x)` | `func` | `real` | `x / 3600000` | `J_to_kWh(6)` | `1.666666667e-06` |
| `Wh_to_J(real)` | `Wh_to_J(x)` | `func` | `real` | `x * 3600` | `Wh_to_J(6)` | `21600` |
| `J_to_Wh(real)` | `J_to_Wh(x)` | `func` | `real` | `x / 3600` | `J_to_Wh(6)` | `0.001666666667` |
| `kW_to_W(real)` | `kW_to_W(x)` | `func` | `real` | `x * 1000` | `kW_to_W(6)` | `6000` |
| `W_to_kW(real)` | `W_to_kW(x)` | `func` | `real` | `x / 1000` | `W_to_kW(6)` | `0.006` |
| `MW_to_W(real)` | `MW_to_W(x)` | `func` | `real` | `x * 1000000` | `MW_to_W(6)` | `6000000` |
| `W_to_MW(real)` | `W_to_MW(x)` | `func` | `real` | `x / 1000000` | `W_to_MW(6)` | `6e-06` |
| `hp_to_W(real)` | `hp_to_W(x)` | `func` | `real` | `x * 745.699872` | `hp_to_W(6)` | `4474.199232` |
| `W_to_hp(real)` | `W_to_hp(x)` | `func` | `real` | `x / 745.699872` | `W_to_hp(6)` | `0.008046132533` |
| `hp_to_kW(real)` | `hp_to_kW(x)` | `func` | `real` | `x * 0.745699872` | `hp_to_kW(6)` | `4.474199232` |
| `kW_to_hp(real)` | `kW_to_hp(x)` | `func` | `real` | `x / 0.745699872` | `kW_to_hp(6)` | `8.046132533` |
| `(real[area]) to_m2` | `(a_v:real, area) to_m2   :90 -> real, none` | `postfix :90` | `real` | `a_v` | `(25 m2) to_m2` | `25` |
| `(real[area]) to_km2` | `(a_v:real, area) to_km2  :90 -> real, none` | `postfix :90` | `real` | `a_v / 1000000` | `(25 m2) to_km2` | `2.5e-05` |
| `(real[area]) to_cm2` | `(a_v:real, area) to_cm2  :90 -> real, none` | `postfix :90` | `real` | `a_v * 10000` | `(25 m2) to_cm2` | `250000` |
| `(real[area]) to_mm2` | `(a_v:real, area) to_mm2  :90 -> real, none` | `postfix :90` | `real` | `a_v * 1000000` | `(25 m2) to_mm2` | `25000000` |
| `(real[area]) to_ha` | `(a_v:real, area) to_ha   :90 -> real, none` | `postfix :90` | `real` | `a_v / 10000` | `(25 m2) to_ha` | `0.0025` |
| `(real[area]) to_acre` | `(a_v:real, area) to_acre :90 -> real, none` | `postfix :90` | `real` | `a_v / 4046.8564224` | `(25 m2) to_acre` | `0.006177634537` |
| `(real[area]) to_ft2` | `(a_v:real, area) to_ft2  :90 -> real, none` | `postfix :90` | `real` | `a_v / 0.09290304` | `(25 m2) to_ft2` | `269.0977604` |
| `(real[area]) to_in2` | `(a_v:real, area) to_in2  :90 -> real, none` | `postfix :90` | `real` | `a_v / 0.00064516` | `(25 m2) to_in2` | `38750.0775` |
| `(real[volume]) to_m3` | `(v_v:real, volume) to_m3  :90 -> real, none` | `postfix :90` | `real` | `v_v` | `(2 m3) to_m3` | `2` |
| `(real[volume]) to_cm3` | `(v_v:real, volume) to_cm3 :90 -> real, none` | `postfix :90` | `real` | `v_v * 1000000` | `(2 m3) to_cm3` | `2000000` |
| `(real[volume]) to_L` | `(v_v:real, volume) to_L   :90 -> real, none` | `postfix :90` | `real` | `v_v * 1000` | `(2 m3) to_L` | `2000` |
| `(real[volume]) to_mL` | `(v_v:real, volume) to_mL  :90 -> real, none` | `postfix :90` | `real` | `v_v * 1000000` | `(2 m3) to_mL` | `2000000` |
| `(real[volume]) to_gal` | `(v_v:real, volume) to_gal :90 -> real, none` | `postfix :90` | `real` | `v_v / 0.003785411784` | `(2 m3) to_gal` | `528.3441047` |
| `(real[volume]) to_ft3` | `(v_v:real, volume) to_ft3 :90 -> real, none` | `postfix :90` | `real` | `v_v / 0.028316846592` | `(2 m3) to_ft3` | `70.62933344` |
| `(real[acceleration]) to_mps2` | `(acc_v:real, acceleration) to_mps2  :90 -> real, none` | `postfix :90` | `real` | `acc_v` | `(9.8 mps2) to_mps2` | `9.8` |
| `(real[acceleration]) to_g_acc` | `(acc_v:real, acceleration) to_g_acc :90 -> real, none` | `postfix :90` | `real` | `acc_v / 9.80665` | `(9.8 mps2) to_g_acc` | `0.9993218887` |
| `(real[acceleration]) to_fps2` | `(acc_v:real, acceleration) to_fps2  :90 -> real, none` | `postfix :90` | `real` | `acc_v / 0.3048` | `(9.8 mps2) to_fps2` | `32.15223097` |
| `(real[force]) to_N` | `(f_v:real, force) to_N   :90 -> real, none` | `postfix :90` | `real` | `f_v` | `(100 N) to_N` | `100` |
| `(real[force]) to_kN` | `(f_v:real, force) to_kN  :90 -> real, none` | `postfix :90` | `real` | `f_v / 1000` | `(100 N) to_kN` | `0.1` |
| `(real[force]) to_MN` | `(f_v:real, force) to_MN  :90 -> real, none` | `postfix :90` | `real` | `f_v / 1000000` | `(100 N) to_MN` | `0.0001` |
| `(real[force]) to_mN` | `(f_v:real, force) to_mN  :90 -> real, none` | `postfix :90` | `real` | `f_v * 1000` | `(100 N) to_mN` | `100000` |
| `(real[force]) to_dyn` | `(f_v:real, force) to_dyn :90 -> real, none` | `postfix :90` | `real` | `f_v * 100000` | `(100 N) to_dyn` | `10000000` |
| `(real[force]) to_lbf` | `(f_v:real, force) to_lbf :90 -> real, none` | `postfix :90` | `real` | `f_v / 4.4482216152605` | `(100 N) to_lbf` | `22.48089431` |
| `(real[force]) to_kgf` | `(f_v:real, force) to_kgf :90 -> real, none` | `postfix :90` | `real` | `f_v / 9.80665` | `(100 N) to_kgf` | `10.19716213` |
| `(real[frequency]) to_Hz` | `(fr_v:real, frequency) to_Hz  :90 -> real, none` | `postfix :90` | `real` | `fr_v` | `(50 Hz) to_Hz` | `50` |
| `(real[frequency]) to_kHz` | `(fr_v:real, frequency) to_kHz :90 -> real, none` | `postfix :90` | `real` | `fr_v / 1000` | `(50 Hz) to_kHz` | `0.05` |
| `(real[frequency]) to_MHz` | `(fr_v:real, frequency) to_MHz :90 -> real, none` | `postfix :90` | `real` | `fr_v / 1000000` | `(50 Hz) to_MHz` | `5e-05` |
| `(real[frequency]) to_GHz` | `(fr_v:real, frequency) to_GHz :90 -> real, none` | `postfix :90` | `real` | `fr_v / (10 ^ 9)` | `(50 Hz) to_GHz` | `5e-08` |
| `(real[frequency]) to_rpm` | `(fr_v:real, frequency) to_rpm :90 -> real, none` | `postfix :90` | `real` | `fr_v * 60` | `(50 Hz) to_rpm` | `3000` |
| `(real[momentum]) to_Ns` | `(p_m:real, momentum) to_Ns    :90 -> real, none` | `postfix :90` | `real` | `p_m` | `(50 Ns) to_Ns` | `50` |
| `(real[momentum]) to_kgmps` | `(p_m:real, momentum) to_kgmps :90 -> real, none` | `postfix :90` | `real` | `p_m` | `(50 Ns) to_kgmps` | `50` |
| `(real[density]) to_kgm3` | `(d_v:real, density) to_kgm3 :90 -> real, none` | `postfix :90` | `real` | `d_v` | `(1000 kgm3) to_kgm3` | `1000` |
| `(real[density]) to_gcm3` | `(d_v:real, density) to_gcm3 :90 -> real, none` | `postfix :90` | `real` | `d_v / 1000` | `(1000 kgm3) to_gcm3` | `1` |
| `(real[current]) to_A` | `(i_v:real, current)     to_A    :90 -> real, none` | `postfix :90` | `real` | `i_v` | `(2 A) to_A` | `2` |
| `(real[current]) to_mA` | `(i_v:real, current)     to_mA   :90 -> real, none` | `postfix :90` | `real` | `i_v * 1000` | `(2 A) to_mA` | `2000` |
| `(real[charge]) to_C` | `(q_v:real, charge)      to_C    :90 -> real, none` | `postfix :90` | `real` | `q_v` | `(10 C) to_C` | `10` |
| `(real[charge]) to_mAh` | `(q_v:real, charge)      to_mAh  :90 -> real, none` | `postfix :90` | `real` | `q_v / 3.6` | `(10 C) to_mAh` | `2.777777778` |
| `(real[voltage]) to_V` | `(u_v:real, voltage)     to_V    :90 -> real, none` | `postfix :90` | `real` | `u_v` | `(12 V) to_V` | `12` |
| `(real[voltage]) to_mV` | `(u_v:real, voltage)     to_mV   :90 -> real, none` | `postfix :90` | `real` | `u_v * 1000` | `(12 V) to_mV` | `12000` |
| `(real[voltage]) to_kV` | `(u_v:real, voltage)     to_kV   :90 -> real, none` | `postfix :90` | `real` | `u_v / 1000` | `(12 V) to_kV` | `0.012` |
| `(real[resistance]) to_ohm` | `(r_v:real, resistance)  to_ohm  :90 -> real, none` | `postfix :90` | `real` | `r_v` | `(4 ohm) to_ohm` | `4` |
| `(real[resistance]) to_kohm` | `(r_v:real, resistance)  to_kohm :90 -> real, none` | `postfix :90` | `real` | `r_v / 1000` | `(4 ohm) to_kohm` | `0.004` |
| `N_to_kN(real)` | `N_to_kN(x)` | `func` | `real` | `x / 1000` | `N_to_kN(6)` | `0.006` |
| `kN_to_N(real)` | `kN_to_N(x)` | `func` | `real` | `x * 1000` | `kN_to_N(6)` | `6000` |
| `N_to_lbf(real)` | `N_to_lbf(x)` | `func` | `real` | `x / 4.4482216152605` | `N_to_lbf(6)` | `1.348853659` |
| `lbf_to_N(real)` | `lbf_to_N(x)` | `func` | `real` | `x * 4.4482216152605` | `lbf_to_N(6)` | `26.68932969` |
| `N_to_dyn(real)` | `N_to_dyn(x)` | `func` | `real` | `x * 100000` | `N_to_dyn(6)` | `600000` |
| `dyn_to_N(real)` | `dyn_to_N(x)` | `func` | `real` | `x / 100000` | `dyn_to_N(6)` | `6e-05` |
| `L_to_m3(real)` | `L_to_m3(x)` | `func` | `real` | `x * 0.001` | `L_to_m3(6)` | `0.006` |
| `m3_to_L(real)` | `m3_to_L(x)` | `func` | `real` | `x * 1000` | `m3_to_L(6)` | `6000` |
| `L_to_mL(real)` | `L_to_mL(x)` | `func` | `real` | `x * 1000` | `L_to_mL(6)` | `6000` |
| `mL_to_L(real)` | `mL_to_L(x)` | `func` | `real` | `x * 0.001` | `mL_to_L(6)` | `0.006` |
| `gal_to_L(real)` | `gal_to_L(x)` | `func` | `real` | `x * 3.785411784` | `gal_to_L(6)` | `22.7124707` |
| `L_to_gal(real)` | `L_to_gal(x)` | `func` | `real` | `x / 3.785411784` | `L_to_gal(6)` | `1.585032314` |
| `(real[temperature]) to_K` | `(t_k:real, temperature)         to_K      :90 -> real, none` | `postfix :90` | `real` | `t_k` | `(300 K) to_K` | `300` |
| `(real[temperature]) to_mK` | `(t_k:real, temperature)         to_mK     :90 -> real, none` | `postfix :90` | `real` | `t_k * 1000` | `(300 K) to_mK` | `300000` |
| `(real[temperature]) to_degC` | `(t_k:real, temperature)         to_degC   :90 -> real, none` | `postfix :90` | `real` | `t_k - 273.15 K` | `(300 K) to_degC` | `26.85` |
| `(real[temperature]) to_degF` | `(t_k:real, temperature)         to_degF   :90 -> real, none` | `postfix :90` | `real` | `(t_k - 273.15 K) * 1.8 + 32 K` | `(300 K) to_degF` | `80.33` |
| `(real[heat_capacity]) to_JpK` | `(hc_v:real, heat_capacity)      to_JpK    :90 -> real, none` | `postfix :90` | `real` | `hc_v` | `(20 JpK) to_JpK` | `20` |
| `(real[specific_heat]) to_JpkgK` | `(sh_v:real, specific_heat)      to_JpkgK  :90 -> real, none` | `postfix :90` | `real` | `sh_v` | `(4184 JpkgK) to_JpkgK` | `4184` |
| `(real[substance]) to_mol` | `(n_v:real, substance)           to_mol    :90 -> real, none` | `postfix :90` | `real` | `n_v` | `(2 mol) to_mol` | `2` |
| `(real[substance]) to_mmol` | `(n_v:real, substance)           to_mmol   :90 -> real, none` | `postfix :90` | `real` | `n_v * 1000` | `(2 mol) to_mmol` | `2000` |
| `(real[substance]) to_umol` | `(n_v:real, substance)           to_umol   :90 -> real, none` | `postfix :90` | `real` | `n_v * 1e6` | `(2 mol) to_umol` | `2000000` |
| `(real[substance]) to_kmol` | `(n_v:real, substance)           to_kmol   :90 -> real, none` | `postfix :90` | `real` | `n_v / 1000` | `(2 mol) to_kmol` | `0.002` |
| `(real[molar_mass]) to_kgpmol` | `(mm_v:real, molar_mass)         to_kgpmol :90 -> real, none` | `postfix :90` | `real` | `mm_v` | `(18 gpmol) to_kgpmol` | `0.018` |
| `(real[molar_mass]) to_gpmol` | `(mm_v:real, molar_mass)         to_gpmol  :90 -> real, none` | `postfix :90` | `real` | `mm_v * 1000` | `(18 gpmol) to_gpmol` | `18` |
| `(real[molarity]) to_molm3` | `(c_v:real, molarity)            to_molm3  :90 -> real, none` | `postfix :90` | `real` | `c_v` | `(2 M_conc) to_molm3` | `2000` |
| `(real[molarity]) to_M_conc` | `(c_v:real, molarity)            to_M_conc :90 -> real, none` | `postfix :90` | `real` | `c_v / 1000` | `(2 M_conc) to_M_conc` | `2` |
| `(real[molarity]) to_mM_conc` | `(c_v:real, molarity)            to_mM_conc:90 -> real, none` | `postfix :90` | `real` | `c_v` | `(2 M_conc) to_mM_conc` | `2000` |
| `(real[molar_energy]) to_Jpmol` | `(me_v:real, molar_energy)       to_Jpmol  :90 -> real, none` | `postfix :90` | `real` | `me_v` | `(50 kJpmol) to_Jpmol` | `50000` |
| `(real[molar_energy]) to_kJpmol` | `(me_v:real, molar_energy)       to_kJpmol :90 -> real, none` | `postfix :90` | `real` | `me_v / 1000` | `(50 kJpmol) to_kJpmol` | `50` |
| `(real[molar_heat_capacity]) to_JpmolK` | `(mhc_v:real, molar_heat_capacity) to_JpmolK :90 -> real, none` | `postfix :90` | `real` | `mhc_v` | `(8.314462618 JpmolK) to_JpmolK` | `8.314462618` |
| `(real[catalytic_activity]) to_kat` | `(kat_v:real, catalytic_activity)  to_kat  :90 -> real, none` | `postfix :90` | `real` | `kat_v` | `(0.5 kat) to_kat` | `0.5` |
| `(real[luminous_intensity]) to_cd` | `(li_v:real, luminous_intensity) to_cd     :90 -> real, none` | `postfix :90` | `real` | `li_v` | `(800 lm) to_cd` | `800` |
| `(real[luminous_intensity]) to_lm` | `(li_v:real, luminous_intensity) to_lm     :90 -> real, none` | `postfix :90` | `real` | `li_v` | `(800 lm) to_lm` | `800` |
| `(real[illuminance]) to_lx` | `(il_v:real, illuminance)        to_lx     :90 -> real, none` | `postfix :90` | `real` | `il_v` | `(200 lx) to_lx` | `200` |
| `(real[illuminance]) to_klx` | `(il_v:real, illuminance)        to_klx    :90 -> real, none` | `postfix :90` | `real` | `il_v / 1000` | `(200 lx) to_klx` | `0.2` |

### 10.15 Module `[physics]` (`Library/physics.math` — 100 overloads)

14 physical constants (`g0`, `c0`, `G_grav`, `h_planck`, `k_B`, `ke_coulomb`, `eps0`, `mu0`, `q_e`, `m_e`, `m_p`, `sigma_sb`, `b_wien`, `rydberg_R`) and 100 functions covering kinematics, dynamics, collisions, rotation, SHM, gravitation, fluids, elasticity, thermodynamics, electromagnetism, AC circuits, waves/optics, quantum mechanics, and special relativity.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `suvat_v(real, real, real)` | `suvat_v(u, a, t)` | `func` | `real` | `u + a * t` | `suvat_v(6, 3, 2)` | `12` |
| `suvat_s(real, real, real)` | `suvat_s(u, a, t)` | `func` | `real` | `u * t + 0.5 * a * (t ^ 2)` | `suvat_s(6, 3, 2)` | `18` |
| `suvat_v2(real, real, real)` | `suvat_v2(u, a, s)` | `func` | `real` | `√((u ^ 2) + 2a * s)` | `suvat_v2(6, 3, 2)` | `6.92820323` |
| `suvat_s_avg(real, real, real)` | `suvat_s_avg(u, v, t)` | `func` | `real` | `0.5 * (u + v) * t` | `suvat_s_avg(6, 3, 2)` | `9` |
| `suvat_nth_sec(real, real, real)` | `suvat_nth_sec(u, a, n)` | `func` | `real` | `u + 0.5 * a * (2n - 1)` | `suvat_nth_sec(6, 3, 2)` | `10.5` |
| `ke(real, real)` | `ke(mass, vel)` | `func` | `real` | `0.5 * mass * (vel ^ 2)` | `ke(6, 3)` | `27` |
| `pe(real, real)` | `pe(mass, height)` | `func` | `real` | `mass * g0 * height` | `pe(6, 3)` | `176.5197` |
| `mech_energy(real, real, real)` | `mech_energy(mass, vel, height)` | `func` | `real` | `ke(mass, vel) + pe(mass, height)` | `mech_energy(6, 3, 2)` | `144.6798` |
| `proj_range(real, real)` | `proj_range(vel, angle_deg)` | `func` | `real` | `(vel ^ 2) * sin(2 * deg2rad(angle_deg)) / g0` | `proj_range(6, 3)` | `0.3837217274` |
| `proj_max_h(real, real)` | `proj_max_h(vel, angle_deg)` | `func` | `real` | `((vel * sin(deg2rad(angle_deg))) ^ 2) / (2 * g0)` | `proj_max_h(6, 3)` | `0.005027500898` |
| `proj_time(real, real)` | `proj_time(vel, angle_deg)` | `func` | `real` | `2 * vel * sin(deg2rad(angle_deg)) / g0` | `proj_time(6, 3)` | `0.06404138772` |
| `momentum(real, real)` | `momentum(mass, vel)` | `func` | `real` | `mass * vel` | `momentum(6, 3)` | `18` |
| `impulse(real, real)` | `impulse(force_f, dt)` | `func` | `real` | `force_f * dt` | `impulse(6, 3)` | `18` |
| `recoil_vel(real, real, real)` | `recoil_vel(m_gun, m_bullet, v_bullet)` | `func` | `real` | `-(m_bullet * v_bullet) / m_gun` | `recoil_vel(6, 3, 2)` | `-1` |
| `friction_force(real, real)` | `friction_force(mu, normal_n)` | `func` | `real` | `mu * normal_n` | `friction_force(6, 3)` | `18` |
| `incline_normal(real, real)` | `incline_normal(mass, theta_deg)` | `func` | `real` | `mass * g0 * cos(deg2rad(theta_deg))` | `incline_normal(6, 3)` | `58.75926196` |
| `incline_slide_acc(real, real)` | `incline_slide_acc(theta_deg, mu_k)` | `func` | `real` | `g0 * (sin(deg2rad(theta_deg)) - mu_k * cos(deg2rad(theta_deg)))` | `incline_slide_acc(6, 3)` | `-28.23371038` |
| `banking_angle_deg(real, real)` | `banking_angle_deg(vel, r)` | `func` | `real` | `rad2deg(atan((vel ^ 2) / (r * g0)))` | `banking_angle_deg(6, 3)` | `50.74359039` |
| `elastic_v1(real, real, real, real)` | `elastic_v1(m1, u1, m2, u2)` | `func` | `real` | `((m1 - m2) * u1 + 2 * m2 * u2) / (m1 + m2)` | `elastic_v1(6, 3, 2, 4)` | `3.5` |
| `elastic_v2(real, real, real, real)` | `elastic_v2(m1, u1, m2, u2)` | `func` | `real` | `((m2 - m1) * u2 + 2 * m1 * u1) / (m1 + m2)` | `elastic_v2(6, 3, 2, 4)` | `2.5` |
| `inelastic_v(real, real, real, real)` | `inelastic_v(m1, u1, m2, u2)` | `func` | `real` | `(m1 * u1 + m2 * u2) / (m1 + m2)` | `inelastic_v(6, 3, 2, 4)` | `3.25` |
| `centripetal_acc(real, real)` | `centripetal_acc(vel, r)` | `func` | `real` | `(vel ^ 2) / r` | `centripetal_acc(6, 3)` | `12` |
| `centripetal_force(real, real, real)` | `centripetal_force(mass, vel, r)` | `func` | `real` | `mass * (vel ^ 2) / r` | `centripetal_force(6, 3, 2)` | `27` |
| `torque_val(real, real, real)` | `torque_val(r, force_f, theta_deg)` | `func` | `real` | `r * force_f * sin(deg2rad(theta_deg))` | `torque_val(6, 3, 2)` | `0.6281909406` |
| `ang_momentum(real, real)` | `ang_momentum(inertia_i, omega)` | `func` | `real` | `inertia_i * omega` | `ang_momentum(6, 3)` | `18` |
| `rot_ke(real, real)` | `rot_ke(inertia_i, omega)` | `func` | `real` | `0.5 * inertia_i * (omega ^ 2)` | `rot_ke(6, 3)` | `27` |
| `hooke_force(real, real)` | `hooke_force(k_spring, disp)` | `func` | `real` | `-k_spring * disp` | `hooke_force(6, 3)` | `-18` |
| `elastic_pe(real, real)` | `elastic_pe(k_spring, disp)` | `func` | `real` | `0.5 * k_spring * (disp ^ 2)` | `elastic_pe(6, 3)` | `27` |
| `pendulum_period(real)` | `pendulum_period(length_m)` | `func` | `real` | `2π * √(length_m / g0)` | `pendulum_period(6)` | `4.914678982` |
| `shm_omega(real, real)` | `shm_omega(k_spring, mass)` | `func` | `real` | `√(k_spring / mass)` | `shm_omega(6, 3)` | `1.414213562` |
| `shm_period_spring(real, real)` | `shm_period_spring(mass, k_spring)` | `func` | `real` | `2π * √(mass / k_spring)` | `shm_period_spring(6, 3)` | `8.885765876` |
| `shm_vel(real, real, real)` | `shm_vel(omega, amp_a, disp_x)` | `func` | `real` | `omega * √((amp_a ^ 2) - (disp_x ^ 2))` | `shm_vel(6, 3, 2)` | `13.41640786` |
| `shm_acc(real, real)` | `shm_acc(omega, disp_x)` | `func` | `real` | `-(omega ^ 2) * disp_x` | `shm_acc(6, 3)` | `-108` |
| `grav_force(real, real, real)` | `grav_force(m1, m2, r)` | `func` | `real` | `G_grav * m1 * m2 / (r ^ 2)` | `grav_force(6, 3, 2)` | `3.003435e-10` |
| `grav_pe(real, real, real)` | `grav_pe(m1, m2, r)` | `func` | `real` | `-G_grav * m1 * m2 / r` | `grav_pe(6, 3, 2)` | `-6.00687e-10` |
| `orbital_vel(real, real)` | `orbital_vel(mass_body, r)` | `func` | `real` | `√(G_grav * mass_body / r)` | `orbital_vel(6, 3)` | `1.155361415e-05` |
| `escape_vel(real, real)` | `escape_vel(mass_body, r)` | `func` | `real` | `√(2 * G_grav * mass_body / r)` | `escape_vel(6, 3)` | `1.633927783e-05` |
| `schwarzschild_r(real)` | `schwarzschild_r(mass_body)` | `func` | `real` | `2 * G_grav * mass_body / (c0 ^ 2)` | `schwarzschild_r(6)` | `8.911392323e-27` |
| `kepler_period(real, real)` | `kepler_period(semi_major_a, mass_body)` | `func` | `real` | `2π * √((semi_major_a ^ 3) / (G_grav * mass_body))` | `kepler_period(6, 3)` | `6525942.678` |
| `hydro_pressure(real, real)` | `hydro_pressure(rho, depth_h)` | `func` | `real` | `rho * g0 * depth_h` | `hydro_pressure(6, 3)` | `176.5197` |
| `buoyant_force(real, real)` | `buoyant_force(rho_fluid, vol_sub)` | `func` | `real` | `rho_fluid * g0 * vol_sub` | `buoyant_force(6, 3)` | `176.5197` |
| `continuity_v2(real, real, real)` | `continuity_v2(a1, v1, a2)` | `func` | `real` | `(a1 * v1) / a2` | `continuity_v2(6, 3, 2)` | `9` |
| `stokes_drag(real, real, real)` | `stokes_drag(eta, r, vel)` | `func` | `real` | `6π * eta * r * vel` | `stokes_drag(6, 3, 2)` | `678.5840132` |
| `terminal_vel(real, real, real, real)` | `terminal_vel(r, rho_sphere, rho_fluid, eta)` | `func` | `real` | `(2 * (r ^ 2) * (rho_sphere - rho_fluid) * g0) / (9 * eta)` | `terminal_vel(6, 3, 2, 4)` | `19.6133` |
| `stress_val(real, real)` | `stress_val(force_f, area)` | `func` | `real` | `force_f / area` | `stress_val(6, 3)` | `2` |
| `strain_val(real, real)` | `strain_val(dl, l0)` | `func` | `real` | `dl / l0` | `strain_val(6, 3)` | `2` |
| `youngs_modulus(real, real, real, real)` | `youngs_modulus(force_f, l0, area, dl)` | `func` | `real` | `(force_f * l0) / (area * dl)` | `youngs_modulus(6, 3, 2, 4)` | `2.25` |
| `sensible_heat(real, real, real)` | `sensible_heat(mass, c_spec, dt)` | `func` | `real` | `mass * c_spec * dt` | `sensible_heat(6, 3, 2)` | `36` |
| `latent_heat(real, real)` | `latent_heat(mass, l_spec)` | `func` | `real` | `mass * l_spec` | `latent_heat(6, 3)` | `18` |
| `thermal_expansion_l(real, real, real)` | `thermal_expansion_l(l0, alpha, dt)` | `func` | `real` | `l0 * (1 + alpha * dt)` | `thermal_expansion_l(6, 3, 2)` | `42` |
| `thermal_cond_rate(real, real, real, real)` | `thermal_cond_rate(k_cond, area, dt, thickness)` | `func` | `real` | `(k_cond * area * dt) / thickness` | `thermal_cond_rate(6, 3, 2, 4)` | `9` |
| `wien_peak_lambda(real)` | `wien_peak_lambda(temp_k)` | `func` | `real` | `b_wien / temp_k` | `wien_peak_lambda(6)` | `0.0004829619925` |
| `stefan_boltzmann_p(real, real, real)` | `stefan_boltzmann_p(area, emissivity, temp_k)` | `func` | `real` | `area * emissivity * sigma_sb * (temp_k ^ 4)` | `stefan_boltzmann_p(6, 3, 2)` | `1.633067833e-05` |
| `carnot_eff(real, real)` | `carnot_eff(t_cold_k, t_hot_k)` | `func` | `real` | `1 - (t_cold_k / t_hot_k)` | `carnot_eff(6, 3)` | `-1` |
| `coulomb_force(real, real, real)` | `coulomb_force(q1, q2, r)` | `func` | `real` | `ke_coulomb * q1 * q2 / (r ^ 2)` | `coulomb_force(6, 3, 2)` | `4.044398307e+10` |
| `ohm_v(real, real)` | `ohm_v(current_i, res_r)` | `func` | `real` | `current_i * res_r` | `ohm_v(6, 3)` | `18` |
| `ohm_i(real, real)` | `ohm_i(volt_v, res_r)` | `func` | `real` | `volt_v / res_r` | `ohm_i(6, 3)` | `2` |
| `ohm_r(real, real)` | `ohm_r(volt_v, current_i)` | `func` | `real` | `volt_v / current_i` | `ohm_r(6, 3)` | `2` |
| `elec_power(real, real)` | `elec_power(volt_v, current_i)` | `func` | `real` | `volt_v * current_i` | `elec_power(6, 3)` | `18` |
| `cap_energy(real, real)` | `cap_energy(cap_c, volt_v)` | `func` | `real` | `0.5 * cap_c * (volt_v ^ 2)` | `cap_energy(6, 3)` | `27` |
| `ind_energy(real, real)` | `ind_energy(ind_l, current_i)` | `func` | `real` | `0.5 * ind_l * (current_i ^ 2)` | `ind_energy(6, 3)` | `27` |
| `res_series(real, real)` | `res_series(r1, r2)` | `func` | `real` | `r1 + r2` | `res_series(6, 3)` | `9` |
| `res_parallel(real, real)` | `res_parallel(r1, r2)` | `func` | `real` | `(r1 * r2) / (r1 + r2)` | `res_parallel(6, 3)` | `2` |
| `cap_series(real, real)` | `cap_series(c1, c2)` | `func` | `real` | `(c1 * c2) / (c1 + c2)` | `cap_series(6, 3)` | `2` |
| `cap_parallel(real, real)` | `cap_parallel(c1, c2)` | `func` | `real` | `c1 + c2` | `cap_parallel(6, 3)` | `9` |
| `parallel_plate_cap(real, real)` | `parallel_plate_cap(area, sep_d)` | `func` | `real` | `eps0 * area / sep_d` | `parallel_plate_cap(6, 3)` | `1.770837563e-11` |
| `rc_tau(real, real)` | `rc_tau(res_r, cap_c)` | `func` | `real` | `res_r * cap_c` | `rc_tau(6, 3)` | `18` |
| `wheatstone_r4(real, real, real)` | `wheatstone_r4(r1, r2, r3)` | `func` | `real` | `(r2 * r3) / r1` | `wheatstone_r4(6, 3, 2)` | `1` |
| `drift_velocity(real, real, real, real)` | `drift_velocity(current_i, n_density, area, q_charge)` | `func` | `real` | `current_i / (n_density * area * q_charge)` | `drift_velocity(6, 3, 2, 4)` | `0.25` |
| `lorentz_mag_force(real, real, real, real)` | `lorentz_mag_force(q, vel, b_field, theta_deg)` | `func` | `real` | `q * vel * b_field * sin(deg2rad(theta_deg))` | `lorentz_mag_force(6, 3, 2, 4)` | `2.511233055` |
| `cyclotron_radius(real, real, real, real)` | `cyclotron_radius(mass, vel, q, b_field)` | `func` | `real` | `(mass * vel) / (q * b_field)` | `cyclotron_radius(6, 3, 2, 4)` | `2.25` |
| `solenoid_b(real, real)` | `solenoid_b(n_turns_per_m, current_i)` | `func` | `real` | `mu0 * n_turns_per_m * current_i` | `solenoid_b(6, 3)` | `2.261946712e-05` |
| `inductive_reactance(real, real)` | `inductive_reactance(freq_hz, ind_l)` | `func` | `real` | `2π * freq_hz * ind_l` | `inductive_reactance(6, 3)` | `113.0973355` |
| `capacitive_reactance(real, real)` | `capacitive_reactance(freq_hz, cap_c)` | `func` | `real` | `1 / (2π * freq_hz * cap_c)` | `capacitive_reactance(6, 3)` | `0.008841941283` |
| `rlc_impedance(real, real, real)` | `rlc_impedance(res_r, xl, xc)` | `func` | `real` | `√((res_r ^ 2) + ((xl - xc) ^ 2))` | `rlc_impedance(6, 3, 2)` | `6.08276253` |
| `rlc_resonant_freq(real, real)` | `rlc_resonant_freq(ind_l, cap_c)` | `func` | `real` | `1 / (2π * √(ind_l * cap_c))` | `rlc_resonant_freq(6, 3)` | `0.03751317984` |
| `transformer_v2(real, real, real)` | `transformer_v2(v1, n1, n2)` | `func` | `real` | `v1 * (n2 / n1)` | `transformer_v2(6, 3, 2)` | `4` |
| `doppler_freq(real, real, real, real)` | `doppler_freq(f0, v_sound, v_obs, v_src)` | `func` | `real` | `f0 * ((v_sound + v_obs) / (v_sound - v_src))` | `doppler_freq(6, 3, 2, 4)` | `-30` |
| `beat_freq(real, real)` | `beat_freq(f1, f2)` | `func` | `real` | `@abs(f1 - f2)` | `beat_freq(6, 3)` | `3` |
| `sound_db(real)` | `sound_db(intensity_i)` | `func` | `real` | `10 * log10(intensity_i / (10 ^ -12))` | `sound_db(6)` | `127.7815125` |
| `snell_angle2(real, real, real)` | `snell_angle2(n1, theta1_deg, n2)` | `func` | `real` | `rad2deg(asin(n1 * sin(deg2rad(theta1_deg)) / n2))` | `snell_angle2(1, 1.5, 30)` | `0.04999429496` |
| `lens_focal(real, real)` | `lens_focal(do_dist, di_dist)` | `func` | `real` | `(do_dist * di_dist) / (do_dist + di_dist)` | `lens_focal(6, 3)` | `2` |
| `mirror_image_v(real, real)` | `mirror_image_v(f_len, u_obj)` | `func` | `real` | `(f_len * u_obj) / (u_obj - f_len)` | `mirror_image_v(6, 3)` | `-6` |
| `magnification_lens(real, real)` | `magnification_lens(v_img, u_obj)` | `func` | `real` | `v_img / u_obj` | `magnification_lens(6, 3)` | `2` |
| `lens_power_dioptre(real)` | `lens_power_dioptre(f_meters)` | `func` | `real` | `1 / f_meters` | `lens_power_dioptre(6)` | `0.1666666667` |
| `young_fringe_width(real, real, real)` | `young_fringe_width(wavelength_m, screen_d, slit_sep_d)` | `func` | `real` | `(wavelength_m * screen_d) / slit_sep_d` | `young_fringe_width(6, 3, 2)` | `9` |
| `malus_law(real, real)` | `malus_law(i0, theta_deg)` | `func` | `real` | `i0 * (cos(deg2rad(theta_deg)) ^ 2)` | `malus_law(6, 3)` | `5.983565686` |
| `brewster_angle(real, real)` | `brewster_angle(n1, n2)` | `func` | `real` | `rad2deg(atan(n2 / n1))` | `brewster_angle(6, 3)` | `26.56505118` |
| `photon_energy(real)` | `photon_energy(freq_hz)` | `func` | `real` | `h_planck * freq_hz` | `photon_energy(6)` | `3.97564209e-33` |
| `photon_wavelength(real)` | `photon_wavelength(wavelength_m)` | `func` | `real` | `h_planck * c0 / wavelength_m` | `photon_wavelength(6)` | `3.310743095e-26` |
| `photoelectric_ke_max(real, real)` | `photoelectric_ke_max(freq_hz, work_fn_j)` | `func` | `real` | `h_planck * freq_hz - work_fn_j` | `photoelectric_ke_max(6, 3)` | `-3` |
| `de_broglie(real, real)` | `de_broglie(mass, vel)` | `func` | `real` | `h_planck / (mass * vel)` | `de_broglie(6, 3)` | `3.681150083e-35` |
| `bohr_radius_n(real)` | `bohr_radius_n(n_orbit)` | `func` | `real` | `5.29177210903 * (10 ^ -11) * (n_orbit ^ 2)` | `bohr_radius_n(6)` | `1.905037959e-09` |
| `bohr_energy_ev(real, real)` | `bohr_energy_ev(n_orbit, z_atomic = 1)` | `func` | `real` | `-13.6 * ((z_atomic ^ 2) / (n_orbit ^ 2))` | `bohr_energy_ev(6, 3)` | `-3.4` |
| `bohr_energy_ev(real)` | `bohr_energy_ev(n_orbit, z_atomic = 1) [default-arg overload]` | `func` | `real` | `-13.6 * ((z_atomic ^ 2) / (n_orbit ^ 2))` | `bohr_energy_ev(6)` | `-0.3777777778` |
| `rydberg_wavelength(real, real)` | `rydberg_wavelength(n1, n2)` | `func` | `real` | `1 / (rydberg_R * ((1 / (n1 ^ 2)) - (1 / (n2 ^ 2))))` | `rydberg_wavelength(2, 3)` | `6.561122764e-07` |
| `lorentz_gamma(real)` | `lorentz_gamma(vel)` | `func` | `real` | `1 / √(1 - (vel / c0) ^ 2)` | `lorentz_gamma(0.6 * c0)` | `1.25` |
| `rel_energy(real, real)` | `rel_energy(mass, vel)` | `func` | `real` | `lorentz_gamma(vel) * mass * (c0 ^ 2)` | `rel_energy(1e-3, 0.6 * c0)` | `1.123443973e+14` |
| `time_dilation(real, real)` | `time_dilation(t_proper, vel)` | `func` | `real` | `t_proper * lorentz_gamma(vel)` | `time_dilation(10, 0.6 * c0)` | `12.5` |
| `length_contraction(real, real)` | `length_contraction(l_proper, vel)` | `func` | `real` | `l_proper / lorentz_gamma(vel)` | `length_contraction(100, 0.6 * c0)` | `80` |

### 10.16 Module `[chemistry]` (`Library/chemistry.math` — 61 overloads)

7 chemical constants (`N_A`, `R_gas`, `R_atm`, `F_faraday`, `T_stp`, `P_stp`, `kw_water`) and 61 functions covering stoichiometry, concentration, colligative properties, ideal & real gas laws, kinetic theory, acid-base/buffers/Ksp, chemical kinetics, nuclear decay, thermochemistry, and electrochemistry/crystallography.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `moles(real, real)` | `moles(mass_g, molar_mass)` | `func` | `real` | `mass_g / molar_mass` | `moles(6, 3)` | `2` |
| `mass_from_moles(real, real)` | `mass_from_moles(n_mol, molar_mass)` | `func` | `real` | `n_mol * molar_mass` | `mass_from_moles(6, 3)` | `18` |
| `particles_from_moles(real)` | `particles_from_moles(n_mol)` | `func` | `real` | `n_mol * N_A` | `particles_from_moles(6)` | `3.613284456e+24` |
| `percent_composition(real, real)` | `percent_composition(mass_elem, molar_mass)` | `func` | `real` | `(mass_elem / molar_mass) * 100` | `percent_composition(6, 3)` | `200` |
| `percent_yield(real, real)` | `percent_yield(actual_y, theor_y)` | `func` | `real` | `(actual_y / theor_y) * 100` | `percent_yield(6, 3)` | `200` |
| `atom_economy(real, real)` | `atom_economy(mw_desired, mw_reactants)` | `func` | `real` | `(mw_desired / mw_reactants) * 100` | `atom_economy(6, 3)` | `200` |
| `molarity(real, real)` | `molarity(n_mol, vol_liters)` | `func` | `real` | `n_mol / vol_liters` | `molarity(6, 3)` | `2` |
| `normality_from_molarity(real, real)` | `normality_from_molarity(molarity_m, n_factor)` | `func` | `real` | `molarity_m * n_factor` | `normality_from_molarity(6, 3)` | `18` |
| `dilution_v1(real, real, real)` | `dilution_v1(c1, c2, v2)` | `func` | `real` | `(c2 * v2) / c1` | `dilution_v1(6, 3, 2)` | `1` |
| `molality(real, real)` | `molality(n_solute, kg_solvent)` | `func` | `real` | `n_solute / kg_solvent` | `molality(6, 3)` | `2` |
| `mole_fraction(real, real)` | `mole_fraction(n_component, n_total)` | `func` | `real` | `n_component / n_total` | `mole_fraction(6, 3)` | `2` |
| `ppm_conc(real, real)` | `ppm_conc(mass_solute_g, mass_soln_g)` | `func` | `real` | `(mass_solute_g / mass_soln_g) * (10 ^ 6)` | `ppm_conc(6, 3)` | `2000000` |
| `raoult_p_soln(real, real)` | `raoult_p_soln(x_solvent, p0_solvent)` | `func` | `real` | `x_solvent * p0_solvent` | `raoult_p_soln(6, 3)` | `18` |
| `bp_elevation(real, real, real)` | `bp_elevation(i_vant, kb, molality_m)` | `func` | `real` | `i_vant * kb * molality_m` | `bp_elevation(6, 3, 2)` | `36` |
| `fp_depression(real, real, real)` | `fp_depression(i_vant, kf, molality_m)` | `func` | `real` | `i_vant * kf * molality_m` | `fp_depression(6, 3, 2)` | `36` |
| `osmotic_pressure(real, real)` | `osmotic_pressure(molarity_m, temp_k)` | `func` | `real` | `molarity_m * R_atm * temp_k` | `osmotic_pressure(6, 3)` | `1.477032588` |
| `vant_hoff_osmotic(real, real, real)` | `vant_hoff_osmotic(i_vant, molarity_m, temp_k)` | `func` | `real` | `i_vant * molarity_m * R_atm * temp_k` | `vant_hoff_osmotic(6, 3, 2)` | `2.954065176` |
| `boyle_p2(real, real, real)` | `boyle_p2(p1, v1, v2)` | `func` | `real` | `(p1 * v1) / v2` | `boyle_p2(6, 3, 2)` | `9` |
| `charles_v2(real, real, real)` | `charles_v2(v1, t1_k, t2_k)` | `func` | `real` | `(v1 * t2_k) / t1_k` | `charles_v2(6, 3, 2)` | `4` |
| `combined_gas_v2(real, real, real, real, real)` | `combined_gas_v2(p1, v1, t1_k, p2, t2_k)` | `func` | `real` | `(p1 * v1 * t2_k) / (p2 * t1_k)` | `combined_gas_v2(6, 3, 2, 4, 5)` | `11.25` |
| `dalton_partial_p(real, real)` | `dalton_partial_p(mole_frac, p_total)` | `func` | `real` | `mole_frac * p_total` | `dalton_partial_p(6, 3)` | `18` |
| `ideal_gas_p(real, real, real)` | `ideal_gas_p(n_mol, temp_k, vol_m3)` | `func` | `real` | `(n_mol * R_gas * temp_k) / vol_m3` | `ideal_gas_p(6, 3, 2)` | `74.83016356` |
| `ideal_gas_v(real, real, real)` | `ideal_gas_v(n_mol, temp_k, press_pa)` | `func` | `real` | `(n_mol * R_gas * temp_k) / press_pa` | `ideal_gas_v(6, 3, 2)` | `74.83016356` |
| `ideal_gas_n(real, real, real)` | `ideal_gas_n(press_pa, vol_m3, temp_k)` | `func` | `real` | `(press_pa * vol_m3) / (R_gas * temp_k)` | `ideal_gas_n(6, 3, 2)` | `1.082451195` |
| `ideal_gas_t(real, real, real)` | `ideal_gas_t(press_pa, vol_m3, n_mol)` | `func` | `real` | `(press_pa * vol_m3) / (n_mol * R_gas)` | `ideal_gas_t(6, 3, 2)` | `1.082451195` |
| `van_der_waals_p(real, real, real, real, real)` | `van_der_waals_p(n_mol, temp_k, vol_l, a_vdw, b_vdw)` | `func` | `real` | `((n_mol * R_atm * temp_k) / (vol_l - n_mol * b_vdw)) - a_vdw * ((n_mol / vol_...` | `van_der_waals_p(6, 3, 2, 4, 5)` | `-36.05275116` |
| `rms_mol_speed(real, real)` | `rms_mol_speed(temp_k, molar_mass_kg)` | `func` | `real` | `√(3 * R_gas * temp_k / molar_mass_kg)` | `rms_mol_speed(6, 3)` | `7.063057108` |
| `graham_rate_ratio(real, real)` | `graham_rate_ratio(molar_mass1, molar_mass2)` | `func` | `real` | `√(molar_mass2 / molar_mass1)` | `graham_rate_ratio(6, 3)` | `0.7071067812` |
| `ph_from_h(real)` | `ph_from_h(h_conc)` | `func` | `real` | `-log10(h_conc)` | `ph_from_h(6)` | `-0.7781512504` |
| `h_from_ph(real)` | `h_from_ph(ph_val)` | `func` | `real` | `10 ^ (-ph_val)` | `h_from_ph(6)` | `1e-06` |
| `poh_from_oh(real)` | `poh_from_oh(oh_conc)` | `func` | `real` | `-log10(oh_conc)` | `poh_from_oh(6)` | `-0.7781512504` |
| `oh_from_poh(real)` | `oh_from_poh(poh_val)` | `func` | `real` | `10 ^ (-poh_val)` | `oh_from_poh(6)` | `1e-06` |
| `poh_from_ph(real)` | `poh_from_ph(ph_val)` | `func` | `real` | `14 - ph_val` | `poh_from_ph(6)` | `8` |
| `ph_from_poh(real)` | `ph_from_poh(poh_val)` | `func` | `real` | `14 - poh_val` | `ph_from_poh(6)` | `8` |
| `ph_from_oh(real)` | `ph_from_oh(oh_conc)` | `func` | `real` | `14 - (-log10(oh_conc))` | `ph_from_oh(6)` | `14.77815125` |
| `pka_from_ka(real)` | `pka_from_ka(ka)` | `func` | `real` | `-log10(ka)` | `pka_from_ka(6)` | `-0.7781512504` |
| `ka_from_pka(real)` | `ka_from_pka(pka)` | `func` | `real` | `10 ^ (-pka)` | `ka_from_pka(6)` | `1e-06` |
| `weak_acid_h(real, real)` | `weak_acid_h(ka, conc_c)` | `func` | `real` | `√(ka * conc_c)` | `weak_acid_h(6, 3)` | `4.242640687` |
| `weak_acid_ph(real, real)` | `weak_acid_ph(ka, conc_c)` | `func` | `real` | `-log10(√(ka * conc_c))` | `weak_acid_ph(6, 3)` | `-0.6276362526` |
| `henderson_hasselbalch(real, real, real)` | `henderson_hasselbalch(pka, conj_base, weak_acid)` | `func` | `real` | `pka + log10(conj_base / weak_acid)` | `henderson_hasselbalch(6, 3, 2)` | `6.176091259` |
| `solubility_ksp_ab(real)` | `solubility_ksp_ab(ksp)` | `func` | `real` | `√ksp` | `solubility_ksp_ab(6)` | `2.449489743` |
| `solubility_ksp_ab2(real)` | `solubility_ksp_ab2(ksp)` | `func` | `real` | `(ksp / 4) ^ (1 / 3)` | `solubility_ksp_ab2(6)` | `1.144714243` |
| `arrhenius_k(real, real, real)` | `arrhenius_k(pre_exp_a, ea_j_mol, temp_k)` | `func` | `real` | `pre_exp_a * exp(-ea_j_mol / (R_gas * temp_k))` | `arrhenius_k(6, 3, 2)` | `5.009574276` |
| `arrhenius_k2(real, real, real, real)` | `arrhenius_k2(k1, ea_j_mol, t1_k, t2_k)` | `func` | `real` | `k1 * exp((ea_j_mol / R_gas) * ((1 / t1_k) - (1 / t2_k)))` | `arrhenius_k2(6, 3, 2, 4)` | `6.566386857` |
| `rate_const_1st(real, real, real)` | `rate_const_1st(t_sec, c0, ct)` | `func` | `real` | `(1 / t_sec) * ln(c0 / ct)` | `rate_const_1st(6, 3, 2)` | `0.06757751802` |
| `half_life_1st(real)` | `half_life_1st(rate_k)` | `func` | `real` | `ln(2) / rate_k` | `half_life_1st(6)` | `0.1155245301` |
| `zero_order_half_life(real, real)` | `zero_order_half_life(c0, rate_k)` | `func` | `real` | `c0 / (2 * rate_k)` | `zero_order_half_life(6, 3)` | `1` |
| `second_order_half_life(real, real)` | `second_order_half_life(c0, rate_k)` | `func` | `real` | `1 / (rate_k * c0)` | `second_order_half_life(6, 3)` | `0.05555555556` |
| `decay_rem(real, real, real)` | `decay_rem(n0, time_t, half_life)` | `func` | `real` | `n0 * (0.5 ^ (time_t / half_life))` | `decay_rem(6, 3, 2)` | `2.121320344` |
| `radiocarbon_age(real)` | `radiocarbon_age(fraction_rem)` | `func` | `real` | `-5730 * (ln(fraction_rem) / ln(2))` | `radiocarbon_age(0.5)` | `5730` |
| `enthalpy_rxn(real, real)` | `enthalpy_rxn(sum_h_prod, sum_h_react)` | `func` | `real` | `sum_h_prod - sum_h_react` | `enthalpy_rxn(6, 3)` | `3` |
| `kp_from_kc(real, real, real)` | `kp_from_kc(kc, temp_k, delta_n_gas)` | `func` | `real` | `kc * ((R_atm * temp_k) ^ delta_n_gas)` | `kp_from_kc(6, 3, 2)` | `0.363604211` |
| `gibbs_free_energy(real, real, real)` | `gibbs_free_energy(dh_j, temp_k, ds_j_k)` | `func` | `real` | `dh_j - temp_k * ds_j_k` | `gibbs_free_energy(6, 3, 2)` | `0` |
| `gibbs_from_keq(real, real)` | `gibbs_from_keq(temp_k, keq)` | `func` | `real` | `-R_gas * temp_k * ln(keq)` | `gibbs_from_keq(6, 3)` | `-54.80622483` |
| `cell_emf(real, real)` | `cell_emf(e_cathode, e_anode)` | `func` | `real` | `e_cathode - e_anode` | `cell_emf(6, 3)` | `3` |
| `gibbs_electro(real, real)` | `gibbs_electro(n_elec, e_cell_v)` | `func` | `real` | `-n_elec * F_faraday * e_cell_v` | `gibbs_electro(6, 3)` | `-1736735.978` |
| `faraday_mass(real, real, real, real)` | `faraday_mass(current_i, time_s, molar_mass, n_elec)` | `func` | `real` | `(current_i * time_s * molar_mass) / (n_elec * F_faraday)` | `faraday_mass(6, 3, 2, 4)` | `9.327842691e-05` |
| `molar_conductivity(real, real)` | `molar_conductivity(kappa_s_cm, molarity_m)` | `func` | `real` | `(kappa_s_cm * 1000) / molarity_m` | `molar_conductivity(6, 3)` | `2000` |
| `nernst_cell(real, real, real)` | `nernst_cell(e0_volts, n_elec, reaction_q)` | `func` | `real` | `e0_volts - (R_gas * 298.15 / (n_elec * F_faraday)) * ln(reaction_q)` | `nernst_cell(6, 3, 2)` | `5.994063754` |
| `crystal_density(real, real, real)` | `crystal_density(z_atoms, molar_mass, a_edge_cm)` | `func` | `real` | `(z_atoms * molar_mass) / ((a_edge_cm ^ 3) * N_A)` | `crystal_density(6, 3, 2)` | `3.736212901e-24` |
| `bragg_d_spacing(real, real, real)` | `bragg_d_spacing(n_order, wavelength, theta_deg)` | `func` | `real` | `(n_order * wavelength) / (2 * sin(deg2rad(theta_deg)))` | `bragg_d_spacing(6, 3, 2)` | `257.8833751` |

### 10.17 Module `[biology]` (`Library/biology.math` — 60 overloads)

60 functions covering Hardy-Weinberg population genetics, linkage/Chi-square, DNA/RNA/PCR molecular biology, Michaelis-Menten enzyme kinetics, cellular energetics, plant water potential, microscopy/morphometrics, cardiac/respiratory/renal physiology, population ecology, and Simpson/Shannon diversity indices.

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `hw_q_from_p(real)` | `hw_q_from_p(p_dom)` | `func` | `real` | `1 - p_dom` | `hw_q_from_p(0.7)` | `0.3` |
| `hw_p_from_q2(real)` | `hw_p_from_q2(q2_rec_freq)` | `func` | `real` | `1 - √q2_rec_freq` | `hw_p_from_q2(0.09)` | `0.7` |
| `hw_homo_dom(real)` | `hw_homo_dom(p_dom)` | `func` | `real` | `p_dom ^ 2` | `hw_homo_dom(0.7)` | `0.49` |
| `hw_hetero(real)` | `hw_hetero(p_dom)` | `func` | `real` | `2 * p_dom * (1 - p_dom)` | `hw_hetero(0.7)` | `0.42` |
| `hw_homo_rec(real)` | `hw_homo_rec(p_dom)` | `func` | `real` | `(1 - p_dom) ^ 2` | `hw_homo_rec(0.3)` | `0.49` |
| `hw_carrier_count(real, real)` | `hw_carrier_count(q2_rec_freq, pop_size)` | `func` | `real` | `{ q_val := √q2_rec_freq p_val := 1 - q_val 2 * p_val * q_val * pop_size }` | `hw_carrier_count(0.7, 1000)` | `273.3200531` |
| `recomb_freq(real, real)` | `recomb_freq(recombinants, total_offspring)` | `func` | `real` | `(recombinants / total_offspring) * 100` | `recomb_freq(6, 3)` | `200` |
| `chi_sq_term(real, real)` | `chi_sq_term(observed, expected)` | `func` | `real` | `((observed - expected) ^ 2) / expected` | `chi_sq_term(6, 3)` | `3` |
| `chargaff_g_from_a(real)` | `chargaff_g_from_a(a_pct)` | `func` | `real` | `50 - a_pct` | `chargaff_g_from_a(6)` | `44` |
| `chargaff_c_from_t(real)` | `chargaff_c_from_t(t_pct)` | `func` | `real` | `50 - t_pct` | `chargaff_c_from_t(6)` | `44` |
| `dna_length_nm(real)` | `dna_length_nm(base_pairs)` | `func` | `real` | `base_pairs * 0.34` | `dna_length_nm(6)` | `2.04` |
| `dna_turns(real)` | `dna_turns(base_pairs)` | `func` | `real` | `base_pairs / 10` | `dna_turns(6)` | `0.6` |
| `dna_weight_da(real)` | `dna_weight_da(base_pairs)` | `func` | `real` | `base_pairs * 650` | `dna_weight_da(6)` | `3900` |
| `codons_for_protein(real)` | `codons_for_protein(amino_acids)` | `func` | `real` | `amino_acids + 1` | `codons_for_protein(6)` | `7` |
| `mrna_bases_for_protein(real)` | `mrna_bases_for_protein(amino_acids)` | `func` | `real` | `3 * (amino_acids + 1)` | `mrna_bases_for_protein(6)` | `21` |
| `pcr_copies(real)` | `pcr_copies(n_cycles)` | `func` | `real` | `2 ^ n_cycles` | `pcr_copies(6)` | `64` |
| `pcr_yield_from(real, real)` | `pcr_yield_from(n0_templates, n_cycles)` | `func` | `real` | `n0_templates * (2 ^ n_cycles)` | `pcr_yield_from(6, 3)` | `48` |
| `res_digest_linear(real)` | `res_digest_linear(cut_sites)` | `func` | `real` | `cut_sites + 1` | `res_digest_linear(6)` | `7` |
| `res_digest_circular(real)` | `res_digest_circular(cut_sites)` | `func` | `real` | `cut_sites` | `res_digest_circular(6)` | `6` |
| `michaelis_menten(real, real, real)` | `michaelis_menten(vmax, km, substrate_s)` | `func` | `real` | `(vmax * substrate_s) / (km + substrate_s)` | `michaelis_menten(6, 3, 2)` | `2.4` |
| `competitive_inh_v(real, real, real, real, real)` | `competitive_inh_v(vmax, km, substrate_s, inh_i, ki)` | `func` | `real` | `(vmax * substrate_s) / (km * (1 + inh_i / ki) + substrate_s)` | `competitive_inh_v(6, 3, 2, 4, 5)` | `1.621621622` |
| `noncomp_inh_v(real, real, real, real, real)` | `noncomp_inh_v(vmax, km, substrate_s, inh_i, ki)` | `func` | `real` | `((vmax / (1 + inh_i / ki)) * substrate_s) / (km + substrate_s)` | `noncomp_inh_v(6, 3, 2, 4, 5)` | `1.333333333` |
| `enzyme_catalytic_eff(real, real)` | `enzyme_catalytic_eff(kcat, km)` | `func` | `real` | `kcat / km` | `enzyme_catalytic_eff(6, 3)` | `2` |
| `q10_rate(real, real, real, real)` | `q10_rate(r1, t1_c, t2_c, q10_coeff = 2)` | `func` | `real` | `r1 * (q10_coeff ^ ((t2_c - t1_c) / 10))` | `q10_rate(6, 3, 2, 4)` | `5.22330338` |
| `q10_rate(real, real, real)` | `q10_rate(r1, t1_c, t2_c, q10_coeff = 2) [default-arg overload]` | `func` | `real` | `r1 * (q10_coeff ^ ((t2_c - t1_c) / 10))` | `q10_rate(6, 3, 2)` | `5.598197949` |
| `q10_from_rates(real, real, real, real)` | `q10_from_rates(r1, r2, t1_c, t2_c)` | `func` | `real` | `(r2 / r1) ^ (10 / (t2_c - t1_c))` | `q10_from_rates(6, 3, 2, 4)` | `0.03125` |
| `respiratory_quotient(real, real)` | `respiratory_quotient(co2_produced, o2_consumed)` | `func` | `real` | `co2_produced / o2_consumed` | `respiratory_quotient(6, 3)` | `2` |
| `aerobic_atp_yield(real)` | `aerobic_atp_yield(glucose_moles)` | `func` | `real` | `38 * glucose_moles` | `aerobic_atp_yield(6)` | `228` |
| `water_potential(real, real)` | `water_potential(psi_s, psi_p)` | `func` | `real` | `psi_s + psi_p` | `water_potential(6, 3)` | `9` |
| `solute_potential(real, real, real)` | `solute_potential(i_ion, molarity_c, temp_k)` | `func` | `real` | `-i_ion * molarity_c * 0.0831 * temp_k` | `solute_potential(6, 3, 2)` | `-2.9916` |
| `diffusion_pressure_deficit(real, real)` | `diffusion_pressure_deficit(osmotic_p, turgor_p)` | `func` | `real` | `osmotic_p - turgor_p` | `diffusion_pressure_deficit(6, 3)` | `3` |
| `stomatal_index(real, real)` | `stomatal_index(stomata_s, epidermal_e)` | `func` | `real` | `(stomata_s / (stomata_s + epidermal_e)) * 100` | `stomatal_index(6, 3)` | `66.66666667` |
| `total_magnification(real, real)` | `total_magnification(ocular_mag, objective_mag)` | `func` | `real` | `ocular_mag * objective_mag` | `total_magnification(6, 3)` | `18` |
| `actual_specimen_size(real, real)` | `actual_specimen_size(image_size, magnification)` | `func` | `real` | `image_size / magnification` | `actual_specimen_size(6, 3)` | `2` |
| `fov_diameter(real, real, real)` | `fov_diameter(fov_low, mag_low, mag_high)` | `func` | `real` | `(fov_low * mag_low) / mag_high` | `fov_diameter(6, 3, 2)` | `9` |
| `mitotic_index(real, real)` | `mitotic_index(dividing_cells, total_cells)` | `func` | `real` | `(dividing_cells / total_cells) * 100` | `mitotic_index(6, 3)` | `200` |
| `cell_surface_sphere(real)` | `cell_surface_sphere(r)` | `func` | `real` | `4π * (r ^ 2)` | `cell_surface_sphere(6)` | `452.3893421` |
| `cell_volume_sphere(real)` | `cell_volume_sphere(r)` | `func` | `real` | `(4 / 3) * π * (r ^ 3)` | `cell_volume_sphere(6)` | `904.7786842` |
| `sa_to_vol_sphere(real)` | `sa_to_vol_sphere(r)` | `func` | `real` | `3 / r` | `sa_to_vol_sphere(6)` | `0.5` |
| `stroke_volume(real, real)` | `stroke_volume(edv_ml, esv_ml)` | `func` | `real` | `edv_ml - esv_ml` | `stroke_volume(6, 3)` | `3` |
| `ejection_fraction(real, real)` | `ejection_fraction(edv_ml, esv_ml)` | `func` | `real` | `((edv_ml - esv_ml) / edv_ml) * 100` | `ejection_fraction(6, 3)` | `50` |
| `cardiac_output(real, real)` | `cardiac_output(hr_bpm, stroke_vol_ml)` | `func` | `real` | `(hr_bpm * stroke_vol_ml) / 1000` | `cardiac_output(6, 3)` | `0.018` |
| `pulse_pressure(real, real)` | `pulse_pressure(systolic_p, diastolic_p)` | `func` | `real` | `systolic_p - diastolic_p` | `pulse_pressure(6, 3)` | `3` |
| `mean_arterial_pressure(real, real)` | `mean_arterial_pressure(systolic_p, diastolic_p)` | `func` | `real` | `diastolic_p + (systolic_p - diastolic_p) / 3` | `mean_arterial_pressure(6, 3)` | `4` |
| `minute_ventilation(real, real)` | `minute_ventilation(tidal_vol_ml, resp_rate_bpm)` | `func` | `real` | `tidal_vol_ml * resp_rate_bpm` | `minute_ventilation(6, 3)` | `18` |
| `alveolar_ventilation(real, real, real)` | `alveolar_ventilation(tidal_vol_ml, dead_space_ml, resp_rate_bpm)` | `func` | `real` | `(tidal_vol_ml - dead_space_ml) * resp_rate_bpm` | `alveolar_ventilation(6, 3, 2)` | `6` |
| `vital_capacity(real, real, real)` | `vital_capacity(tv_ml, irv_ml, erv_ml)` | `func` | `real` | `tv_ml + irv_ml + erv_ml` | `vital_capacity(6, 3, 2)` | `11` |
| `total_lung_capacity(real, real)` | `total_lung_capacity(vc_ml, rv_ml)` | `func` | `real` | `vc_ml + rv_ml` | `total_lung_capacity(6, 3)` | `9` |
| `net_filtration_pressure(real, real, real)` | `net_filtration_pressure(ghp, bcops, chp)` | `func` | `real` | `ghp - (bcops + chp)` | `net_filtration_pressure(6, 3, 2)` | `1` |
| `renal_clearance(real, real, real)` | `renal_clearance(urine_conc_u, urine_flow_v, plasma_conc_p)` | `func` | `real` | `(urine_conc_u * urine_flow_v) / plasma_conc_p` | `renal_clearance(6, 3, 2)` | `9` |
| `bmi_calc(real, real)` | `bmi_calc(mass_kg, height_m)` | `func` | `real` | `mass_kg / (height_m ^ 2)` | `bmi_calc(6, 3)` | `0.6666666667` |
| `pop_exp_growth(real, real, real)` | `pop_exp_growth(n0, r_intrinsic, time_t)` | `func` | `real` | `n0 * exp(r_intrinsic * time_t)` | `pop_exp_growth(6, 3, 2)` | `2420.572761` |
| `pop_doubling_time(real)` | `pop_doubling_time(r_intrinsic)` | `func` | `real` | `ln(2) / r_intrinsic` | `pop_doubling_time(6)` | `0.1155245301` |
| `pop_logistic_dNdt(real, real, real)` | `pop_logistic_dNdt(r_intrinsic, n_cur, k_carrying)` | `func` | `real` | `r_intrinsic * n_cur * ((k_carrying - n_cur) / k_carrying)` | `pop_logistic_dNdt(6, 3, 2)` | `-9` |
| `pop_logistic_nt(real, real, real, real)` | `pop_logistic_nt(n0, k_carrying, r_intrinsic, time_t)` | `func` | `real` | `k_carrying / (1 + ((k_carrying - n0) / n0) * exp(-r_intrinsic * time_t))` | `pop_logistic_nt(6, 3, 2, 4)` | `3.000503278` |
| `lincoln_petersen_pop(real, real, real)` | `lincoln_petersen_pop(marked_m1, total_catch_c, recaptured_m2)` | `func` | `real` | `(marked_m1 * total_catch_c) / recaptured_m2` | `lincoln_petersen_pop(6, 3, 2)` | `9` |
| `trophic_energy_at(real, real)` | `trophic_energy_at(base_energy, trophic_steps)` | `func` | `real` | `base_energy * (0.1 ^ trophic_steps)` | `trophic_energy_at(6, 3)` | `0.006` |
| `net_primary_prod(real, real)` | `net_primary_prod(gpp, respiration_r)` | `func` | `real` | `gpp - respiration_r` | `net_primary_prod(6, 3)` | `3` |
| `simpson_diversity_vec(vec)` | `simpson_diversity_vec(counts:vec)` | `func` | `real` | `{ tot   := counts.sum() numer := 0 (ni in counts) { numer += ni * (ni - 1) } ...` | `simpson_diversity_vec(vec(2, 4, 6))` | `0.6666666667` |
| `shannon_diversity_vec(vec)` | `shannon_diversity_vec(counts:vec)` | `func` | `real` | `{ tot := counts.sum() h   := 0 (ni in counts) { if (ni > 0) { p_i := ni / tot...` | `shannon_diversity_vec(vec(2, 4, 6))` | `1.011404265` |

### 10.18 Module `[user_physics]` (`Test/user_physics.math` — 44 overloads)

Sample host-loaded extension demonstrating constant `tau_const`, host `@extern` callbacks (`fx_rate`, `fx_set_rate`, `convert_fx`, `fx_with_fee`, `to_inr`, `to_usd`, `vec_sum_fx`, `fx_history`, `fx_sma`, `save_var`, `load_var`, `elem`, `molar_mass_*`), concrete & generic unit functions (`avg_speed`, `scale_unit`, `unit_span`), custom unit groups (`B`, `KB`, `MB`, `GB` in `data_size` and `bandwidth`), default parameters (`hyp_3d`, `log_base`), and multi-statement block functions (`sum_squares_to`, `vec_dot_self`, `safe_sqrt_ret`, `describe_vec2`, `grade_band`).

| Overload Signature | `.math` Declaration Header | Fixity & Prec | Return | Definition / Implementation | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| `rocket_dv(real, real, real)` | `rocket_dv(isp_s, m0_kg, mf_kg)` | `func` | `real` | `isp_s * g0 * ln(m0_kg / mf_kg)` | `rocket_dv(6, 3, 2)` | `23.85752641` |
| `fspl_ratio(real, real)` | `fspl_ratio(dist_m, lambda_m)` | `func` | `real` | `((4π * dist_m) / lambda_m) ^ 2` | `fspl_ratio(6, 3)` | `631.6546817` |
| `classify_mach(real)` | `classify_mach(m) -> str` | `func` | `str` | `{ if (m < 0.8) { "subsonic" } or (m < 1.2) { "transonic" } or (m < 5.0) { "su...` | `classify_mach(6)` | `"hypersonic"` |
| `planet_g(real)` | `planet_g(id)` | `func` | `real` | `switch (id) { case 1 : { 3.7 } case 2 : { 8.87 } case 3 : { 9.80665 } case 4 ...` | `planet_g(6)` | `0` |
| `collatz_steps(real)` | `collatz_steps(n)` | `func` | `real` | `{ steps := 0 jump(Loop) Tick : { steps += 1 } Loop : { if (n <= 1) { jump(Fin...` | `collatz_steps(6)` | `8` |
| `fx_rate(str, str)` | `fx_rate(from:str, to:str)` | `func` | `real` | `@fx_rate(from, to)` | `fx_rate("USD", "INR")` | `84` |
| `fx_set_rate(str, real)` | `fx_set_rate(ccy:str, per_usd)` | `func` | `real` | `@fx_set_rate(ccy, per_usd)` | `fx_set_rate("INR", 85)` | `85` |
| `convert_fx(real, str, str)` | `convert_fx(amount, from:str, to:str)` | `func` | `real` | `{ assert(amount >= 0, "Currency amount cannot be negative") amount * @fx_rate...` | `convert_fx(10, "USD", "INR")` | `840` |
| `fx_with_fee(real, str, str, real)` | `fx_with_fee(amount, from:str, to:str, fee_pct = 1.5)` | `func` | `real` | `{ assert(amount >= 0, "Currency amount cannot be negative") assert(fee_pct >=...` | `fx_with_fee(100, "USD", "INR", 2)` | `8232` |
| `fx_with_fee(real, str, str)` | `fx_with_fee(amount, from:str, to:str, fee_pct = 1.5) [default-arg overload]` | `func` | `real` | `{ assert(amount >= 0, "Currency amount cannot be negative") assert(fee_pct >=...` | `fx_with_fee(100, "USD", "INR")` | `8274` |
| `(real) to_inr` | `(usd) to_inr :90` | `postfix :90` | `real` | `usd * @fx_rate("USD", "INR")` | `6 to_inr` | `504` |
| `(real) to_usd` | `(eur) to_usd :90` | `postfix :90` | `real` | `eur * @fx_rate("EUR", "USD")` | `6 to_usd` | `6.52173913` |
| `vec_sum_fx(vec, str)` | `vec_sum_fx(prices_usd:vec, target_ccy:str)` | `func` | `real` | `prices_usd.sum() * @fx_rate("USD", target_ccy)` | `vec_sum_fx(vec(10, 20), "INR")` | `16800` |
| `fx_history(str, str, real)` | `fx_history(from:str, to:str, days) -> vec` | `func` | `vec` | `@fx_history(from, to, days)` | `fx_history("USD", "INR", 4)` | `[82.5, 83, 83.5, 84]` |
| `fx_sma(str, str, real, real)` | `fx_sma(from:str, to:str, days, window)` | `func` | `real` | `sma(@fx_history(from, to, days), window)` | `fx_sma("USD", "INR", 5, 3)` | `83.5` |
| `save_var(str, real)` | `save_var(key:str, val)` | `func` | `real` | `@kv_set(key, val)` | `save_var("k", 99)` | `99` |
| `load_var(str)` | `load_var(key:str)` | `func` | `real` | `@kv_get(key)` | `save_var("k", 99) ; load_var("k")` | `99` |
| `elem(str)` | `elem(sym:str)` | `func` | `real` | `@elem_mass(sym)` | `elem("C")` | `12.011` |
| `molar_mass_h2o()` | `molar_mass_h2o()` | `func` | `real` | `2 * @elem_mass("H")  + @elem_mass("O")` | `molar_mass_h2o()` | `18.015` |
| `molar_mass_co2()` | `molar_mass_co2()` | `func` | `real` | `@elem_mass("C")      + 2 * @elem_mass("O")` | `molar_mass_co2()` | `44.009` |
| `molar_mass_nacl()` | `molar_mass_nacl()` | `func` | `real` | `@elem_mass("Na")     + @elem_mass("Cl")` | `molar_mass_nacl()` | `58.44` |
| `molar_mass_glucose()` | `molar_mass_glucose()` | `func` | `real` | `6 * @elem_mass("C")  + 12 * @elem_mass("H") + 6 * @elem_mass("O")` | `molar_mass_glucose()` | `180.156` |
| `(real[distance]) + (real[distance])` | `(x:real, distance) "+" (y:real, distance) :<50 -> real, distance` | `infix_L :50` | `real[distance]` | `@add(x, y)` | `(100 m) + (100 m)` | `200 [distance]` |
| `(real[distance]) - (real[distance])` | `(x:real, distance) "-" (y:real, distance) :<50 -> real, distance` | `infix_L :50` | `real[distance]` | `@sub(x, y)` | `(100 m) - (100 m)` | `0 [distance]` |
| `(real[distance]) / (real[time])` | `(d:real, distance) "/" (t:real, time) :<60 -> real, speed` | `infix_L :60` | `real[speed]` | `@div(d, t)` | `(100 m) / (5 sec)` | `20 [speed]` |
| `(real[speed]) * (real[time])` | `(v:real, speed)    "*" (t:real, time) :<60 -> real, distance` | `infix_L :60` | `real[distance]` | `@mul(v, t)` | `(20 mps) * (5 sec)` | `100 [distance]` |
| `avg_speed(real[distance], real[time])` | `avg_speed(d:real, distance, t:real, time) -> real, speed` | `func` | `real[speed]` | `d / t` | `avg_speed((100 m), (5 sec))` | `20 [speed]` |
| `scale_unit(real, real)` | `scale_unit(x:real, T, k)      -> real, T` | `func` | `real` | `x * k` | `scale_unit(6, 3)` | `18` |
| `unit_span(real, real)` | `unit_span(a:real, T, b:real, T) -> real, T` | `func` | `real` | `@abs(a - b)` | `unit_span(6, 3)` | `3` |
| `(real) B` | `(x) B  :95 -> real, data_size` | `postfix :95` | `real[data_size]` | `x` | `6 B` | `6 [data_size]` |
| `(real) KB` | `(x) KB :95 -> real, data_size` | `postfix :95` | `real[data_size]` | `x * 1024` | `6 KB` | `6144 [data_size]` |
| `(real) MB` | `(x) MB :95 -> real, data_size` | `postfix :95` | `real[data_size]` | `x * (1024 ^ 2)` | `6 MB` | `6291456 [data_size]` |
| `(real) GB` | `(x) GB :95 -> real, data_size` | `postfix :95` | `real[data_size]` | `x * (1024 ^ 3)` | `6 GB` | `6442450944 [data_size]` |
| `(real[data_size]) / (real[time])` | `(sz:real, data_size) "/" (t:real, time) :<60 -> real, bandwidth` | `infix_L :60` | `real[bandwidth]` | `@div(sz, t)` | `(2 KB) / (5 sec)` | `409.6 [bandwidth]` |
| `(real[bandwidth]) * (real[time])` | `(bw:real, bandwidth) "*" (t:real, time) :<60 -> real, data_size` | `infix_L :60` | `real[data_size]` | `@mul(bw, t)` | `(500 B / 1 sec) * (5 sec)` | `2500 [data_size]` |
| `hyp_3d(real, real, real)` | `hyp_3d(x, y, z = 0)` | `func` | `real` | `√(x^2 + y^2 + z^2)` | `hyp_3d(6, 3, 2)` | `7` |
| `hyp_3d(real, real)` | `hyp_3d(x, y, z = 0) [default-arg overload]` | `func` | `real` | `√(x^2 + y^2 + z^2)` | `hyp_3d(6, 3)` | `6.708203932` |
| `log_base(real, real)` | `log_base(x, base = 10)` | `func` | `real` | `ln(x) / ln(base)` | `log_base(6, 3)` | `1.630929754` |
| `log_base(real)` | `log_base(x, base = 10) [default-arg overload]` | `func` | `real` | `ln(x) / ln(base)` | `log_base(6)` | `0.7781512504` |
| `sum_squares_to(real)` | `sum_squares_to(n)` | `func` | `real` | `{ s := 0 (i in 1...n) { s += i^2 } s }` | `sum_squares_to(6)` | `91` |
| `vec_dot_self(vec)` | `vec_dot_self(v:vec)` | `func` | `real` | `{ s := 0 (x in v) { s += x^2 } s }` | `vec_dot_self(vec(2, 4, 6))` | `56` |
| `safe_sqrt_ret(real)` | `safe_sqrt_ret(x)` | `func` | `real` | `{ if (x < 0) { return -1 } if (x == 0) { return 0 } res := √x return res * 10 }` | `safe_sqrt_ret(6)` | `24.49489743` |
| `describe_vec2(vec2)` | `describe_vec2(p:vec2) -> str` | `func` | `str` | `switch (p) { case vec2(0, 0)           : { "origin" } case vec2(0, y)        ...` | `describe_vec2(vec2(3, 4))` | `"general"` |
| `grade_band(real)` | `grade_band(score) -> str` | `func` | `str` | `switch (score) { case < 0, > 100 : { "invalid" } case >= 90      : { "A" } ca...` | `grade_band(6)` | `"F"` |


### 10.19 Complete Hardcoded Grammar & Language Constructs Reference (`Core/Code/parser.c` & `Core/lib/hardcode/hardcode.c`)

In addition to the `.math`-declared operators and functions in Sections 10.1–10.18, the parser and hardcode layer provide the following syntactic constructs and overloads:

| Construct Category | Syntax Form / Overload | Semantics | Verified Example | Result |
| :-- | :-- | :-- | :-- | :-- |
| **Variable Declaration (Dynamic)** | `name := expr` | Declares variable in current scope with dynamic entity & unit | `x := 15 ; x + 5` | `20` |
| **Variable Declaration (Entity-Locked)** | `name : Type := expr` | Locks variable entity type (`real`, `complex`, `vec2`, `vec<T>`…) | `x : real := 10 ; x += 5` | `15` |
| **Variable Declaration (Unit-Locked)** | `name : unit := expr` | Locks variable unit group while keeping entity dynamic | `d : distance := 50m ; d += 50im` | `50 + 50i [distance]` |
| **Variable Declaration (Entity & Unit Locked)** | `name : Type, unit := expr` | Locks both entity type and unit group | `d : real, distance := 1 km ; d + 500 m` | `1500 [distance]` |
| **Multi-Variable Scalar Broadcast** | `a, b, ... := scalar` | Declares/assigns multiple variables to the same scalar | `a, b, c := 7 ; a + b + c` | `21` |
| **Multi-Variable Destructuring Unpack** | `a, b, ... := composite` | Unpacks struct fields, `vec` elements, or `mat` entries in order | `r1, r2 := quad_roots(1, -5, 6) ; r1 * 10 + r2` | `32` |
| **Same-Scope Grouping `( ... )`** | `( stmt1 ; stmt2 ; ... )` | Evaluates statements in the current lexical scope | `(a := 3 ; b := 4 ; a + b) + a` | `10` |
| **Child Lexical Scope `{ ... }`** | `{ stmt1 ; stmt2 ; ... }` | Evaluates statements in an isolated child lexical scope | `x := 1 ; { x := 99 } ; x` | `1` |
| **Implicit Multiplication** | `2x`, `3(x+1)`, `(a+b)(c+d)`, `2 sin(x)`, `2 √16` | Inserts `*` at precedence 60 (right-binding 65 so `2x^2` is `2*(x^2)`) | `x := 4 ; 2x^2 + 3x + 1` | `45` |
| **Matrix / Tensor Literal (1D–4D)** | `[a, b, c]`, `[a, b ; c, d]`, `[[a, b], [c, d]]` | Constructs 1D–4D dense `$mat` tensor | `[1, 2 ; 3, 4] * [5, 6 ; 7, 8]` | `[[19, 22], [43, 50]]` |
| **Vector / String / Struct / Matrix Indexing** | `v[i]`, `s[i]`, `st[i]`, `M[r, c]`, `M[r][c]` | 0-based read/write indexing (negative `i` counts from end) | `v := vec(10, 20, 30) ; v[-1]` | `30` |
| **Vector / String / Matrix Range Slicing** | `v[a...b]`, `v[a...b by k]`, `s[a...b by k]`, `M[r1...r2, c1...c2]` | Extracts slice/submatrix or broadcasts assignment `v[a...b] = val` | `"abcdef"[0...5 by 2]` | `"ace"` |
| **1-Variable Vector Comprehension** | `[expr for x in iter]` / `[expr for x in iter if cond]` | Builds `$vec` over range, `vec`, `mat`, or `str` | `[x^2 for x in 1...6 if x % 2 == 0]` | `[4, 16, 36]` |
| **2-Variable Indexed Comprehension** | `[expr for i, x in iter]` / `[expr for i, x in iter if cond]` | Builds `$vec` with 0-based index `i` and element `x` | `[i * 10 + x for i, x in vec(5, 6, 7)]` | `[5, 16, 27]` |
| **Parameterized Container Constructors** | `vec<T>(...)`, `vec<T, u>(...)`, `list<T>(...)`, `stack<T>(...)`, `queue<T>(...)` | Constructs type-locked (and optionally unit-locked) `$vec` container | `v := vec<vec2>(vec2(1, 2), vec2(3, 4)) ; v[1].x` | `3` |
| **Lambda Closure (0, 1, or 2 Params)** | `() => expr`, `x => expr`, `(a, b) => { ... }` | Creates first-class `$fn` closure capturing lexical variables | `k := 5 ; f := (a, b) => (a + b) * k ; f(2, 4)` | `30` |
| **Uniform Function Call Syntax (UFCS)** | `obj.fn(a, b, ...)` | Rewrites method call to `fn(obj, a, b, ...)` when `fn` is not a struct field | `[4, 7 ; 2, 6].transpose().det()` | `10` |
| **Pipe Operator (`\|>`)** | `x \|> fn`, `x \|> fn(a, b)`, `x \|> (v => ...)` | Passes `x` as 1st argument to `fn` or invokes closure | `vec(1, 2, 3, 4) \|> map(x => x * 10) \|> sum` | `100` |
| **Multi-Branch `if / or / then / else`** | `if (c1) { b1 } or (c2) { b2 } else { b3 }` | Lazy multi-branch conditional expression | `x := 7 ; if (x < 5) { 1 } or (x < 10) { 2 } else { 3 }` | `2` |
| **Pattern-Matched `switch`** | `switch (v) { case p1, p2 if g : { ... } else : { ... } }` | Matches values, ranges, `<`/`<=`/`>`/`>=`, struct destructuring & guards | `switch (vec2(0, 9)) { case vec2(0, y) : { y * 2 } else : { 0 } }` | `18` |
| **1-Var & 2-Var `for-in` Loop** | `(x in iter) { ... }`, `x in iter { ... }`, `(i, x in iter) { ... }` | Iterates over range (O(1) memory), `vec`, `mat`, or `str` | `s := 0 ; (i, c in "abc") { s += (i + 1) } ; s` | `6` |
| **Pre-Tested `while` Loop** | `(cond) ?? { body }` | Evaluates `body` while `cond` is non-zero | `n := 1 ; (n < 16) ?? { n *= 2 } ; n` | `16` |
| **Post-Tested `do-while` Loop** | `{ body } ?? (cond)` / `(body) ?!? (cond)` | Executes `body` once before testing `cond` at the end of each pass | `k := 10 ; { k += 1 } ?? (k < 5) ; k` | `11` |
| **Tagged Scope & Loop Control** | `Tag : { ... }`, `break(Tag, v)`, `continue(Tag)`, `jump(Tag)`, `run(Tag)` | Named scope/loop for multi-level `break`/`continue`, `jump`, and `run` | `Outer : (i in 1...4) { (j in 1...4) { if (i*j == 9) { break(Outer, i*10+j) } } }` | `33` |
| **Dual Variable/Unit `in` Disambiguation** | `25km in m`, `25m in km`, `25m in 1km`, `25m in (km)` | Resolves RHS of `in` as unit conversion or container membership by context | `km := vec(10, 25) ; (25m in km) * 1000 + (25 in km)` | `26` |



### 10.20 Six Engine & Language Expansions Reference (Verified 22 October 2026)

1. **Arbitrary-Arity (0–16 Params), Default-Argument & Mutually Recursive Lambdas + Session Function Definitions (`:=` and `=`)**:
   - Up to **16 parameters** (`MEM_FN_MAX_PARAMS = 16`) on both anonymous lambdas `(a1, ..., a16) => expr` and named functions `f(a1, ..., a16) := expr` or `f(a1, ..., a16) = expr`.
   - **Optional default parameters**: `f(x, y = 10, z = 100) := x + y + z` and `(x, k = 3, off = 1) => x * k + off` evaluate default expressions dynamically in the callee frame when omitted.
   - **Direct & forward-referenced mutual recursion**: `is_even_r(n) := n == 0 ? 1 : is_odd_r(n - 1) ; is_odd_r(n) := n == 0 ? 0 : is_even_r(n - 1)` and `ack(m, n) = ...` resolve sibling closures dynamically across statements and session evaluations, and serialize losslessly in `MSP1|...` session snapshots.
2. **Linear Algebra Decompositions (`LU`, `QR`, `Cholesky`, `Eigen`, `SVD`, `pinv`, `lstsq`) & Coupled ODE Systems**:
   - **Matrix-Vector & Structural Ops**: `M * v`, `v * M`, `outer(u, v)`, `kron(A, B)`, `vstack(A, B)`, `hstack(A, B)`, `tril(A, k = 0)`, `triu(A, k = 0)`, plus `==`, `!=`, `~` on `$mat` and `$vec`.
   - **Decompositions & Solvers**: `lu_l(A)`, `lu_u(A)`, `lu_p(A)` ($PA = LU$), `qr_q(A)`, `qr_r(A)` (modified Gram-Schmidt QR), `chol(A)` (Cholesky $LL^T = A$), `eigvals(A)`, `eigvecs(A)` (exact 2×2 characteristic roots & cyclic Jacobi symmetric solver), `power_iter(A, steps = 40)`, `power_vec(A, steps = 40)`, `svd_s(A)`, `svd_u(A)`, `svd_v(A)` (one-sided Jacobi SVD), `pinv(A)` (Moore-Penrose pseudoinverse), `lstsq(A, b)` (SVD least-squares solver), `matrix_rank(A)`, `cond(A)`, `mat_norm1(A)`, `mat_norm_inf(A)`, `spectral_norm(A)`.
   - **Coupled & System ODE Solvers (4th-Order Runge-Kutta)**:
     - `ode_rk4_sys(f, t0, y0, t1, steps = 100)` and lazy `ode_rk4_sys(t, y, rhs_vec, t0, y0, t1, steps = 100)`
     - `ode_rk4_sys_traj(t, y, rhs_vec, t0, y0, t1, steps = 50)` (returns $(steps + 1) \times N$ trajectory matrix)
     - `ode_coupled2(t, x, y, dx_dt, dy_dt, t0, x0, y0, t1, steps = 100)` and `ode_coupled3(t, x, y, z, dx, dy, dz, t0, x0, y0, z0, t1, steps = 100)`
3. **Ready-to-Use Android NDK JNI Bridge (`Core/jni/` & `JNI/`) + Interactive Session REPL (`--repl`)**:
   - C11 JNI entry points (`Java_com_mathengine_MathEngineBridge_*` in `Core/jni/math_engine_jni.h` & `.c`) with self-contained JNI type definitions when `<jni.h>` is absent, paired with `JNI/MathEngineBridge.java` and `JNI/MathEngineBridge.kt`.
   - Enhanced `--repl` in `Test/main.c` supporting `ans` auto-binding and `:help`, `:vars`, `:snaps`, `:compact`, `:modules`, `:enable <mod>`, `:disable <mod>`, `:diff <var> <expr>`, `:simplify <expr>`, `:load <file.math>`, `:reset`, and `:quit`.
4. **Symbolic Expression Differentiation, Algebraic Simplification & Symbolic Taylor Series**:
   - `sym_simplify(expr)` / `simplify(expr)`: Recursive algebraic identity reduction (`x + 0 -> x`, `x * 1 -> x`, `x * 0 -> 0`, `x - x -> 0`, `x / x -> 1`, `(x^a)^b -> x^(a*b)`), constant folding, and like-term combination (`2*x + 3*x -> 5 * x`).
   - `sym_diff(x, expr)` and `sym_diff_n(x, expr, order)`: Exact analytical differentiation across `+`, `-`, `*`, `/`, `^` (general power + logarithmic differentiation), `sin`, `cos`, `tan`, `sec`, `csc`, `cot`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `exp`, `ln`, `log10`, `log2`, `sqrt`/`√`, `cbrt`, `abs` with automatic simplification.
   - `sym_eval(sym_str, x, val)` and `sym_taylor(x, expr, x0 = 0, order = 4)`, plus C APIs `eng_sym_simplify` and `eng_sym_diff`.
5. **String Formatting (`fmt` / `format`), Backtracking Regex Engine & First-Class `$map` Dictionary Type**:
   - **Formatting & String Ops**: `fmt(template, ...)` (`{}`, `{:.2f}`, `{:x}`, `{:X}`, `{:b}`, `{:d}`, `{:e}`) and `format(template, ...)` (`%s`, `%d`, `%04d`, `%.2f`, `%g`, `%x`, `%X`, `%b`, `%%`), plus `upper`, `lower`, `trim`, `ltrim`, `rtrim`, `pad_left`, `pad_right`, `index_of`, `contains`, `str_count`, `replace`, `replace_first`, `split`, `lines`, `words`.
   - **Regex Engine**: `regex_match(s, pat)`, `regex_find(s, pat)`, `regex_find_all(s, pat)`, `regex_replace(s, pat, repl)`, `regex_split(s, pat)` supporting `^`, `$`, `.`, `*`, `+`, `?`, `[abc]`, `[a-z]`, `[^...]`, `\d`, `\D`, `\w`, `\W`, `\s`, `\S`, `(group)`, and `|`.
   - **First-Class `$map` Dictionary**: `map()`, `dict("k1", v1, "k2", v2, ...)`, `m["k"]` read/insert/update/`+=`, `"k" in m` (`has_key`), `m.get_or("k", def)`, `m.remove("k")`, `m.keys()`, `m.vals()`, `m.entries()`, `m.len()`, `m.clear()`, `(k in m) { ... }`, `(k, v in m) { ... }`, `[expr for k, v in m]`, JSON-style `m_` (`{"k1": v1, ...}`) & `_"{...}"` round-trip, and session snapshot persistence.
6. **Arena Defragmentation / Mark-Sweep Sliding Compaction on Scratch Pool (`mem_compact_scratch`)**:
   - 6-stage Mark-Sweep / Sliding Compaction over `f64_pool[perm_pool_top .. pool_top)` tracing roots from active call stack frames (`mem->stack[0..sp)`), closure captures (`mem->fns`), and caller roots, sliding live blocks down to eliminate abandoned reallocation holes and updating all internal `data_idx` pointers.
   - Exposed in MEP as `gc_compact()` and `pool_used()`, in C as `eng_compact_scratch` and `eng_scratch_pool_used`, and triggered automatically at loop boundaries when scratch pool usage exceeds 75%.

## 11. Calculus and Solvers

These are lazy constructs: the first argument names a variable (or two variables `t, y` for `ode_rk4`), and the expression is re-evaluated for each sample of that variable.

| Call | Overload Signatures | Meaning | Example |
| :-- | :-- | :-- | :-- |
| `diff(x, expr, x0)` | 3-arg | First derivative (5-point stencil) | `diff(x, x^3 + 2*x, 3)` is `29` |
| `diff2(x, expr, x0)` | 3-arg | Second derivative | `diff2(x, x^4, 2)` is `48` |
| `diff_n(x, expr, x0, n)` | 4-arg | $n$-th derivative ($n \in 1..6$) | `diff_n(x, x^4, 2, 3)` is `48` |
| `integrate(x, expr, a, b[, steps])`, `ntg(x, expr, a, b)` | 4-arg & 5-arg | Composite Simpson's rule integration | `integrate(x, 3*(x^2), 0, 2)` is `8` |
| `lim(x, expr, x0)`, `lim_left`, `lim_right` | 3-arg | Limits with Richardson extrapolation | `lim(x, sin(x)/x, 0)` is `1` |
| `sigma(k, expr, lo, hi)`, `prod(k, expr, lo, hi)` | 4-arg | Discrete summation and product | `sigma(k, k^2, 1, 5)` is `55` |
| `solve(x, eq)`, `solve(x, eq, guess)`, `solve(x, eq, a, b)` | 2-arg, 3-arg, 4-arg | Hybrid Newton–secant–bisection root finder | `solve(x, x^2 == 4, 1)` is `2` |
| `fmin(x, expr, a, b)`, `fmax(x, expr, a, b)` | 4-arg | Golden-section extremum finder on $[a, b]$ | `fmin(x, (x-2)^2, 0, 5)` is `2` |
| `ode_rk4(t, y, dy_dt, t0, y0, t1[, steps])` | 6-arg & 7-arg | 4th-order Runge-Kutta IVP ODE solver | `ode_rk4(t, y, y, 0, 1, 1)` is `2.718281828` |

Two-sided `lim` rejects jump discontinuities with `Two-sided limit does not exist (left -1 != right 1)`.

Polynomials are coefficient vectors `[c0, c1, ..., cn]`. `poly_val`, `poly_deriv`, `poly_integ(c, C = 0)`, `poly_add`, `poly_mul`, `poly_newton(c, x0, iters = 25)` are in `algebra`; `calculus` adds `poly_eval`, `poly_diff`, `poly_diff_at`, `poly_diff2_at`, `poly_def_integ`, `tangent_slope`, `normal_slope`, `tangent_y`, `normal_y`, `poly_curvature`, `quad_critical_x(a, b, c = 0)`, `quad_extremum_y`, `poly_newton_root`, `poly_avg_val`, `poly_arc_length`, `poly_vol_disk`, `poly_vol_shell`, `trapz_vec`, `simpson_vec`, `taylor_exp(x, terms = 12)`, `taylor_sin(x, terms = 10)`, `taylor_cos(x, terms = 10)`.

## 12. Writing Extensions

An extension is UTF-8 text with `#` comments. The host passes it to `user_compile_unit` + `user_link_ext` (or `user_compile_and_link` / `user_compile_ext`); the engine does not care about file names or numerical prefixes and resolves `<[dep]` dependencies topologically. A compile or link error leaves the engine exactly as it was.

| Declaration | Form |
| :-- | :-- |
| Module | `[my_ext]` header, then `<[arith]` for each dependency. Text without a header joins `default`. |
| Constant | `tau_const = 2π` or `"name" : real = 3.14` |
| Function (shorthand) | `hyp_3d(x, y, z = 0) = √(x^2 + y^2 + z^2)`. Untyped parameters are `real`, the result defaults to `real`, and each default value synthesises a shorter overload. |
| Function (typed) | `avg_speed(d:real, distance, t:real, time) -> real, speed = d / t`. The body can be a `{ ... }` block using `return`, `if`, loops, and newline statement separation. |
| Prefix operator | `"√"(x) :80 = @sqrt(x)` or `not(x) :25 = @lnot(x)` |
| Postfix operator | `(n) "²" :95 = n * n` or `(x) km :95 -> real, distance = x * 1000` |
| Infix operator | `(a) "<>" (b) :<30 = @ne(a, b)` or `(a) plus (b) :<45 = a + b`, where `:<` is left-associative and `:>` is right-associative |
| Struct type | `$point(px, py)` (fields default to `real`, or `$circle(center:vec2, radius)`), constructed with `point(3, 4)`; self-referential structs are rejected |
| Dimensional unit group (`@dim`) | `@dim speed = m^1 * s^-1` binds a unit group to its 7D SI base exponents $(\text{m}, \text{kg}, \text{s}, \text{A}, \text{K}, \text{mol}, \text{cd})$ |
| Custom unit group | name a new group after the result type: `(x) KB :95 -> real, data_size = x * 1024` |
| Unit-typed parameters | `(d:real, distance)`; the generic `T` accepts any group and preserves it: `unit_span(a:real, T, b:real, T) -> real, T = @abs(a - b)` |

## 13. Embedding API and Limits

| Function | Purpose |
| :-- | :-- |
| `eng_create()`, `eng_create_blank()`, `eng_destroy(eng)` | heap-allocate & initialize an isolated `Engine` (~683.7 KB) and free it |
| `eng_init(&eng)` | boot all standard modules into a static/caller-allocated `Engine` |
| `eng_init_blank(&eng)` | empty engine with no types or operators (for tools and tests) |
| `eng_eval(&eng, src, &out)` | evaluate a stateless script; returns false and sets the error on failure |
| `eng_format_value(&eng, v, buf, cap)` | render any value to text |
| `eng_val_type`, `eng_type_name`, `eng_val_unit`, `eng_unit_name` | inspect a result's entity type and unit group |
| `eng_last_error(&eng)` | last error message (127 characters max) |
| `eng_set_module`, `eng_is_module_active` | toggle modules by name in O(1) |
| `eng_register_extern(&eng, name, fn, ctx)` | bind `@name(...)` to a C callback |
| `eng_session_open`, `eng_session_close`, `eng_session_reset`, `eng_session_eval` | parallel in-memory evaluation sessions |
| `eng_session_snapshot_next`, `eng_session_snapshot_rewind`, `eng_session_snapshot_count`, `eng_session_snapshot_at`, `eng_session_restore` | iterative order-tagged raw-text snapshots & session continuation |
| `user_build_create`, `user_build_destroy`, `user_build_init`, `user_compile_unit`, `user_link_ext`, `user_link_ext_buf`, `user_compile_and_link`, `user_compile_ext`, `user_load_mathc` | separated compilation, topological `<[dep]` linking, and little-endian `.mathc` binary cache load |

**Capacity limits** (compile-time constants in `mem_arena.h`, `sym_table.h`, `compiler.h`, `mathc_bin.h`, and `session.h`):

| Resource | Limit |
| :-- | :-- |
| Expression length | 65,534 bytes |
| String pool (`MEM_SRC_BUF_CAP`) | 49,152 bytes |
| AST nodes (`MEM_MAX_AST_NODES`) | 16,384 |
| Float pool (`MEM_MAX_F64_POOL`) | 8,192 doubles (~90×90 matrix max; returns `MEM_NO_IDX` on exhaustion) |
| First-class lambdas (`MEM_MAX_FN_POOL`) | 16 closures per evaluation |
| Call stack (`MEM_MAX_CALL_STACK`) / local variables (`PARSER_MAX_LOCALS`) | 256 slots / 64 locals per scope |
| Evaluation depth / parse nesting | 128 |
| Evaluation step budget | 2,000,000 per eval |
| Tagged scopes | 16 |
| Modules / types / unit groups | 32 / 64 / 64 |
| Overloads / symbols / constants / externs | 2,048 / 2,048 / 128 / 32 |
| Parallel in-memory sessions / snapshots / vars per session | 8 (`ENG_MAX_SESSIONS`) / 32 (`ENG_SESSION_MAX_EVALS`) / 16 (`ENG_SESSION_MAX_VARS`) |

## 14. Quirks and Notes

- **`++`, `--`, `+++`, `---` do not modify variables.** They are pure operators returning `x + 1`, `x - 1`, `x + 2`, `x - 2`, in prefix or postfix form. `a := 5 ; a++ ; a` is `5`. Use `a += 1`.
- **Stateless `eng_eval` vs. Stateful `eng_session_eval`.** Variables do not persist between stateless `eng_eval` calls; use `eng_session_eval` when you want variables, locks, containers, and closures to persist across evaluations in a session. Only extensions, constants, and host state persist across stateless `eng_eval` calls.
- **`**` is only the `vec3` dot product** and binds at 20, below `+`, so parenthesise it. For `vec2` use `dot(a, b)`.
- **Prefix `!` vs `not` vs `~`**: `~` is infix approximate equality (`<= 0.001`), while boolean NOT is `!` (precedence 80) or `not` (precedence 25, binding below relational comparisons so `not x == 5` works without parentheses). Both `!` and `not` require a boolean `0` or `1` operand.
- **Golden ratio**: `phi`, `φ`, and `ϕ` are all defined and equal `1.618033989`.
- **Ranges**: `1..5` is exclusive `[2, 3, 4]`, `1...5` is inclusive `[1, 2, 3, 4, 5]`, `1..<5` is `[1, 2, 3, 4]`, and `1<..5` is `[2, 3, 4, 5]`.

## 15. Common Error Messages

| Message | Cause |
| :-- | :-- |
| `Undefined symbol or constant 'x'` | unknown name, or a variable from an earlier stateless `eng_eval` call |
| `Module '[name]' is already loaded` | supplying the same named `[module]` raw text a second time |
| `Duplicate declaration of 'fn' in module '[default]'` | supplying the same headerless `[default]` raw text a second time |
| `Unresolved module dependency '<[dep]' required by '[mod]'` | linking a module whose `<[dep]` was neither loaded nor staged |
| `Circular module dependency involving '[mod]'` | cycle in `<[dep]` declarations across staged modules |
| `Incomplete session snapshots: missing evaluation seq:k` | restoring a session with a missing evaluation snapshot in `1..count` |
| `Broken session snapshot chain at seq:k` | mixing snapshots from different sessions or out-of-sequence hash chain |
| `Corrupted session snapshot checksum mismatch at seq:k` | modified or truncated snapshot string passed to `eng_session_restore` |
| `Unexpected trailing token 'y'` | two statements on one line without `;` |
| `Unexpected token in expression` | incomplete expression such as `1 +` |
| `No active overload for '**' with types (real, real)` | no definition matches the operand types |
| `No compatible type found for 'xyz'` | prefix `_` string could not be parsed into any valid non-string type |
| `Unit mismatch for '+' with (real [distance], real [angle])` | adding incompatible unit groups |
| `Type mismatch: variable holds concrete type 'real', cannot assign 'complex'` | assigning against a type lock |
| `Cannot use reserved keyword 'if' as a variable name` | keyword used as a name or tag |
| `Vector type mismatch: cannot push 'vec2' into vec<real>` | heterogeneous vector or typed `vec<T>` violation |
| `Vector unit mismatch: cannot push unit 'angle' into vec<real, distance>` | unit lock violation on `vec<T, unit>` |
| `Struct 'vec2' constructor expects 2 field argument(s), got 1` | wrong number of arguments passed to a struct constructor |
| `Matrix rows/sub-tensors must have matching dimensions` | jagged matrix literal |
| `Operand 5 out of range for boolean negation '!': expected 0 or 1` | `!` or `not` on a non-boolean scalar |
| `Division by zero` | divisor is exactly zero |

## 16. Index of Annotated Construct Sections in `Core/lib/delegate/` and `Core/lib/hardcode/`

Every section in `Core/lib/delegate/` (`delegate.h`, `delegate.c`) and `Core/lib/hardcode/` (`hardcode.h`, `hardcode.c`) is marked with a two-line banner explaining what it handles and which language construct it implements:

### 16.1 `Core/lib/delegate/delegate.h` & `Core/lib/delegate/delegate.c`

1. **Intrinsic Lookup Table (`k_delegate_intrinsics` / `delegate_lookup_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @f64..@fromstr (intrinsic lookup table)*/`
   - `/*Construct @<intrinsic> symbol-to-IntrinsicId resolution in .math declarations*/`
2. **Additive Unit Compatibility (`delegate_is_zero_unitless_scalar`, `delegate_check_additive_unit_compat`)**:
   - `/*Below section is the C's native for operator delegate @add, @sub, @eq..@approx (additive unit compatibility)*/`
   - `/*Construct Same-unit enforcement & zero-scalar allowance on +, -, ==, !=, <, <=, >, >=, ~*/`
3. **Dynamic Vector, List, Stack & Queue Delegates (`delegate_eval_vec_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @vnew, @vfill, @vpush, @vpop, @vshift, @vpeek, @vget, @vset, @vlen, @vcap, @vresize, @vshrink, @vclear, @vrange, @vrange_step, @vin*/`
   - `/*Construct Dynamic vector ($vec<T>), list<T>, stack<T>, queue<T>, range (.., ..., ..<, <.., by), and membership (in) delegates*/`
4. **First-Class Lambda & Higher-Order Collection Delegates (`delegate_eval_fn_hof_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @fcall, @vmap, @vfilter, @vreduce, @vany, @vall, @vfind, @vcount*/`
   - `/*Construct First-class lambda invocation ($fn) & higher-order collection pipeline delegates (.map, .filter, .reduce, .fold, .any, .all, .find, .count_if)*/`
5. **Multidimensional Matrix/Tensor Delegates (`delegate_eval_mat_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @mzero, @mfill, @meye, @mrows, @mcols, @mrank, @mlen, @mshape, @mget, @mset, @madd, @msub, @mmul, @mscal, @mtrans, @mdet, @mtrace, @minv, @mlinsolve, @mhadamard*/`
   - `/*Construct Multidimensional matrix/tensor ($mat) creation, indexing, arithmetic, Hadamard .*, ./, .^, transpose, determinant, trace, Gauss-Jordan inverse, and NxN linear solver delegates*/`
6. **Lazy Calculus & Solver Meta-Intrinsics (`delegate_eval_calculus_meta`)**:
   - `/*Below section is the C's native for operator delegate @calc_diff, @calc_diff2, @calc_diffn, @calc_integ, @calc_lim, @calc_lim_l, @calc_lim_r, @calc_sigma, @calc_prod, @calc_solve, @calc_fmin, @calc_fmax, @calc_ode*/`
   - `/*Construct Numerical differentiation (diff/diff2/diff_n), Simpson integration (integrate/ntg), Richardson limits (lim), discrete sum/product (sigma/prod), root solver (solve), extrema (fmin/fmax), and 4th-order Runge-Kutta ODE solver (ode_rk4)*/`
7. **String Conversion & Scalar Math Delegates (`delegate_format_value_str`, `delegate_eval_fromstr`, `delegate_eval_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @tostr, @fromstr, @type_name, @unit_name, @err, @add..@raw*/`
   - `/*Construct Postfix "_" (@tostr plain string conversion), prefix "_" (@fromstr string-to-most-appropriate-type parser), introspection (@type_name/@unit_name), and scalar arithmetic/trig/comparison delegates*/`

### 16.2 `Core/lib/hardcode/hardcode.h` & `Core/lib/hardcode/hardcode.c`

1. **Reserved Keywords & Generic Wildcards**:
   - `/*code section below manages hardcoding the reserved control-flow keywords and generic type/unit wildcards*/`
   - `/*Construct if, or, then, else, default, switch, case, jump, run, break, continue, return & generic T/any/none*/`
2. **Default Parameter Overload Synthesis**:
   - `/*code section below manages hardcoding the automatic synthesis of lower-arity overloads for default parameters*/`
   - `/*Construct fn(a, b = default_expr) default parameter synthesis in .math declarations*/`
3. **Assignment & Relational Operator Classification**:
   - `/*code section below manages hardcoding the classification of assignment, compound-assignment, and relational symbols*/`
   - `/*Construct :=, =, +=, -=, *=, /=, %=, ^= and relational/additive operators ==, !=, <, <=, >, >=, ~*/`
4. **Implicit Multiplication & Default Newline `;` Statement Separation**:
   - `/*code section below manages hardcoding implicit multiplication juxtaposition and newline-as-semicolon statement separation*/`
   - `/*Construct Implicit multiplication (2x, 3(x+1), 2 sin(x)) and default newline ';' statement separator*/`
5. **Lambda Lookahead & Comprehension Variable Scanning**:
   - `/*code section below manages hardcoding lookahead for parenthesized lambdas and vector comprehension iterator variables*/`
   - `/*Construct (a, b) => expr lambda lookahead and [expr for x in iter if cond] / [expr for i, x in iter if cond] comprehension variable pre-registration*/`
6. **Dual Variable/Unit Disambiguation for `"25km in m"` & Unparenthesized Iterator Variables**:
   - `/*code section below manages hardcoding the differentiating "25km in m" when "m" is also declared a variable or vector*/`
   - `/*Construct Dual variable/unit disambiguation for "in" (25km in m, 25m in km, 25m in (km), 25m in 1km) and undefined loop iterator variables (i in 1...5 {}, i, x in v {})*/`
7. **Parameterized Container Nomenclature (`vec<T>()`, `list<T>()`, `stack<T>()`, `queue<T>()`)**:
   - `/*code section below manages hardcoding the parameterized container nomenclature vec<T>(), list<T>(), stack<T>(), and queue<T>()*/`
   - `/*Construct Generic type-annotated container constructors vec<T>(...), list<T>(...), stack<T>(...), queue<T>(...) and variadic evaluation*/`
8. **Pattern-Matched `switch`, Keywordless `for-in` Iteration & Destructuring Unpack**:
   - `/*code section below manages hardcoding runtime evaluation of pattern-matched switch equality, for-in loops, and destructuring unpack*/`
   - `/*Construct switch (expr) { case ... }, (x in iter) { ... } / [expr for x in iter], and a, b := rhs destructuring*/`
