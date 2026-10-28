# Math Engine (MEP) — User Exposure & Language Specification Guide

This reference documents **everything a user or library author can define on their own** in the Math Engine (MEP)—from extending the language grammar in `.math` specification modules (custom consuming lexical types, flat and nested C-style structs, 7D SI and custom domain unit groups, prefix/postfix/infix operators with custom precedence, polymorphic type- and unit-dispatched functions, lazy bound-variable `var_x` calculus/solver operators, constants, and `@extern` host bindings) to writing **complete, self-contained multi-statement programs** using lexical scopes, slot locks, destructuring, closures, pipelines, pattern-matched `switch` blocks, $O(1)$ skip-range loops, comprehensions, and tagged-scope state machines.

Every construct and declaration form in this guide has been compiled and verified directly against the MEP C11 core engine.

## 1. The Two Layers of User Exposure

MEP exposes its extensibility across two distinct layers that share the exact same parser, type/unit dispatcher, and 8-byte AST evaluator:

| Layer | Where It Lives | How It Is Loaded / Executed | What the User Can Define |
| :--- | :--- | :--- | :--- |
| **Layer 1: Language Specification Modules (`.math` / `.mathc`)** | `.math` source strings or precompiled `.mathc` binary images | `user_compile_ext()`, `user_compile_unit()` + `user_link_ext()`, `user_load_mathc()`, or CLI `:load <file.math>` / `mathc` | `[module]` headers, `<[dep]` dependencies, syntax delimiters (`"(" => @lparen`), consuming token regex types (`$type ~ pat => @...`), flat/nested structs (`$struct(...)`), 7D SI unit groups (`@dim(...)`), custom non-SI units, constants, prefix/postfix/infix operators (`:prec`, `:<prec`, `:>prec`), overloaded functions, default parameters, lazy `var_x` functions, `@<intrinsic>` delegates, and `@<extern>` host bindings. |
| **Layer 2: Runtime Expressions & Session Programs** | Expression strings passed to stateless or stateful evaluation | `eng_eval()`, `eng_session_eval()`, `MathEngineKt.eval()`, `MathEngineKt.evalSession()`, or REPL | Local & session variables (`:=`, `=`, `+=`), concrete type/unit locks (`x : real, distance := 50m`), broadcast & destructuring assignments, inline recursive functions (`f(x, y = 1) := ...`), closures (`(x, y) => ...`), UFCS (`.`), pipelines (`\|>`), conditionals (`? :`, `if/or/else`), pattern-matched `switch`, loops (`??`, `?!?`, `(i, x in ...)`), comprehensions (`[e for ...]`), and tagged scopes (`Tag : { ... }`). |

Because MEP is a **Blank-Slate Core** (`eng_init_blank` has zero built-in operators, types, or delimiters), user-authored `.math` modules have the **exact same privileges and capabilities** as the standard library modules in `Library/*.math`.

## 2. Module System & Topological Dependency Linking

Every `.math` specification file is organized into one or more named `[module]` sections (or a headerless `[default]` section) with explicit `<[dependency]` declarations.

### 2.1 Module Header & Dependency Syntax

```math
# Declare module name in square brackets at the start of a line
[aerospace_nav]

# Declare required modules using `<[module_name]` (one per line)
<[boot_syntax]
<[boot_types]
<[boot_control]
<[arith]
<[trig]
<[geom_ext]
<[vec]
<[units]
<[physics]
```

### 2.2 Module Rules & Linker Guarantees

| Feature | Syntax / API | Behavior & Compile-Time Guardrails |
| :--- | :--- | :--- |
| **Named Module Header** | `[mod_name]` | Assigns declarations to `mod_name` (unbounded module capacity: inline SBO for `<= 64` modules, automatic heap expansion beyond 64). |
| **Headerless Extension** | *(no `[...]` header)* | Automatically placed into module `[default]` (`mod_id = 0`). |
| **Dependency Declaration** | `<[dep_name]` | Ensures `dep_name` is active and compiled before the current module. |
| **Order-Independent Multi-Unit Linking** | `user_compile_unit()` + `user_link_ext()` | Stages `.math` units in any order and runs Kahn's topological sort over the `<[dep]` DAG. |
| **Duplicate Module Guard** | `[existing_mod]` | Rejected with `"Module '[existing_mod]' is already loaded"`. |
| **Missing Dependency Guard** | `<[unknown_mod]` | Rejected with `"Unresolved module dependency '<[unknown_mod]' required by '[...]'"` and full transactional rollback. |
| **Circular Dependency Guard** | `[a] <[b]` and `[b] <[a]` | Rejected with `"Circular module dependency involving '[a]'"` and full transactional rollback. |
| **O(1) Runtime Module Toggling** | `eng_set_module(eng, "mod_name", false)` | Updates `sym_mod_active(tab)[mod_id]` in $O(1)$ time, immediately enabling or disabling all types, units, operators, functions, and constants in that module. |

## 3. Defining Custom Entity Types (`$` Declarations)

In a `.math` specification, top-level declarations starting with `$` register a new entity type (`TypeDef`, up to `SYM_MAX_TYPES = 64`). MEP supports three forms of `$` type declarations:
1. **Consuming Lexical Types (`$name ~ pattern => @parser_intrinsic`)**: Tokenized directly by the lexer using maximal-munch pattern matching.
2. **Flat & Nested C-Style Struct Types (`$name(field1[:type1], field2[:type2], ...)`)**: Value-typed composite records stored contiguously in `MemArena.f64_pool`.
3. **Intrinsic Container & Wildcard Types (`$name ~ => @type_intrinsic`)**: Binds container types (`$vec`, `$mat`, `$fn`, `$map`) and polymorphic wildcards (`$T`, `$any`, `$none`).

### 3.1 Consuming Lexical Types & Pattern Grammar

A consuming type teaches the lexer how to recognize a new literal token directly from raw source characters and which C parser intrinsic should convert the matched slice into an 8-byte `Value`:

```math
$type_name ~ <pattern_atoms...> => @<parser_intrinsic>
```

#### Pattern Atoms & Quantifiers Supported in `compiler.c` / `lexer.c`

| Pattern Atom / Quantifier | Syntax in `.math` | Meaning | Example |
| :--- | :--- | :--- | :--- |
| **Decimal Digit** | `\d` | Matches any ASCII digit `'0'..'9'` (`PAT_DIGIT`). | `\d+` |
| **Alphabetic Letter** | `\a` | Matches any ASCII letter `'a'..'z'` or `'A'..'Z'` (`PAT_ALPHA`). | `\a+` |
| **Exact String Literal** | `"..."` | Matches the exact quoted character sequence (`PAT_LIT`). | `"0x"`, `"b"`, `"."` |
| **Positive Character Class** | `[abc...]` | Matches any single character listed inside brackets (`PAT_CHARS`, max 31 bytes). | `[+-]`, `[01]`, `[eE]` |
| **Negated Character Class** | `[^abc...]` | Matches any single character **not** listed inside brackets (`PAT_NOT_CHARS`). | `[^"]*` |
| **Grouping & Alternation** | `(...)` and `\|` | Groups sub-patterns (`PAT_GROUP_START`/`END`) and tries alternatives (`PAT_ALT`). | `("0" [xX] \d+ \| \d+)` |
| **Quantifiers** | `+`, `*`, `?` | One-or-more (`QUANT_PLUS`), zero-or-more (`QUANT_STAR`), or optional (`QUANT_OPT`). | `\d+ ("." \d+)?` |

#### Available Parser Intrinsics (`=> @...`)

| Parser Intrinsic | Target Storage | Token Conversion Semantics |
| :--- | :--- | :--- |
| `@f64` | `double` (`real`) | Strips `_` digit separators, parses `0x...` hex, `0b...` binary, or decimal/scientific float via `strtod`. |
| `@f64_trim1` | `double` (custom consuming type) | Strips `_` separators, trims the final 1-byte suffix character (e.g., `"i"` or `"b"`), and parses the numeric prefix via `strtod`. |
| `@i64` | `double` (integer-valued) | Strips `_` separators and parses signed base-10 integer via `strtoll`. |
| `@str_slice` | `StrSlice` (`str`) | Strips outer `"..."` quotes, unescapes `\\`, `\"`, `\n`, `\t`, `\r`, and interns the UTF-8 slice in `src_buf`. |

#### Tested Example: Defining a Custom Basis-Point Literal Type (`$bp_lit`)

```math
# Recognizes literals like `50b` or `1_250.5b` as entity type `bp_lit`
$bp_lit ~ \d+ ("_" \d+)* ("." \d+)? "b" => @f64_trim1

# Function accepting `bp_lit` and converting basis points (1 bp = 0.01% = 0.0001) to a fraction
bp_to_frac(x:bp_lit) = @raw(x) * 0.0001
```

- **Tested Evaluation**: `bp_to_frac(50b)` $\implies$ `0.005` (`real`, `none`)
- **Tested Introspection**: `type_of(50b)` $\implies$ `"bp_lit"` (`str`, `none`)

### 3.2 Flat & Nested C-Style Struct Types (`$struct(...)`)

Non-consuming structs pack one or more scalar or nested struct fields into contiguous `double` slots in `MemArena.f64_pool`.

```math
# Fields without `:type` default automatically to `:real`
$interval(lo, hi)

# Nested structs reference previously declared struct types (`vec2`)
$aabb2d(min_pt:vec2, max_pt:vec2)
$particle2d(pos:vec2, vel:vec2, mass)
```

#### What Defining `$name(...)` Automatically Exposes to the User

1. **Positional Constructor**: `interval(10, 20)`, `aabb2d(vec2(1, 2), vec2(6, 10))`, `particle2d(vec2(0, 0), vec2(2, 0), 4)` (validated for exact field count and field entity types at evaluation time).
2. **Compile-Time Resolved Dot Field Read & Mutation (`.` and `=`, `+=`, `*=`)**:
   - `box.min_pt.x` resolves at compile time (`AST_FIELD_LOAD`) to flat offset `0`; `box.max_pt.y` resolves to flat offset `3`.
   - `p.pos.x += 5` mutates only that field slot in place.
3. **Zero-Based Positional Bracket Indexing (`s[i]` and `s[i][j]`)**:
   - `iv[0]` reads `iv.lo`; `iv[1]` reads `iv.hi`.
   - On nested structs, `box[1][0] += 10` mutates `box.max_pt.x` in place.
4. **Positional Variable Destructuring (`a, b := struct_val`)**:
   - `lo_v, hi_v := interval(10, 20)` unpacks the flat fields into `lo_v = 10` and `hi_v = 20`.
5. **Structural `switch` Pattern Matching**:
   - `case interval(0, hi) if hi > 10 : { ... }` checks runtime struct type, matches literal fields, binds pattern variables, and evaluates the `if` guard.
6. **Homogeneous Parameterized Containers (`vec<struct_type>()`)**:
   - `pts := vec<particle2d>()` creates a growable vector whose element stride equals `particle2d.flat_size` (`5` doubles).

> **Compile-Time Guardrail**: Self-referential structs (`$node(val:real, next:node)`) are rejected at compile time with `"Self-referential struct field 'next' in '$node' is not allowed"`.

#### Tested Example: `$aabb2d` and `$particle2d` Methods

```math
aabb_width(box:aabb2d)  = box.max_pt.x - box.min_pt.x
aabb_height(box:aabb2d) = box.max_pt.y - box.min_pt.y
aabb_area(box:aabb2d)   = aabb_width(box) * aabb_height(box)
aabb_contains(box:aabb2d, p:vec2) =
    (p.x >= box.min_pt.x && p.x <= box.max_pt.x) &&
    (p.y >= box.min_pt.y && p.y <= box.max_pt.y)

step_particle(p:particle2d, fx, fy, dt) -> particle2d = {
    ax := fx / p.mass
    ay := fy / p.mass
    vx := p.vel.x + ax * dt
    vy := p.vel.y + ay * dt
    px := p.pos.x + vx * dt
    py := p.pos.y + vy * dt
    particle2d(vec2(px, py), vec2(vx, vy), p.mass)
}

particle_ke(p:particle2d) = 0.5 * p.mass * (p.vel.x^2 + p.vel.y^2)
```

| Tested Expression | Engine Output | Type & Unit |
| :--- | :--- | :--- |
| `box := aabb2d(vec2(1, 2), vec2(6, 10)) ; aabb_area(box) * 10 + aabb_contains(box, vec2(3, 5))` | `401` | `real`, `none` |
| `p := particle2d(vec2(0, 0), vec2(2, 0), 4) ; p = step_particle(p, 8, 12, 0.5) ; particle_ke(p)` | `22.5` | `real`, `none` |

## 4. Defining Custom Physical & Domain Unit Systems

Every runtime `Value` carries an orthogonal 8-bit `unit_id` (`0..63`, up to `SYM_MAX_UNITS = 64`) alongside its `type_id`. Users can define two kinds of custom unit groups:

### 4.1 7D SI Dimensional Unit Groups (`@dim`)

Using the top-level `@dim(unit_group, m, kg, s, A, K, mol, cd)` directive, you bind a unit group to a 7-tuple of integer SI base exponents:

$$\vec{d} = (\text{m},\, \text{kg},\, \text{s},\, \text{A},\, \text{K},\, \text{mol},\, \text{cd}) \in [-128, 127]^7$$

Once registered with `@dim`, MEP's dimensional algebra engine (`delegate_resolve_mul_unit`, `delegate_resolve_div_unit`, `delegate_resolve_pow_unit`, `delegate_resolve_root_unit`) **automatically synthesizes and cancels** your new unit group whenever existing SI units are multiplied, divided, raised to a power, rooted, differentiated, or integrated!

#### Tested Example: Defining `jerk` ($\text{m}\cdot\text{s}^{-3}$) and `dynamic_viscosity` ($\text{Pa}\cdot\text{s} = \text{m}^{-1}\cdot\text{kg}\cdot\text{s}^{-1}$)

```math
# 1. Register 7D SI exponents: (m=1, kg=0, s=-3, A=0, K=0, mol=0, cd=0)
@dim(jerk, 1, 0, -3, 0, 0, 0, 0)
(x) mps3 :95 -> real, jerk = x
(x:real, jerk) to_mps3 :90 -> real, none = x

# 2. Register 7D SI exponents: (m=-1, kg=1, s=-1, A=0, K=0, mol=0, cd=0)
@dim(dynamic_viscosity, -1, 1, -1, 0, 0, 0, 0)
(x) Pa_s :95 -> real, dynamic_viscosity = x
(x) cP   :95 -> real, dynamic_viscosity = x * 0.001
```

| Tested Expression | Engine Output | Synthesized Unit | Why It Works |
| :--- | :--- | :--- | :--- |
| `(12 mps2) / (3 sec)` | `4` | `jerk` | `acceleration` $(1,0,-2,0,0,0,0)$ $-$ `time` $(0,0,1,0,0,0,0)$ $=$ $(1,0,-3,0,0,0,0)$ (`jerk`) |
| `((12 mps2) / (3 sec)) to_mps3` | `4` | `none` | Postfix extractor `to_mps3` matches `real, jerk` and strips unit to `none` |
| `(500 cP) + (1.5 Pa_s)` | `2` | `dynamic_viscosity` | Both normalize to SI $\text{Pa}\cdot\text{s}$ (`0.5 + 1.5 = 2`) under `dynamic_viscosity` |
| `(2 Pa) * (3 sec)` | `6` | `dynamic_viscosity` | `pressure` $(-1,1,-2,0,0,0,0)$ $+$ `time` $(0,0,1,0,0,0,0)$ $=$ $(-1,1,-1,0,0,0,0)$ |

### 4.2 Custom Non-SI Domain Unit Groups

If you want a domain-specific unit group that does not correspond to physical SI dimensions (for example, `data_size` and `bandwidth` in `Test/user_physics.math`, or `pixel` for UI geometry), simply reference the unit group name in the return annotation `-> real, unit_group` of a postfix operator without a `@dim` directive, and define explicit cross-unit operators as needed:

```math
# Postfix unit suffixes at precedence 95 create values in the `pixel` unit group
(x) px  :95 -> real, pixel = x
(x) kpx :95 -> real, pixel = x * 1000
(p:real, pixel) to_px :90 -> real, none = p

# Custom data_size & bandwidth unit groups with explicit cross-unit `/` and `*`
(x) B  :95 -> real, data_size = x
(x) KB :95 -> real, data_size = x * 1024
(x) MB :95 -> real, data_size = x * (1024 ^ 2)

(sz:real, data_size) "/" (t:real, time) :<60 -> real, bandwidth = @div(sz, t)
(bw:real, bandwidth) "*" (t:real, time) :<60 -> real, data_size = @mul(bw, t)
```

| Tested Expression | Engine Output | Type & Unit |
| :--- | :--- | :--- |
| `(2 kpx + 500 px) to_px` | `2500` | `real`, `none` |
| `(10 MB) / (2 sec)` | `5242880` | `real`, `bandwidth` |
| `(2.5 km) in m` | `2500` | `real`, `distance` |

## 5. Defining Custom Global Constants

Constants can be declared with either an unquoted identifier or a quoted symbol, with an optional `: type[, unit]` annotation. The right-hand side is evaluated at module compile time and stored in `SymTable.consts` (`SYM_MAX_CONSTS = 128`).

```math
# 1. Shorthand unquoted constant (evaluates full expression `π * (3 - √5)` at compile time)
golden_angle_rad = π * (3 - √5)

# 2. Explicitly typed & unit-tagged constant
"earth_escape_v" : real, speed = 11186 mps
```

| Tested Expression | Engine Output | Type & Unit |
| :--- | :--- | :--- |
| `round(golden_angle_rad, 6)` | `2.399963` | `real`, `none` |
| `earth_escape_v in kph` | `40269.6` | `real`, `speed` |
| `earth_escape_v to_kph` | `40269.6` | `real`, `none` |

## 6. Defining Custom Operators (Prefix, Postfix, Left-Infix, Right-Infix & Word Operators)

MEP has no hardcoded arithmetic or domain operators—every operator is an entry in `SymTable.overloads` (`SYM_MAX_OVERLOADS = 2048`) with a **Fixity** (`FIX_PREFIX`, `FIX_POSTFIX`, `FIX_INFIX_LEFT`, `FIX_INFIX_RIGHT`) and an integer **Precedence** (`1..100`).

### 6.1 Complete Operator Declaration Syntax Table

| Operator Fixity | Declaration Syntax in `.math` | Associativity | Precedence Specifier |
| :--- | :--- | :--- | :--- |
| **Prefix Unary (Quoted)** | `"op" (x[:type[, unit]]) :prec [-> rt[, ru]] = body` | Right-to-left unary | `:prec` (e.g. `:80`) |
| **Prefix Unary (Word)** | `op(x[:type[, unit]]) :prec [-> rt[, ru]] = body` | Right-to-left unary | `:prec` (e.g. `:25`) |
| **Postfix Unary** | `(x[:type[, unit]]) "op" :prec [-> rt[, ru]] = body` *(quotes optional for word ops)* | Left-to-right unary | `:prec` (e.g. `:90` or `:95`) |
| **Binary Infix (Left-Associative)** | `(a[:t1[, u1]]) "op" (b[:t2[, u2]]) :<prec [-> rt[, ru]] = body` | Left-to-right (`a op b op c == (a op b) op c`) | `:<prec` (or `:prec`) |
| **Binary Infix (Right-Associative)** | `(a[:t1[, u1]]) "op" (b[:t2[, u2]]) :>prec [-> rt[, ru]] = body` | Right-to-left (`a op b op c == a op (b op c)`) | `:>` followed by `prec` |

### 6.2 Choosing Precedence (`1..100` Reference Scale)

When designing custom operators, choose a precedence number relative to standard library operators:

| Precedence Band | Standard Operators at This Level | Recommended Use for Custom Operators |
| :--- | :--- | :--- |
| `95` | `.` (field/UFCS), `[...]`, unit suffixes (`km`, `deg`, `i`) | Custom postfix unit constructors (`px`, `KB`, `mps3`) |
| `90` | `!` (factorial), `++`/`--` (postfix), `to_m`/`to_deg` | Custom postfix extractors (`span`, `mid`, `to_px`) |
| `85` | `^`, `.^` (right-associative power), `++`/`--` (prefix) | Tight-binding exponentiation or prefix incrementers |
| `80` | `-`, `+`, `√`, `~`, `!` (prefix unary) | Custom prefix unary operators (`"±" (x) :80`) |
| `60` | `*`, `/`, `%`, `.*`, `./` (multiplicative) | Custom multiplicative or parallel-combination operators (`par :>60`) |
| `50` | `+`, `-` (additive) | Custom additive or interval constructors (`(a) "±" (b) :<50`) |
| `45` | `...`, `..<`, `>..`, `>..<` (`:<45`), `plus`/`minus` | Custom range or low-priority additive word operators |
| `42` | `in` (`:<42`) | Custom container/interval membership overloads (`(x:real) in (iv:interval) :<42`) |
| `35`–`55` | `into` (`:>55`), `&` (`:<35`), `\|` (`:<34`) | Custom insertion/composition overloads |
| `30` | `==`, `!=`, `<`, `<=`, `>`, `>=`, `~` | Custom equality and approximate equality (`~`) on user structs |
| `12` / `8` / `5` / `1` | `\|>` (`12`), `=>` (`8`), `:=`/`=` (`5`), `;` (`1`) | Control flow, closures, assignment, sequencing |

### 6.3 Tested Example: Interval Arithmetic Operators (`±`, `span`, `mid`, `+`, `-`, `in`) & Parallel Operator (`par`)

```math
# 1. Prefix unary `±x` constructing symmetric interval [-|x|, +|x|] (precedence 80)
"±" (x:real) :80 -> interval = interval(-@abs(x), @abs(x))

# 2. Postfix unary operators `iv span` and `iv mid` (precedence 90)
(iv:interval) "span" :90 = iv.hi - iv.lo
(iv:interval) "mid"  :90 = 0.5 * (iv.lo + iv.hi)

# 3. Left-associative binary infix `a ± b` constructing [a - |b|, a + |b|] (precedence 50)
(a:real) "±" (b:real) :<50 -> interval = interval(a - @abs(b), a + @abs(b))

# 4. Overloading `+`, `-`, and `in` on custom `$interval` struct
(a:interval) "+" (b:interval) :<50 -> interval = interval(a.lo + b.lo, a.hi + b.hi)
(a:interval) "-" (b:interval) :<50 -> interval = interval(a.lo - b.hi, a.hi - b.lo)
(x:real)     in  (iv:interval) :<42            = (x >= iv.lo) && (x <= iv.hi)

# 5. Right-associative word operator `a par b` for parallel impedance/resistance (precedence 60)
(a:real) par (b:real) :>60 = (a * b) / (a + b)
```

| Tested Expression | Engine Output | Type & Unit |
| :--- | :--- | :--- |
| `(±4).lo * 10 + (±4).hi` | `-36` | `real`, `none` |
| `(100 ± 5) + (50 ± 2)` | `interval(lo=143, hi=157)` | `interval`, `none` |
| `((100 ± 5) span) * 1000 + ((100 ± 5) mid)` | `10100` | `real`, `none` |
| `(103 in (100 ± 5)) * 10 + (108 in (100 ± 5))` | `10` | `real`, `none` |
| `6 par 3 par 2` | `1` | `real`, `none` |

## 7. Defining Custom Functions, Polymorphic Overloads, Default Parameters & Lazy `var_x` Operators

### 7.1 Function Declaration Forms & Parameter Annotations

In `.math` files, functions can be written with concise defaults (unannotated parameters default to `:real` with omitted unit, and unannotated return types default to `-> real`) or with full orthogonal entity-type and physical-unit constraints:

```math
# Form 1: Concise shorthand (all params & return default to `real`)
hyp_3d(x, y, z = 0) = √(x^2 + y^2 + z^2)

# Form 2: Concrete entity type + concrete physical unit locks
avg_speed(d:real, distance, t:real, time) -> real, speed = d / t

# Form 3: Generic unit wildcard `T` (accepts any unit and propagates that exact unit to the output)
damp_step(x:real, T, target:real, T, alpha = 0.25) -> real, T = x + alpha * (target - x)

# Form 4: Concrete unitless lock `none` (rejects unit-tagged arguments)
bit_parity(n:real, none) -> real, none = n % 2

# Form 5: Generic entity wildcard `T` (or `any`)
identity_val(x:T) -> T = x
```

### 7.2 How Orthogonal Type & Unit Multiple Dispatch Scores Overloads

When multiple overloads share the same name and parameter count, `eval_dispatch_overload` (`Core/Code/evaluator.c`) scores each active candidate per parameter and picks the highest-scoring match:

| Parameter Annotation in `.math` | Runtime Argument Match Requirement | Dispatch Score Added |
| :--- | :--- | :--- |
| **Concrete Entity Type** (`x:vec2`, `x:real`, `x:mat`) | Argument `type_id` must equal declared `type_id` | **`+4`** (type score) |
| **Generic Entity Wildcard** (`x:T` or `x:any`) | Matches any runtime `type_id` | **`+1`** (type score) |
| **Concrete Physical Unit** (`x:real, distance`) | Argument `unit_id` must equal `distance` | **`+8`** (unit score) |
| **Concrete Unitless Lock** (`x:real, none`) | Argument must have no unit (`unit_id == 0`) | **`+8`** (unit score) |
| **Generic Unit Wildcard** (`x:real, T` or `x:real, any`) | Binds `bound_T = arg_unit`; all `, T` params must share `bound_T` (or be unitless `0`) | **`+2`** (unit score) |
| **Omitted Unit Annotation** (`x:real` or `x`) | Accepts any unit (unit-agnostic fallback) | **`+1`** (unit score) |

#### Tested Example: Polymorphic `norm_l1` and Unit-Preserving `damp_step`

```math
damp_step(x:real, T, target:real, T, alpha = 0.25) -> real, T = x + alpha * (target - x)

norm_l1(v:vec2) = @abs(v.x) + @abs(v.y)
norm_l1(v:vec3) = @abs(v.x) + @abs(v.y) + @abs(v.z)
norm_l1(v:vec)  = v.map(x => @abs(x)).sum()
```

| Tested Expression | Engine Output | Type & Unit |
| :--- | :--- | :--- |
| `damp_step(10m, 30m) + damp_step(10m, 30m, 0.5)` | `35` | `real`, `distance` |
| `norm_l1(vec2(-3, 4)) * 100 + norm_l1(vec3(-1, -2, 3)) * 10 + norm_l1(vec(-5, 2, -1))` | `768` | `real`, `none` |

### 7.3 Defining Custom Lazy Bound-Variable (`var_x`) Higher-Order Functions

One of MEP's most powerful language-design features is **automatic bound-variable closure synthesis** (`SYM_META_LAZY_MASK` in `Core/compile/compiler.c` lines 685–711):
- Whenever a `.math` function's leading parameter(s) are named with the **`var_` prefix** (such as `var_x`, `var_k`, or `var_t, var_y`), the compiler automatically marks the function as a lazy bound-variable operator and types the immediately following expression parameter(s) as `$fn` closures!
- At call sites, the caller passes a **bare variable identifier** (`x` or `t`) and a **symbolic expression** (`3 * x^2` or `(4 mps2) * t`), and the parser automatically compiles that expression into a callable lexical closure `expr(x)`!
- Furthermore, if the expression parameter is named **`eq`**, the parser sets `SYM_META_LAZY_EQ_BIT` (`0x40`), which automatically rewrites an equation `lhs == rhs` (or `lhs = rhs` / `lhs ~ rhs`) at the call site into the residual closure `(x) => (lhs) - (rhs)`!

#### Tested Example: Authoring a Custom Midpoint Integrator (`riemann_mid`) and Bisection Equation Solver (`bisect_solve`)

```math
# 1. `var_x` turns `expr` into a 1-arg closure `expr(x)` at every call site!
riemann_mid(var_x, expr, a, b, n = 100) = {
    steps := @floor(n)
    if (steps < 1) { error("riemann_mid() requires n >= 1") }
    h   := (b - a) / steps
    a_u := a + 0 * h
    acc := (h * expr(a_u + 0.5 * h))
    (i in 1..<steps) {
        xm  := a_u + (i + 0.5) * h
        acc += expr(xm) * h
    }
    calc_snap(acc, 1e-9)
}

# 2. Naming the second parameter `eq` after `var_x` automatically rewrites `lhs == rhs` into `lhs - rhs`!
bisect_solve(var_x, eq, lo, hi, tol = 1e-9) = {
    a  := lo
    b  := hi
    fa := eq(a)
    fb := eq(b)
    if (fa * fb > 0) { error("bisect_solve() requires opposite signs at endpoints") }
    (iter in 1...80) {
        m  := 0.5 * (a + b)
        fm := eq(m)
        if (@abs(fm) <= tol || @abs(b - a) <= tol) { return calc_snap(m, 1e-8) }
        if (fa * fm <= 0) {
            b  = m
            fb = fm
        } else {
            a  = m
            fa = fm
        }
    }
    calc_snap(0.5 * (a + b), 1e-8)
}
```

| Tested Expression | Engine Output | Type & Unit |
| :--- | :--- | :--- |
| `round(riemann_mid(x, 3 * x^2, 0, 2, 200), 5)` | `7.99995` | `real`, `none` |
| `riemann_mid(t, (4 mps2) * t, 0 sec, 5 sec, 100)` | `50` | `real`, `distance` |
| `bisect_solve(x, x^3 - x - 2 == 0, 1, 2)` | `1.521379707` | `real`, `none` |

## 8. Writing Complete Programs with MEP Language Constructs

Whether inside a `.math` function body `{ ... }` or in a top-level expression/session script, MEP provides a Turing-complete procedural and functional programming language (bounded by `MEM_MAX_EVAL_STEPS = 2,000,000` steps per evaluation and `128` user recursion frames so programs always terminate deterministically).

### 8.1 Scopes, Statement Sequencing, Variables, Slot Locks & Destructuring

| Construct | Syntax | Semantics & Rules | Tested Example | Output |
| :--- | :--- | :--- | :--- | :--- |
| **Multi-Statement Same-Line (`;`)** | `stmt1 ; stmt2 ; expr` | `;` is required when placing multiple statements on the same line. | `a := 4 ; b := 5 ; a * b` | `20` |
| **Multi-Statement Newline Separation** | `stmt1 \n stmt2 \n expr` | Newline `\n` acts as an automatic `;` between complete statements inside `{ ... }` or top-level scripts. | `{\n a := 4\n b := 5\n a + b\n}` | `9` |
| **Same-Scope Parentheses `( ... )`** | `(a := 3 ; b := 4 ; a + b)` | Variables declared inside `(...)` remain visible in the enclosing scope. | `(a := 3 ; b := 4 ; a + b) * 10 + a` | `73` |
| **Isolated Lexical Block `{ ... }`** | `{ a := 99 ; a + 1 }` | Opens an isolated scope; inner `:=` declarations shadow outer variables without overwriting them. | `x := 10 ; y := { x := 99 ; x + 1 } ; x * 1000 + y` | `10100` |
| **Entity Type Lock** | `x : real := 5` | Locks variable slot to `real`; allows unit changes (`x = 50m`) but rejects `x += 5i`. | `x : real := 5 ; x = 50m ; x` | `50` (`distance`) |
| **Physical Unit Lock** | `x : distance := 50m` | Locks variable slot to `distance`; allows entity promotion (`x += 20im` $\to$ `complex`) while rejecting `60 deg`. | `x : distance := 50m ; x += 20im ; x` | `50 + 20i` (`distance`) |
| **Combined Type & Unit Lock** | `x : real, distance := 50m` | Locks slot to both `real` and `distance`. | `x : real, distance := 50m ; x += 250cm ; x` | `52.5` (`distance`) |
| **Typed Container Lock** | `v : vec<real, distance> := ...` | Locks vector elements to `real` with unit `distance`. | `q := queue<real, distance>(10m, 20m) ; q.dequeue()` | `10` (`distance`) |
| **Multi-Variable Broadcast** | `x, y, z := 7` | Broadcasts a scalar across multiple variables (`x, y += 3` also supported). | `x, y := 5 ; x, y += 3 ; x * 10 + y` | `88` |
| **Struct & Vector Destructuring** | `x, y := vec2(3, 4)` | Unpacks struct fields or `$vec`/`$mat` elements positionally into variables. | `r1, r2 := quad_roots(1, -5, 6) ; r1 * 10 + r2` | `32` |

### 8.2 Conditionals (`? :`, `if / or / else`), Distributed Comparisons & Pattern-Matched `switch`

> **Important Syntax Rule for `if` Chains**: MEP uses **`or (cond) { ... }`** (or `then (cond) { ... }`) for else-if branches—never `else if (...)`. Writing `else if` is rejected at parse time with a diagnostic error directing you to use `or (cond) { ... }`.

```math
# 1. Multi-branch `if ... or ... else` chain
classify_mach(m) -> str = {
    if (m < 0.8) {
        "subsonic"
    } or (m < 1.2) {
        "transonic"
    } or (m < 5.0) {
        "supersonic"
    } else {
        "hypersonic"
    }
}

# 2. Structural pattern-matched `switch` with literals, ranges, nested skip ranges,
#    relational guards (`<`, `>=`), struct destructuring, vector destructuring, and `if` guards
describe_point(p:vec2) -> str = switch (p) {
    case vec2(0, 0)           : { "origin" }
    case vec2(0, y) if y > 0  : { "positive-y-axis" }
    case vec2(x, 0)           : { "x-axis" }
    case vec2(x, y) if x == y : { "diagonal" }
    else                      : { "general" }
}
```

| Conditional / Pattern Construct | Tested Expression | Engine Output |
| :--- | :--- | :--- |
| **Chained Right-Associative Ternary** | `s := 85 ; (s >= 90) ? "A" : (s >= 80) ? "B" : "C"` | `"B"` |
| **Distributed Comparison (`&`, `\|`, `!`)** | `x := 1 ; y := 3 ; (x & !y == 1 \| 2)` | `1` |
| **Switch Relational + Range + Skip-Range Cases** | `switch (80) { case < 0, > 100 : { -1 } case 1..<25...75>..100 : { 10 } else : { 5 } }` | `10` |
| **Switch Vector Destructuring + `if` Guard** | `switch ([0, 12]) { case [0, y] if y > 5 : { y * 3 } else : { 0 } }` | `36` |
| **Switch Value-Returning Early `break(val)`** | `switch (2) { case 2 : { x := 25 ; if (x > 20) { break(x * 2) } ; 999 } else : { 0 } }` | `50` |

### 8.3 Generic `...` Ranges, Nested Exclusion (Skip) Ranges & Loops

MEP unifies all ranges around the 3-dot `...` operator (`[a, b]`) with `<` and `>` angle brackets indicating open bounds (`a..<b`, `a>..b`, `a>..<b`). Chaining `..<` with `...` and `>..` creates **nested exclusion (skip) ranges** that iterate and test membership in **$O(1)$ scratch memory without allocating a vector**:

| Range / Loop Form | Syntax | Semantics | Tested Example | Output |
| :--- | :--- | :--- | :--- | :--- |
| **Closed Range** | `a...b` | $[a, b]$ inclusive (ascending or descending) | `5...1` | `[5, 4, 3, 2, 1]` |
| **Half-Open & Open Ranges** | `a..<b`, `a>..b`, `a>..<b` | $[a, b)$, $(a, b]$, $(a, b)$ | `1>..<5` | `[2, 3, 4]` |
| **Single-Interval Skip Range** | `a..<x...y>..b` | Includes $[a, x-1]$ and $[y+1, b]$, skipping $[x, y]$ | `1..<4...7>..10` | `[1, 2, 3, 8, 9, 10]` |
| **Single-Point Skip Range** | `a..<x>..b` | Skips the single point $x$ | `1..<5>..10` | `[1, 2, 3, 4, 6, 7, 8, 9, 10]` |
| **Multi-Interval Skip Range** | `a..<x1...y1>..<x2...y2>..<b` | Chains up to 8 disjoint segments | `s := 0 ; (x in 1..<25...50>..<75...90>..<100) { s += x } ; s` | `2655` |
| **Stepped Range (`by`)** | `rng by step` | Custom stride across single or nested skip ranges | `(1..<4...7>..10) by 2` | `[1, 3, 9]` |
| **Pre-Tested While Loop** | `(cond) ?? { body }` | Evaluates `body` while `cond != 0` | `s := 0 ; i := 1 ; (i <= 5) ?? { s += i ; i += 1 } ; s` | `15` |
| **Post-Tested Do-While Loop** | `{ body } ?? (cond)` or `(body) ?!? (cond)` | Executes `body` once before testing `cond` | `k := 10 ; { k += 1 } ?? (k < 5) ; k` | `11` |
| **1-Var & 2-Var `in` Loop** | `(x in coll) { ... }` / `(i, x in coll) { ... }` | Iterates ranges ($O(1)$), `$vec`, `$mat`, `$str` chars, or `$map` `(key, val)` pairs | `m := dict("a", 10, "b", 20) ; s := 0 ; (k, v in m) { s += v } ; s` | `30` |
| **Labeled `break(Tag, val)` & `continue(Tag)`** | `Outer : (i in 1...5) ?? { ... break(Outer, val) }` | Exits or advances an outer labeled loop and returns `val` as the loop's value | `Outer : (i in 1...5) ?? { (j in 1...5) ?? { if (i * j == 12) { break(Outer, i * 100 + j) } } }` | `304` |
| **Filtered & Indexed Comprehension** | `[expr for i, x in iter if cond]` | Builds a `$vec` from any range, skip range, vector, matrix, string, or map | `[i * 10 + x for i, x in vec(3, 4, 5, 6) if i % 2 == 1]` | `[14, 36]` |

### 8.4 Tagged-Scope State Machines (`Tag : { ... }`, `jump(Tag)`, `run(Tag)`, `Tag()`)

Inside any block or sequence, up to 16 scopes can be labeled with `Tag : { ... }`:
- **`jump(Tag)`**: Transfers the sequence program counter (`pc`) directly to `Tag` (supporting both forward jumps and backward state-machine loops in $O(1)$ stack space).
- **`run(Tag)` or `Tag()`**: Re-executes the body of `Tag` inline as a zero-overhead local subroutine.
- **`break(Tag, val)`**: Exits `Tag` immediately with value `val`.

```math
collatz_steps(n) = {
    steps := 0
    jump(Loop)
    Tick : { steps += 1 }
    Loop : {
        if (n <= 1) {
            jump(Finish)
        } or ((n % 2) == 0) {
            n /= 2
            run(Tick)
            jump(Loop)
        } else {
            n = 3n + 1
            Tick()
            jump(Loop)
        }
    }
    Finish : { steps }
}
```
- **Tested Evaluation**: `collatz_steps(6)` $\implies$ `8` (`real`, `none`).

### 8.5 First-Class Closures (`=>`), Inline Recursive Functions (`:=`), UFCS (`.`) & Pipelines (`|>`)

Users can define functions on the fly inside expressions or sessions without compiling a `.math` module:

```math
# 1. Lexical closure capturing outer variable `k` with default arguments
k := 10 ; scale := (x, mul = 2) => x * mul + k ; scale(5) * 100 + scale(5, 3)
# => 2025

# 2. Inline recursive function definition (persists across evaluations in a session)
tri(n) := n <= 1 ? 1 : n + tri(n - 1) ; tri(10) + (6).tri()
# => 76

# 3. UFCS dot-chaining and functional pipe operator `|>`
(1...5).map(x => x^2).filter(x => x > 5).sum()
# => 50

vec(1, 2, 3, 4, 5) |> map(x => x^2) |> filter(x => x > 5) |> sum
# => 50
```

## 9. Complete Reference of `@` Intrinsics Available in `.math` Specs

When writing `.math` specifications, you can delegate low-level operations to C intrinsics using `@<intrinsic_name>` (`Core/lib/delegate/delegate.c`):

| Category | `@` Intrinsic Names | Purpose |
| :--- | :--- | :--- |
| **Token & Type Intrinsics** | `@f64`, `@f64_trim1`, `@i64`, `@str_slice`, `@vec_type`, `@mat_type`, `@fn_type`, `@map_type`, `@complex_type`, `@any_type`, `@none_unit` | Used after `=>` in `$type` declarations. |
| **Syntax Delimiters** | `@lparen`, `@rparen`, `@lbrace`, `@rbrace`, `@lbrack`, `@rbrack`, `@comma` | Binds `( )`, `{ }`, `[ ]`, and `,` in `[boot_syntax]`. |
| **Control & Meta Intrinsics** | `@seq`, `@store`, `@while`, `@do_while`, `@if`, `@or_branch`, `@then_branch`, `@else_branch`, `@switch_block`, `@case_branch`, `@for_kw`, `@ternary`, `@colon`, `@jump_tag`, `@run_tag`, `@break_loop`, `@continue_loop`, `@return_fn`, `@lambda`, `@pipe`, `@cand`, `@cor`, `@cnot`, `@field`, `@fcall` | Core control flow, closures, field access, and short-circuit/distributed logic. |
| **Arithmetic & Bitwise** | `@add`, `@sub`, `@mul`, `@div`, `@mod`, `@neg`, `@abs`, `@floor`, `@ceil`, `@pow`, `@sqrt`, `@cbrt`, `@exp`, `@ln`, `@eq`, `@ne`, `@lt`, `@le`, `@gt`, `@ge`, `@band`, `@bor`, `@bxor`, `@bnot`, `@bshl`, `@bshr` | IEEE-754 + 7D SI unit-aware math and 64-bit integer bitwise operations. |
| **Trigonometric & Special** | `@sin`, `@cos`, `@tan`, `@asin`, `@acos`, `@atan`, `@atan2`, `@erf`, `@gamma`, `@lgamma` | Angle-unit-aware (`deg`/`rad`) trig and special functions. |
| **Vector & Range (`$vec`)** | `@vnew`, `@vfill`, `@vpush`, `@vpop`, `@vshift`, `@vget`, `@vset`, `@vlen`, `@vcap`, `@vresize`, `@vshrink`, `@vclear`, `@vrange`, `@vrange_cat`, `@vrange_step` | Dynamic vector, stack, queue, and range primitives. |
| **Dictionary (`$map`)** | `@map_new`, `@map_set`, `@map_get`, `@map_has`, `@map_del` | String-keyed hash/table dictionary primitives. |
| **Matrix / Tensor (`$mat`)** | `@mrank`, `@mlen`, `@mdim`, `@mreshape` | 1D–4D dense tensor metadata and reshaping (all linear algebra is written in pure `.math` on top of these). |
| **String & Regex (`$str`)** | `@tostr`, `@fromstr`, `@str_fmt`, `@regex_match`, `@regex_find`, `@regex_find_all`, `@regex_replace`, `@regex_split` | String conversion (`x_`, `_s`), `{}` formatting, and backtracking regex engine. |
| **Symbolic CAS** | `@sym_diff`, `@sym_integ`, `@sym_solve`, `@sym_simplify`, `@sym_eval` | AST-level analytical differentiation, integration, polynomial solving, and simplification. |
| **Unit Transfer, GC & Error** | `@raw(x[, donor])`, `@type_name(x)`, `@unit_name(x[, axis])`, `@gc_compact(0)`, `@pool_used(0)`, `@err(msg)` | Strips or transfers unit tags (`with_unit`), queries runtime types/SI exponents, compacts scratch arena, or raises custom runtime errors. |

## 10. Complete, Tested End-to-End Programs

Every complete program below was executed against the compiled engine and verified in `verify_docs_examples.c`.

### 10.1 Program 1: Orbital Mechanics Classifier (`simulate_orbit_energy`)

Computes specific orbital energy $\epsilon = \frac{v^2}{2} - \frac{\mu}{r}$ for a spacecraft around Earth, classifies the trajectory via a multi-branch `if ... or ... else` chain with approximate equality `~`, and formats a diagnostic report string using postfix stringification `_`:

```math
simulate_orbit_energy(r0_km, v0_kms) := {
    mu_earth := 398600.4418
    eps := 0.5 * (v0_kms^2) - (mu_earth / r0_km)
    kind := if (eps < -1e-6) {
        "elliptical"
    } or (eps ~ 0) {
        "parabolic"
    } else {
        "hyperbolic"
    }
    kind + ":" + (round(eps, 2)_)
} ;
simulate_orbit_energy(6778, 7.67)
```

- **Verified Output**: `"elliptical:-29.39"` (`str`, `none`)

### 10.2 Program 2: Telemetry Packet Classifier with Filtered Comprehensions & Nested Skip-Range `switch`

Filters out corrupted negative packet sizes using a comprehension `[p for p in pkts if p > 0]`, iterates with 2-variable `(i, p in valid)` indexing, and scores each packet using a pattern-matched `switch` with a nested exclusion range `case 1..<25...75>..100 :` (matching `1..24` and `76..100` while skipping `25..75`):

```math
analyze_packets(pkts:vec) := {
    valid := [p for p in pkts if p > 0]
    score := 0
    (i, p in valid) {
        band := switch (p) {
            case 1..<25...75>..100 : { 10 }
            case 25...75           : { 5 }
            case > 100             : { 20 }
            else                   : { 0 }
        }
        score += (i + 1) * band
    }
    score
} ;
analyze_packets(vec(10, -5, 50, 80, 150))
```

- **Trace**: `valid` is `[10, 50, 80, 150]`.
  - `i=0, p=10` (in `1..24` of `1..<25...75>..100`) $\to$ `1 * 10 = 10`
  - `i=1, p=50` (in `25...75`) $\to$ `2 * 5 = 10`
  - `i=2, p=80` (in `76..100` of `1..<25...75>..100`) $\to$ `3 * 10 = 30`
  - `i=3, p=150` (`> 100`) $\to$ `4 * 20 = 80`
- **Verified Output**: `130` (`real`, `none`)

### 10.3 Program 3: Quantitative Finance Internal Rate of Return (`irr`) Using Custom `bisect_solve`

Combines an inline Net Present Value function `npv(cf:vec, r)` using 2-variable `(t, c in cf)` iteration with the user-defined lazy `var_x` equation solver `bisect_solve(r, npv(flows, r) == 0, 0.0, 0.5)`:

```math
npv(cf:vec, r) := {
    s := 0
    (t, c in cf) { s += c / ((1 + r) ^ t) }
    s
} ;
flows := vec(-1000, 400, 400, 400) ;
irr := bisect_solve(r, npv(flows, r) == 0, 0.0, 0.5) ;
round(irr * 100, 2)
```

- **Verified Output**: `9.7` (`real`, `none` — representing `9.7%` IRR for a `-1000` outlay followed by three `+400` annual cash flows).
