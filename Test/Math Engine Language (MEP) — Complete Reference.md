# Math Engine Language (MEP) — Complete Reference & Tutorial Guidebook

*Verified against the engine source and a live sanitizer build (`gcc -std=c11 -Wall -Wextra -Wconversion -Werror -fsanitize=address,undefined`), 20 October 2026. This single reference & tutorial guidebook in `Test/` merges the complete language specification, chapter-by-chapter tutorial walkthroughs, and all new language features.*

## 1. Overview & Architecture

MEP is an extension-driven expression language and C11 engine for scientific and everyday math. A small C core provides memory, parsing, and evaluation; almost everything else (operators, units, types, functions) is defined in text extensions (`.math`) that are compiled into the engine at start-up.

Key properties:

- **Embeddable.** The engine is plain C11, makes zero `malloc` calls, never touches the filesystem, and runs from fixed-size arenas (`sizeof(Engine) ≈ 247 KB`, `sizeof(Value) == 8`, `sizeof(AstNode) == 8`). It targets the Android NDK but builds anywhere.
- **Extensible.** Operators, precedences, types, unit groups, constants, and functions are all declared in `.math` source, not hard-coded in C.
- **Unit-aware.** Every value carries an entity type (`real`, `complex`, `vec2`…) and an orthogonal unit tag (`distance`, `angle`, `mass`…). Mixing incompatible units is an error.
- **Isolated evaluations.** Each call to `eng_eval` is a fresh script: variables you declare do not survive to the next call (section 4.4). Loaded extensions and host callbacks do.

### 1.1 Repository Layout & Modular Separation of Concerns

- **`Core/Code/` & `Core/Headers/` (Core Pipeline)**: Arena (`mem_arena`), symbol table (`sym_table`), pattern-driven lexer (`lexer`), Pratt parser (`parser`), AST evaluator (`evaluator`), and top-level engine wrapper (`math_engine`).
- **`Core/compile/` (Compilation, Topological Linking, `.mathc` Binary & User Loader)**:
  - `compiler.h` / `compiler.c`: `[module]` unit staging (`ext_compile_unit`), duplicate module rejection, and declaration compiler (`ext_compile_slice`).
  - `linker.h` / `linker.c`: `<[dep]` dependency DAG resolution, missing/circular dependency detection via Kahn's topological sort, and transactional linking (`ext_link_units`, `ext_compile_and_link`, `ext_load_str`).
  - `mathc_bin.h` / `mathc_bin.c`: 40-byte `MathcHeader`, FNV-1a checksum, pre-copy structural validator, and O(1) `.mathc` serializer/loader (`ext_save_mathc_mem`, `ext_load_mathc_mem`).
  - `user_loader.h` / `user_loader.c`: Host-facing user extension compile, link, and `.mathc` load API (`user_compile_unit`, `user_link_ext`, `user_compile_and_link`, `user_compile_ext`, `user_load_mathc`).
  - `ext_loader.h`: Umbrella header including `compiler.h`, `linker.h`, and `mathc_bin.h`.
- **`Core/session/` (Parallel In-Memory Sessions & Order-Tagged Snapshots)**:
  - `session.h` / `session.c`: Isolated parallel session lifecycle (`eng_session_open`, `eng_session_close`, `eng_session_reset`) and stateful evaluation (`eng_session_eval`).
  - `snapshot.h` / `snapshot.c`: Canonical value expression serializer, order-tagged 32-bit FNV-1a integrity hash-chained raw-text snapshot builder (`MSP1|seq:...|prev:...|hash:...`), iterative snapshot reader (`eng_session_snapshot_next`), and full-history session continuation (`eng_session_restore`).
- **`Core/lib/delegate/` (`delegate.h` / `delegate.c` — C `@` Intrinsics)**: Contains the `@<intrinsic>` registry and all C implementations behind `@` delegates (`@add`, `@sin`, `@vpush`, `@vshift`, `@vpeek`, `@tostr`, `@fromstr`, `@mmul`, `@calc_integ`…). Every section in `delegate.h` and `delegate.c` is annotated with `/*Below section is the C's native ...*/` and `/*Construct ...*/` banners (section 16).
- **`Core/lib/hardcode/` (`hardcode.h` / `hardcode.c` — Hardcoded Grammar & Ergonomic Rules)**: Isolates the grammar rules fixed in C (newline statement separators, reserved words, implicit multiplication, undefined iterator variables `i in 1...5 {}`, dual unit/variable `"in"` disambiguation `25m in km`, parameterized containers `vec<T>()` / `list<T>()` / `stack<T>()` / `queue<T>()`, comprehensions, lambdas, and pattern-matched `switch`). Every section in `hardcode.h` and `hardcode.c` is annotated with `/*code section below manages hardcoding ...*/` and `/*Construct ...*/` banners (section 16).
- **`Library/*.math` (Standard Library)**: Order-independent `.math` modules (no numerical prefixes) compiled and linked by `Library/mathc.c` into `stdlib_precompiled.h` (`Library/stdlib.mathc`) and loaded in one `memcpy` by `eng_init`.
- **`Test/`**: Contains the host harness & 322-test verification suite (`Test/main.c`), sample user extension (`Test/user_physics.math`), and this canonical guidebook (`Test/Math Engine Language (MEP) — Complete Reference.md`).

## 2. Quick Start & Tutorial Tour

### 2.1 Embedding in C (`math_engine.h`)

```c
#include "math_engine.h"
#include <stdio.h>

/* Engine (~693 KB) holds isolated arenas + 256 KB .mathc buffer: use eng_create() or static */

int main(void) {
    if (!eng_init(&g_eng)) { fprintf(stderr, "%s\n", eng_last_error(&g_eng)); return 1; }
    Value out;
    if (eng_eval(&g_eng, "ke(2 kg, 36 kph)", &out)) {
        char buf[128];
        eng_format_value(&g_eng, out, buf, sizeof buf);
        printf("%s\n", buf);              /* 100 */
    } else {
        fprintf(stderr, "%s\n", eng_last_error(&g_eng));
    }
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
[x^2 for x in 1...10 if x % 2 == 0]    # [4, 16, 36, 64, 100]
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

**Numbers** are decimal: digits with an optional fractional part (`42`, `3.14159`). There is no exponent notation (`1e3` is read as `1` followed by the identifier `e3`) and no digit separators. Negative numbers are the unary minus operator applied to a literal. Results print with 10 significant digits (`1/3` prints `0.3333333333`; very large values use `1.23456789e+11`).

**Strings** are double-quoted literals with no escapes (`"USD"`). They support `==`, `!=`, `type_of`, being returned from functions, being passed to host callbacks, and converting to/from typed values via the `"_"` operators (section 5.1).

**Constants** `π` and `pi` (`3.141592654`), `e` (`2.718281828`), and `phi`, `φ`, and `ϕ` (golden ratio `1.618033989`) are predefined in `[arith]`.

**Statements** are separated by a newline or by `;`:

```text
x := 10
y := 20
x + y            # 30   (newline acts as ';')

x := 10 ; y := 20 ; x + y     # on one line, ';' is compulsory
x := 10 y := 20               # ERROR: Unexpected trailing token 'y'
```

A line ending in an infix operator continues onto the next line. A line starting with a prefix-capable operator (`-x`, `!c`, `_"42"`) starts a new statement. Implicit multiplication never crosses a newline. A trailing `;` before `}`, `)`, `]` or end of input is ignored.

**Reserved words** cannot be variable or parameter names: `if or then else default switch case jump run break continue return`. (`loop` can be used as a loop keyword or as a user scope tag `loop : { ... }`.)

## 4. Types, Units and Variables

### 4.1 Entity Types

| Type | Meaning | Example |
| :-- | :-- | :-- |
| `real` | 64-bit IEEE-754 float | `42`, `3.14` |
| `iota` | pure imaginary scalar | `5i`, `(3 + 4)i` |
| `complex` | complex struct (`re`, `im`) | `1 + 2i`, `5 into 5i` |
| `str` | UTF-8 string slice | `"Euler"`, `42_` |
| `vec` | growable homogeneous container (`vec<T>`, `list<T>`, `stack<T>`, `queue<T>`) | `vec<real>(1, 2, 3)` |
| `mat` | 1D–4D dense tensor | `[1, 2 ; 3, 4]` |
| `fn` | first-class lambda closure | `x => x^2` |
| `vec2`, `vec3`, `circle`, `mat2` | fixed-layout structs | `vec2(1, 2)` |

`type_of(x)` returns the entity type name as a string; `unit_of(x)` returns the unit group name (`"distance"`, or `"none"`).

### 4.2 Units

Postfix unit operators bind at precedence 95 and normalise to SI base units, tagging the value with its unit group: `100 km` is `100000 [distance]`, `90 deg` is `1.5708 [angle]`. Addition and subtraction require matching groups: `50m + 20cm` gives `50.2 [distance]`, while `100cm + 60°` fails with `Unit mismatch for '+'`. Multiplying by a plain number keeps the unit; dividing across groups can produce a new one (`distance / time` gives `speed`). Groups are declared in `.math` files, so you can invent your own (section 12).

Because postfix operators only fire after a complete operand, `m := 5 ; m + 10` treats `m` as a variable, while `50m` is fifty metres. `50im` and `50mi` both give `50i [distance]`.

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

`{ ... }` opens a lexical scope and evaluates to its last expression. `:=` inside it shadows an outer variable; `=` mutates it:

```text
x := 1 ; { x := 99 } ; x          # 1
x := 1 ; { x = 99 } ; x           # 99
{ a := 3 ; b := 4 ; a^2 + b^2 }   # 25
```

Variables live only for the current `eng_eval` call. `eng_eval("x := 10")` followed by `eng_eval("x")` fails with `Undefined symbol or constant 'x'`. Pass multi-step work as one script, or keep state in the host through extern callbacks (section 13).

## 5. Operators and Precedence

Higher numbers bind tighter. Everything here is declared in the library, so extensions can add to it.

| Prec | Kind | Operators |
| --: | :-- | :-- |
| 95 | postfix / member | `.` field or UFCS call, `v[i]`, `f(x)`, `i` (imaginary), `_` (`@tostr` stringifier), unit suffixes (`km`, `deg`, `°`, `kg`, `hr`…) |
| 90 | postfix | `!` factorial, `to_<unit>` conversions, `++ +++ -- ---` |
| 85 | right infix, prefix | `^` exponent (`-x^2 = -(x^2)`, `2^3^2 = 512`); prefix `++ +++ -- ---`, prefix `_` (`@fromstr` string-to-type parser) |
| 80 | prefix | `-`, `+`, `√`, `!` (boolean NOT) |
| 60 | left infix | `*`, `/`, `%`, and implicit multiplication (`60 -> 65`) |
| 55 | left/right infix | `in` (left, membership and unit conversion), `into` (right, composite constructor & container insert) |
| 50 | left infix | `+`, `-` |
| 45 | left infix | `..` (exclusive), `...` (inclusive), `..<` (right-open), `<..` (left-open), `plus`, `minus` |
| 35 / 34 | left infix | `&`, `\|` (distributed combinators) |
| 30 | left infix | `==  !=  <  <=  >  >=`, `~` (approximately equal) |
| 25 | prefix | `not` |
| 20 | left infix | `&&`, `and`, `nand`, and `**` (vec3 dot product) |
| 18 / 15 | left infix | `xor`, `^^` / `\|\|`, `or`, `nor` |
| 10 | right infix | `cond ? a : b` |
| 8 | right infix | `=>` lambda |
| 5 | right infix | `:=  =  +=  -=  *=  /=  %=  ^=` |
| 3 | left infix | `??` while-loop |
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
| `5 into 5`, `50m into 20cm`, `5i into 3i`, `(1+2i) into (3+4i)`, `vec2 into vec2`, `vec3 into vec3` | same entity & unit | **Likewise entities & units return their sum**: `5 into 5` is `10`, `50m into 20cm` is `50.2 [distance]`, `5i into 3i` is `8i` |
| `5 into 5i`, `5i into 5` | `complex` | Cross-entity `real` + `iota` combines into `5 + 5i` (`complex(5, 5)`) |
| `2 into (3 + 4i)` | `complex` | Adds real component into complex: `5 + 4i` |
| `3i into (3 + 4i)` | `complex` | Adds imaginary component into complex: `3 + 7i` |
| `7 into vec2(3, 4)` or `vec2(3, 4) into 7` | `vec3` | Promotes 2D `vec2(x, y)` + `z` into 3D `vec3(x=3, y=4, z=7)` |
| `item into v` (`v` is `vec`/`list`/`stack`/`queue`) | `vec` | Appends `item` into `v` (`@vpush(v, item)`), acting as an infix alternative to `v.push(item)` |

Because `"into"` is right-associative, `10 into 20 into vec<real>()` chains naturally (`20` is pushed first, then `10`, yielding `[20, 10]`), and `v := vec<real>() ; 10 into v ; 20 into v` yields `[10, 20]`.

### 5.3 Implicit Multiplication, Booleans, Approximate Equality & Complex Numbers

- **Implicit multiplication** inserts `*` when a value, variable, or `)` is directly followed by an identifier, `(`, or `√`: `x := 4 ; 2x^2 + 3x + 1` is `45` (the `^` binds first), `3(x+1)(x-1)` works, and `2 3` is a syntax error.
- **Booleans** are reals: `0` is false, anything non-zero is true, and comparisons return `1` or `0`. Prefix `!` flips only `0` and `1`; `!5` raises `Operand 5 out of range for boolean negation '!': expected 0 or 1`. Use `not` or `!(x == 5)` for general conditions.
- **Approximate equality** `a ~ b` is true when `|a - b| <= 0.001`: `5.001 ~ 5` is `1`, `5.01 ~ 5` is `0`.
- **Distributed comparisons** let one comparison fan out over `&`, `|`, and `!`: `x == 1 | 3` means `(x == 1) | (x == 3)`, `(x & y == 1 | 2)` means `((x == 1) | (x == 2)) & ((y == 1) | (y == 2))`, and `x == !(1 | 2)` means `!((x == 1) | (x == 2))`.
- **Complex numbers.** The postfix `i` makes an imaginary scalar and is typed by multiple dispatch: `1 + 1i` (`1 + 1i`), `(3 + 4)i` (`7i`), `((3 + 4)i)i` (`-7`), `(1 + 2i)i` (`-2 + 1i`), `(3 + 4i) / (1 - 2i)` (`-1 + 2i`).
- **Four Range Operators.**
  - `a..b` (2-dot): **exclusive–exclusive** `(a, b)` — `1..5` builds `[2, 3, 4]`.
  - `a...b` (3-dot): **inclusive–inclusive** `[a, b]` — `1...5` builds `[1, 2, 3, 4, 5]`.
  - `a..<b`: **inclusive–exclusive** `[a, b)` — `1..<5` builds `[1, 2, 3, 4]`.
  - `a<..b`: **exclusive–inclusive** `(a, b]` — `1<..5` builds `[2, 3, 4, 5]`.

## 6. Control Flow

Every construct is an expression with a value.

**Conditionals.** `if` takes `or` for else-if branches and `else` for the fallback; a 3-argument function form also exists.

```text
x := 7
if (x < 5) { "small" } or (x < 10) { "medium" } else { "large" }   # "medium"
if(1, 10, 20)                      # 10
x == 7 ? "yes" : "no"              # ternary
```

**`switch`** matches exact values, inclusive/exclusive ranges, relational guards, and destructuring patterns; patterns combine with `,` and take an optional `if` guard. Cases end with `else`.

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

**Loops & Unparenthesized Iterator Variables.** Either a parenthesized header `(var in iterable) { ... }` or an **unparenthesized** header `var in iterable { ... }` (or `var in iterable ?? { ... }`) automatically declares `var` as the loop iterator variable and loops over a range, `vec`, or `mat`—while standalone `undef_var in 1...10` without a loop body still fails with `Undefined symbol or constant 'undef_var'`. `loop` accepts a range, a condition, or nothing. `(cond) ?? { ... }` is a lazy while.

```text
sum := 0 ; (i in 1...5) { sum += i } ; sum                  # 15
sum := 0 ; i in 1...5 { sum += i } ; sum                    # 15 (unparenthesized iterator)
sum := 0 ; k in 1..<4 ?? { sum += k * 10 } ; sum            # 60
k := 0 ; loop (k < 4) { k += 1 } ; k                        # 4
k := 0 ; loop { k += 1 ; if (k >= 7) { break } } ; k        # 7
n := 0 ; (n < 5) ?? { n += 2 } ; n                          # 6
```

**Jumps.** `break`, `break expr`, `continue`, `return`, `return expr`. Loops evaluate to the value of `break expr`:

```text
found := (x in 1...20) { if (x^2 > 50) { break x } }   # 8
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
| `vec<T>(...)` / `vec(...)` | `vec<T>()`, `vec<T>(cap)`, `vec<T>(count, init)`, `vec<T>(a, b, c, ...)` | Growable homogeneous vector; locks element type to `T` (and optional unit in `vec<real, distance>(...)`) even when created empty |
| `list<T>(...)` / `list(...)` | `list<T>()`, `list<T>(a)`, `list<T>(a, b, ...)` | Element-wise list constructor (1 or 2 args are elements, never capacity/fill) |
| `stack<T>(...)` / `stack(...)` | `stack<T>()`, `stack<T>(a, b, ...)` | LIFO stack sublist supporting `.push(x)` / `x into s`, `.pop()`, `.peek()` / `.top()` / `.back()`, `.is_empty()` |
| `queue<T>(...)` / `queue(...)` | `queue<T>()`, `queue<T>(a, b, ...)` | FIFO queue sublist supporting `.enqueue(x)` / `.push(x)` / `x into q`, `.dequeue()` / `.shift()` / `.pop_front()`, `.front()`, `.is_empty()` |

All elements in a container must share the locked element type `T` (and unit lock, if specified):
- `v := vec<vec2>() ; 10 into v` raises `Vector type mismatch: cannot push 'real' into vec<vec2>`.
- `q := queue<real, distance>() ; q.enqueue(60 deg)` raises `Vector unit mismatch`.

| Call / Operator | Result |
| :-- | :-- |
| `push(v, x)`, `v.push(x)`, `x into v` | append to back (mutates `v`) |
| `pop(v)`, `v.pop()` | remove and return last element (LIFO pop) |
| `peek(v)`, `top(v)`, `back(v)` | inspect last element without removing (`@vpeek(v, -1)`) |
| `enqueue(q, x)`, `q.enqueue(x)` | append to back of queue (`@vpush(q, x)`) |
| `dequeue(q)`, `shift(q)`, `pop_front(q)` | remove and return first element (`@vshift(q)`, FIFO dequeue) |
| `front(q)`, `q.front()` | inspect first element without removing (`@vpeek(q, 0)`) |
| `is_empty(v)`, `v.is_empty()` | returns `1` if `len(v) == 0`, else `0` |
| `at(v, i)`, `set(v, i, x)`, `v[i]` | read / write; negative `i` counts from the end |
| `len`, `cap`, `resize(v, n)`, `shrink`, `clear` | size and capacity management |
| `sum`, `mean`, `variance`, `stddev`, `sma(v, w)` | statistical reductions (`variance` is population variance) |

Slicing returns a new vector and also assigns: `v := vec(10, 20, 30, 40, 50) ; v[1...3]` is `[20, 30, 40]`, `v[0..4]` is `[20, 30, 40]`, `v[-3...-1]` is `[30, 40, 50]`, `v[3...1]` reverses, and `v[1...3] = 99` broadcasts.

### 7.2 Matrices (`mat`)

Literals use `[...]`: `[1, 2, 3]` (1D), `[[1, 2], [3, 4]]` or `[1, 2 ; 3, 4]` (2D), and nested forms up to four dimensions. Index with `M[r, c]` or `M[r][c]`, and slice with ranges: `M[0...1, 1...2]` gives a submatrix, `M[2, 0...1]` a row.

Arithmetic: `A + B`, `A - B`, `A * B` (matrix product), `s * A`, `A / s`, `-A`. Functions: `det`, `trace`, `transpose`, `inv` (Gauss–Jordan), `mat_zeros(r, c)`, `mat_ones(r, c)`, `mat_eye(n)`, `mat_fill(r, c, v)`, `mat_rows`, `mat_cols`, `mat_rank`, `mat_len`, `mat_shape`. Example: `[[1,2],[3,4]] * [[5,6],[7,8]]` is `[[19, 22], [43, 50]]`.

### 7.3 Comprehensions

`[expr for x in iterable if cond]` builds a vector from a range, `vec` or `mat`:

```text
[x^2 for x in 1...5]                      # [1, 4, 9, 16, 25]
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

| Type | Fields | Notes |
| :-- | :-- | :-- |
| `complex` | `re`, `im` | `conj`, `cabs`, `carg`, `cexp_i`, `polar`; full `+ - * /`; `into` composition |
| `vec2` | `x`, `y` | `+`, `-`, scalar `*`; `dot`, `cross2`, `vec2_mag`, `vec2_dist`; `z into vec2` -> `vec3` |
| `vec3` | `x`, `y`, `z` | as vec2, plus `a ** b` dot product, `cross3`, `dot3`, `lerp` |
| `circle` | `center:vec2`, `radius` | `circle_area`, `circle_circum`, `move_circle` |
| `mat2` | `a b c d` | `det`, `trace`, `transpose`, `inv_mat2`, `m * vec2` |

Structs are built by calling the type name (`vec2(3, 4)`, `circle(vec2(3, 7), 5)`) and support field access (`v.x`) and C-style indexing for reading and writing, including nested access:

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
2. **Postfix `to_<unit>`** (precedence 90) converts and returns a plain number: `5 km to_m` is `5000`, `100 cm to_m` is `1`, `90 deg to_rad` is `1.570796327`, `10 kph to_mps` is `2.777777778`.
3. **Pairwise functions** `<from>_to_<to>(x)`: `km_to_m`, `deg_to_rad`, `kg_to_g`, `hr_to_sec`, `atm_to_Pa`, `kWh_to_J` and about 100 more.

Temperature uses functions rather than suffixes: `c_to_k`, `k_to_c`, `c_to_f`, `f_to_c`, `f_to_k`, `k_to_f`.

**Unit suffixes by group** (all precedence 95, normalised to SI):

| Group | Suffixes |
| :-- | :-- |
| angle (rad) | `deg`, `°`, `rad`, `arcmin`, `arcsec` |
| distance (m) | `m`, `km`, `cm`, `mm`, `um`, `nm`, `mi`, `nmi`, `yd`, `ft`, `inch` |
| mass (kg) | `kg`, `gram`, `mg`, `ton`, `lb`, `oz`, `amu` |
| time (s) | `ms`, `us`, `ns`, `sec`, `min_t`, `hr`, `day` |
| speed (m/s) | `mps`, `kph`, `mph`, `knots` |
| pressure (Pa) | `Pa`, `kPa`, `MPa`, `bar`, `atm`, `psi`, `torr` |
| energy (J) | `J`, `kJ`, `MJ`, `cal`, `kcal`, `Wh`, `kWh`, `eV` |
| power (W) | `W`, `kW`, `MW`, `hp` |
| unitless ratios | `pct` (x 0.01), `dB` (amplitude ratio), `dBm` (watts) |

## 10. Standard Library Catalogue

`eng_init` loads 14 named modules plus an implicit `default` module (where headerless user extensions land). Each module can be switched off and on in O(1) with `eng_set_module`; disabling `units` makes `100 km` fail with `Undefined symbol or constant 'km'`.

| Module | Contents |
| :-- | :-- |
| `boot_syntax`, `boot_types`, `boot_control` | brackets, `$real $iota $str $vec $mat $fn`, assignment, loops, `if/switch`, jumps, `_` (`@tostr`/`@fromstr`), `into`, `vec<T>/list<T>/stack<T>/queue<T>`, sublist helpers (`enqueue`, `dequeue`, `shift`, `pop_front`, `peek`, `top`, `back`, `front`, `is_empty`), collection and calculus constructs, `type_of`, `unit_of`, `error`, `assert` |
| `arith` | operators; `abs floor ceil round(x, digits = 0) trunc fract sign min max clamp abs_diff is_even is_odd is_close gcd lcm npr ncr fib is_prime mod_pow`; `sum mean variance stddev sma`; constants `π pi e phi φ ϕ`; commercial maths |
| `trig` | `sin cos tan sec csc cot asin acos atan atan2 sinh cosh tanh asinh acosh atanh ln log10 log2 logb exp sqrt cbrt root hypot sigmoid` |
| `complex_ext` | `complex`, `conj`, `cabs`, `carg`, `cexp_i`, `polar`, complex `into` overloads |
| `geom_ext` | `vec2`, `vec3`, `circle`; `dot`, `cross2`, `cross3`, `dot3`, `vec2_mag`, `vec3_mag`, `vec2_dist`, `vec3_dist`, `lerp`, `circle_area`, `circle_circum`, `sphere_vol`, `sphere_area`, `vec2` $\to$ `vec3` `into` overloads |
| `algebra` | `mat2`, `det`, `trace`, `transpose`, `inv`, `inv_mat2`, `solve_2x2`, `quad_disc`, `quad_roots`, `quad_croot`, polynomial helpers, `mat_*` |
| `calculus` | polynomial and geometric calculus (section 11) |
| `units`, `conversions` | section 9 |
| `physics` | about 100 functions and constants |
| `chemistry` | about 60 functions and constants |
| `biology` | about 60 functions |

## 11. Calculus and Solvers

These are lazy constructs: the first argument names a variable, and the expression is re-evaluated for each sample of that variable.

| Call | Meaning | Example |
| :-- | :-- | :-- |
| `diff(x, expr, x0)` | first derivative (5-point stencil) | `diff(x, x^3 + 2*x, 3)` is `29` |
| `diff2(x, expr, x0)`, `diff_n(x, expr, x0, n)` | higher derivatives | `diff2(x, x^4, 2)` is `48` |
| `integrate(x, expr, a, b[, steps])`, `ntg` | Simpson's rule | `integrate(x, 3*(x^2), 0, 2)` is `8` |
| `lim`, `lim_left`, `lim_right` | limits with Richardson extrapolation | `lim(x, sin(x)/x, 0)` is `1` |
| `sigma(k, expr, lo, hi)`, `prod(...)` | discrete sum and product | `sigma(k, k^2, 1, 5)` is `55` |
| `solve(x, eq[, guess])`, `solve(x, eq, a, b)` | root finding | `solve(x, x^2 == 4, 1)` is `2` |
| `fmin(x, expr, a, b)`, `fmax(...)` | extremum on an interval | `fmin(x, (x-2)^2, 0, 5)` is `2` |

Two-sided `lim` rejects jump discontinuities with `Two-sided limit does not exist (left -1 != right 1)`.

Polynomials are coefficient vectors `[c0, c1, ..., cn]`. `poly_val`, `poly_deriv`, `poly_integ`, `poly_add`, `poly_mul`, `poly_newton` are in `algebra`; `calculus` adds `poly_eval`, `poly_diff`, `poly_diff_at`, `poly_def_integ`, `tangent_slope`, `normal_slope`, `poly_curvature`, `quad_critical_x`, `poly_arc_length`, `poly_vol_disk`, `poly_vol_shell`, `trapz_vec`, `simpson_vec`, `taylor_exp`, `taylor_sin`, `taylor_cos`.

## 12. Writing Extensions

An extension is UTF-8 text with `#` comments. The host passes it to `user_compile_ext`; the engine does not care about file names or extensions. A compile error leaves the engine exactly as it was.

| Declaration | Form |
| :-- | :-- |
| Module | `[my_ext]` header, then `<[arith]` for each dependency. Text without a header joins `default`. |
| Constant | `tau_const = 2π` or `"name" : real = 3.14` |
| Function (shorthand) | `hyp_3d(x, y, z = 0) = √(x^2 + y^2 + z^2)`. Untyped parameters are `real`, the result defaults to `real`, and each default value synthesises a shorter overload. |
| Function (typed) | `avg_speed(d:real, distance, t:real, time) -> real, speed = d / t`. The body can be a `{ ... }` block using `return`, `if`, loops, and newline statement separation. |
| Prefix operator | `"√"(x) :80 = @sqrt(x)` or `not(x) :25 = @lnot(x)` |
| Postfix operator | `(n) "²" :95 = n * n` or `(x) km :95 -> real, distance = x * 1000` |
| Infix operator | `(a) "<>" (b) :<30 = @ne(a, b)` or `(a) plus (b) :<45 = a + b`, where `:<` is left-associative and `:>` is right-associative |
| Struct type | `$point(px, py)` (fields default to `real`, or `$circle(center:vec2, radius)`), constructed with `point(3, 4)` |
| Unit group | name a new group after the result type: `(x) KB :95 -> real, data_size = x * 1024` |
| Unit-typed parameters | `(d:real, distance)`; the generic `T` accepts any group and preserves it: `unit_span(a:real, T, b:real, T) -> real, T = @abs(a - b)` |

## 13. Embedding API and Limits

| Function | Purpose |
| :-- | :-- |
| `eng_create()`, `eng_create_blank()`, `eng_destroy(eng)` | heap-allocate & initialize an isolated `Engine` (~693 KB) and free it |
| `eng_init(&eng)` | boot all standard modules into a static/caller-allocated `Engine` |
| `eng_init_blank(&eng)` | empty engine with no types or operators (for tools and tests) |
| `eng_eval(&eng, src, &out)` | evaluate a script; returns false and sets the error on failure |
| `eng_format_value(&eng, v, buf, cap)` | render any value to text |
| `eng_val_type`, `eng_type_name`, `eng_val_unit`, `eng_unit_name` | inspect a result |
| `eng_last_error(&eng)` | last error message (127 characters max) |
| `eng_set_module`, `eng_is_module_active` | toggle modules by name |
| `eng_register_extern(&eng, name, fn, ctx)` | bind `@name(...)` to a C callback |
| `eng_session_open`, `eng_session_close`, `eng_session_reset`, `eng_session_eval` | parallel in-memory evaluation sessions |
| `eng_session_snapshot_next`, `eng_session_snapshot_rewind`, `eng_session_snapshot_count`, `eng_session_snapshot_at`, `eng_session_restore` | iterative order-tagged raw-text snapshots & session continuation |
| `user_build_create`, `user_build_destroy`, `user_build_init`, `user_compile_unit`, `user_link_ext`, `user_link_ext_buf`, `user_compile_and_link`, `user_compile_ext`, `user_load_mathc` | separated compilation, topological `<[dep]` linking, and little-endian `.mathc` binary cache load |

**Capacity limits** (compile-time constants in `mem_arena.h` and `sym_table.h`):

| Resource | Limit |
| :-- | :-- |
| Expression length | 65,534 bytes |
| String pool | 32,768 bytes |
| AST nodes | 8,192 |
| Float pool (vectors, structs, matrices) | 8,192 doubles (~90×90 matrix max; returns `MEM_NO_IDX` on exhaustion) |
| First-class lambdas (`MEM_MAX_FN_POOL`) | 16 closures per evaluation |
| Call stack | 256 slots |
| Evaluation depth / parse nesting | 128 |
| Evaluation step budget | 2,000,000 per `eng_eval` |
| Tagged scopes | 16 |
| Modules / types / unit groups | 32 / 64 / 64 |
| Overloads / symbols / constants / externs | 2,048 / 2,048 / 128 / 32 |

## 14. Quirks and Notes

- **`++`, `--`, `+++`, `---` do not modify variables.** They are pure operators returning `x + 1`, `x - 1`, `x + 2`, `x + 2`, in prefix or postfix form. `a := 5 ; a++ ; a` is `5`. Use `a += 1`.
- **Variables do not persist between `eng_eval` calls.** Only extensions, constants and host state do.
- **`**` is only the `vec3` dot product** and binds at 20, below `+`, so parenthesise it. For `vec2` use `dot(a, b)`.
- **Prefix `!` vs `~`**: `~` is infix approximate equality (`<= 0.001`), and boolean NOT is `!` (or `not`).
- **Golden ratio**: `phi`, `φ`, and `ϕ` are all defined and equal `1.618033989`.
- **Ranges**: `1..5` is exclusive `[2, 3, 4]`, `1...5` is inclusive `[1, 2, 3, 4, 5]`, `1..<5` is `[1, 2, 3, 4]`, and `1<..5` is `[2, 3, 4, 5]`.

## 15. Common Error Messages

| Message | Cause |
| :-- | :-- |
| `Undefined symbol or constant 'x'` | unknown name, or a variable from an earlier `eng_eval` call |
| `Unexpected trailing token 'y'` | two statements on one line without `;` |
| `Unexpected token in expression` | incomplete expression such as `1 +` |
| `No active overload for '**' with types (real, real)` | no definition matches the operand types |
| `No compatible type found for 'xyz'` | prefix `_` string could not be parsed into any valid non-string type |
| `Unit mismatch for '+' with (real [distance], real [angle])` | adding incompatible unit groups |
| `Type mismatch: variable holds concrete type 'real', cannot assign 'complex'` | assigning against a type lock |
| `Cannot use reserved keyword 'if' as a variable name` | keyword used as a name or tag |
| `Vector type mismatch: cannot push 'vec2' into vec<real>` | heterogeneous vector or typed `vec<T>` violation |
| `Vector unit mismatch: cannot push unit 'angle' into vec<real, distance>` | unit lock violation on `vec<T, unit>` |
| `Matrix rows/sub-tensors must have matching dimensions` | jagged matrix literal |
| `Operand 5 out of range for boolean negation '!': expected 0 or 1` | `!` on a non-boolean |
| `Division by zero` | divisor is exactly zero |

## 16. Index of Annotated Construct Sections in `Core/lib/delegate/` and `Core/lib/hardcode/`

Every section in `Core/lib/delegate/` (`delegate.h`, `delegate.c`) and `Core/lib/hardcode/` (`hardcode.h`, `hardcode.c`) is marked with a two-line banner explaining what it handles and which language construct it implements:

### 16.1 `Core/lib/delegate/delegate.h` & `Core/lib/delegate/delegate.c`

1. **Intrinsic Lookup Table (`k_intrinsics` / `delegate_lookup_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @f64..@fromstr (intrinsic lookup table)*/`
   - `/*Construct @<intrinsic> symbol-to-IntrinsicId resolution in .math declarations*/`
2. **Additive Unit Compatibility (`delegate_is_zero_unitless_scalar`, `delegate_check_additive_unit_compat`)**:
   - `/*Below section is the C's native for operator delegate @add, @sub, @eq..@approx (additive unit compatibility)*/`
   - `/*Construct Same-unit enforcement & zero-scalar allowance on +, -, ==, !=, <, <=, >, >=, ~*/`
3. **Dynamic Vector, List, Stack & Queue Delegates (`eval_vec_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @vnew, @vfill, @vpush, @vpop, @vshift, @vpeek, @vget, @vset, @vlen, @vcap, @vresize, @vshrink, @vclear, @vrange, @vin*/`
   - `/*Construct Dynamic vector ($vec<T>), list<T>, stack<T>, queue<T>, range (.., ..., ..<, <..), and membership (in) delegates*/`
4. **First-Class Lambda & Higher-Order Collection Delegates (`eval_fn_hof_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @fcall, @vmap, @vfilter, @vreduce, @vfold, @vany, @vall, @vfind, @vcount*/`
   - `/*Construct First-class lambda invocation ($fn) & higher-order collection pipeline delegates (.map, .filter, .reduce, .fold, .any, .all, .find, .count_if)*/`
5. **Multidimensional Matrix/Tensor Delegates (`eval_mat_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @mzero, @mfill, @meye, @mrows, @mcols, @mrank, @mlen, @mshape, @mget, @mset, @madd, @msub, @mmul, @mscale, @mneg, @mtr, @mdet, @mtrace, @minv*/`
   - `/*Construct Multidimensional matrix/tensor ($mat) creation, indexing, arithmetic, transpose, determinant, trace, and Gauss-Jordan inverse delegates*/`
6. **Lazy Calculus & Solver Meta-Intrinsics (`delegate_eval_calculus_meta`)**:
   - `/*Below section is the C's native for operator delegate @calc_diff, @calc_diff2, @calc_diffn, @calc_integ, @calc_lim, @calc_lim_l, @calc_lim_r, @calc_sigma, @calc_prod, @calc_solve, @calc_fmin, @calc_fmax*/`
   - `/*Construct Numerical differentiation (diff/diff2/diff_n), Simpson integration (integrate/ntg), Richardson limits (lim), discrete sum/product (sigma/prod), root solver (solve), and extrema (fmin/fmax)*/`
7. **String Conversion & Scalar Math Delegates (`delegate_format_value_str`, `eval_fromstr_intrinsic`, `delegate_eval_intrinsic`)**:
   - `/*Below section is the C's native for operator delegate @tostr, @fromstr, @typeof, @unitof, @error, @add..@raw*/`
   - `/*Construct Postfix "_" (@tostr plain string conversion), prefix "_" (@fromstr string-to-most-appropriate-type parser), introspection (@typeof/@unitof), and scalar arithmetic/trig/comparison delegates*/`

### 16.2 `Core/lib/hardcode/hardcode.h` & `Core/lib/hardcode/hardcode.c`

1. **Reserved Keywords & Generic Wildcards**:
   - `/*code section below manages hardcoding the reserved control-flow keywords and generic type/unit wildcards*/`
   - `/*Construct if, or, then, else, default, switch, case, loop, jump, run, break, continue, return & generic T/any/none*/`
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
   - `/*Construct (a, b) => expr lambda lookahead and [expr for x in iter if cond] comprehension variable pre-registration*/`
6. **Dual Variable/Unit Disambiguation for `"25km in m"` & Unparenthesized Iterator Variables**:
   - `/*code section below manages hardcoding the differentiating "25km in m" when "m" is also declared a variable or vector*/`
   - `/*Construct Dual variable/unit disambiguation for "in" (25km in m, 25m in km, 25m in (km), 25m in 1km) and undefined loop iterator variables (i in 1...5 {})*/`
7. **Parameterized Container Nomenclature (`vec<T>()`, `list<T>()`, `stack<T>()`, `queue<T>()`)**:
   - `/*code section below manages hardcoding the parameterized container nomenclature vec<T>(), list<T>(), stack<T>(), and queue<T>()*/`
   - `/*Construct Generic type-annotated container constructors vec<T>(...), list<T>(...), stack<T>(...), queue<T>(...) and variadic evaluation*/`
8. **Pattern-Matched `switch`, Keywordless `for-in` Iteration & Destructuring Unpack**:
   - `/*code section below manages hardcoding runtime evaluation of pattern-matched switch equality, for-in loops, and destructuring unpack*/`
   - `/*Construct switch (expr) { case ... }, (x in iter) { ... } / [expr for x in iter], and a, b := rhs destructuring*/`
