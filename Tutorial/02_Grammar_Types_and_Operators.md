# Chapter 2 — Grammar, Types, Shorthand Declarations, Default Params, Implicit Multiplication & Postfix `"i"`

This chapter explains how `.math` files define lexical types, syntax delimiters, operator fixities/precedences, streamlined function/constant declarations, default parameters, implicit multiplication, and complex numbers.

## 1. Module Headers & Dependencies

Every `.math` file organizes declarations into named modules with `[module_name]` and declares required modules with `<[dependency_name]`:

```math
[complex_ext]
<[arith]
<[trig]
```

## 2. Consuming Lexical Types vs. Scalar Operator Types (`00_bootstrap.math`)

```math
[boot_types]
<[boot_syntax]

$real ~ \d+ ("." \d+)?       => @f64
$iota ~                      => @f64_trim1
$str  ~ "\"" [^"]* "\""      => @str_slice
$vec  ~                      => @vec_type
$mat  ~                      => @mat_type
```

- **`$real`**: Matches integer and decimal literals (`42`, `3.14159`) and parses them into 64-bit IEEE-754 floats (`@f64`).
- **`$str`**: Matches double-quoted UTF-8 strings (`"USD"`, `"Euler Formula"`) into zero-copy 4-byte `StrSlice` handles (`@str_slice`).
- **`$iota`**: Has **no** regex pattern after `~`; `$iota` values are constructed by the **postfix `"i"` operator** in `Library/30_complex.math`.

## 3. Operator Fixities & Binding Precedence

| Fixity | `.math` Declaration Syntax | Example |
| :--- | :--- | :--- |
| **Prefix Unary** | `"sym"(x:T) :prec -> Ret = body` | `"√"(x:real) :80 -> real = @sqrt(x)` |
| **Postfix Unary** | `(x:T) "sym" :prec -> Ret = body` | `(n:real) "!" :90 -> real = ...` |
| **Left-Assoc Infix** | `(a:T1) "sym" (b:T2) :<prec -> Ret = body` | `(a:real) "+" (b:real) :<50 -> real = @add(a, b)` |
| **Right-Assoc Infix** | `(a:T1) "sym" (b:T2) :>prec -> Ret = body` | `(a:real) "^" (b:real) :>85 -> real = @pow(a, b)` |
| **Function Call** | `"name"(a:T1, b:T2, ...) -> Ret = body` | `"hypot"(a:real, b:real) -> real = √(a^2 + b^2)` |

## 4. Streamlined Function & Constant Declarations + Default Parameter Values

In `.math` files, identifier function/constant names do not require quotes, untyped parameters default to `:real`, and omitted `-> Ret` defaults to `-> real`. Trailing parameters can also specify default values (`param = default_expr`), which automatically synthesize lower-arity overloads:

```math
# Shorthand constant declarations (quotes and `:real` optional)
tau_const  = 2π
golden_ang = 360 / (φ ^ 2)

# Shorthand function declarations with default parameter values
hyp_3d(x, y, z = 0)        = √(x^2 + y^2 + z^2)
log_base(x, base = 10)     = ln(x) / ln(base)
round(x:real, digits = 0)  = ...
fx_with_fee(amount:real, from:str, to:str, fee_pct:real = 1.5) -> real = { ... }
```

## 5. Natural Implicit Multiplication (`2x`, `2π`, `3(x + 1)`, `(x + 1)(x - 1)`, `2 sin(x)`)

When a value, variable, constant, or closing `)` is immediately followed by an identifier, `(...)`, or prefix-only operator (`√`), the parser inserts multiplication `*` at binding power `60 -> 65`:
- `x := 4 ; 2x^2 + 3x + 1` -> `45` (because `^` at `:85` binds tighter than implicit `*`, `2x^2` parses as `2 * (x^2)`)
- `x := 5 ; 3(x + 1) + (x + 1)(x - 1)` -> `42`
- `2 sin(π / 2) + 2 √16 + (2π / π)` -> `12`

## 6. Postfix `"i"` Operator & Complex Arithmetic (`30_complex.math`)

```math
$complex(re:real, im:real)

(x:real)    "i" :95 -> iota    = @raw(x)
(x:iota)    "i" :95 -> real    = -@raw(x)
(z:complex) "i" :95 -> complex = complex(-z.im, z.re)
```

Because `"i"` is a postfix operator with multiple dispatch, it works on literals (`1 + 1i`), parenthesized expressions (`(3 + 4)i` -> `7i`), function calls (`2 + sin(π / 2)i` -> `2 + 1i`), chained imaginary units (`((3 + 4)i)i` -> `-7`), and complex rotation (`(1 + 2i)i` -> `-2 + 1i`).
