# Cmath Engine

A lightweight, fast, embeddable **C11 math expression engine** built on a
*blank-slate* architecture. The C core ships with **zero built-in types,
operators, or syntax** — every capability is compiled from modular `.math`
files into a portable `.mathc` binary image. Modules can be enabled or disabled
in **O(1)** via a bitmask flip, letting a host program reshape the language on
the fly.

The engine has **no filesystem, directory, or OS dependencies**: file I/O and
paths belong to the host frontend, while the engine compiles `.math` text and
loads `.mathc` images strictly in memory.

## Highlights

- **Blank-slate core** — the engine knows nothing until `.math` teaches it.
- **Embedded precompiled stdlib** — `eng_init()` boots the whole standard
  library from an embedded byte image in O(1) `memcpy` time, no parsing, no I/O.
- **Pure in-memory user extensions** — `user_compile_ext` turns raw `.math`
  text into host-managed `.mathc` bytes; `user_load_mathc` loads them in O(1).
- **AOT `.math` → `.mathc` + C header compiler** (`mathc`) for build-time baking.
- **Pattern-driven lexer** — consuming `$type` patterns (`$real`, `$str`, `$iota`, `$vec`).
- **Pratt (top-down operator precedence) parser** — fixity and precedence are data.
- **Multiple dispatch** — operators/functions overloaded on operand types.
- **Generic dynamic vectors** — growable/shrinkable `$vec` of *any* `T`.
- **Flat C-style structs** — `$vec2`, `$vec3`, nested `$circle`, compile-time field offsets.
- **Dual value encoding** — 8-byte IEEE-754 **NaN-boxing** (default) or a 12-byte **tagged union**.
- **UTF-8 aware** — operators and constants like `√`, `π`, `^`, `!` work natively.
- **Hardened builds** — fortify source, stack protector, ASan/UBSan when available.

## Architecture

```
Host Application (Test/main.c)
     │   Handles all platform file I/O, paths & storage
     │
     ├──► [math_engine]  Top-Level Wrapper — auto-boots embedded stdlib
     │        ├──► [mem_arena]    Isolated memory & 8B/12B Value packing
     │        ├──► [sym_table]    Dynamic types, structs, modules & overloads
     │        ├──► [lexer]        Pattern-driven consuming types & syntax tokens
     │        ├──► [parser]       Top-down operator precedence (Pratt) parser
     │        ├──► [ext_loader]   In-memory `.math` compiler & `.mathc` serializer
     │        └──► [evaluator]    Type-dispatched AST evaluator & `@` intrinsics
     │
     └──► [user_loader]  In-memory user extension utility
              1. user_compile_ext : raw `.math` text -> hooks Engine + returns `.mathc` bytes
              2. user_load_mathc  : `.mathc` bytes   -> hooks Engine in O(1) `memcpy` time
```

`Core/Code/*.c` is the engine; everything under `Test/` is a normal host
application. The engine never touches files or directories — the host reads
`.math` text, hands it to the engine, and decides where to persist the returned
`.mathc` bytes.

## Repository Layout

```
CMakeLists.txt
makefile
Core/
  Headers/    math_engine.h  mem_arena.h  sym_table.h  lexer.h
              parser.h  ext_loader.h  evaluator.h  user_loader.h
              stdlib_precompiled.h   (auto-generated, embedded stdlib image)
  Code/       math_engine.c  mem_arena.c  sym_table.c  lexer.c
              parser.c  ext_loader.c  evaluator.c  user_loader.c
Library/
  mathc.c            Standalone AOT `.math` -> `.mathc` + header compiler
  00_bootstrap.math  10_arith.math  20_trig.math  30_complex.math  40_geom.math
  stdlib.mathc       Compiled standard library image (generated)
Test/
  main.c             Host demo & automated test suite
  user_physics.math  Sample user extension (structs, funcs, vectors)
```

## Building

### With the provided makefile

```sh
make build      # debug build (default) via Clang -> DebugBuild/myapp
make run        # build if needed, then run
make clean      # remove the selected build dir

make build release   # optimized build -> ReleaseBuild/myapp
make run release
```

Overrides:

```sh
make build CC=gcc            # use GCC
make build USE_TAGGED_12B=1  # 12-byte tagged-union Value
```

### With CMake directly

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/myapp
```

The host reads paths relative to the working directory, so run the test from
the repository root (the binary looks for `Test/user_physics.math`).

### Value encoding

| `USE_TAGGED_12B` | `Value` | Encoding |
| --- | --- | --- |
| `0` (default) | 8 bytes | IEEE-754 NaN-boxing |
| `1` | 12 bytes | Tagged union |

```sh
cmake -S . -B build -DUSE_TAGGED_12B=ON
```

### Recompiling the standard library

`Core/Headers/stdlib_precompiled.h` is the `mathc` image of `Library/*.math`
embedded as a C array. Regenerate it (and `Library/stdlib.mathc`) after editing
the standard library:

```sh
make stdlib
# or directly:
cmake --build DebugBuild --target stdlib
```

`mathc` itself can be used standalone:

```sh
./DebugBuild/mathc out.mathc out_header.h my_array input1.math input2.math ...
```

## The `.math` Language

### Bootstrap (`00_bootstrap.math`)

Defines the primitives everything depends on: parentheses, commas, member
access `.`, sequencing `;`, assignment `:=` / `=`, the while-loop operator `??`,
lazy `if(...)`, the consuming lexical types, and the dynamic `$vec` container.

```
$real ~ \d+ ("." \d+)?       => @f64
$iota ~ \d+ ("." \d+)? "i"   => @f64_trim1
$str  ~ "\"" [^"]* "\""      => @str_slice
$vec  ~                      => @vec_type
```

### Operators

```
(a:real) "+" (b:real) :<50 -> real = @add(a, b)   # left-assoc, precedence 50
(a:real) "^" (b:real) :>75 -> real = @pow(a, b)   # right-assoc, precedence 75
"-"      (x:real)     :80 -> real = @neg(x)       # prefix unary
(x:real) "++"         :90 -> real = x + 1         # postfix
```

- `:<N` left-associative infix at precedence `N`
- `:>N` right-associative infix at precedence `N`
- `:N`  prefix / postfix at precedence `N`
- Maximal munch resolves overlapping operators (`++`, `+++`).

### Functions & multiple dispatch

```
"sin"(x:real)    -> real = @sin(x)
(a:complex) "+" (b:complex) :<50 -> complex = complex(a.re + b.re, a.im + b.im)
```

The same symbol may carry many overloads; the evaluator selects by argument type.

### Structs

```
$vec2(x:real, y:real)
$circle(center:vec2, radius:real)
```

Fields are read/written via `.`; lvalues support assignment:
`c.center.x = c.center.x + dx`.

### Dynamic vectors (`$vec` of any `T`)

```
vec()                         -> vec  = @vnew(0)     # empty
vec(cap)                      -> vec                 # reserved capacity
vec(count, init)              -> vec                 # fill
push(v, item)                 -> vec
pop(v)                        -> T
at(v, idx)                    -> T                   # negative idx counts from end
set(v, idx, val)              -> vec
len(v) / cap(v)               -> real
resize(v, new_len) / shrink(v) / clear(v)
```

Vectors are homogeneous: pushing a `vec2` into a `vec<real>` is rejected at
runtime with a clear type-mismatch error.

### Factorial implemented *in the language*

```
(n:real) "!" :90 -> real = (
    acc := 1;
    i   := n;
    (i > 1) ?? (
        acc = acc * i;
        i   = i - 1
    );
    acc
)
```

## Usage (C API)

```c
#include "math_engine.h"   /* + user_loader.h for user extensions */

Engine eng;
eng_init(&eng);                    /* auto-boot embedded stdlib, zero parsing */

Value v;
if (eng_eval(&eng, "2 + 3 * 4", &v)) {
    char buf[128];
    eng_format_value(&eng, v, buf, sizeof buf);
    printf("%s\n", buf);           /* 14 */
}

/* O(1) module toggling */
eng_set_module(&eng, "trig", false);
```

Loading a user extension from raw `.math` text and reloading the compiled image:

```c
const uint8_t *mathc = NULL;
size_t mathc_len = 0;

/* Compile raw text, hook it into the engine, get back `.mathc` bytes */
user_compile_ext(&eng, raw_math_text, &mathc, &mathc_len);
/* ...host saves `mathc`/`mathc_len` somewhere... */

/* Later: load a compiled image from the host in O(1) memcpy time */
user_load_mathc(&eng, saved_bytes, saved_len);
```

Key entry points (`math_engine.h` + `user_loader.h`):

| Function | Purpose |
| --- | --- |
| `eng_init` | Init engine and auto-boot the embedded precompiled stdlib (returns `bool`) |
| `eng_init_blank` | Init a completely blank-slate engine (no types or ops) |
| `eng_eval` | Parse & evaluate an expression |
| `eng_set_module` / `eng_is_module_active` | Toggle a module in O(1) |
| `eng_format_value` | Format any `Value` to a string |
| `eng_val_type` / `eng_type_name` | Inspect a value's type |
| `eng_last_error` | Last error message |
| `user_compile_ext` | Raw `.math` text → hooks engine + returns `.mathc` bytes |
| `user_load_mathc` | `.mathc` bytes → hooks engine in O(1) |

## The `.mathc` Binary Format

A `.mathc` image is a compact, ABI- and OS-neutral snapshot of the engine's
permanent tables. It starts with a 40-byte `MathcHeader` (magic `"MTHC"`,
version, type/module/symbol/constant counts, arena watermarks, module bitmask)
followed by the payload (`payload_bytes`). Because the header is fixed-width and
the payload is copied verbatim, loading is a single `memcpy` — hence O(1) boot.

## Standard Library Modules

Loaded by `eng_init` from the embedded image:

| Module | Contents |
| --- | --- |
| `boot_syntax` | `(` `)` `,` grouping/argument delimiters |
| `boot_types` | `$real`, `$iota`, `$str`, `$vec` consuming types |
| `boot_control` | `.`, `;`, `:=`, `=`, `??`, `if`, and the `vec`/`push`/`at`/... API |
| `arith` | `+ - * / % ^`, comparisons, unary ops, word ops, `π`, `e` |
| `trig` | `sin` `cos` `tan` `ln` `exp` `abs` `hypot`, `gcd`, `abs_diff`, ... |
| `complex_ext` | `$complex`, overloaded complex arithmetic, `cabs`, `conj` |
| `geom_ext` | `$vec2`, `$circle`, `+`/`-`/`dot`, `move_circle`, `circle_area` |

User modules (e.g. `[user_physics]`) layer on top by declaring their
dependencies (`<[arith]`, `<[trig]`, ...).

## Example Expressions

| Expression | Result | Type |
| --- | --- | --- |
| `2 + 3 * 4` | `14` | real |
| `2 ^ 3 ^ 2` | `512` | real |
| `√16 + 5!` | `124` | real |
| `5++ + +++10` | `18` | real |
| `10 plus 25` | `35` | real |
| `1 + 1i` | `1 + 1i` | complex |
| `(1 + 2i) * (3 + 4i)` | `-5 + 10i` | complex |
| `cabs(3 - 4i)` | `5` | real |
| `sin(π / 2)` | `1` | real |
| `ln(e ^ 4)` | `4` | real |
| `gcd(1071, 462)` | `21` | real |
| `vec2(5, 7) - vec2(1, 2)` | `vec2(x=4, y=5)` | vec2 |
| `dot(vec2(3, 4), vec2(2, 1))` | `10` | real |
| `circle(vec2(1, 2), 5).center.y` | `2` | real |
| `vec3(1, 2, 3) ** vec3(4, 5, 6)` | `32` | real *(user ext)* |
| `mech_energy(2, 10, 5)` | `198.0665` | real *(user ext)* |
| `v := vec() ; push(v, 10) ; push(v, 20) ; push(v, 30) ; v` | `[10, 20, 30]` | vec |
| `pts := vec() ; push(pts, vec2(1,2)) ; at(pts, -1)` | `vec2(x=1, y=2)` | vec2 |

The host demo (`Test/main.c`) verifies the blank-slate guarantee, auto-boots the
stdlib, exercises the `user_loader` workflow (compile raw text → `.mathc` →
reload), prints an architecture report, and runs a 36-case automated suite
(stdlib + user extension + generic vector) as pass/fail.

## Design Goals

1. **Zero built-ins** — a predictable, minimal, auditable core.
2. **Data-driven language** — syntax and semantics live in `.math`, not C.
3. **No engine-side I/O** — the host owns files; the engine owns memory only.
4. **Instant boot** — precompiled, embedded, O(1) `memcpy` load.
5. **Composability** — modules layer via `<[parent]` and toggle O(1).
6. **Small footprint** — fixed-capacity arenas, compact 8-byte AST nodes.
7. **Portability** — plain C11 + libm, ABI/OS-neutral binary format.

## License

No license file is currently included. Add one before distributing or reusing
this code.
