# Cmath Engine

A lightweight, fast, embeddable **C11 mathematical programming engine** built on
a *blank-slate* architecture. The engine ships with **zero hardcoded math,
types, or operators** — every delimiter, type, operator, function, constant,
control structure, and unit type group is compiled from modular `.math` files
into a portable `.mathc` binary image. Modules can be enabled or disabled in
**O(1)** via a bitmask flip.

The design is a deliberate **three-way separation of concerns**:

- **`Core/Code/` + `Core/Headers/`** — the engine pipeline (arena, symbol table,
  lexer, Pratt parser, extension loader, AST evaluator).
- **`Core/lib/delegate/`** — every C-backed `@` intrinsic (`@add`, `@sin`,
  `@vnew`, `@mzero`, `@calc_diff`, `@calc_solve`, ...).
- **`Core/lib/hardcode/`** — the small set of *hardcoded* grammar/ergonomic rules
  (reserved keywords, default `:real`/`-> real` fallback, implicit
  multiplication, keywordless loops, destructuring), kept explicitly isolated so
  they are auditable in one place.
- **`Library/`** — all operators, precedences, types, units, structs, constants,
  and scientific functions.

The engine core (`Core/`) has **no filesystem, path, or networking
dependencies**. The host frontend owns all file I/O; the engine compiles
`.math` text and loads `.mathc` images strictly in memory, and can call back
into host-provided C functions (`@extern`) for things like live web APIs,
databases, or sensor reads.

## Highlights

- **Blank-slate core** — `eng_init_blank()` rejects even `1 + 2`. Nothing is
  built in.
- **Embedded precompiled stdlib** — `eng_init()` boots the entire scientific
  standard library from an embedded byte image in O(1) `memcpy` time
  (zero parsing, zero I/O).
- **Pure in-memory user extensions** — `user_compile_ext` compiles raw `.math`
  text into host-managed `.mathc` bytes; `user_load_mathc` reloads them in O(1).
- **AOT `.math` → `.mathc` + C-header compiler** (`mathc`) for build-time baking.
- **Host extern callbacks** — `eng_register_extern()` binds `@name(...)` calls
  in `.math` to native C functions (HTTP, storage, hardware, ...).
- **Rich control flow** — `{ }` scope blocks, `if ... or ... else`, `switch`/
  `case`, ternary `? :`, `Tag : { ... }` labeled scopes with `jump`/`run`, and
  `break`/`break(Tag)`. Plus keywordless `(i in 1...5) { ... }` /
  `(v in vec) { ... }` loops and the `loop` keyword.
- **Streamlined `.math` ergonomics** — unquoted shorthand declarations
  (`fn(a, b) = ...`, `c = ...`), optional `:real`/`-> real` types, default
  parameters (`round(x, digits = 0)`), implicit multiplication (`2x`, `3(x + 1)`,
  `2π`), range (`1..5`, `1...5`) & membership (`x in vec`) operators, and UFCS
  dot-chaining (`v.push(10).vec_sum()`).
- **Solvers & extrema** — the general equation solver `solve(x, expr == val, guess)`
  plus `fmin`/`fmax` over an interval.
- **Expressive assignments & destructuring** — multi-variable declarations
  `x, y := 5`, compound assignments (`+=`, `-=`, `*=`, `/=`, `%=`, `^=`),
  struct/vector/matrix destructuring (`r1, r2 := quad_roots(1, -5, 6)`), and
  dynamic slot promotion (`x := 5 ; x = x + 5i` → `5 + 5i : complex`).
- **Custom errors & assertions** — `error("msg")` and `assert(cond, "msg")` abort
  evaluation with a user message.
- **Distributed boolean logic** — `&& || ! and or not nor xor nand`, plus the
  comparison shorthand `(x & y == 1 | 2)`.
- **Orthogonal `(EntityType, UnitType)` tags** — values carry a type *and* a
  unit group. Unit groups (`distance`, `angle`, `mass`, `speed`, `pressure`,
  ...) are registered from `.math`, so users can define their own.
- **Concrete variable locks** — `x : real := 5`, `x : distance := 50m`,
  `x : real, distance := 50m`, `x, y : real, distance := 50m`.
- **Generic unit `T`** — same-unit operators/functions preserve the unit
  (`scale_unit`, `unit_span`); cross-unit operators transform dimensions
  (`distance / time -> speed`).
- **Same-unit conversions** — the universal infix operator `(2.5 km) in (1 m)`,
  unit-checked postfix operators (`to_m`, `to_rad`, `to_psi`, ...), and pairwise
  functions (`km_to_m`, `deg_to_rad`, ...).
- **Strict boolean `!` & approximate equality `~`** — `!` flips only boolean
  conditions (`!(x == 5)`, distributed `(x == !(1 | 2))`); `~` tests closeness
  (`5.001 ~ 5`).
- **Array & tensor literals** — C-style `v[i]` subscripting for vectors/structs,
  and a dedicated `$mat` N-dimensional matrix/tensor type with `[...]` literals,
  `M[r, c]` indexing, matrix arithmetic, `det`/`trace`/`transpose`/`inv`/`mat_rank`.
- **Calculus** — derivatives (`diff`, `diff2`, `diff_n`), definite integrals
  (`integrate`/`ntg`), two-sided & one-sided limits (`lim`, `lim_left`,
  `lim_right`), discrete `sigma`/`prod`, Taylor series, arc length, and solids of
  revolution.
- **Broad curriculum** — commercial/general arithmetic (percentages, interest,
  EMI, work & time, progressions), high-school physics, chemistry, and
  quantitative biology.
- **Pattern-driven lexer** — consuming `$type` regex patterns.
- **Pratt parser** — operator fixity and precedence are data.
- **Multiple dispatch** — operators/functions overloaded on operand types
  *and* unit groups.
- **Generic dynamic vectors** — growable/shrinkable `$vec` of *any* `T`, with
  variadic literals `vec(10, 20, 30)` / `list(a, ...)` and `a..b` / `a...b` ranges.
- **Flat C-style structs** — `$vec2`, `$vec3`, `$circle`, `$mat2`, `$complex`.
- **Rich standard library** — arithmetic/combinatorics/number theory/stats,
  trigonometry, complex numbers, geometry, algebra (matrices, quadratics,
  polynomials), calculus, units & conversions, physics, chemistry, and biology.
- **Pure 8-byte NaN-boxed values** — every `Value` is a single 64-bit register
  (`sizeof(Value) == 8`); unitless `real`s are stored inline with zero pool
  allocation, and everything else lives in the quiet-NaN payload space.
- **Reusable `mathengine` library** — the core builds as a static library
  (`libmathengine.a`) ready for host embedding or Android NDK integration.
- **Guardrailed evaluation** — bounded AST nesting depth (128), a per-eval step
  budget, oversized-input rejection, and transactional extension rollback.
- **UTF-8 aware** — `√`, `π`, `°`, `i`, `^`, `!` and more work natively.
- **Hardened builds** — fortify source, stack protector, ASan/UBSan when available.

## Architecture

```
Host Application (Test/main.c)
     │   Owns all platform file I/O, paths, storage & networking
     │
     ├──► [math_engine]  Top-Level Wrapper — auto-boots embedded stdlib,
     │        │          registers host externs
     │        ├──► [mem_arena]    Isolated memory & 8-byte NaN-boxed Values
     │        ├──► [sym_table]    Types, structs, modules, overloads, units, externs
     │        ├──► [lexer]        Pattern-driven consuming types & syntax tokens
     │        ├──► [parser]       Top-down operator precedence (Pratt) parser
     │        ├──► [ext_loader]   In-memory `.math` compiler & `.mathc` serializer
     │        ├──► [evaluator]    Type-dispatched AST evaluator
     │        │
     │        ├──► [delegate]     C `@<intrinsic>` implementations (@add, @sin,
     │        │                   @vnew, @mzero, @calc_diff, @calc_solve, ...)
     │        └──► [hardcode]     Hardcoded grammar & ergonomic rules (reserved
     │                            keywords, implicit *, UFCS, `(i in ...){}`, ...)
     │
     └──► [user_loader]  In-memory user extension utility
               1. user_compile_ext : raw `.math` text -> hooks Engine + returns `.mathc`
               2. user_load_mathc  : `.mathc` bytes   -> hooks Engine in O(1)
```

`Core/Code/*.c`, `Core/lib/delegate/*.c`, and `Core/lib/hardcode/*.c` form the
engine (built together as the `mathengine` static library); `Test/` and
`Tutorial/` are host-side. The engine never touches files or directories — the
host reads `.math` text, hands it to the engine, and decides where to persist
the returned `.mathc` bytes.

## Repository Layout

```
CMakeLists.txt
makefile
Core/
  Headers/    math_engine.h  mem_arena.h  sym_table.h  lexer.h
              parser.h  ext_loader.h  evaluator.h  user_loader.h
              stdlib_precompiled.h   (auto-generated embedded stdlib image)
  Code/       math_engine.c  mem_arena.c  sym_table.c  lexer.c
              parser.c  ext_loader.c  evaluator.c  user_loader.c
  lib/
    delegate/ delegate.h  delegate.c   C `@<intrinsic>` implementations
    hardcode/ hardcode.h  hardcode.c   Hardcoded grammar & ergonomic rules
Library/
  mathc.c             Standalone AOT `.math` -> `.mathc` + header compiler
  00_bootstrap.math   Syntax, lexical types, control flow, scopes/booleans,
                      `$vec`, `$mat`, expression-based calculus, `!`/`~`
  10_arith.math       Arithmetic, comparisons, number theory, stats, commercial
  20_trig.math        Trig, hyperbolic, exponential & log functions
  30_complex.math     Postfix `i` operator & complex arithmetic
  40_geom.math        2D/3D vectors, circle/sphere geometry
  50_algebra.math     2x2 & multidim matrices, quadratics, polynomial algebra
  55_calculus.math    Differential/integral calculus, Taylor, solids of rev.
  60_units.math       SI/Imperial unit type groups & postfix suffixes
  65_conversions.math Same-unit conversions (`in`, `to_*`, `<u1>_to_<u2>`)
  70_physics.math     Mechanics, EM, waves, thermo, modern physics
  80_chemistry.math   Stoichiometry, gas laws, pH, kinetics, electrochem
  90_biology.math     Genetics, molecular biology, physiology, ecology
  stdlib.mathc        Compiled standard library image (generated)
Test/
  main.c              Host demo & 247-case automated test suite
  user_physics.math   Sample user extension (host-backed functions)
Tutorial/
  00_Index_and_Overview.md  ...  09_Calculus_Arith_and_Sciences.md
```

## Building

### With the provided makefile

```sh
make build                     # debug build (default) via Clang -> DebugBuild/myapp
make run                       # build if needed, then run
make clean                     # remove the selected build dir

make build release             # optimized build -> ReleaseBuild/myapp
make build CC=gcc              # override the compiler
```

### With CMake directly

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/myapp
```

The core is built as the static library target **`mathengine`**
(`libmathengine.a`); the demo (`myapp`) and the AOT compiler (`mathc`) both link
against it, so the engine can be embedded directly into other hosts.

Run from the repository root (the demo looks for `Test/user_physics.math`).

### Value encoding

Every `Value` is a strictly **8-byte IEEE-754 NaN-box** (`sizeof(Value) == 8`):
unitless `$real`s are stored inline as `double` bits with zero pool allocation,
while `$iota`, unit-tagged scalars, `$str`, structs, `$vec`, and `$mat` live in
the quiet-NaN payload space.

### Recompiling the standard library

`Core/Headers/stdlib_precompiled.h` is the `mathc` image of `Library/*.math`
embedded as a C array. Regenerate it (and `Library/stdlib.mathc`) after editing
the standard library:

```sh
make stdlib
```

Standalone use of `mathc`:

```sh
./DebugBuild/mathc out.mathc out_header.h my_array in1.math in2.math ...
```

## The `.math` Language

### Modules

Every `.math` file groups declarations into named modules with `[module_name]`
and declares dependencies with `<[dependency]`. Dependencies control load order
and the active module bitmask used for O(1) toggling.

```
[complex_ext]
<[arith]
<[trig]
```

### Lexical types & delimiters

```
"(" => @lparen
"{" => @lbrace
"}" => @rbrace
"[" => @lbrack
"]" => @rbrack
$real ~ \d+ ("." \d+)?       => @f64
$str  ~ "\"" [^"]* "\""      => @str_slice
$vec  ~                      => @vec_type
$mat  ~                      => @mat_type
```

`$iota` (the imaginary scalar) has no lexical pattern — it is produced by the
postfix `i` operator (`5i`, `(3 + 4)i`, `sin(x)i`).

### Operators

```
(a:real) "+" (b:real) :<50 -> real = @add(a, b)   # left-assoc, precedence 50
(a:real) "^" (b:real) :>85 -> real = @pow(a, b)   # right-assoc; binds tighter than unary '-'
"-"      (x:real)     :80 -> real = @neg(x)       # prefix unary
(x:real) "++"         :90 -> real = x + 1         # postfix
```

- `:<N` left-assoc infix at precedence `N` · `:>N` right-assoc infix
- `:N` prefix / postfix · maximal munch resolves `++`, `+++`
- `@name(...)` calls a private C intrinsic (or a host-registered extern)

### Functions, dispatch & control flow

```
"sin"(x:real)    -> real = @sin(x)
(a:complex) "+" (b:complex) :<50 -> complex = complex(a.re + b.re, a.im + b.im)
```

All control flow comes from a small set of bootstrap intrinsics:

```
( stmt1 ; stmt2 ; final_expr )   # sequencing
x := value                       # local variable (also '=' for mutation)
(cond) ?? ( body )               # lazy while loop
if(cond, then_expr, else_expr)   # lazy conditional
```

The `boot_control` module layers a full statement grammar on top:

```
x = { a := 3 ; b := 4 ; a^2 + b^2 }   # value-returning scope block
{ x = x + 5 ; 9999 }                  # isolated scope (block value discarded)
x, y := 5                             # multi-variable declaration
x += 1 ; x -= 2 ; x *= 3 ; x ^= 2     # compound assignments

if (c1) { b1 } or (c2) { b2 } else { b3 }          # else-if chain
switch (v) { case 1 : { ... } else : { ... } }     # switch / case (numbers & strings)
(cond) ? x : y                                     # right-assoc ternary

Tag : { ... jump(Tag) ... }           # labeled scope; jump/run/break(Tag)
Loop : (cond) ?? { ... break(Loop) }  # labeled while loop
```

Expression-based calculus constructs are also bootstrap intrinsics:

```
diff(x, expr, x0)          # derivative of expr at x = x0 (5-point stencil)
diff2(x, expr, x0)         # second derivative
diff_n(x, expr, x0, order) # nth derivative
integrate(x, expr, a, b)   # definite integral (composite Simpson)
lim(x, expr, x0)           # two-sided limit (Richardson extrapolation)
lim_left / lim_right       # one-sided limits
sigma(k, expr, lo, hi)     # discrete summation
prod(k, expr, lo, hi)      # discrete product
solve(x, expr == val)      # general equation solver (optional guess / bracket)
fmin(x, expr, a, b)        # minimum over [a, b]
fmax(x, expr, a, b)        # maximum over [a, b]
```

### Ergonomic shorthand, loops & destructuring

Declarations may omit types (falling back to `real`) and quote-free names:

```
tau_const = 2π                              # unquoted constant
hyp_3d(x, y, z = 0) = √(x^2 + y^2 + z^2)    # shorthand fn + default param
log_base(x, base = 10) = ln(x) / ln(base)   # default parameter
round(x, digits = 0)                        # default parameter
```

Implicit multiplication lets math read naturally:

```
x := 4 ; 2x^2 + 3x + 1        # 45
2 sin(π / 2) + 2 √16          # 10
```

Ranges, membership, keywordless loops, and UFCS chaining:

```
1..5          # [1, 2, 3, 4, 5]
1...5         # [1, 2, 3, 4, 5]
(3 in 1...5)  # 1
(i in 1...5) { s += i }        # keywordless range loop
(v in arr) { tot += v }        # keywordless vector loop
loop (k <= 4) { acc += k ; k += 1 }
loop { k += 1 ; if (k == 7) { break } }
v := vec() ; v.push(10).push(20).vec_sum()   # UFCS dot-chaining
```

Destructuring and custom errors:

```
r1, r2 := quad_roots(1, -5, 6)     # r1 = 3, r2 = 2
a, b, c := vec(10, 20, 30)
error("msg") ; assert(cond, "msg")
```

### Array subscripting & multidimensional matrices

Vectors and structs support C-style `v[i]` subscripting for reading and in-place
mutation (`=`, `+=`, `-=`, `*=`, `/=`), including negative indices on `$vec`:

```
v := vec3(2, 4, 6) ; v[0] = 10 ; v[1] += 5 ; v[2] *= 3 ; v   # vec3(x=10, y=9, z=18)
a := vec(3, 10) ; a[-1] = 30                                 # negative index from end
```

`$mat` is a dedicated 1D–4D matrix/tensor type built from `[...]` literals
(nested or semicolon row-separated):

```
M := [[1, 2, 3], [4, 5, 6]]     # or [1, 2, 3 ; 4, 5, 6]
M[0, 1] = 9 ; M[1][0] += 10     # multi-index & chained read/write
T := [[[1,2],[3,4]], [[5,6],[7,8]]] ; T[1, 0, 1]
A * B ; det(M) ; trace(M) ; transpose(M) ; inv(M) ; mat_rank(M)
```

Factorial, implemented *in the language*:

```
(n:real) "!" :90 -> real = (
    acc := 1;
    k   := n;
    (k > 1) ?? (
        acc = acc * k;
        k   = k - 1
    );
    acc
)
```

### Unit type groups & concrete locks

Every value carries an orthogonal `(EntityType, UnitType)` tag pair. Unit groups
are declared in `.math` after the return type — never hardcoded in C:

```
(x:real) "m"  :95 -> real, distance = x
(x:real) "km" :95 -> real, distance = x * 1000
(x:real) "sec":95 -> real, time     = x

(d:real, distance) "/" (t:real, time)  :<60 -> real, speed    = @div(d, t)
(v:real, speed)    "*" (t:real, time)  :<60 -> real, distance = @mul(v, t)

"scale_unit"(x:real, T, k:real)   -> real, T = x * k            # generic unit T
"unit_span"(a:real, T, b:real, T) -> real, T = @abs(a - b)      # same-unit T
```

Functions accept a generic unit `T` (preserved on the result) or a concrete unit
group (checked at call time). Variables can lock their entity type, unit group,
or both:

```
x := 5                          # dynamic entity & unit
x : real := 5                   # entity locked to real
x : distance := 50m             # unit locked to distance (entity may promote)
x : real, distance := 50m       # both locked
x, y : real, distance := 50m    # multi-variable concrete declaration
```

### Structs & dynamic vectors

```
$vec2(x:real, y:real)
$vec3(x:real, y:real, z:real)
$circle(center:vec2, radius:real)
$mat2(a:real, b:real, c:real, d:real)
```

Fields flatten into contiguous `double` slots with compile-time offset
resolution (`c.center.y`). `$vec` is a homogeneous dynamic array of any `T`:
`vec()`, `vec(cap)`, `vec(count, init)`, `push`, `pop`, `at` (negative index
from end), `set`, `len`, `cap`, `resize`, `shrink`, `clear`.

```
v := vec() ; push(v, 10) ; push(v, 20) ; v          # [10, 20]
```

### Host externs

Any `@name(...)` that is not a built-in C intrinsic compiles to an
`AST_EXTERN` node. The host binds it at runtime:

```c
bool eng_register_extern(Engine *eng, const char *name,
                         EngExternFn fn, void *user_ctx);
```

```math
"fx_rate"(from:str, to:str) -> real = @fx_rate(from, to)
```

Callbacks receive evaluated `Value` arguments and use helpers such as
`val_get_f64`, `val_as_cstr`, `val_from_f64`, `val_from_cstr`, and
`val_from_f64_vec`.

## Standard Library Modules

| Module | File | Contents (selected) |
| --- | --- | --- |
| `boot_syntax` | `00_bootstrap` | `(` `)` `{` `}` `[` `]` `,` delimiters |
| `boot_types` | `00_bootstrap` | `$real`, `$iota`, `$str`, `$vec`, `$mat` |
| `boot_control` | `00_bootstrap` | `.` (member access & UFCS), `;`, `:=`, `=`, `??`, `loop`, `if`/`or`/`else`, `switch`/`case`, `? :`, `jump`/`run`/`break`, logical & distributed operators, `!`/`~`, array `v[i]`, calculus & solver constructs (`diff`/`integrate`/`lim`/`sigma`/`prod`/`solve`/`fmin`/`fmax`), ranges `..`/`...`, `in`, `error`/`assert`, `type_of`/`unit_of`, compound assignments, vector API |
| `arith` | `10_arith` | `+ - * / % ^`, comparisons, `&&`/`\|\|`, `abs`/`min`/`max`/`clamp`, `gcd`/`lcm`/`ncr`/`npr`/`fib`/`is_prime`/`mod_pow`, `vec_sum`/`mean`/`variance`/`stddev`/`sma`, percentages/interest/EMI/work-time/progressions, `π`/`pi`/`e`/`phi` |
| `trig` | `20_trig` | `sin cos tan sec csc cot`, inverses, `sinh cosh tanh` (+inverses), `ln log10 log2 logb exp sqrt cbrt hypot`, `sigmoid` |
| `complex_ext` | `30_complex` | postfix `i`, `$complex`, overloaded `+ - * /` (generic over unit `T`), `conj cabs carg cexp_i polar` |
| `geom_ext` | `40_geom` | `$vec2/$vec3/$circle`, vector arithmetic & `**` dot, `cross2/cross3`, magnitudes/distances, `circle_area`, `sphere_vol` |
| `algebra` | `50_algebra` | `$mat2`, `$mat` multidim matrices (arithmetic, `det`/`trace`/`transpose`/`inv`, `mat_zeros`/`mat_eye`/`mat_rank`/`mat_shape`), `solve_2x2`, `quad_roots`, `quad_croot`, Horner `poly_val`, `poly_deriv/integ/add/mul`, `poly_newton` |
| `calculus` | `55_calculus` | symbolic polynomial calculus (`poly_diff`, `poly_def_integ`, `poly_newton_root`), tangent/normal lines, curvature, arc length, solids of revolution, `trapz_vec`/`simpson_vec`, `taylor_exp/sin/cos` |
| `units` | `60_units` | postfix `deg ° rad arcmin arcsec pct dB dBm`; length/mass/time/speed/pressure/energy/power unit groups; temperature conversions |
| `conversions` | `65_conversions` | universal `in` operator, `to_<unit>` postfix conversions, pairwise `<u1>_to_<u2>` functions for distance/angle/mass/time/speed/pressure/energy/power |
| `physics` | `70_physics` | CODATA constants; mechanics & SUVAT, gravity/orbital, circuits & EM, waves/optics, thermodynamics, quantum & relativity |
| `chemistry` | `80_chemistry` | constants; stoichiometry, gas laws, colligative properties, pH/pOH & buffers, kinetics/decay, electrochemistry & solid state |
| `biology` | `90_biology` | Hardy-Weinberg genetics, molecular biology & PCR, enzyme kinetics & Q10, plant water relations, microscopy, human physiology, ecology & diversity indices |

## Usage (C API)

```c
#include "math_engine.h"   /* + user_loader.h for user extensions */

Engine eng;
if (!eng_init(&eng)) { /* embedded stdlib is now ready */ }

Value v;
if (eng_eval(&eng, "ke(2 kg, 36 kph)", &v)) {
    char buf[128];
    eng_format_value(&eng, v, buf, sizeof buf);
    printf("%s\n", buf);           /* 100 */
}

eng_set_module(&eng, "units", false);   /* O(1) toggle */
```

Compiling and reloading a user extension:

```c
const uint8_t *mathc = NULL;
size_t mathc_len = 0;

/* Compile raw `.math` text, hook it in, get back `.mathc` bytes */
user_compile_ext(&eng, raw_math_text, &mathc, &mathc_len);
/* ...host persists `mathc`/`mathc_len`... */

/* Later: load a compiled image in O(1) memcpy time */
user_load_mathc(&eng, saved_bytes, saved_len);
```

Key entry points (`math_engine.h` + `user_loader.h`):

| Function | Purpose |
| --- | --- |
| `eng_init` | Init engine + auto-boot embedded precompiled stdlib (returns `bool`) |
| `eng_init_blank` | Init a completely blank-slate engine |
| `eng_eval` | Parse & evaluate an expression |
| `eng_register_extern` | Bind `@name(...)` to a host C callback |
| `eng_set_module` / `eng_is_module_active` | Toggle a module in O(1) |
| `eng_format_value` | Format any `Value` to a string |
| `eng_val_type` / `eng_type_name` | Inspect a value's entity type |
| `eng_val_unit` / `eng_unit_name` | Inspect a value's unit type group |
| `eng_last_error` | Last error message |
| `user_compile_ext` | Raw `.math` text → hooks engine + returns `.mathc` bytes |
| `user_load_mathc` | `.mathc` bytes → hooks engine in O(1) |

## The `.mathc` Binary Format

A `.mathc` image is a compact, ABI- and OS-neutral snapshot of the engine's
permanent tables. It begins with a 40-byte `MathcHeader` (magic `"MTHC"`,
version, type/module/symbol/constant counts, arena watermarks, module bitmask)
followed by the payload. Because the header is fixed-width and the payload is
copied verbatim, loading is a single `memcpy` — hence O(1) boot. Host-registered
externs are intentionally *not* serialized; they are re-bound by the host after
loading.

## Example Expressions

| Expression | Result | Type |
| --- | --- | --- |
| `2 ^ 3 ^ 2` | `512` | real |
| `√16 + 5!` | `124` | real |
| `ncr(10, 3) + npr(5, 2)` | `140` | real |
| `mod_pow(2, 10, 1000)` | `24` | real |
| `(3 + 4)i` | `7i` | iota |
| `((3 + 4)i)i` | `-7` | real |
| `(-5 + 10i) / (3 + 4i)` | `1 + 2i` | complex |
| `x = { a := 3 ; b := 4 ; a^2 + b^2 } ; x` | `25` | real |
| `x, y := 5 ; x, y += 3 ; x * 10 + y` | `88` | real |
| `switch ("Au") { case "Au" : { 79 } else : { 0 } }` | `79` | real |
| `s := 85 ; (s >= 90) ? 4 : (s >= 80) ? 3 : 2` | `3` | real |
| `x := 5 ; x = x + 5i ; x` | `5 + 5i` | complex |
| `cross3(vec3(1,0,0), vec3(0,1,0))` | `vec3(x=0, y=0, z=1)` | vec3 |
| `solve_2x2(2, 3, 8, 5, -1, 3)` | `vec2(x=1, y=2)` | vec2 |
| `quad_croot(1, 2, 5)` | `-1 + 2i` | complex |
| `sin(30 deg) + cos(60°)` | `1` | real |
| `1.5 km + 25000 cm + 500 mm` | `1750.5` | real |
| `2 atm + 1 bar` | `302650` | real |
| `100m / 10sec` | `10` | real *(speed)* |
| `unit_of(100m / 10sec)` | `"speed"` | str |
| `(2.5 km) in (1 m)` | `2500` | real |
| `(180 deg) to_rad` | `3.141592654` | real |
| `vec3(2,4,6)[1] += 5` | `9` | real |
| `det([4, 7 ; 2, 6])` | `10` | real |
| `inv([4, 7 ; 2, 6]) * [10, 10]` | `[-1, 2]` | mat |
| `diff(x, x^3 + 2*x, 3)` | `29` | real |
| `integrate(x, 3 * (x^2), 0, 2)` | `8` | real |
| `lim(x, sin(x) / x, 0)` | `1` | real |
| `sigma(k, k^2, 1, 5)` | `55` | real |
| `solve(x, x^2 == 9, 2)` | `3` | real |
| `fmin(x, (x - 3)^2 + 5, 0, 10)` | `3` | real |
| `1...5` | `[1, 2, 3, 4, 5]` | vec |
| `x := 4 ; 2x^2 + 3x + 1` | `45` | real |
| `s := 0 ; (i in 1...5) { s += i } ; s` | `15` | real |
| `r1, r2 := quad_roots(1, -5, 6) ; r1 * 10 + r2` | `32` | real |
| `5.001 ~ 5` | `1` | real |
| `ke(2 kg, 36 kph)` | `100` | real |
| `lorentz_gamma(0.6 * c0)` | `1.25` | real |
| `ph_from_h(0.001) + poh_from_ph(3)` | `14` | real |
| `pcr_copies(10)` | `1024` | real |
| `michaelis_menten(100, 5, 5)` | `50` | real |
| `fx_rate("USD", "INR")` | `84` | real *(host extern)* |
| `50 to_inr` | `4200` | real *(host extern)* |

## Tutorial

A full chapter-by-chapter guide lives in [`Tutorial/`](Tutorial/00_Index_and_Overview.md):

1. Getting Started & Host API
2. Grammar, Types & Operators
3. Control Flow, Structs & Vectors
4. Algebra, Polynomials & Matrices
5. Units, Physics & Chemistry
6. Host Extern Callbacks & Networking
7. Unit Groups, Concrete Variable Locks & Generic Unit `T`
8. Conversions, Array Indexing & Multidimensional Matrices
9. Calculus, Commercial Arithmetic & Quantitative Sciences

## Testing

The host demo (`Test/main.c`) verifies the blank-slate guarantee, auto-boots the
stdlib, exercises the `user_loader` workflow (compile `.math` → `.mathc` →
reload), registers host externs, prints an architecture report, and runs a
**247-case** automated suite (stdlib + control flow + booleans + multi-var +
concrete units + array/matrix + calculus + conversions + algebra + physics +
chemistry + biology + host externs + ergonomics), followed by type-safety &
reserved-keyword guardrails, O(1) module-toggling checks, and an **8-case audit regression
suite** (unit constants in `.mathc`, transactional rollback, nesting-depth and
input-length guards, self-referential structs, pool integrity, headerless user
extensions, and safe float→int indexing). Run it with `make run`.

## Design Goals

1. **Zero hardcoded math** — every type, operator, unit, and function comes from
   `.math`; the only hardcoded grammar/ergonomic rules are quarantined in
   `Core/lib/hardcode/` so they are predictable and auditable.
2. **Data-driven language** — syntax and semantics live in `.math`, not C.
3. **No engine-side I/O** — the host owns files/network; the engine owns memory.
4. **Instant boot** — precompiled, embedded, O(1) `memcpy` load.
5. **Composability** — modules layer via `<[parent]` and toggle O(1).
6. **Host extensibility** — native callbacks via `@extern` with no core coupling.
7. **Small footprint** — fixed-capacity arenas, compact 8-byte AST nodes.
8. **Portability** — plain C11 + libm, ABI/OS-neutral binary format.

## License

Licensed under the **Apache License, Version 2.0**. See [`LICENSE`](LICENSE).
