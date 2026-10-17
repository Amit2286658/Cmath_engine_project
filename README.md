# Cmath Engine

A lightweight, fast, embeddable **C11 math expression engine** built around a
*blank-slate* architecture. The C core ships with **zero built-in types,
operators, or syntax** — every capability is loaded at runtime from modular
`.math` extension files. Modules can be enabled or disabled in **O(1)** via a
bitmask flip, allowing a host program to reshape the language on the fly.

## Highlights

- **Blank-slate core** — the engine knows nothing until a `.math` file teaches it.
- **Pattern-driven lexer** — consuming `$type` patterns (`$real`, `$str`, `$iota`).
- **Pratt (top-down operator precedence) parser** — fixity and precedence are data, not code.
- **Multiple dispatch** — operators/functions overloaded on operand types.
- **Flat C-style structs** — `$vec2(x:real, y:real)`, nested structs, compile-time field offsets.
- **Dual value encoding** — 8-byte IEEE-754 **NaN-boxing** (default) or a 12-byte **tagged union**.
- **Modular `.math` library** — bootstrap, arithmetic, trig, complex, geometry.
- **Private `@` intrinsics** — C functions exposed to `.math` definitions only.
- **UTF-8 aware** — operators and constants like `√`, `π`, `^`, `!` work natively.
- **Hardened builds** — fortify source, stack protector, and ASan/UBSan when available.

## Architecture

```
User Program (main.c)
     │
     ▼
[math_engine]   Top-Level Facade / Coordinator
     ├──► [mem_arena]    Isolated memory & 8B/12B Value packing
     ├──► [sym_table]    Dynamic types, flat structs, modules & overloads
     ├──► [lexer]        Pattern-driven consuming types & maximal-munch ops
     ├──► [parser]       Top-down operator precedence (Pratt) parser
     ├──► [ext_loader]   `.math` file & `lib/` folder bootstrapper
     └──► [evaluator]    Type-dispatched AST evaluator & private `@` intrinsics
```

Every source module is self-contained. Changing the memory backend or the
`Value` encoding only requires editing `mem_arena.{h,c}` — the lexer, parser,
and evaluator need no changes.

## Repository Layout

```
CMakeLists.txt
Makefile
Code/
  Core/
    Headers/    math_engine.h  mem_arena.h  sym_table.h  lexer.h
                parser.h  ext_loader.h  evaluator.h
    Memory/     mem_arena.c
    Engine/     math_engine.c  sym_table.c  lexer.c  parser.c
                ext_loader.c  evaluator.c
  Demo/         main.c
  Library/      00_bootstrap.math  10_arith.math  20_trig.math
                30_complex.math    40_geom.math
```

## Building

### With the provided Makefile

```sh
make build      # debug build (default) via Clang -> DebugBuild/myapp
make run        # build if needed, then run
make clean      # remove the selected build dir

make build release   # optimized build -> ReleaseBuild/myapp
make run release
make clean release
```

Override the compiler with `CC`:

```sh
make build CC=gcc
```

### With CMake directly

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/myapp
```

### Value encoding

Select the value representation at compile time with `USE_TAGGED_12B`:

```sh
cmake -S . -B build -DCMAKE_C_FLAGS="-DUSE_TAGGED_12B=1"
```

- `USE_TAGGED_12B=0` (default) — 8-byte NaN-boxed `Value` (real/iota/string/complex/struct in 64 bits).
- `USE_TAGGED_12B=1` — 12-byte tagged union.

### Library location

The demo loads `.math` files from a folder passed as `argv[1]`, defaulting to
`Library` relative to the working directory:

```sh
./build/myapp Code/Library
```

## The `.math` Language

### Bootstrap (`00_bootstrap.math`)

Defines the syntax primitives everything else depends on: parentheses `(` `)`,
commas, member access `.`, sequencing `;`, assignment `:=` / `=`, the while-loop
operator `??`, lazy `if(...)`, and the consuming lexical types.

```
$real ~ \d+ ("." \d+)?       => @f64
$iota ~ \d+ ("." \d+)? "i"   => @f64_trim1
$str  ~ "\"" [^"]* "\""      => @str_slice
```

### Operators

```
(a:real) "+" (b:real) :<50 -> real = @add(a, b)   # left-assoc, precedence 50
(a:real) "^" (b:real) :>75 -> real = @pow(a, b)   # right-assoc, precedence 75
"-"      (x:real)     :80 -> real = @neg(x)       # prefix unary
(x:real) "++"         :90 -> real = x + 1         # postfix
```

- `:<N` = left-associative infix at precedence `N`
- `:>N` = right-associative infix at precedence `N`
- `:N`  = prefix/postfix at precedence `N`
- Maximal munch resolves overlapping operators (`++`, `+++`).

### Functions & dispatch

```
"sin"(x:real)    -> real = @sin(x)
(a:complex) "+" (b:complex) :<50 -> complex = complex(a.re + b.re, a.im + b.im)
```

The same symbol can carry many overloads; the evaluator picks by argument type.

### Structs

```
$vec2(x:real, y:real)
$circle(center:vec2, radius:real)
```

Fields are read/written via `.` and, for lvalues, via assignment:
`c.center.x = c.center.x + dx`.

### Factorial implemented *in the language*

```clojure
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
#include "Core/Headers/math_engine.h"

Engine eng;
eng_init(&eng);                       /* blank slate: no types, no ops */
eng_boot_dir(&eng, "Code/Library");   /* load .math modules           */

Value v;
if (eng_eval(&eng, "2 + 3 * 4", &v)) {
    char buf[128];
    eng_format_value(&eng, v, buf, sizeof buf);
    printf("%s\n", buf);              /* 14 */
}

/* O(1) module toggling */
eng_set_module(&eng, "complex_ext", false);
```

Key entry points (`math_engine.h`):

| Function | Purpose |
| --- | --- |
| `eng_init` | Create a blank-slate engine |
| `eng_boot_dir` | Load all `.math` files from a folder |
| `eng_load_ext` / `eng_load_ext_file` | Load source string / file |
| `eng_set_module` / `eng_is_module_active` | Toggle a module in O(1) |
| `eng_eval` | Parse & evaluate an expression |
| `eng_format_value` | Format any `Value` to a string |
| `eng_val_type` / `eng_type_name` | Inspect a value's type |
| `eng_last_error` | Last error message |

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

The bundled demo (`Code/Demo/main.c`) verifies the blank-slate guarantee,
loads the library, prints an architecture report, and runs the expression suite
above as an automated pass/fail test.

## Design Goals

1. **Zero built-ins** — a predictable, minimal, auditable core.
2. **Data-driven language** — syntax and semantics live in `.math`, not C.
3. **Composability** — modules layer via `<[parent]` dependencies and toggle instantly.
4. **Small footprint** — fixed-capacity arenas, compact AST nodes, no hidden allocation.
5. **Portability** — plain C11 + libm, with no mandatory external dependencies.

## License

No license file is currently included. Add one before distributing or reusing
this code.
