# Cmath Engine

A lightweight, fast, embeddable **C11 mathematical programming engine** built on
a *blank-slate* architecture. The engine ships with **zero hardcoded math,
types, or operators** — every delimiter, type, operator, function, constant,
control structure, and unit type group is compiled and dependency-linked from
modular `.math` files into a portable `.mathc` binary image. Modules can be
enabled or disabled in **O(1)** via a bitmask flip.

---

## Section 1 — Codebase Documentation

### Architecture

The codebase is organized by responsibility into dedicated folders for readability:

- **`Core/Code/` + `Core/Headers/`** — the core engine pipeline: isolated memory
  arena (`mem_arena`), symbol/type/overload registry (`sym_table`), pattern-driven
  lexer (`lexer`), Pratt parser (`parser`), AST evaluator (`evaluator`), and
  top-level engine wrapper (`math_engine`).
- **`Core/compile/`** — extension compilation, topological dependency linking,
  `.mathc` binary serialization, and host-facing user extension loading:
  - `compiler.h` / `compiler.c`: `[module]` unit staging (`ext_compile_unit`),
    duplicate module rejection, and module declaration compilation (`ext_compile_slice`).
  - `linker.h` / `linker.c`: `<[dep]` dependency DAG resolution, missing/circular
    dependency detection via Kahn's topological sort, and transactional linking
    (`ext_link_units`, `ext_compile_and_link`, `ext_load_str`).
  - `mathc_bin.h` / `mathc_bin.c`: 40-byte `MathcHeader`, FNV-1a checksum,
    pre-copy structural validation, and O(1) `.mathc` binary serialization/loading
    (`ext_save_mathc_mem`, `ext_load_mathc_mem`).
  - `user_loader.h` / `user_loader.c`: host-facing in-memory user extension
    compiler, linker, and `.mathc` loader (`user_compile_unit`, `user_link_ext`,
    `user_compile_and_link`, `user_compile_ext`, `user_load_mathc`).
  - `ext_loader.h`: umbrella header including `compiler.h`, `linker.h`, and `mathc_bin.h`.
- **`Core/session/`** — isolated parallel in-memory evaluation sessions and
  order-tagged raw-text evaluation snapshots:
  - `session.h` / `session.c`: parallel session lifecycle (`eng_session_open`,
    `eng_session_close`, `eng_session_reset`) and stateful evaluation (`eng_session_eval`).
  - `snapshot.h` / `snapshot.c`: canonical value expression serializer,
    order-tagged FNV-1a hash-chained snapshot builder (`MSP1|seq:...|prev:...|hash:...`),
    iterative snapshot reader (`eng_session_snapshot_next`), and full-history
    session restoration (`eng_session_restore`).
- **`Core/lib/delegate/`** — every C-backed `@` intrinsic (`@add`, `@sin`,
  `@vnew`, `@mzero`, `@calc_diff`, `@calc_solve`, `@tostr`, `@fromstr`, ...).
- **`Core/lib/hardcode/`** — the small set of *hardcoded* grammar/ergonomic rules
  (reserved keywords, default `:real`/`-> real` fallback, implicit
  multiplication, keywordless loops, destructuring, parameterized `<T>`
  containers), kept explicitly isolated so they are auditable in one place.
- **`Library/`** — all operators, precedences, types, units, structs, constants,
  and scientific functions, written without numerical filename prefixes and
  linked via `<[dep]` topological resolution.

The engine core (`Core/`) has **no filesystem, path, or networking
dependencies**. The host frontend owns all file I/O; the engine compiles and
links `.math` text, loads `.mathc` images, and manages parallel evaluation
sessions strictly in memory, and can call back into host-provided C functions
(`@extern`) for things like live web APIs, databases, or sensor reads.

```
Host Application (Test/main.c)
     │   Owns all platform file I/O, paths, storage & networking
     │
     ├──► [Core/Headers + Core/Code]  Core Engine Pipeline
     │        ├──► [math_engine]  Top-level wrapper, auto-boot stdlib & stateless eval
     │        ├──► [mem_arena]    Isolated memory & 8-byte NaN-boxed Values
     │        ├──► [sym_table]    Types, structs, modules, overloads, units, externs
     │        ├──► [lexer]        Pattern-driven consuming types & syntax tokens
     │        ├──► [parser]       Top-down operator precedence (Pratt) parser
     │        └──► [evaluator]    Type- & unit-dispatched AST evaluator
     │
     ├──► [Core/compile]              Compilation, Linking & `.mathc` Serialization
     │        ├──► [compiler]     Stage raw `.math` module units & compile declarations
     │        ├──► [linker]       Topological `<[dep]` DAG linker & cycle detector
     │        ├──► [mathc_bin]    `.mathc` binary serializer, validator & O(1) loader
     │        └──► [user_loader]  Host API: user_compile_unit, user_link_ext,
     │                            user_compile_and_link, user_compile_ext, user_load_mathc
     │
     ├──► [Core/session]              Parallel In-Memory Sessions & Snapshots
     │        ├──► [session]      eng_session_open / close / reset / eval
     │        └──► [snapshot]     eng_session_snapshot_next / rewind / at / restore
     │
     └──► [Core/lib]                  C Intrinsics & Hardcoded Grammar Rules
              ├──► [delegate]     C `@<intrinsic>` implementations (@add, @sin, ...)
              └──► [hardcode]     Hardcoded grammar & ergonomic rules (implicit *, UFCS, ...)
```

### Repository Layout

```
CMakeLists.txt
makefile
README.md
Core/
  Headers/    math_engine.h  mem_arena.h  sym_table.h  lexer.h
              parser.h  evaluator.h
              stdlib_precompiled.h   (auto-generated embedded stdlib image)
  Code/       math_engine.c  mem_arena.c  sym_table.c  lexer.c
              parser.c  evaluator.c
  compile/    compiler.h     compiler.c      (.math unit staging & declaration compiler)
              linker.h       linker.c        (topological <[dep] DAG linker & cycle guard)
              mathc_bin.h    mathc_bin.c     (.mathc binary serializer, validator & loader)
              user_loader.h  user_loader.c   (host user extension compile/link/load API)
              ext_loader.h                   (umbrella header for compiler/linker/mathc_bin)
  session/    session.h      session.c       (parallel in-memory session lifecycle & eval)
              snapshot.h     snapshot.c      (order-tagged raw-text snapshots & restore)
  lib/
    delegate/ delegate.h     delegate.c      (C `@<intrinsic>` implementations)
    hardcode/ hardcode.h     hardcode.c      (hardcoded grammar & ergonomic rules)
Library/
  mathc.c           Standalone AOT `.math` compiler & dependency-resolved linker
  bootstrap.math    Syntax, lexical types, control flow, scopes/booleans,
                    `$vec`, `$mat`, `$fn`, expression-based calculus, `_`, `into`
  arith.math        Arithmetic, comparisons, number theory, stats, commercial
  trig.math         Trig, hyperbolic, exponential & log functions
  complex.math      Postfix `i` operator, complex arithmetic & `into` composition
  geom.math         2D/3D vectors, circle/sphere geometry & `into` promotion
  algebra.math      2x2 & multidim matrices, quadratics, polynomial algebra
  calculus.math     Differential/integral calculus, Taylor, solids of rev.
  units.math        SI/Imperial unit type groups & postfix suffixes
  conversions.math  Same-unit conversions (`in`, `to_*`, `<u1>_to_<u2>`)
  physics.math      Mechanics, EM, waves, thermo, modern physics
  chemistry.math    Stoichiometry, gas laws, pH, kinetics, electrochem
  biology.math      Genetics, molecular biology, physiology, ecology
  stdlib.mathc      Compiled & linked standard library image (generated)
Test/
  main.c            Host demo & automated verification suite (Sections [1]–[9])
  user_physics.math Sample user extension (host-backed functions)
  Math Engine Language (MEP) — Complete Reference.md
                    Canonical language specification & tutorial guidebook
```

### Building

#### With the provided makefile

```sh
make build                     # debug build (default) via Clang -> DebugBuild/myapp
make run                       # build if needed, then run
make clean                     # remove the selected build dir

make build release             # optimized build -> ReleaseBuild/myapp
make build CC=gcc              # override the compiler
```

#### With CMake directly

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/myapp
```

The core is built as the static library target **`mathengine`**
(`libmathengine.a`); the demo (`myapp`) and the AOT compiler/linker (`mathc`)
both link against it, so the engine can be embedded directly into other hosts.

Run from the repository root (the demo looks for `Test/user_physics.math`).

### Value Encoding

Every `Value` is a strictly **8-byte IEEE-754 NaN-box** (`sizeof(Value) == 8`):
unitless `$real`s are stored inline as `double` bits with zero pool allocation,
while `$iota`, unit-tagged scalars, `$str`, structs, `$vec`, `$mat`, and `$fn`
live in the quiet-NaN payload space.

### Separated Compilation and Dependency-Resolved Linking (`Core/compile/`)

Neither user-defined modules nor the engine's own `Library/*.math` files require
numerical filename prefixes (`00_`, `10_`, etc.). Extension building in
`Core/compile/` is split into dedicated files and phases:

1. **Compilation (`Core/compile/compiler.c` — `ext_compile_unit` / `user_compile_unit`)**:
   Scans each raw `.math` source string, splits it into individual `[module]`
   blocks, extracts `<[dep_name]` dependency declarations into a `UserBuildCtx`
   (`ExtBuildCtx`) staging buffer, and verifies that no named `[module]` is
   already loaded in the engine or duplicated across compilation units.
2. **Linking (`Core/compile/linker.c` — `ext_link_units` / `user_link_ext`)**:
   Constructs the module dependency DAG over all `<[dep_name]` declarations,
   verifies that every dependency exists (either already loaded in `Engine` or
   staged in `UserBuildCtx`), detects circular dependencies via Kahn's
   topological sort algorithm, compiles module bodies in resolved topological
   order via `ext_compile_slice` with full transactional rollback on error, and
   serializes a single unified `.mathc` binary image (`Core/compile/mathc_bin.c`)
   that is guaranteed to work.

`Library/mathc.c` uses this exact same two-phase pipeline for the standard
library: `CMakeLists.txt` and `makefile` pass `Library/algebra.math`,
`Library/arith.math`, `Library/biology.math`, `Library/bootstrap.math`, etc. in
alphabetical order, and `mathc` topologically links all 14 standard library
modules into `Library/stdlib.mathc` and `Core/Headers/stdlib_precompiled.h`:

```sh
make stdlib
```

Standalone use of `mathc` (input `.math` files may be passed in any order):

```sh
./DebugBuild/mathc out.mathc out_header.h my_array mod_c.math mod_a.math mod_b.math
```

### Module Persistence vs. Expression Evaluation & Duplicate Load Protection

- **Permanent Lifetime of Loaded Modules**: Once modules are loaded into an
  `Engine` instance — whether compiled & linked from raw `.math` text
  (`user_compile_unit` + `user_link_ext`, `user_compile_and_link`, or
  `user_compile_ext`) or loaded from a precompiled `.mathc` binary
  (`user_load_mathc`) — they reside in the **permanent region** of `MemArena`
  and `SymTable` for the **entire lifetime of the `Engine`**. You never pass
  library source text again when evaluating expressions.
- **Duplicate Load Rejection**: Supplying the same named `[module]` raw text
  more than once into an engine where that module is already loaded (or
  supplying duplicate declarations in `[default]`) is rejected as an error
  (`"Module '[name]' is already loaded"` or `"Duplicate declaration of '...' in module '[default]'"`)
  with transactional rollback, preventing duplicate types, constants, or
  overloads.
- **Separate APIs for Module Loading vs. Expression Evaluation**:
  - **Module Loading Functions (`Core/compile/user_loader.h`)**: `user_build_init`,
    `user_compile_unit`, `user_link_ext`, `user_compile_and_link`,
    `user_compile_ext`, and `user_load_mathc`.
  - **Expression Evaluation Functions (`Core/Headers/math_engine.h` & `Core/session/session.h`)**:
    `eng_eval` (stateless one-shot evaluation) and `eng_session_eval` (stateful
    session evaluation).

### Parallel In-Memory Sessions & Order-Tagged Raw-Text Snapshots (`Core/session/`)

To support stateful multi-step workflows without losing variables between calls,
`Core/session/` provides **independent, parallel, unnamed in-memory sessions**
(`session.h` / `session.c`) and **order-tagged raw-text snapshots**
(`snapshot.h` / `snapshot.c`):

- **Parallel & Unnamed (`session.c`)**: `eng_session_open(&eng)` allocates an
  isolated session slot in memory and returns an integer `session_id`
  (`0 .. ENG_MAX_SESSIONS - 1`). Multiple sessions can evaluate expressions in
  parallel on the same `Engine` without sharing variables, while stateless
  `eng_eval` calls remain completely isolated.
- **Independent of User Library Loading**: Compiling/linking new user modules
  (or toggling modules via `eng_set_module`) is independent of sessions; any
  active session can immediately use newly linked modules.
- **Order-Tagged Iterative Raw-Text Snapshots (`snapshot.c`)**: Every successful
  `eng_session_eval` records one order-tagged raw-text snapshot
  (`MSP1|seq:<n>|prev:<hex>|hash:<hex>|vars:<k>|...|res:<type>:<unit>=<val>`)
  capturing the evaluation sequence number (`seq:1, 2, ...`), a cryptographic
  FNV-1a hash chain (`prev` and `hash`), all active session variables (including
  entity types, unit groups, concrete type/unit locks, containers, structs, and
  lambda closures), and the evaluation result.
- **Iterative Retrieval & Full-Information Restoration (`snapshot.c`)**:
  - Calling `eng_session_snapshot_next(&eng, sid)` in a loop yields each
    evaluation's snapshot as raw text in order (`seq:1`, `seq:2`, ...) and
    returns `NULL` when complete.
  - Calling `eng_session_restore(&eng, snapshots, count)` takes an array of
    raw-text snapshots (even if supplied out of order), uses the order tags and
    hash chain to verify that **full, contiguous, uncorrupted history** (`1 .. count`)
    is present, reconstructs the session state in memory, and returns a new
    `session_id` ready to continue evaluating from `seq:count + 1` onwards.

### The `.mathc` Binary Format (`Core/compile/mathc_bin.h`)

A `.mathc` image is a compact, ABI- and OS-neutral snapshot of the engine's
permanent tables. It begins with a 40-byte `MathcHeader` (magic `"MTHC"`,
version, type/module/symbol/constant counts, arena watermarks, module bitmask,
and 32-bit FNV-1a checksum) followed by the payload. Because the header is
fixed-width and the payload is copied verbatim after pre-copy bounds and
checksum verification, loading is O(1) `memcpy` time. Host-registered externs
are intentionally *not* serialized; they are re-bound by the host after loading.

### C API Usage

#### 1. Stateless Evaluation & Module Toggling

```c
#include "math_engine.h"
#include "user_loader.h"

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

#### 2. Compiling & Linking Out-of-Order User Modules into a Single `.mathc` Binary

```c
UserBuildCtx build;
user_build_init(&build);

/* Compile user modules in ANY order (even if mod_aero depends on mod_base) */
user_compile_unit(&eng, &build, "[mod_aero]\n<[mod_base]\nlift(v) = base_fn(v) * 2\n");
user_compile_unit(&eng, &build, "[mod_base]\n<[arith]\nbase_fn(x) = x + 10\n");

/* Link in topological dependency order -> hooks `eng` + outputs single `.mathc` */
const uint8_t *mathc = NULL;
size_t mathc_len = 0;
user_link_ext(&eng, &build, &mathc, &mathc_len);

/* Later (or in another process): load the linked `.mathc` binary in O(1) time */
user_load_mathc(&eng, saved_bytes, saved_len);
```

#### 3. Parallel In-Memory Sessions, Iterative Snapshots & Session Continuation

```c
int s1 = eng_session_open(&eng);
int s2 = eng_session_open(&eng);

eng_session_eval(&eng, s1, "x : real, distance := 50m ; scale := a => a * 3", &v);
eng_session_eval(&eng, s2, "x := 999", &v);            /* independent of s1 */
eng_session_eval(&eng, s1, "x += 25m ; scale(4)", &v); /* x is 75m, v is 12 */

/* Iteratively extract order-tagged raw-text snapshots from s1 */
const char *saved[32];
size_t n_snaps = 0;
const char *raw = NULL;
while ((raw = eng_session_snapshot_next(&eng, s1)) != NULL) {
    saved[n_snaps++] = raw; /* host can persist each raw-text snapshot */
}

/* Later: restore and continue the session from its snapshots */
int s_cont = eng_session_restore(&eng, saved, n_snaps);
eng_session_eval(&eng, s_cont, "x + 25m", &v);         /* 100 [distance] */
eng_session_close(&eng, s1);
eng_session_close(&eng, s2);
eng_session_close(&eng, s_cont);
```

Key entry points (`math_engine.h`, `session.h`, `snapshot.h`, `user_loader.h`):

| Function | Header | Purpose |
| --- | --- | --- |
| `eng_init` / `eng_init_blank` | `Core/Headers/math_engine.h` | Init engine + auto-boot embedded stdlib, or init blank-slate engine |
| `eng_eval` | `Core/Headers/math_engine.h` | Stateless expression parse & evaluation |
| `eng_register_extern` | `Core/Headers/math_engine.h` | Bind `@name(...)` to a host C callback |
| `eng_set_module` / `eng_is_module_active` | `Core/Headers/math_engine.h` | Toggle a module in O(1) |
| `eng_format_value` / `eng_val_type` / `eng_val_unit` / `eng_last_error` | `Core/Headers/math_engine.h` | Format values and inspect types, units, or errors |
| `eng_session_open` / `eng_session_close` / `eng_session_reset` | `Core/session/session.h` | Open, close, or reset an isolated parallel in-memory session |
| `eng_session_eval` | `Core/session/session.h` | Evaluate an expression in a session (persists state & appends order-tagged snapshot) |
| `eng_session_snapshot_next` / `eng_session_snapshot_rewind` | `Core/session/snapshot.h` | Iteratively retrieve each evaluation's order-tagged snapshot as raw text |
| `eng_session_snapshot_count` / `eng_session_snapshot_at` | `Core/session/snapshot.h` | Query snapshot count or inspect a snapshot by index |
| `eng_session_restore` | `Core/session/snapshot.h` | Verify full order-tagged snapshot chain and continue session in memory |
| `user_build_init` / `user_compile_unit` / `user_link_ext` | `Core/compile/user_loader.h` | Multi-unit order-independent `.math` compilation & topological linking into `Engine` + `.mathc` |
| `user_compile_and_link` / `user_compile_ext` | `Core/compile/user_loader.h` | Batch or single-string `.math` compile & link convenience wrappers |
| `user_load_mathc` | `Core/compile/user_loader.h` | `.mathc` bytes → hooks engine in O(1) |

### Testing

The host demo (`Test/main.c`) verifies the blank-slate guarantee, auto-boots the
stdlib, exercises the `user_loader` workflow, registers host externs, prints an
architecture report, and runs an automated suite (stdlib + control flow +
booleans + multi-var + concrete units + array/matrix + calculus + conversions +
algebra + physics + chemistry + biology + host externs + ergonomics), followed
by type-safety & reserved-keyword guardrails, O(1) module-toggling checks, the
10-case audit regression & 3,000-iteration fuzzer suite, the separated
compilation & dependency-resolved linking suite (reverse-order modules,
duplicate module/declaration rejection, missing/circular dependency rejection),
and the parallel in-memory session & order-tagged snapshot suite. Run it with
`make run`.

### Design Goals

1. **Zero hardcoded math** — every type, operator, unit, and function comes from
   `.math`; the only hardcoded grammar/ergonomic rules are quarantined in
   `Core/lib/hardcode/` so they are predictable and auditable.
2. **Data-driven language** — syntax and semantics live in `.math`, not C.
3. **Order-independent compilation & linking** — modules declare `<[parent]`
   dependencies and link topologically (`Core/compile/`) without relying on
   filename prefixes.
4. **No engine-side I/O** — the host owns files/network; the engine owns memory.
5. **Instant boot** — precompiled, embedded, O(1) `memcpy` load.
6. **Stateful parallel sessions** — isolated in-memory sessions (`Core/session/`)
   with iterative order-tagged raw-text snapshots for host-managed persistence
   and continuation.
7. **Host extensibility** — native callbacks via `@extern` with no core coupling.
8. **Small footprint & portability** — fixed-capacity arenas, compact 8-byte
   `Value` and `AstNode`, plain C11 + libm, ABI/OS-neutral binary format.

---

## Section 2 — MEP Language Specification

*This section documents the complete MEP (Math Engine Language) specification, adapted from the canonical guidebook in `Test/Math Engine Language (MEP) — Complete Reference.md`.*

### 2.1 Overview

MEP is an extension-driven expression language and C11 engine for scientific and everyday math. A small C core provides memory, parsing, linking, and evaluation; almost everything else (operators, units, types, functions) is defined in text extensions (`.math`) that are compiled and linked into the engine.

Key properties:

- **Embeddable.** The engine is plain C11, makes zero `malloc` calls, never touches the filesystem, and runs from fixed-size arenas (`sizeof(Value) == 8`, `sizeof(AstNode) == 8`). It targets the Android NDK but builds anywhere.
- **Extensible & Order-Independent.** Operators, precedences, types, unit groups, constants, and functions are all declared in `.math` source and linked topologically via `<[dep]` declarations.
- **Unit-aware.** Every value carries an entity type (`real`, `complex`, `vec2`…) and an orthogonal unit tag (`distance`, `angle`, `mass`…). Mixing incompatible units is an error.
- **Stateless or Session-Based Evaluations.** `eng_eval` runs an isolated one-shot script, while `eng_session_eval` runs inside an isolated parallel session that persists variables and emits an order-tagged raw-text snapshot per evaluation. Loaded extensions and host callbacks persist across the engine's lifetime independently of sessions.

### 2.2 Lexical Structure

**Source text** is UTF-8. Symbols such as `π`, `φ`, `ϕ`, `°`, and `√` are ordinary tokens.

**Comments** start with `#` and run to the end of the line.

**Numbers** are decimal: digits with an optional fractional part (`42`, `3.14159`). There is no exponent notation (`1e3` is read as `1` followed by the identifier `e3`) and no digit separators. Negative numbers are the unary minus operator applied to a literal. Results print with 10 significant digits (`1/3` prints `0.3333333333`; very large values use `1.23456789e+11`).

**Strings** are double-quoted literals with no escapes (`"USD"`). They support `==`, `!=`, `type_of`, being returned from functions, being passed to host callbacks, and converting to/from typed values via the `"_"` operators.

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

### 2.3 Types, Units and Variables

#### 2.3.1 Entity Types

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

#### 2.3.2 Units

Postfix unit operators bind at precedence 95 and normalise to SI base units, tagging the value with its unit group: `100 km` is `100000 [distance]`, `90 deg` is `1.5708 [angle]`. Addition and subtraction require matching groups: `50m + 20cm` gives `50.2 [distance]`, while `100cm + 60°` fails with `Unit mismatch for '+'`. Multiplying by a plain number keeps the unit; dividing across groups can produce a new one (`distance / time` gives `speed`). Groups are declared in `.math` files, so you can invent your own.

Because postfix operators only fire after a complete operand, `m := 5 ; m + 10` treats `m` as a variable, while `50m` is fifty metres. `50im` and `50mi` both give `50i [distance]`.

#### 2.3.3 Declaring and Assigning Variables

`:=` declares, `=` assigns to an existing variable. Compound forms are `+= -= *= /= %= ^=`.

| Declaration | Entity lock | Unit lock | Effect |
| :-- | :-- | :-- | :-- |
| `x := 5` | dynamic | dynamic | may become complex or change unit |
| `x : real := 5` | `real` | dynamic | `x += 5i` is rejected |
| `x : distance := 50m` | dynamic | `distance` | `x += 50im` promotes to `50 + 50i [distance]`; `x += 60°` is rejected |
| `x : real, distance := 50m` | `real` | `distance` | `x += 10cm` gives `50.1`; unitless assignment is rejected |
| `v : vec<vec2> := vec<vec2>()` | `vec` (elem `vec2`) | dynamic | locks variable to `$vec` |

Multiple variables: `x, y := 5` broadcasts a scalar. Destructuring unpacks a struct, `vec` or `mat` in order: `r1, r2 := quad_roots(1, -5, 6)` gives `r1 = 3`, `r2 = 2`; `a, b, c := vec(10, 20, 30)`.

#### 2.3.4 Scope, Stateless Calls, and Sessions

`{ ... }` opens a lexical scope and evaluates to its last expression. `:=` inside it shadows an outer variable; `=` mutates it:

```text
x := 1 ; { x := 99 } ; x          # 1
x := 1 ; { x = 99 } ; x           # 99
{ a := 3 ; b := 4 ; a^2 + b^2 }   # 25
```

In stateless `eng_eval` calls, variables live only for that single call (`eng_eval("x := 10")` followed by `eng_eval("x")` fails with `Undefined symbol or constant 'x'`). In session calls (`eng_session_eval`), top-level variables (along with their concrete type/unit locks, containers, structs, and lambda closures) persist across evaluations within the session and are captured in each evaluation's order-tagged raw-text snapshot.

### 2.4 Operators and Precedence

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

#### 2.4.1 Plain String Conversion Operators (`x_` and `_s`)

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

#### 2.4.2 The `"into"` Insert & Composition Operator

`"into"` is a right-associative infix operator (`:>55`) that sums likewise entities & units, composes cross-entity values, or inserts an element into a container:

| Expression | Result Type | Behavior / Result |
| :-- | :-- | :-- |
| `5 into 5`, `50m into 20cm`, `5i into 3i`, `(1+2i) into (3+4i)`, `vec2 into vec2`, `vec3 into vec3` | same entity & unit | **Likewise entities & units return their sum**: `5 into 5` is `10`, `50m into 20cm` is `50.2 [distance]`, `5i into 3i` is `8i` |
| `5 into 5i`, `5i into 5` | `complex` | Cross-entity `real` + `iota` combines into `5 + 5i` (`complex(5, 5)`) |
| `2 into (3 + 4i)` | `complex` | Adds real component into complex: `5 + 4i` |
| `3i into (3 + 4i)` | `complex` | Adds imaginary component into complex: `3 + 7i` |
| `7 into vec2(3, 4)` or `vec2(3, 4) into 7` | `vec3` | Promotes 2D `vec2(x, y)` + `z` into 3D `vec3(x=3, y=4, z=7)` |
| `item into v` (`v` is `vec`/`list`/`stack`/`queue`) | `vec` | Appends `item` into `v` (`@vpush(v, item)`), acting as an infix alternative to `v.push(item)` |

Because `"into"` is right-associative, `10 into 20 into vec<real>()` chains naturally (`20` is pushed first, then `10`, yielding `[20, 10]`), and `v := vec<real>() ; 10 into v ; 20 into v` yields `[10, 20]`.

#### 2.4.3 Implicit Multiplication, Booleans, Approximate Equality & Complex Numbers

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

### 2.5 Control Flow

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

### 2.6 Collections, Matrices, Comprehensions & Lambdas

#### 2.6.1 Parameterized Dynamic Vectors, Lists, Stacks & Queues

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
| `peek(v)`, `top(v)`, `back(v)` | inspect last element without removing (`@vpeek(v, 0)`) |
| `enqueue(q, x)`, `q.enqueue(x)` | append to back of queue (`@vpush(q, x)`) |
| `dequeue(q)`, `shift(q)`, `pop_front(q)` | remove and return first element (`@vshift(q)`, FIFO dequeue) |
| `front(q)`, `q.front()` | inspect first element without removing (`@vpeek(q, 1)`) |
| `is_empty(v)`, `v.is_empty()` | returns `1` if `len(v) == 0`, else `0` |
| `at(v, i)`, `set(v, i, x)`, `v[i]` | read / write; negative `i` counts from the end |
| `len`, `cap`, `resize(v, n)`, `shrink`, `clear` | size and capacity management |
| `sum`, `mean`, `variance`, `stddev`, `sma(v, w)` | statistical reductions (`variance` is population variance) |

Slicing returns a new vector and also assigns: `v := vec(10, 20, 30, 40, 50) ; v[1...3]` is `[20, 30, 40]`, `v[0..4]` is `[20, 30, 40]`, `v[-3...-1]` is `[30, 40, 50]`, `v[3...1]` reverses, and `v[1...3] = 99` broadcasts.

#### 2.6.2 Matrices (`mat`)

Literals use `[...]`: `[1, 2, 3]` (1D), `[[1, 2], [3, 4]]` or `[1, 2 ; 3, 4]` (2D), and nested forms up to four dimensions. Index with `M[r, c]` or `M[r][c]`, and slice with ranges: `M[0...1, 1...2]` gives a submatrix, `M[2, 0...1]` a row.

Arithmetic: `A + B`, `A - B`, `A * B` (matrix product), `s * A`, `A / s`, `-A`. Functions: `det`, `trace`, `transpose`, `inv` (Gauss–Jordan), `mat_zeros(r, c)`, `mat_ones(r, c)`, `mat_eye(n)`, `mat_fill(r, c, v)`, `mat_rows`, `mat_cols`, `mat_rank`, `mat_len`, `mat_shape`. Example: `[[1,2],[3,4]] * [[5,6],[7,8]]` is `[[19, 22], [43, 50]]`.

#### 2.6.3 Comprehensions

`[expr for x in iterable if cond]` builds a vector from a range, `vec` or `mat`:

```text
[x^2 for x in 1...5]                      # [1, 4, 9, 16, 25]
pts := list<vec2>(vec2(1, 2), vec2(3, 4), vec2(5, 6))
[p.x + p.y for p in pts if p.x > 1]       # [7, 11]
```

#### 2.6.4 Lambdas and Higher-Order Functions

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

### 2.7 Structs and Math Types

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

### 2.8 Unit Conversions & Dual Variable/Unit Disambiguation

There are three ways to convert within a unit group; converting across groups is always an error.

1. **The `in` operator** supports both bare unit names (`25km in m` -> `25000 [distance]`, preserving the unit group) and parenthesised unit quantities (`(2.5 km) in (1 m)` -> `2500 [none]`).
   - **Dual Variable/Unit Disambiguation**: What if `m` or `km` is *also* declared as a local variable or vector?
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

### 2.9 Standard Library Catalogue

`eng_init` loads 14 named modules plus an implicit `default` module (where headerless user extensions land). Each module can be switched off and on in O(1) with `eng_set_module`; disabling `units` makes `100 km` fail with `Undefined symbol or constant 'km'`.

| Module | Source File | Contents |
| :-- | :-- | :-- |
| `boot_syntax`, `boot_types`, `boot_control` | `Library/bootstrap.math` | brackets, `$real $iota $str $vec $mat $fn`, assignment, loops, `if/switch`, jumps, `_` (`@tostr`/`@fromstr`), `into`, `vec<T>/list<T>/stack<T>/queue<T>`, sublist helpers (`enqueue`, `dequeue`, `shift`, `pop_front`, `peek`, `top`, `back`, `front`, `is_empty`), collection and calculus constructs, `type_of`, `unit_of`, `error`, `assert` |
| `arith` | `Library/arith.math` | operators; `abs floor ceil round(x, digits = 0) trunc fract sign min max clamp abs_diff is_even is_odd is_close gcd lcm npr ncr fib is_prime mod_pow`; `sum mean variance stddev sma`; constants `π pi e phi φ ϕ`; commercial maths |
| `trig` | `Library/trig.math` | `sin cos tan sec csc cot asin acos atan atan2 sinh cosh tanh asinh acosh atanh ln log10 log2 logb exp sqrt cbrt root hypot sigmoid` |
| `complex_ext` | `Library/complex.math` | `complex`, `conj`, `cabs`, `carg`, `cexp_i`, `polar`, complex `into` overloads |
| `geom_ext` | `Library/geom.math` | `vec2`, `vec3`, `circle`; `dot`, `cross2`, `cross3`, `dot3`, `vec2_mag`, `vec3_mag`, `vec2_dist`, `vec3_dist`, `lerp`, `circle_area`, `circle_circum`, `sphere_vol`, `sphere_area`, `vec2` -> `vec3` `into` overloads |
| `algebra` | `Library/algebra.math` | `mat2`, `det`, `trace`, `transpose`, `inv`, `inv_mat2`, `solve_2x2`, `quad_disc`, `quad_roots`, `quad_croot`, polynomial helpers, `mat_*` |
| `calculus` | `Library/calculus.math` | polynomial and geometric calculus |
| `units`, `conversions` | `Library/units.math`, `Library/conversions.math` | unit groups and conversion operators |
| `physics` | `Library/physics.math` | about 100 functions and constants |
| `chemistry` | `Library/chemistry.math` | about 60 functions and constants |
| `biology` | `Library/biology.math` | about 60 functions |

### 2.10 Calculus and Solvers

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

### 2.11 Writing Extensions

An extension is UTF-8 text with `#` comments. The host compiles and links one or more extension texts via `user_compile_unit` + `user_link_ext` (or `user_compile_and_link` / `user_compile_ext`); the engine does not care about file names or numerical prefixes and resolves `<[dep]` dependencies topologically. A compile or link error leaves the engine exactly as it was.

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
| Unit group | name a new group after the result type: `(x) KB :95 -> real, data_size = x * 1024` |
| Unit-typed parameters | `(d:real, distance)`; the generic `T` accepts any group and preserves it: `unit_span(a:real, T, b:real, T) -> real, T = @abs(a - b)` |

### 2.12 Embedding API and Capacity Limits

| Function | Purpose |
| :-- | :-- |
| `eng_init(&eng)` | boot all standard modules from the embedded image |
| `eng_init_blank(&eng)` | empty engine with no types or operators (for tools and tests) |
| `eng_eval(&eng, src, &out)` | stateless evaluation of a script; returns false and sets the error on failure |
| `eng_session_open(&eng)`, `eng_session_close(&eng, sid)`, `eng_session_reset(&eng, sid)` | open, close, or reset an isolated parallel in-memory session (`Core/session/session.h`) |
| `eng_session_eval(&eng, sid, src, &out)` | evaluate a script inside session `sid`, persisting variables and emitting an order-tagged snapshot (`Core/session/session.h`) |
| `eng_session_snapshot_next(&eng, sid)`, `eng_session_snapshot_rewind(&eng, sid)` | iteratively retrieve each evaluation's order-tagged snapshot as raw text (`Core/session/snapshot.h`) |
| `eng_session_snapshot_count(&eng, sid)`, `eng_session_snapshot_at(&eng, sid, idx)` | inspect session snapshot count or retrieve snapshot by index (`Core/session/snapshot.h`) |
| `eng_session_restore(&eng, snaps, count)` | verify full order-tagged snapshot sequence and continue session in memory (`Core/session/snapshot.h`) |
| `eng_format_value(&eng, v, buf, cap)` | render any value to text |
| `eng_val_type`, `eng_type_name`, `eng_val_unit`, `eng_unit_name` | inspect a result |
| `eng_last_error(&eng)` | last error message (127 characters max) |
| `eng_set_module`, `eng_is_module_active` | toggle modules by name |
| `eng_register_extern(&eng, name, fn, ctx)` | bind `@name(...)` to a C callback |
| `user_build_init`, `user_compile_unit`, `user_link_ext`, `user_compile_and_link`, `user_compile_ext`, `user_load_mathc` | in-memory extension compilation, topological dependency linking, and `.mathc` binary cache load (`Core/compile/user_loader.h`) |

**Capacity limits** (compile-time constants in `mem_arena.h`, `sym_table.h`, `compiler.h`, `mathc_bin.h`, and `session.h`):

| Resource | Limit |
| :-- | :-- |
| Expression length | 65,534 bytes |
| String pool | 32,768 bytes |
| AST nodes | 8,192 |
| Float pool (vectors, structs, matrices) | 8,192 doubles |
| Call stack | 256 slots |
| Evaluation depth / parse nesting | 128 |
| Evaluation step budget | 2,000,000 per eval |
| Tagged scopes | 16 |
| Modules / types / unit groups | 32 / 64 / 64 |
| Overloads / symbols / constants / externs | 2,048 / 2,048 / 128 / 32 |
| Parallel in-memory sessions | 8 (`ENG_MAX_SESSIONS`) |
| Snapshots / persistent vars per session | 32 (`ENG_SESSION_MAX_EVALS`) / 16 (`ENG_SESSION_MAX_VARS`) |

### 2.13 Quirks and Notes

- **`++`, `--`, `+++`, `---` do not modify variables.** They are pure operators returning `x + 1`, `x - 1`, `x + 2`, `x - 2`, in prefix or postfix form. `a := 5 ; a++ ; a` is `5`. Use `a += 1`.
- **Stateless `eng_eval` vs. Stateful `eng_session_eval`**: Variables do not persist between stateless `eng_eval` calls; use `eng_session_eval` when you want variables, locks, containers, and closures to persist across evaluations in a session. Loaded extensions, constants, and host callbacks persist across the engine's lifetime independently of sessions.
- **`**` is only the `vec3` dot product** and binds at 20, below `+`, so parenthesise it. For `vec2` use `dot(a, b)`.
- **Prefix `!` vs `~`**: `~` is infix approximate equality (`<= 0.001`), and boolean NOT is `!` (or `not`).
- **Golden ratio**: `phi`, `φ`, and `ϕ` are all defined and equal `1.618033989`.
- **Ranges**: `1..5` is exclusive `[2, 3, 4]`, `1...5` is inclusive `[1, 2, 3, 4, 5]`, `1..<5` is `[1, 2, 3, 4]`, and `1<..5` is `[2, 3, 4, 5]`.

### 2.14 Common Error Messages

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
| `Matrix rows/sub-tensors must have matching dimensions` | jagged matrix literal |
| `Operand 5 out of range for boolean negation '!': expected 0 or 1` | `!` on a non-boolean |
| `Division by zero` | divisor is exactly zero |

---

## License

Licensed under the **Apache License, Version 2.0**. See [LICENSE](https://www.google.com/search?q=LICENSE).
