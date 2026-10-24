# The MEP Language — Complete Guide with Machine-Verified Examples

**Version:** 1.0 · **Date:** 2026-10-24
**Companion documents:** [`Engine_Architecture_Guide.md`](Engine_Architecture_Guide.md) (how the engine works) ·
[`engine-audit-2026-10-24.md`](engine-audit-2026-10-24.md) (code audit) ·
`Test/Math Engine Language (MEP) — Complete Reference.md` (the 555-test-verified reference).

---

## ⚙️ How the examples in this guide were produced

Every `⟿` result in this guide is the **literal output of the engine**, not prose.
Examples were executed through a small host harness (`mepverify`) that:

- runs each example block inside a **freshly created `Engine` instance**
  (`eng_create`), so blocks are fully isolated from each other;
- evaluates each `expr ⟿` line with `eng_eval` (stateless) or
  `eng_session_eval` (when the block starts with an `@sess` line) and prints the
  value with `eng_format_value`, or `error: <eng_last_error(...)>` on failure;
- supports `@ext` (compile+link a user `.math` extension), `@mod|name=0|1`
  (O(1) module toggle), `@snap_save` / `@snap_restore` (session snapshot
  banking via `eng_session_snapshot_*`), `@close`, and `@snapdump`.

Formatting conventions below:

| Output form | Meaning |
|-------------|---------|
| `42`, `3.5` | real value (unit-less) |
| `25000` | real with a unit tag; **the tag is not printed** — query it with `unit_of(...)` |
| `"text"` | first-class string |
| `3i`, `2 - 3i` | `$iota` (imaginary) / `$complex` |
| `[1, 2, 3]` | dynamic `$vec` container |
| `[[1, 2], [3, 4]]` | `$mat` (1D and 2D brackets shown; ranks 3–4 nest deeper) |
| `vec2(x=1, y=2)` | flat struct display (`$vec2`, `$vec3`, `$mat2`, `$circle`, `$complex` similarly) |
| `<fn/1>` | lambda/first-class function of 1 parameter |
| `{}`… | `$map` dictionary |
| `error: …` | runtime error text (exact engine message) |

Unless a block starts with `@sess`, examples are *stateless* — each line is
independent. Blocks that share variables open with `@sess` and optionally carry
state commands (`@close`, `@snap_save`, …); the guide shows those lines
verbatim, including their own `⟿` results.

---

## 1. Orientation

MEP is an **extension-driven expression language**: a small C11 core (lexer,
parser, evaluator, arena allocator, symbol tables, linker) plus **text modules**
(`.math`) that declare *everything user-visible* — types, units, operators,
functions, constants, and struct layouts. There are **no hardcoded math,
types, unit names, or operator glyphs** in the engine beyond grammar/ergonomic
rules; change the text, and you change the language.

```text
2 + 2 * 2 ⟿ 6
(2 + 2) * 2 ⟿ 8
25km in m ⟿ 25000
sin(pi/2) ⟿ 1
```

An expression **is** the program. Statements chain with `;` or (inside control
blocks / groups) newlines; the value of any expression is the value of its last
statement:

```text
x := 3; y := x * x; y + 1 ⟿ 10
```

### 1.1 Stateless vs session evaluations

The two APIs every host uses, straight from the harness. `eng_eval` is
*stateless* — `q := 3^2` in one call does not survive to the next:

```text
q := 3^2 ⟿ 9
```

Open a **session** (`@sess` line) when an example needs memory across lines:

```text
@sess
q := 3^2 ⟿ 9
r := sqrt(q) ⟿ 3
q + r ⟿ 12
@close ⟿ ok: session closed
```

Sessions persist variables, type/unit locks, containers, structs, and lambda
closures, and emit an order-tagged, hash-chained **snapshot** per evaluation
(section 19).

---

## 2. Lexical structure

- **Comments** `# …` run to end of line.
- **Strings** are double quoted: `"USD"`, `"Hello, "`. Special chars escape
  with backslash (`"\"quoted\""`, `"a\nb"` shows as-is inside the engine).
- **Reals**: `42`, `3.5`, `1e3`, `.5`-style decimals, `_` digit separators
  (`10_000`), hex `0xff` and binary `0b101`.
- **Booleans**: there are **no `true`/`false` literals** — predicates return
  `1`/`0` reals, truthiness is *non-zero* (section 5).

```text
0xff ⟿ 255
0b101 + 10_000 ⟿ 10005
1e3 ⟿ 1000
-9 / 2 ⟿ -4.5
```

- **Reserved words** (cannot be variable/parameter/tag names):
  `if or then else default switch case jump run break continue return`.
  (`loop` is *not* reserved and works as a tag or variable name.)
- The engine is **UTF-8**; unicode glyphs like `π φ √ ° °C …` are ordinary
  symbols declared in modules (`π = 3.141592653589793; "√" (x) :80 = @sqrt(x)`),
  so `√9 **works** and with units `90deg` is an angle.

```text
√9 ⟿ 3
2π ⟿ 6.283185307
```

### 2.1 Operator & keyword symbols

`and`, `or`, `xor`, `nand`, `nor`, `not`, `plus`, `minus`, `into` are ordinary
symbols (declared in `arith.math`), not grammar: they live in the same
overload-dispatch world as `+`/`-` and can be shadowed per module like any
other operator.

---

## 3. Modules & the `.math` layer

The engine loads 17 named modules from `Library/` at startup (plus an implicit
`default` module where headerless user extensions land):

| Module | Content |
|--------|---------|
| `boot_syntax`/`boot_types`/`boot_control` (`bootstrap.math`) | delimiters, consuming types (`$real $iota $str $vec $mat $fn $map`), wildcards (`$T $any $none`), `; := =` compound assigns, `=> |> call`, `?? ?!?`, ternary, `! & |`, control keywords, symbolic `sym_*` API, `_` casts, `error/assert` |
| `arith` | `+ - into * / %`, word ops (`plus minus`), `^`, comparisons, `~` approx, logic words, prefix ops, `++/--/!`, rounding & predicates, number theory, percentage/interest/finance, progressions, bitwise, PMFs |
| `trig` | circular/hyperbolic & inverses, `ln`, `log*`, `exp`, `sqrt`, `cbrt`, `root`, `hypot`, `sigmoid`, `erf/erfc`, `gamma/lgamma/beta_fn`, `norm_pdf/norm_cdf` |
| `complex_ext` (`complex.math`) | postfix `i`, `$complex`, full complex algebra & `into` composition |
| `units` | 30+ SI/imperial dimensional groups and every postfix unit suffix |
| `conversions` | universal `in`, `to_*` member calls, pair converters |
| `vec` | dynamic vectors/lists/stacks/queues/maps + ranges + HOFs + stats + sort |
| `string` | string algorithms, `fmt`, base formatters, case/trim/pad/split/replace, regex |
| `mat` | `$mat` algebra, Hadamard ops, `transpose/trace/det/inv/linsolve`, decompositions |
| `geom_ext` (`geom.math`) | `$vec2 $vec3 $circle`, dot/cross/mag/dist/lerp, circle/sphere |
| `algebra` | `$mat2`, `det/trace/transpose/inv/solve_2x2`, quadratics, polynomial suite |
| `calculus` | `diff/diff_n/integrate/lim*/sigma/prod/solve/fmin/fmax`, RK4 ODE family, polynomial helpers |
| `physics` | CODATA constants + ~90 equation-of-physics functions |
| `chemistry` | constants + stoichiometry/gases/solutions/electrochem |
| `biology` | Hardy-Weinberg, genetics, molecular biology, growth |
| `conversions` complement + `stdlib.mathc` | precompiled image of all modules |

Module **toggling** is O(1) — disable and re-enable at runtime:

```text
@mod|trig=0 ⟿ ok: trig=0
sin(0) ⟿ error: No active overload for 'sin' with type (real)
@mod|trig=1 ⟿ ok: trig=1
sin(0) ⟿ 0
```

### 3.1 User extensions (`.math` text, compiled & linked at runtime)

`@ext` compiles a `.math` module body, **topologically links it** (`<[dep]`),
and hot-swaps the engine's serialized 48-byte-header `.mathc` image. Extension
symbols are callable **unqualified** in the global namespace:

```text
@ext ⟿ ok: extension linked, mathc bytes=216137
[units_demo]
uin(y) = y * 2
@sess
uin(5) ⟿ 10
```

Declaring operators in extensions uses the same grammar as built-ins
(`(a) plus (b) :<45 = a + b`), complete with overloads, precedence (`:<` left /
`:>` right assoc), and type variables (section 6.2).

---

## 4. Variables & assignment

```text
x := 1; x += 2; x ⟿ 3
z := 1; z = 42; z ⟿ 42
```

| Operator | Meaning |
|----------|---------|
| `:=` | declare (or re-declare) & store; evaluates to the stored value |
| `=` | store into an *existing* variable (mutate) |
| `+= -= *= /= %= ^=` | compound forms (`x += y` ≡ `x = x + y` via `@store`) |
| `,` | multi-declaration / destructuring (`r1, r2 := quad_roots(1, 0, -4)`) |

**Scoping:**

- *( ) is a **same-scope grouping**: declarations inside survive the group:
- *{ }* opens a **child lexical scope**: `:=`-declarations are private and shadow
  outer names; `=` mutates outer variables through:

```text
{ x := 3; x * x } ⟿ 9
w := 1; { w := 99; }; w ⟿ 1
```

Assignment evaluates to the assigned value (as the session examples show),
which is why `@sess` blocks print the assignments.

### 4.1 Type & unit locks on session variables

A session variable's *type* and *unit* persist between evaluations
(`MSP1|…|q:real:none:*:*=9` in the snapshot section); assignments that change a
locked type/unit are rejected at runtime. `raw_val(x)` strips the unit,
`with_unit(v, donor)` adopts the donor's unit:

```text
unit_of(with_unit(7, 3km)) ⟿ "distance"
raw_val(25km) ⟿ 25000
```

---

## 5. Truthiness, comparisons & logic

Predicates return `1`/`0`; any non-zero value is truthy (including non-empty
strings/containers):

```text
2 < 3 < 5 ⟿ 1
1 < 4 == 4 ⟿ 0
"x" == "x" ⟿ 1
if 0 { 1 } else { 2 } ⟿ 2
if 5 { 7 } else { 9 } ⟿ 7
if "" { 1 } else { 2 } ⟿ 2
if [1] { 3 } else { 4 } ⟿ 3
```

Comparisons **chain left-associatively** with arithmetic precedence folding
(`1 < 4 == 4` ⇒ `(1 < 4) == 4` ⇒ `1 == 4` ⇒ `0`).

| Operator | Precedence | Signature (from `arith.math`) |
|----------|-----------|-------------------------------|
| `~` | `:‹30` | `(a:real, T) ~ (b:real, T)` — approx (`abs(a-b) ≤ 0.001+1e-12`); lifted per type in `[complex_ext] [vec] [mat] [geom_ext] [algebra]` |
| `&&` / `and` | `:‹20` | short-circuit generative: `(a != 0) ? ((b != 0) ? 1 : 0) : 0` |
| `xor`, `^^` | `:‹18` | `((a != 0) != (b != 0))` |
| `||` / `or` | `:‹15` | `(a != 0) ? 1 : ((b != 0) ? 1 : 0)` |
| `nand` | `:‹20` · `nor` | `:‹15` — negations |
| `not(x)` | function `:25` | strict `@lnot` |
| `!` prefix | `:80` | strict boolean prefix-`NOT` on `(a:real, none)` |
| `&`, `|` | `:‹35`, `:‹34` | distributed-combinator forms (see §6.4) |

`~` composes over containers and structs exactly like `==`:
`vec(1,2) ~ vec(1.0001, 2.0001) ⟿ 1` (exact elementwise semantics in each
module's declaration).

Nothing: use `==` for exact membership and `~` is *not* chained; `~=`
**does not exist** (there is `!(a == b)`, or `is_close(a, b, tol)`).

---

## 6. Arithmetic & operators

### 6.1 Core scalar operators (module `[arith]`)

| Form | Precedence | Definition |
|------|-----------|------------|
| `(a) "+" (b)` | `:‹50` | `@add` — **unit-propagating** (see §11) |
| `(a) "-" (b)` | `:‹50` | `@sub` — unit-propagating |
| `(a) "*" (b)` | `:‹60` | `@mul` (unit algebra multiplies) |
| `(a) "/" (b)` | `:‹60` | `@div` — **real division**, no integer form |
| `(a) "%" (b)` | `:‹60` | `@mod` — C-fmod semantics: sign of the *dividend* |
| `(a) "^" (b)` | `:>85` | right-assoc exponentiation (`-x^2 = -(x^2)`) |
| `(a) into (b)` | `:>55` | `@add` ("sum into") — one of *several* `into` meanings (§6.5) |
| `(a) plus/minus (b)` | `:‹45` | inlined aliases for `+`/`-` |
| `-(x)` / `+(x)` / `√(x)` | `:80` | `@neg` / identity / `@sqrt` |
| `(x) "!"` postfix | `:90` | factorial (non-negative integer ≤ 170) |
| `++x` `+++x` prefix | `:85` | `x+1` / `x+2` |
| `x++` `x+++` postfix | `:90` | `x+1` / `x+2` — **do not mutate**; value forms only |

```text
7 / 2 ⟿ 3.5
10 % 3 ⟿ 1
-10 % 3 ⟿ -1
2^3^2 ⟿ 512
-3^2 ⟿ -9
(1 + 2)(3 + 4) ⟿ 21
5! ⟿ 120
5 << 2 ⟿ 20
~5 ⟿ -6
++4 ⟿ 5
4++ ⟿ 5
```

Note `(1 + 2)(3 + 4)` — implicit multiplication between parenthesized groups
(ergonomic rule of `hardcode.c`); `2a`-style adjacency composes the same way.

### 6.2 Overload dispatch & type variables

Every operator declared in `.math` carries a **signature**: `(a:real, T) "+" (b:real, T) :<50 -> real, T = @add(a, b)`.

- `(a:real, T)` — parameter `a` of type `real`, **plus a type variable `T`**
  (the unit tag of that side). Type variables are matched & bound together with
  the dispatch (evaluator's MULTIPLE DISPATCH: *type match 4/1 points, unit
  generic binds, unit locked +8, omitted +1*).
- `-> real, T` — return type/unit (compound tags preserve physically).
- `:‹/›N` — associativity & precedence (left/right).
- `@add` — the C intrinsic that *implements* the body (MEP never hardcodes
  math; bodies are either intrinsic calls or pure MEP expressions).

The dispatch scores overloads (exact type +4, wildcard `T` +1, generic/unit
binding +2, **unit-locked** exact +8) and the winner binds `T`. Generic
binding allows the zero special case (`0` is unit-free and joins any unit),
which keeps `0 + 5km` valid.

### 6.3 Integer vs real

`/` is always real division (`7 / 2 ⟿ 3.5`); there *is* no `//`. For
integer-style work use `floor`, `trunc`, `@floor`-backed helpers, or
`% gcd`-family functions. Bitwise ops (`<< >> bit_and …`) declare
`(a:real, none)` — integer-valued unitless reals.

```text
5 << 2 ⟿ 20
bit_and(12, 10) ⟿ 8
popcount(11) ⟿ 3
gcd(12, 18) ⟿ 6
```

### 6.4 Distributed `&` / `|` comparisons

`hardcode.c` flattens `x & y == 1 | 2` into elementwise relational forms so
short guards read naturally (`case > 10:` patterns in switch rely on the same
machinery). The *additive/relational* classification of an operator is
**derived per overload** (walk the overload's body AST; audit §4 P5) — module
declared `+`-bodies behave; custom operators that compose additive/relational
kernels in their bodies do too.

### 6.5 The many lives of `into`

`into` is a *library operator* with a family of right-biasing overloads
(`:>55`), all morphing the LEFT operand "into" shape of the right:

| Overload | In module | Effect |
|----------|-----------|--------|
| `(a:real) into (b:real)` | `[arith]` | `a + b` |
| `(item) into (v:vec)` | `[vec]` | **push** item (append) |
| `(a:str) into (b:str)` | `[string]` | concatenate |
| `(a:iota) into (b:iota)` | `[complex_ext]` | add |
| `(a:real) into (b:iota)` | `[complex_ext]` | **construct** `complex(a, b)` |
| `(a) into (z:complex)` | `[complex_ext]` | add `a` into real part |
| `(a:vec2) into (b:real)` | `[geom_ext]` | promote: `vec2 +` z ⇒ **`vec3`** |
| `(a:mat2) into (b:mat2)` | `[algebra]` | elementwise add |

```text
3 into 4 ⟿ 7
2 into vec(1, 2) ⟿ [2, 2]
"a" into "bc" ⟿ "abc"
5 into 5i ⟿ 5 + 5i
vec2(1, 2) into 3 ⟿ vec3(x=1, y=2, z=3)
3 into vec2(1, 2) ⟿ vec3(x=1, y=2, z=3)
```

(`2 into vec(1, 2)` looks strange but is exactly right: `vec(1, 2)` is *one
copy of 2* — see the container constructor rules in §8.1.)

---

## 7. Predicates, rounding & helpers

```text
abs(-3) ⟿ 3
floor(2.7) ⟿ 2
ceil(2.1) ⟿ 3
round(2.5678, 2) ⟿ 2.57
round(2.5678) ⟿ 3
trunc(-3.75) ⟿ -3
fract(3.75) ⟿ 0.75
sign(-9) ⟿ -1
clamp(7, 0, 5) ⟿ 5
is_close(1.0, 1.001) ⟿ 0
is_nan(nan) ⟿ 1
is_inf(inf) ⟿ 1
max(2, 7) ⟿ 7
```

Constants: `π`/`pi`, `e`, `phi`/`φ`/`ϕ`, `inf` (= `1e300*1e300`), `nan`
(= `(-1)^0.5`) — declared as ordinary constants in `[arith]`, which is why
`unit_of(pi)`-style introspection is possible on anything.

---

## 8. Collections

### 8.1 Constructors & the variadic rule (verified exactly)

| Call | Result | Why |
|------|--------|-----|
| `vec()` / `vec<real>()` | empty `$vec` | 0-arg ctor |
| `vec(n)` (unitless real) | empty `$vec` with **capacity n** (len 0) | hardcode variadic path |
| `vec(n, v)` | `$vec` of **n copies of v** | 2-arg fill |
| `vec(x, y, …)` (≥3 args, or any 2-arg call whose 1st arg is not a unitless real) | `$vec` **literal list** `[x, y, …]` | variadic ctor |
| `vec<T>(…)` / `list<T>(…)` / `stack<T>(…)` / `queue<T>(…)` | as above with the element type locked to `T` | `hardcode.c` container-type spec |
| `list(v)` / `stack(v)` / `queue(v)` | 1-element vec containing `v` | singleton forms |
| `map()` / `dict()` | empty `$map` | |
| `map(k1, v1, k2, v2, …)` | `$map` built pairwise (even count) | variadic map ctor (2-arg HOF `map(coll, fn)` untouched) |

```text
vec(1, 2) ⟿ [2]
vec(3, 4).len() ⟿ 3
vec(1, 2, 3) ⟿ [1, 2, 3]
vec(5, 6, 7) ⟿ [5, 6, 7]
vec<real>(3, 2) ⟿ [2, 2, 2]
```

⚠️ **The #1 beginner trap:** `vec(1, 2, 3)` is a 3-element list, but
`vec(1, 2)` is *one copy of 2* (`[2]`). Prefer `[x, y, z]` literals (which make
`$mat`) or comprehensions when you want literal contents.

`cap(v)` reserves; growth doubles (`.push` after fresh start reports
`cap ⟿ 4`), `resize` pads with zeros: `resize(vec(3), 2) ⟿ [0, 0]`.

Element type must be a **declared type name** — `stack<int>()` is an error;
use `stack<real>()`:

```text
st := stack<real>(); st.push(1); st.push(2); st.pop() ⟿ 2
q := queue<real>(); q.enqueue(1); q.enqueue(2); q.dequeue() ⟿ 1
ls := list<real>(); ls.push(9); ls.at(0) ⟿ 9
v2 := vec<real>(); 5 into v2; cap(v2) ⟿ 4
```

### 8.2 `$vec` vs `$mat` literals (know your brackets!)

```text
type_of([1, 2]) ⟿ "mat"
type_of(vec(1, 2)) ⟿ "vec"
type_of([i^2 for i in 1...5]) ⟿ "vec"
```

- `[a, b, c]` / `[a b; c d]` literals are `$mat` (rank 1 = row vector, rank 2…4 nesting).
- `vec(...)`/`list<T>` and **comprehensions** produce `$vec` (dynamic).
- `sum(v:vec)` and friends take `$vec`; `sum([4, 6, 8])` reports
  `error: No active overload for 'sum' with type (mat)`.
  Convert: `[x for x in [4, 6, 8]]` or use `mat_sum(m)`.

### 8.3 Indexing, slicing & member calls

`v[i]` (negative = from end), `v[a...b]` inclusive, `v[a..<b]` half-open,
`v[a...b by s]` stepped, `m[i, j]` for `$mat`, `s[i]`/`s[a...b]` for strings:

```text
[1, 2, 3][-1] ⟿ 3
[1, 2, 3][1...2] ⟿ [2, 3]
m := [1, 2; 3, 4]; m[1][0] ⟿ 3
"abcdef"[0..<3] ⟿ "abc"
at("abcdef", 1...3) ⟿ "bcd"
"HELLO".len() ⟿ 5
[1, 2, 3].len() ⟿ error: No active overload for 'len' with type (mat)
```

`.name(...)` postfix calls are UFCS: `v.len()` ≡ `len(v)` (declared for
`$vec`, `$map`, `$str`; `$mat` uses `mat_len`, `mat_rows`, `mat_cols`).

### 8.4 Ranges

```text
1...5 ⟿ [1, 2, 3, 4, 5]
[i for i in 1..<5] ⟿ [1, 2, 3, 4]
[i for i in 2...10 by 3] ⟿ [2, 5, 8]
[i for i in 5...1] ⟿ [5, 4, 3, 2, 1]
range(0, 1, 0.25) ⟿ [0, 0.25, 0.5, 0.75, 1]
linspace(0, 1, 5) ⟿ [0, 0.25, 0.5, 0.75, 1]
```

| Operator | Precedence | Semantics |
|----------|-----------|-----------|
| `a...b` | `:‹45` | inclusive `@vrange(a, b, 0)` |
| `a..<b` | `:‹45` | excludes `b` (`@vrange(a, b, 1)`) |
| `a<..b` | `:‹45` | excludes `a` (`@vrange(a, b, 2)`) — **needs both ends** (`<..5` alone is an error) |
| `a..b` | `:‹45` | *exclusive both ends* (`@vrange(a, b, 3)`) — `[i^2 for i in 1..5] ⟿ [4, 9, 16]` |
| `r by step` | `:‹44` | steps an existing range (`@vrange_step`) |

Negative steps count down naturally (`5...1 ⟿ [5,4,3,2,1]`).

### 8.5 Iteration & comprehensions

Iteration has **two exact forms** (and that's it):

1. **Parenthesized loop** — `(x in v) { body }` value/`(i, x in v) { body }` indexed:
   the body's last expression is discarded;

2. **Comprehension** — `[expr for x in rng]` / `[expr for i, x in v]` with
   `if` filter, produces a `$vec`:

```text
v := 0; (i, x in vec(10, 20, 30)) { v = i * x }; v ⟿ 60
[i * 10 + x for i, x in vec(5, 6, 7)] ⟿ [5, 16, 27]
[x for x in 1...10 if x % 3 == 0] ⟿ [3, 6, 9]
sum([i^2 for i in 1...5]) ⟿ 55
```

The bare `for` keyword **only** belongs to comprehensions; `for x in vs { }`
*outside* brackets is a parse error:

```text
s := 0; for i in 1...3 { s += i }; s ⟿ error: Unexpected 'for' outside of '[expr for x in ...]' comprehension
```

while-loops and do-whiles are lazy operators `??` / `?!?` (precedence 3):

```text
k := 0; (k < 3) ?? { k += 1 }; k ⟿ 3
k := 0; { k += 1 } ?!? (k < 3); k ⟿ 3
```

### 8.6 Higher-order functions (`$vec`/`$mat`, all pure MEP)

| Function | Arity | Note |
|----------|-------|------|
| `map(v, f)` | returns `$vec` | `[f(x) for x in v]`; `$mat` overload reshapes |
| `filter(v, f)` | `$vec` | `filter(m:mat, f)` → `$vec` of kept flat elems |
| `reduce(v, f)` / `reduce(v, init, f)` / `fold(v, init, f)` | | left fold |
| `any(v, f)`, `all(v, f)` | short-circuit | |
| `find(v, f)`, `count_if(v, f)` | | first match value / count |
| `zip_with(a, b, f)` | pairwise | shape-checked |

```text
map(vec(4, 6, 8), x => x*2) ⟿ [8, 12, 16]
[4, 6, 8] |> map(x => x*2) ⟿ [8, 12, 16]
filter(vec(1, 2, 3, 4), x => x > 2) ⟿ [3, 4]
reduce(vec(1, 2, 3, 4), 1, (a, b) => a*b) ⟿ 24
reduce(vec(1, 2, 3), (a, b) => a+b) ⟿ 6
zip_with(vec(1, 2, 3), vec(10, 20, 30), ((a, b) => a + b)) ⟿ [11, 22, 33]
```

⚠️ **lambda literal arity rule:** multi-parameter lambdas must be
`((a, b) => body)` — the *whole* thing is one lambda whose parameter list is
`(a, b)`. `expr(t, (a, b => a*b))`-style splits into a group! (see §9.1).

### 8.7 Dictionaries (`$map`)

```text
m0 := map(); put(m0, "k1", 3); m0["k1"] ⟿ 3
m1 := map(); put(m1, "k1", 3); "k1" in m1 ⟿ 1
m2 := map(); put(m2, "k1", 3); put(m2, "k1", 4); m2["k1"] ⟿ 4
m3 := map(); put(m3, "a", 1); put(m3, "b", 2); vals(m3) ⟿ [1, 2]
m4 := map(); get_or(m4, "nope", 99) ⟿ 99
m5 := map(); put(m5, "a", 1); remove(m5, "a"); len(m5) ⟿ 0
```

All of: `map()/dict()`, `map_set/put/set`, `map_get/get/(get_or)`,
`map_has/has_key/(k in m)`, `map_del/remove`, `keys/vals/values/entries`,
`len/is_empty/clear` — keys are strings (any value stringify via `_`).

### 8.8 Vector statistics & algorithms

```text
sort_desc(vec(3, 1, 2)) ⟿ [3, 2, 1]
unique(vec(1, 2, 2, 3)) ⟿ [1, 2, 3]
median(vec(3, 1, 2)) ⟿ 2
argmax(vec(4, 9, 2)) ⟿ 1
cumsum, cumprod, diff_vec, min, max, sma(v, w), sample_variance, corr …
```

(All declared in `[vec]` §5–6: `vec_sum sum mean variance stddev sma sort
sort_desc reverse vec_min vec_max min max argmin argmax cumsum cumprod
diff_vec dot norm linspace unique zip_with median sample_variance
sample_stddev cov corr linreg_slope linreg_intercept linreg_r2`.)

---

## 9. Functions, lambdas & closures

### 9.1 Lambda arrow syntax

```text
(sq := (x => x*x); sq(9)) ⟿ 81
g := ((a, b) => a * b); g(3, 4) ⟿ 12
5 |> (x => x + 1) ⟿ 6
f1 := x => x + 1; call(f1, 10) ⟿ 11
f2 := (a, b) => a * b; call(f2, 3, 4) ⟿ 12
double(z) = z * 2; double ⟿ <fn/1>
double(z) = z * 2; double(3.5) ⟿ 7
fac(n) = (n <= 1) ? 1 : n * fac(n - 1); fac(5) ⟿ 120
```

- **One parameter:** `x => body` or one-parameter group `(x => body)`.
- **Multi-parameter:** `((a, b) => body)` — the outer group makes the whole
  thing a lambda whose parameter list is `(a, b)`. Writing
  `(a, b => body)` parses as an ordinary parenthesized group `[a, lambda(b)]`
  instead. (This is the single most common syntax trap; see the ODE
  experiment in §16.1.)
- **Named functions:** `name(p1, p2) = body` — plain session/extension syntax
  with **recursion supported**; function symbols display as `<fn/N>`.
- **Default parameters:** `f(x, digits = 0) = …` — verified with
  `round`/`norm_pdf` (§17).

### 9.2 Pipes

`x |> f` (`:<12`, `@pipe`) feeds the left value as the first argument of a
function/callable on the right:

```text
[1, 2, 3, 4] |> filter((x => x % 2 == 0)) ⟿ [2, 4]
[4, 6, 8] |> map(x => x*2) ⟿ [8, 12, 16]
```

### 9.3 Lazy-bound calculus-style calls (varargs)

`diff(x, x^2, 3)`-style calls bind the *symbol* `x` analytically (not its
value). The engine detects "first argument is an identifier" and binds it as a
parameter (parser 6f lazy-call path). This is how the calculus/symbolic
modules take expression arguments.

### 9.4 Closure semantics — a verified limitation

Lambdas bind **lexically by slot index**; a lambda that *escapes* its defining
frame (returned through another lambda) does not capture the enclosing frame's
locals — the escapee reads whatever the *calling* frame has in that slot
(zero/stale):

```text
mkv := (n => (x => x + n)); f := mkv(10); f(5) ⟿ error: No active overload for '*' with types (fn, real)
mkv := (n => (x => x + n)); add_n(10)(5) ⟿ error: Undefined function or struct 'add_n'
outer_fn() = { z := 42; (x => x + z) }; inner := outer_fn(); inner(1) ⟿ error: No active overload for '*' with types (fn, real)
```

**Rule of thumb:** closures over *parameters and current-scope variables* are
fine (that's every HOF usage); closures over locals **of a returned lambda**
are not. State → session variables (§19), not captured locals. (Listed in §22
limitations and in the architecture guide.)

---

## 10. Control flow

### 10.1 Ternary

`(cond) ? (a) : (b)` (`:›10`, lazy — only the selected branch evaluates):

```text
1 ? 2 : 3 ⟿ 2
0 ? 2 : 3 ⟿ 3
```

### 10.2 `if`

Keyword form (grammar): `if cond { … } else { … }`; multi-branch via `or`:

```text
if 1 { "y" } else { "n" } ⟿ "y"
if 0 { 1 } or 1 { 2 } else { 3 } ⟿ 2
```

`or`/`then`/`else`/`default` also exist as *functions*
(`or(cond, body) = @or_branch(...)` — see `[boot_control]` §10), and the
3-argument functional form `if(c, a, b)` is eagerly dispatched through a
dedicated meta-intrinsic path (`evaluator.c` `INTR_IF` fast path).

### 10.3 `switch` — pattern matching

```text
switch (5) { case 1, 2 : { "low" }  case 3...10 : { "mid" }  else : { "other" } } ⟿ "mid"
switch (77) { case < 10 : { "small" }  else : { "big" } } ⟿ "big"
switch (9) { case 1...4 : { "lo" } case 5...8 : { "mid" } else : { "hi" } } ⟿ "hi"
```

Patterns: exact values, **comma lists** (`case 1, 2`), **inclusive ranges**
(`case 3...10`), **relational guards** (`case < 10`, `case > 100`),
**destructuring** (`case vec2(0, y)` binds `y`) and **guard composites**
(`if x == y`):

```text
(switch (vec2(0, 3)) { case vec2(0, y) : { y * 10 } case vec2(x, y) if x == y : { x * 100 } else : { 0 } }) ⟿ 30
```

Cases never fall through; the matching block's last expression is the switch
value. `else :` (or `default :`) closes the switch. `break`/`return` exit a
case early (documented in the Big Reference §switch).

### 10.4 Loop exits & tagged scopes

```text
acc := 0; (x in vec(1, 2, 3)) { acc += x; if (acc >= 3) { break } }; acc ⟿ 3
```

`break`/`continue` (`@break_loop`, `@continue_loop`) work in loops; tagged
scopes (`tag : { … }`) plus `jump(tag)`, `run(tag)`, `break(tag)`,
`break(tag, val)`, `continue(tag)` implement multi-level exits and
run-loop-restarts (`AST_TAGGED_SCOPE`/`AST_RUN_TAG` iterate-restart machinery,
evaluator.c). `return`/`return(val)` exits functions and `switch` cases.

---

## 11. Units & dimensional analysis

Units are **ordinary symbols with a dimensional tag**. `[units]` declares
~30 dimensional groups (`@dim(group, m, kg, s, A, K, mol, cd)` exponents) and
every suffix as a postfix operator (`:95`):

```text
25km ⟿ 25000
unit_of(25km) ⟿ "distance"
type_of(25km) ⟿ "real"
3km + 500m ⟿ 3500
5km in mi ⟿ 3.106855961
sin(90deg) ⟿ 1
2min_t + 30sec ⟿ 150
```

Key behaviors (all verified):

- **the unit tag is metadata, not text**: `25km` prints `25000`; ask the tag
  with `unit_of(x)` / `unit_dim(x, axis)` (7D SI exponent vector) /
  `dim_vec(x)` / `same_dim(a, b)`.
- **`+`/`-` are unit-checked** — but a wrong *token* never gets that far:
  `s` is not part of the `time` group's suffix set (`sec` is), so the engine
  rejects the term as an unknown symbol:
- **`in` converts within a group** (`(val, T) in (unit, T) = @div(val, unit)`
  in `[conversions]`, `:<42`): `25km in m`. Temperature intents use the
  explicit `to_degC` member forms (§18.1).
- **units flow through** arithmetic, stored variables, function returns
  (`-> real, distance`), structs, comprehensions, lambdas — and even
  **complex numbers**: `(50m)i` is an imaginary meter value whose
  `unit_of ⟿ "distance"`.

```text
3km + 2s ⟿ error: Undefined symbol or constant 's'
```
- postfix `i` on a unitful value follows the unit: `(50m)i ⟿ 50i`.

Special suffixes worth knowing: angle `deg ° rad arcmin arcsec`, ratio
`pct dB dBm`; `min_t` exists because plain `min` is the function.

### 11.1 Conversions module

Two families:

1. **`in`** — universal, any same-unit-group pair (§11).
2. **`to_<unit>` member calls** on a tagged value — `(25km).to_mi()`
   style postfix conversions, plus hundreds of explicit
   `km_to_mi`, `m_to_cm`, `ft_to_in`, `sec_to_min`, `mps_to_kph` pair
   functions (the full lists are `[conversions]` §2–10).

```text
25km in cm ⟿ 2500000
```

---

## 12. Complex numbers (`$iota` / `$complex`)

Postfix `i` (`:95`) turns a real into `$iota` (imaginary); `real into iota`
composes to `$complex`; the whole algebra (from `[complex_ext]`) is verified:

```text
3i, conjugates, composition:
3 + 4i ⟿ 3 + 4i
conj(2 + 3i) ⟿ 2 - 3i
re(3 + 4i) ⟿ 3
im(3 + 4i) ⟿ 4
cabs(complex(3, 4)) ⟿ 5
z := 1 + 1i; z^2 ⟿ 1.22465e-16 + 2i
```

| Function | Note |
|----------|------|
| `cabs`, `carg` | modulus / argument (atan2) — see fence below |
| `polar(r, θ)` | converts a complex to `polar` struct |
| `cexp_i(θ)` | `cos θ + i·sin θ` — **e^{iθ}** |
| `exp(z) ln(z) sin(z) cos(z) tan(z) sinh cosh tanh` | full complex transcendentals |
| `z ^ p` | complex/real exponents, `:>85` |
| `√z`, `sqrt(z)` | principal root |

```text
carg(1 + 1i) ⟿ 0.7853981634
polar(2, pi/2) ⟿ 1.22465e-16 + 2i
cexp_i(pi) ⟿ -1 + 1.22465e-16i
```

`cexp_i` covers Euler's identity — the result of the fence above is
`−1 + 1.2e-16i` (the residual 1.2e-16 is the `sin π` float artifact).

⚠️ `i` is a postfix operator, not an identifier — a **bare** `i` is simply
not a value:

```text
exp(i * pi) ⟿ error: Undefined symbol or constant 'i'
```

Use the multiplied/juxtaposition forms (`5 into 5i`, `z := 1 + 1i`) or
`cexp_i(...)` instead.

---

## 13. Strings

```text
"hello, " + "MEP!" ⟿ "hello, MEP!"
substr("hello", 1, 3) ⟿ "ell"
len("hello") ⟿ 5
upper("mep") ⟿ "MEP"
split("a,b,c", ",") ⟿ ["a", "b", "c"]
index_of("hello", "l", 0) ⟿ 2
replace("aaa", "a", "b") ⟿ "bbb"
str_count("banana", "an") ⟿ 2
pad_left("7", 3, "0") ⟿ "007"
"a" into "bc" ⟿ "abc"
```

Formatting via `@str_fmt` brace syntax (`fmt(f, a, …)` / `format`):

```text
fmt("{} + {} = {}", 1, 2, 3) ⟿ "1 + 2 = 3"
fmt("{:b}", 5) ⟿ "101"
fmt("{:X}", 255) ⟿ "FF"
fmt("{:e}", 1234.5) ⟿ "1.234500e+03"
```

Regex family (`@regex_*`): `regex_match find find_all replace split`:

```text
regex_find_all("a1 b22 c333", "[a-z]\\d+") ⟿ ["a1", "b22", "c333"]
```

String helpers in `[string]`: `is_empty substr str_repeat str_reverse
starts_with ends_with chars str_join (sep="", default) to_bin to_hex words
lines trim/ltrim/rtrim lower/upper (as to_upper/to_lower aliases)
index_of contains replace_first`.

### 13.1 The `_` cast operators

`x_` (postfix `:95`) → canonical string; `_"…"` (prefix `:85`) → parse into the
best runtime type:

```text
_"42" ⟿ 42
(3.5_) ⟿ "3.5"
_"100 km" ⟿ 100000
```

(`_"100 km"` parses a unit-ful literal and **converts to the SI base unit**
— km ⇒ m scaling. `fromstr` covers reals/complex/matrices/structs per the
reference.)

---

## 14. Structs (`$vec2 $vec3 $circle $mat2 $complex`)

Declared with `$name(field, field, …)`; flat, non-consuming, field-units
default `:real`. Construction is a plain call, fields read with `.`:

```text
p := vec2(3, 4); p.x ⟿ 3
mm2 := mat2(1, 2, 3, 4); mm2.a ⟿ 1
c := circle(center:vec2, radius) — construction: circle(vec2(0, 0), 2)
circle_area(circle(vec2(0, 0), 2)) ⟿ 12.56637061
sphere_vol(3) ⟿ 113.0973355
```

Operators are *per-struct* declared overloads in `[geom_ext]`/`[algebra]`:

```text
vec2(3, 4) + vec2(1, 1) ⟿ vec2(x=4, y=5)
mat2(1, 2, 3, 4) * mat2(1, 0, 0, 1) ⟿ mat2(a=1, b=2, c=3, d=4)
det(mat2(1, 2, 3, 4)) ⟿ -2
```

Geometry inventory (`[geom_ext]`): `dot dot3 cross2 cross3 vec2_mag
vec2_dist vec3_mag vec3_dist lerp vec3_path_len move_circle circle_area
circle_circum sphere_vol sphere_area`; the `**` operator is the vec3 dot
product (`(a:vec3) "**" (b:vec3) :‹20 = a.x*b.x + a.y*b.y + a.z*b.z`).

$mat2 (`[algebra]`): arithmetic, `det trace transpose inv_mat2`,
`solve_2x2` (Cramer), quadratics:

```text
quad_roots(1, 0, -4) ⟿ vec2(x=2, y=-2)
quad_disc(1, 0, -4) ⟿ 16
solve_2x2(1, 1, 3, 1, -1, 1) ⟿ vec2(x=2, y=1)
```

Poly suite uses **dynamic `$vec` coefficient vectors** (ascending):

```text
poly_eval(vec(1, 0, 1), 2) ⟿ 5
poly_deriv(vec(1, 2, 3)) ⟿ [2, 6]
poly_integ(vec(1, 2), 0) ⟿ [0, 2]
poly_add(vec(1, 2), vec(0, 1, 1)) ⟿ [2, 1, 1]
poly_mul(vec(1, 1), vec(1, 1)) ⟿ [1]
poly_newton(vec(1, 0, -4), 1) ⟿ 0.5
```

(Newton verified against root of `1 − 4x² = 0` → `0.5`.)

---

## 15. Matrices & tensors (`$mat`, 1D–4D)

### 15.1 Literals, shape, access

```text
m := [1, 2; 3, 4]; mat_shape(m) ⟿ [2, 2]
mat_rows([1, 2, 3; 4, 5, 6]) ⟿ 2
```

`;` separates rows inside bracket literals; nesting `[[1,2],[3,4]]` is
equivalent; ranks 1–4 (`$mat` carries rank + per-axis dims). Element write:
`m[i, j] = v` (index-assignment through `eval_index_op`).

### 15.2 Arithmetic, Hadamard & powers (verified)

```text
[1, 2; 3, 4] + [1, 0; 0, 1] ⟿ [[2, 2], [3, 5]]
[1, 2; 3, 4] .* [2, 2; 2, 2] ⟿ [[2, 4], [6, 8]]
[1, 2; 3, 4] ^ 2 ⟿ [[7, 10], [15, 22]]
[[0.6, -0.7], [-0.2, 0.4]] inverse sanity:
inv([4, 7; 2, 6]) ⟿ [[0.6, -0.7], [-0.2, 0.4]]
t := transpose([1, 2; 3, 4]); mat_shape(t) ⟿ [2, 2]
```

`*` is C-kernel `@mmul` (mat·mat, mat·vec, vec·mat); scalar `*`/`/` map
elementwise; `.* ./ .^` are Hadamard (pure MEP, §audit Q2 candidates for
C-kernel acceleration); `^` integer power via binary exponentiation.

### 15.3 Solvers & decompositions

```text
A := [2, 1; 1, 3]; b := [5, 10]; linsolve(A, b) ⟿ [1, 3]
e := eigvals([2, 0; 0, 3]) ⟿ [3, 2]
```

| Family | Functions |
|--------|-----------|
|_core_ | `det inv linsolve (mat,vec)/(mat,mat)` — C kernels `@mdet @minv @mlinsolve` |
| shapes | `mat_rank mat_len mat_dim mat_rows mat_cols mat_shape reshape flatten` |
| builders | `mat_fill(r, c, v) mat_zeros mat_ones mat_eye diag (both ways)` |
| reductions | `trace mat_sum mat_mean mat_min mat_max frobenius_norm mat_norm1 mat_norm_inf` |
| products | `outer kron vstack hstack tril triu` |
| decomposition | `lu_l lu_u lu_p` (MEP LU+pivot), `qr_q qr_r` (Gram–Schmidt), `chol`, `eigvals eigvecs` (`@meig*`), `svd_s svd_u svd_v` (`@msvd`) |
| advanced | `pinv lstsq power_vec power_iter matrix_rank spectral_norm cond` |
| higher-order | `map filter reduce fold any all find count_if mat_zip` on flat |

---

## 16. Calculus (`[calculus]`, all numerical)

Everything takes a **symbol argument** (`x` unquoted!) + an inline expression
(lazy binding), preserving units through `raw_val/with_unit`:

```text
diff(x, x^2, 3) ⟿ 6
diff_n(x, x^3, 2, 2) ⟿ 12
integrate(x, x^2, 0, 2) ⟿ 2.666666667
solve(x, x^2 - 4, 1) ⟿ 2
sigma(k, k^2, 1, 4) ⟿ 30
prod(k, k, 1, 5) ⟿ 120
fmax(x, -(x - 2)^2, 0, 5) ⟿ 2
fmin(x, (x - 3)^2, 0, 5) ⟿ 3
lim(x, sin(x)/x, 0) ⟿ 1
```

(`lim_left/lim_right/ntg` variants; `calc_integ_fn` is Simpson with
`steps = 200` default.)

### 16.1 ODEs — the RK4 family

**Lazy-symbol convention (critical).** The scalar solvers take the *names* of
`time` and `state` variables as **two leading bare symbols** (unquoted, they
need not be assigned anywhere), then the right-hand side as a plain **expression
in those two names** — *not* a function value. Passing a fn/lambda in the rhs
slot fails (`error: No active overload for 'raw_val' with type (fn)`):

```text
ode_rk4(t, y, -y, 0, 1, 1) ⟿ 0.3678794412
ode_rk4(t, y, 2 * t, 0, 0, 3) ⟿ 9
ode_rk4(t, y, t + y, 0, 1, 1, 200) ⟿ 3.436563657
```

`ode_rk4(t, y, rhs, t0, y0, t1, steps=100)` = classic RK4 from `(t0, y0)` to
`t1` (`steps` clamped to 1..4096). Units flow through the whole solve:

```text
edu := ode_rk4(t, y, 10 mps, 0 sec, 5 m, 4 sec); unit_of(edu) + ":" + (edu to_m)_ ⟿ "distance:45"
```

Family members (results carry the initial value's unit; `steps` default shown):

- `ode_rk4_vec(t, y, rhs, t0, y0, t1, steps=10)` — returns the whole
  trajectory as a `$vec` (`steps + 1` points incl. `y0`):

```text
ode_rk4_vec(t, y, -y, 0, 1, 1, 3) ⟿ [1, 0.716564, 0.513464, 0.367929]
```

- `ode_coupled2(t, x, y, dx_rhs, dy_rhs, t0, x0, y0, t1, steps=100)` /
  `ode_coupled3(… z, dz_rhs …)` — coupled 2-D / 3-D first-order systems,
  result is the final-state vec.
- `ode_rk4_sys` — N-D systems, **two overloads**: a closure *fn*
  `(t, y_vec) => rhs_vec` (y is *index*-addressed inside) or the lazy
  `(var_t, var_y, rhs_vec, …)` form; `ode_rk4_sys_traj(…)` additionally returns
  the trajectory as an `(steps+1) × N` `$mat`:

```text
ys := ode_rk4_sys((t, y) => [y[1], -y[0]], 0, list(1, 0), π, 200); ys ⟿ [-1, -1.59371e-09]
trix := ode_rk4_sys_traj(t, y, [y[1], -y[0]], 0, [1, 0], π, 8); (mat_rows(trix)_ + "x" + mat_cols(trix)_ + ":" + (trix[8, 0])_) ⟿ "9x2:-0.9998000243"
```

(Harmonic oscillator `y'' = -y` as the classic 2-vector first-order system;
`steps=200` recovers `cos π = -1` to 9 decimals.)

---

## 17. Statistics & probability

```text
mean, variance, stddev (population) & sample_variance sample_stddev; sma window;
corr/cov verified: cov(vec(1, 2, 3), vec(2, 4, 6)) ⟿ error: Undefined symbol or constant 'corr'
linreg_slope(vec(1, 2, 3), vec(2, 4, 6)) ⟿ 2
norm_pdf(0) ⟿ 0.3989422804
```

PMF/PDF set (`[arith]` §13b): `binom_pmf poisson_pmf geom_pmf exp_pdf
exp_cdf`; `[trig]` adds `norm_pdf/norm_cdf` with `(mu = 0, s = 1)` defaults.

## 17.1 The symbolic API (string domain, `[boot_control]` §11)

```text
sym_diff("x", "x^2 + 3*x") ⟿ "2 * x + 3"
sym_integ("x", "x^2") ⟿ "x ^ 3 / 3"
sym_solve("x", "x^2 - 4") ⟿ "[-2, 2]"
sym_eval("x^2 + 1", x, 3) ⟿ 10
simplify(x*1 + 0) ⟿ "x"
sym_taylor("x", "exp(x)", 0, 4) ⟿ "1 + x + 0.5 * x ^ 2 + 0.1666666667 * x ^ 3 + 0.04166666667 * x ^ 4"
```

`sym_eval`'s second argument is the *variable name as a symbol* (unquoted).
All output is canonical engine-formatted text; round-trip via `_"…"`.

---

## 18. Domain modules quick reference

The four "kitchen-sink" science modules ship as pure `.math` declarations —
a classroom library, not a store. Everything below is included at boot; the
results carry units when the arguments do, and the unit tag is *not* shown in
any of these outputs (see the display conventions at the top of this guide).

### 18.1 `[conversions]` — same-unit-group conversions

Two access paths operate on **unit-tagged values** (§11):

1. The universal infix operator `in` converts within one unit group:
`25km in m`, `98.6 degF in K`. The right side is a **bare unit token**.
2. Postfix `to_*` member converters per group (`x to_km`, `(310.15 K) to_degC`).
3. Pair-name helpers (`km_to_m(x)`, `c_to_k(t)`), usually unitless arithmetic.

Verified samples:

```text
98.6 degF in K ⟿ 310.15
(310.15 K) to_degC ⟿ 37
2.5 hr in sec ⟿ 9000
72 mph in mps ⟿ 32.18688
1 atm in Pa ⟿ 101325
1 eV in J ⟿ 1.602176634e-19
1 J in eV ⟿ 6.241509074e+18
c_to_k(100) ⟿ 373.15
f_to_c(98.6) ⟿ 37
100000 cm in km ⟿ 1
```

Unit groups and their suffix tokens (base unit first):

| group | tokens |
|---|---|
| `distance` | `km m cm mm um nm mi nmi yd ft inch` |
| `angle` | `deg° rad arcmin arcsec` (full turn separately) |
| `mass` | `kg gram mg ton lb oz` |
| `time` | `day hr min_t sec ms us ns` (**no `s` token**!) |
| `speed` | `mps kph mph knots` |
| `pressure` | `Pa kPa MPa bar atm psi torr` |
| `energy` / `power` | `J kJ MJ cal kcal Wh kWh eV / W kW MW hp` |
| `area` / `volume` | `km2 cm2 mm2 acre yd2 ft2 in2 / m3 cm3 mm3 L liter mL gal ft3 in3` |
| `accel` / `force` | `mps2 g_acc fps2 / N kN MN mN dyn lbf kgf` |
| `frequency` / `momentum` / `density` | `Hz kHz MHz GHz rpm / Ns kgmps / kgm3 gcm3` |
| `temperature` | `K kelvin mK degC degF` (absolute-Kelvin semantics) |
| SI remainder | `mol …`, `molar_*`, `cd`, `lx`, `cat` (and `V ohm F A W J …` electric set) |

### 18.2 `[physics]`

CODATA constants: `g0 c0 G_grav h_planck k_B ke_coulomb eps0 mu0 q_e m_e m_p
sigma_sb b_wien rydberg_R` (bare SI values). Catalogue by section:

- **Kinematics / projectiles:** `suvat_v suvat_s suvat_v2 suvat_s_avg
  suvat_nth_sec`, `proj_range proj_max_h proj_time`
- **Energy / momentum / collisions:** `ke pe mech_energy momentum impulse
  recoil_vel friction_force incline_normal incline_slide_acc
  banking_angle_deg elastic_v1 elastic_v2 inelastic_v`
- **Circular / rotational / SHM:** `centripetal_acc centripetal_force
  torque_val ang_momentum rot_ke hooke_force elastic_pe pendulum_period
  shm_omega shm_period_spring shm_vel shm_acc`
- **Gravitation / orbits:** `grav_force grav_pe orbital_vel escape_vel
  schwarzschild_r kepler_period`
- **Fluids / thermo:** `hydro_pressure buoyant_force continuity_v2
  stokes_drag terminal_vel stress_val strain_val sensible_heat latent_heat
  thermal_expansion_l thermal_cond_rate wien_peak_lambda
  stefan_boltzmann_p carnot_eff`
- **EM & circuits:** `coulomb_force ohm_v ohm_i ohm_r elec_power cap_energy
  cap_c ind_energy res_series res_parallel cap_series cap_parallel plate_cap
  rc_tau wheatstone drift_velocity lorentz_mag_force cyclotron_radius
  solenoid_b inductive_reactance capacitive_reactance rlc_impedance
  rlc_resonant_freq transformer_v2`
- **Waves / optics / quantum / relativity:** `doppler_freq beat_freq
  sound_db snell_angle2 lens_focal mirror_image_v magnification_lens
  lens_power_diopt young_fringe_width malus_law brewster_angle photon_energy
  photoelectric_ke_max de_broglie bohr_radius bohr_energy rydberg_wavelength
  lorentz_gamma rel_energy time_dilation length_contraction`

```text
suvat_s(0, 2, 3) ⟿ 9
ke(2, 3) ⟿ 9
proj_range(20, 45) ⟿ 40.78864852
pendulum_period(1) ⟿ 2.006409293
ideal_gas_v(1, 300, 101325) ⟿ 0.02461720982
eje := ke(2 kg, 3 mps); unit_of(eje) + ":" + (eje)_ ⟿ "energy:9"
```

(Seconds follow the same rule as §16.1: the RK4-unit one-liner multiplied
`10 mps` with `sec`, so type `sec` not `s`.)

### 18.3 `[chemistry]`

Constants: `N_A R_gas R_atm F_faraday T_stp P_stp kw_water`.

- **Stoichiometry / solutions:** `moles mass_from_moles particles_from_moles
  percent_composition percent_yield atom_economy molarity normality_from_molarity
  dilution_v1 molality mole_fraction ppm_conc`
- **Colligative / gases:** `raoult_p_soln bp_elevation fp_depression
  osmotic_pressure vant_hoff_osmotic boyle_p2 charles_v2 combined_gas_v2
  dalton_partial_p ideal_gas_p ideal_gas_v ideal_gas_n ideal_gas_t
  van_der_waals_p rms_mol_speed graham_rate_ratio`
- **Acid–base:** `ph_from_h h_from_ph poh_from_oh oh_from_poh poh_from_ph
  ph_from_poh ph_from_oh pka_from_ka ka_from_pka weak_acid_h weak_acid_ph
  henderson_hasselbalch solubility_ksp_ab solubility_ksp_ab2`
- **Kinetics / nuclear / thermochemistry / electrochemistry / solid state:**
  `arrhenius_k arrhenius_k2 rate_const_1st half_life_1st zero_order_half_life
  second_order_half_life decay_rem radiocarbon_age enthalpy_rxn kp_from_kc
  gibbs_free_energy gibbs_from_keq cell_emf e_cell_v faraday_mass nernst_cell
  reaction_q crystal_density bragg_d_spacing`

```text
moles(58.44, 58.44) ⟿ 1
ph_from_h(0.001) ⟿ 3
weak_acid_ph(1.8e-5, 0.1) ⟿ 2.872363747
decay_rem(100, 2 * 5730, 5730) ⟿ 25
half_life_1st(0.02) ⟿ 34.65735903
```

### 18.4 `[biology]`

- **Population genetics:** `hw_q_from_p hw_p_from_q2 hw_homo_dom hw_hetero
  hw_homo_rec hw_carrier_count recomb_freq chi_sq_term`
- **Molecular biology:** `chargaff_g_from_a chargaff_c_from_t dna_length_nm
  dna_turns dna_weight_da codons_for_protein mrna_bases_for_protein
  pcr_copies pcr_yield_from res_digest_linear res_digest_circular`
- **Enzymes / biochemistry:** `michaelis_menten competitive_inh_v
  noncomp_inh_v enzyme_catalytic_eff q10_rate q10_from_rates
  respiratory_quotient aerobic_atp_yield`
- **Plant physiology / cell / microscopy:** `water_potential solute_potential
  diffusion_pressure_deficit stomatal_index total_magnification
  actual_specimen_size fov_diameter mitotic_index cell_surface_sphere
  cell_volume_sphere sa_to_vol_sphere`
- **Human physiology:** `stroke_volume ejection_fraction cardiac_output
  pulse_pressure mean_arterial_pressure minute_ventilation
  alveolar_ventilation vital_capacity total_lung_capacity
  net_filtration_pressure renal_clearance bmi_calc`
- **Ecology:** `pop_exp_growth pop_doubling_time pop_logistic_n
  lincoln_petersen_pop trophic_energy_at simpson_diversity_vec
  shannon_diversity_vec`

```text
pcr_copies(10) ⟿ 1024
bmi_calc(70, 1.75) ⟿ 22.85714286
michaelis_menten(100, 2, 4) ⟿ 66.66666667
hw_hetero(0.6) ⟿ 0.48
```

---

## 19. Engine integration: sessions, snapshots, extensions & the CLI

The library is embedded in a C host (`#include "engine.h"`, link
`libmathengine.a`); every feature shown here is reached through *data*, never
through a special-cased call path. This section documents the public surface
as exercised by the bootstrapped binary `DebugBuild/myapp` and the
re-verification harness of §22.

### 19.1 The session model

State lives in numbered sessions (engine memory maps carry 8 session slots;
`eng_session_open`/`eng_session_close` manage them). In the doc harness this
maps to:

- `@sess` **primes** the block: the *next* value line opens a new session.
  Subsequent value lines continue in it (variables persist, `⟿` keeps
  printing each line's result).
- `@cont` — the explicit "continue in the current session" record
  (manifest syntax; docs mostly rely on `@sess` priming).
- `@close` — closes and frees the current session.

Stateless one-liners do **not** touch sessions at all (§1: fresh engine,
no leakage).

### 19.2 Snapshots — tamper-evident full history (`MSP1`)

Each evaluated line produces an **MSP1 snapshot record** of the whole session
state:

```
MSP1|seq:1|prev:00000000|hash:B01BE54B|vars:1|qq:real:none:*:*=42|res:real:none=42
```

`seq` grows per line, `prev` chains to the previous record's hash (so a
session's whole history is one hash-linked trail), `vars` serializes
name/type/unit/value, `res` is the line result. Harness commands (§22 table):

- `@snapdump` — print the current session's full MSP1 chain;
- `@snap_save` — bank the chain; `@snap_restore` — replay the bank into a
  *new* session (building hashes from the replayed history).

Verified end-to-end:

```text
@sess
qq := 42 ⟿ 42
@snapdump ⟿ MSP1|seq:1|prev:00000000|hash:B01BE54B|vars:1|qq:real:none:*:*=42|res:real:none=42
@close ⟿ ok: session closed
```

```text
@sess
kk := 7 ⟿ 7
@snap_save ⟿ ok: banked 1 snapshots
@snapdump ⟿ MSP1|seq:1|prev:00000000|hash:67C3AB25|vars:1|kk:real:none:*:*=7|res:real:none=7
```

### 19.3 `.math` extensions & module toggles

`@ext [modname] decl…` compiles a user module **topologically** (`<[dep]`
links pull dependencies), links it into the live engine, and reports the
`.mathc` image size. Verified workflow — extension, session, snapshot,
restore, module toggling (note how unloading a module removes even the
postfix `i` syntax it defines):

```text
@ext ⟿ ok: extension linked, mathc bytes=216137
[units_demo]
uin(y) = y * 2
@sess
uin(5 km) ⟿ 10000
w := uin(42) ⟿ 84
@snap_save ⟿ ok: banked 2 snapshots
@close ⟿ ok: session closed
@snap_restore ⟿ ok: restored as session 0
w ⟿ 84
@mod|complex_ext=0 ⟿ ok: complex_ext=0
carg(3 + 4i) ⟿ error: Undefined symbol or constant 'i'
@mod|complex_ext=1 ⟿ ok: complex_ext=1
carg(3 + 4i) ⟿ 0.927295218
@close ⟿ ok: session closed
@snapdump ⟿ (no snapshots)
```

Read it line by line: `uin(5 km)` returned `10000` — the extension multiplied
the *raw numeric* slot (`5 km` → 5000 → 10000) and the distance tag flowed
through untouched; the snapshot bank carried `w` across `@close` and the
restore rebuilt it as `84`; disabling `[complex_ext]` deleted the `i`
postfix suffix **and** `carg` from the parser, re-enabling restored both.

User `.math` also gets the same durability: `user_compile_ext` returns the
compiled `.mathc` bytes (host stores/`memcpy`-loads them in O(1) via
`user_load_mathc`) — no re-parse, ever.

### 19.4 Introspection & diagnostics

```text
type_of(3.5) ⟿ "real"
type_of(vec(1, 2)) ⟿ "vec"
type_of(x => x * 2) ⟿ "fn"
unit_of(9 J) ⟿ "energy"
unit_of(3) ⟿ "none"
unit_dim(5 J, 0) * 100 + unit_dim(5 J, 1) * 10 + (-unit_dim(5 J, 2)) + same_dim(10 N * 2 m, 5 J) * 1000 ⟿ 1212
pool_used() > 0 ⟿ 1
```

`unit_dim(v, axis)` returns the 7-D SI exponent from
`@dim(group, m, kg, s, A, K, mol, cd)` (verified axes: `0` = length, `1` =
mass, `2` = time); `same_dim` checks dimensional compatibility
(`10 N * 2 m` is dimension-compatible with `5 J`). Memory side:
`pool_used()` reports the scratch-pool occupancy and `gc_compact()` returns
the bytes reclaimed by mark-sweep compaction — safe to call between
expressions and live values survive (the 555-test suite grows a vec by 120
pushes, compacts to a positive reclaimed count, then reads all 123
elements back).

Error channels are exact strings:

```text
error("bad thing") ⟿ error: bad thing
assert(0, "should fail") ⟿ error: should fail
2 + undefined_sym ⟿ error: Undefined symbol or constant 'undefined_sym'
```

### 19.5 The CLI (`DebugBuild/myapp`) — eval, REPL, bench

`myapp` bootstraps + links any `Test/user_physics.math` lying around, then:

```console
$ ./DebugBuild/myapp "1 + 2 * 3" "u := 5 km; u in m" "(1...6).filter(x => x % 2 == 0)"
7
5000 [distance]
[2, 4, 6]
```

(The *CLI* prints unit tags in brackets; the API/harness display stays bare.)

`./DebugBuild/myapp --repl` opens a type-in command loop:

```
MEP Interactive Session REPL (19 modules, 1380 overloads, 30 constants)
  Commands: :help, :vars, :snaps, :compact, :modules, :enable <mod>,
            :disable <mod>, :diff <x> <expr>, :integ <x> <expr>,
            :solve <x> <eq>, :simplify <expr>, :load <file.math>, :reset, :quit
```

`:modules` lists every linked module and its activity state —

```
[ 0] default        (active)
[ 1] boot_syntax    (active)
[ 4] arith          (active)
[ 7] units          (active)
[15] physics        (active)
[17] calculus       (active)
[18] user_physics   (active)      ← auto-linked user extension
```

(19 total; only a carve-out shown here, `:disable`/`:enable <mod>` flips
them exactly like `@mod`.) `./DebugBuild/myapp --bench` — numbers from this
host, 2026-10-24, `clock()` CPU time:

```
[1] O(1) .mathc stdlib boot (5000 iters)     : 3632.54 ms (726.51 us/boot)
[2] Scalar + trig + factorial (5000 evals)   : 2132.82 ms (426.56 us/eval)
[3] Vector HOF map/filter/sum (5000 evals)   : 2914.97 ms (582.99 us/eval)
[4] 3x3 Gaussian linsolve (5000 evals)       : 2721.69 ms (544.34 us/eval)
[5] 50-step RK4 ODE solver (1000 evals)      : 2225.74 ms (2225.74 us/eval)
```

Expect ±50 % frame across machines/loads; *relative* structuring (boot ≪
RK4) is the invariant.

---

## 20. Precedence master table

Tiers listed **tightest-binding first**; each row's reproduction is
machine-verified (the number shown fabricated). `;`-separated and
assignment-bearing lines are *statements*, so they only interact with
expression tiers at their boundary. Note that individual overload sites in
`.math` can pin their own precedence with the `:<N>` suffix annotation
(`Library/units.math` uses `:95` for unit suffixes, `:90` for member
convertors, `Library/conversions.math` pins `in` at `:<42`).

| # | Tier | Verified reproduction |
|---|---|---|
| 1 | **Postfix**: `[idx]`, `..slice`, `.member()`, `_` stringify, `!` factorial, `i` unit, `x++`/`++x` value forms | `"abcdef"[0..<3]` → `"abc"`; `(eje)_` → `"9"`; `c1++` → `6` while `c1` stays `5` |
| 2 | **Unit-suffix juxtaposition** (`:95` members in `[units]`): `5km 10 mps 310.15 K …` | `(2 m) * (3 m)` → area dim; `unit_of(arm)` → `"area"` |
| 3 | **Power** `^` (right-assoc!) | `2 ^ 3 ^ 2` → `512`; `-3 ^ 2` → `-9` (unary binds below `^`) |
| 4 | **Implicit multiply in `.math` layer** / `√` prefix, `not` prefix | `-3 ^ 2` → `-9`; `√16` → `4` |
| 5 | **`* / %`** (left-assoc) | `6 / 2 * 3` → `9` |
| 6 | **`+ -`** (left-assoc; also str–str concat) | `1 + 2 * 3` → `7` |
| 7 | **Ranges** `..`, `...`, `..<`, `<..` + `by` | `[i^2 for i in 1..5]` → `[4, 9, 16]`, `1..<4` excludes the end |
| 8 | **Comparisons** `== != < <= > >= ~` (left-assoc chaining) | `1 < 4 == 4` → `0` (=(1<4)==4) |
| 9 | **`not` prefix** | `not 0 && 0` → `0` (prefix binds tight) |
| 10 | **`&&` and**, then **`||` or / `^^` xor / `nand`** | `1 || 0 && 0` → `1` (AND tighter) |
| 11 | **Pipe** `\|>` into right-hand call | `16 \|> sqrt` → `4` |
| 12 | **`into`** (push result into container last argument) | `v2 := vec(); 7 into v2; v2` → `[7]` |
| 13 | **Assignments** `:=` `+=` `-=` `*=` `/=` `%=` `^=` | `x := 9; x %= 4; x` → `1` |
| 14 | **Statement separator** `;` / newline | `u := 5 km; u in m` → `5000 [distance]` (CLI display) |

Every reproduction in the table is machine-verified below:

```text
"abcdef"[0..<3] ⟿ "abc"
arm := (2 m) * (3 m); unit_of(arm) ⟿ "area"
2 ^ 3 ^ 2 ⟿ 512
-3 ^ 2 ⟿ -9
tor := ke(2, 3); (tor)_ ⟿ "9"
5! ⟿ 120
not 0 && 0 ⟿ 0
6 / 2 * 3 ⟿ 9
1 + 2 * 3 ⟿ 7
[i^2 for i in 1..5] ⟿ [4, 9, 16]
[i for i in 0..<4] ⟿ [0, 1, 2, 3]
1 < 4 == 4 ⟿ 0
1 || 0 && 0 ⟿ 1
16 |> sqrt ⟿ 4
v2 := vec(); 7 into v2; v2 ⟿ [7]
x := 9; x %= 4; x ⟿ 1
```

(Session-backed postfix value forms — `c1++` vs `c1` — are verified in §21.)

---

## 21. Verified quirks & limitations

Every line below is machine-verified against the live engine (§22); these
are *facts about this engine*, phrased against the expectation a reader may
bring from other languages.

```text
1 < 4 == 4 ⟿ 0
-3 ^ 2 ⟿ -9
7 % 2 ⟿ 1
7 / 2 ⟿ 3.5
7.5 % 2 ⟿ 1.5
-7 % 3 ⟿ -1
sum([1, 2]) ⟿ error: No active overload for 'sum' with type (mat)
0.1 + 0.2 == 0.3 ⟿ 1
0.1 + 0.2 == 0.30001 ⟿ 0
1 == "1" ⟿ error: No active overload for '==' with types (real, str)
7 mod 2 ⟿ error: Undefined symbol or constant 'mod'
"abc" + 5 ⟿ error: No active overload for '+' with types (str, real)
vec(2, 9) ⟿ [9, 9]
vz := vec(1, 7); vz.resize(3); vz ⟿ [7, 0, 0]
exp(i * π) ⟿ error: Undefined symbol or constant 'i'
0 / 0 ⟿ error: Division by zero
true ⟿ error: Undefined symbol or constant 'true'
```

(The engine never stores `true`/`false` literals — see item 1.)

Increment/update quirk — value forms don't mutate:

```text
@sess
c1 := 5 ⟿ 5
c1++ ⟿ 6
c1 ⟿ 5
@close ⟿ ok: session closed
```

Prose list (each item verified in this guide or the audit batches):

1. **No `true/false` literals** — predicates and ternaries take `1`/`0`.
2. **Chained comparisons left-assoc** — `a < b == c` parses as
   `(a < b) == c`, not as a mathematical conjunction.
3. **`^` right-assoc; unary minus below `^`** (`-3 ^ 2` is -9, not 9).
4. **`%` follows the C `fmod` contract** — sign of the dividend; and `/`
   never truncates (`7 / 2` → `3.5`; make integers with `floor`).
5. **No `mod`/`//`/`let`/`where`/`~=`** — these tokens error
   ("Undefined symbol or constant 'mod'").
6. **`==` has a tiny tolerance guard** (0.1 + 0.2 equals 0.3), `~` a much
   looser one (≤ 0.001 scale): pick `~` for numeric-equ wording, `==` for
   near-exact.
7. **No implicit cross-type coercion** — `"abc" + 5` is an overload error:
    build strings with `fmt(...)` or the postfix `_` stringify
    (`(expr)_`, demonstrated in §20 row 1 / §18.2).
8. **Bracket literals are `$mat`** — `[1, 2]` is not a `$vec`; elementwise
   helpers (`.* .+ ./`) and most HOF family dispatch on `$vec`. Flatten or
   bracket-map into `vec(...)` when needed (`sum([1,2])` errors on a mat argument).
9. **`i` is postfix-only** (`5 into 5i` works, standalone `i` doesn't).
10. **`x++` / `++x` are value forms** — they yield x+1 without mutating.
11. **Unit keywords can't be guessed** — `s` is not a suffix (`sec` is),
    so `3km + 2s` fails with `Undefined symbol or constant 's'`.
12. **`/` is always real division** — `7 / 2` → `3.5`; use `floor`/`trunc`
    for integer-style splits.
13. **Compound containers lock types** — `vec<int>(…)` errors
    ("Unknown element type 'int' in 'vec<...>'"); use `real`/`str`.
14. **`map()` wants even pairs** — odd key/value count errors out.
15. **`str_join` takes a `$vec`** — `str_join("ab", "-")` errors on (str, str).
16. **Bare `for` outside comprehension is a parse error** — exact message:
    "Unexpected 'for' outside of '[expr for x in ...]' comprehension".
17. **`ode_rk4`'s rhs is an expression, not a fn** — passing a fn/lambda into
    its third slot errors `error: No active overload for 'raw_val' with type
    (fn)` (the system solvers take closures, §16.1).
18. **Snapshot ops script deterministically** — `@snap_save` + `@snap_restore`
    rebuild state hash-for-hash (`ok: restored as session 0`, §19.2), and a
    `@snapdump` after a restore/close legitimately shows an empty chain for
    the *new* session, not a broken bank.

---

## 22. Appendix — the verification harness & re-verification pipeline

Everything in this guide was emitted from one deterministic pipeline:

1. **Harness binary** — `mepverify` compiled against the engine static lib:

```console
$ gcc -std=c11 -O2 -I Core/Headers -I Core/session -I Core/compile \
      mepverify.c DebugBuild/libmathengine.a -lm -o mepverify
```

2. **Doc driver** — `verify_examples.js` (Node ≥ 16, no deps): scans docs
   for ```text fences, executes each record, replaces `expr ⟿ «id»` with
   the engine's exact output, re-runs are idempotent:

```console
$ node verify_examples.js docs/MEP_Language_Guide.md docs/*.md
```

3. **Engine end-point** — the same `DebugBuild/myapp` you would ship to your
   CLI users (`--eval`, `--repl`, `--bench`).

### Harness command syntax (`.mf` manifest / doc fences)

| command | result line | semantics |
|---|---|---|
| `@eval\n<expr>` | `⟿ result` | fresh-engine evaluation (default for bare value lines) |
| `@sess\n<expr>` | `⟿ result` | open a session and eval its first line |
| `@cont\n<expr>` | `⟿ result` | eval into the current session |
| `@ext\n<name…>` | `ok: extension linked, mathc bytes=N` | compile+link `.math` body, topologically linked |
| `@mod\|<name>=0/1` | `ok: <name>=0/1` | disable/enable a module (syntax-class on/off) |
| `@snapdump` | MSP1 chain \| `(no snapshots)` | print current session's snapshot chain |
| `@snap_save` | `ok: banked N snapshots` | bank the current MSP1 chain |
| `@snap_restore` | `ok: restored as session <id>` | replay banked chain into a new session |
| `@reset_bank` | `ok: bank cleared` | clear the snapshot bank |
| `@close` | `ok: session closed` | close the current session |

(The driver's rule set: `@sess`-primed blocks open sessions on the first
value line; `#` lines are prose; `@ext` bodies may not contain `@` lines,
and `@snapdump` pays off as the *first* record of its own block if you need
to *see* the chain.)



