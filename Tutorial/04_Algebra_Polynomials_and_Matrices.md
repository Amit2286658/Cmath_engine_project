# Chapter 4 — Algebra, Polynomials, Matrices & Equation Solvers

This chapter covers the `[algebra]` module (`Library/50_algebra.math`) and explains how the engine handles both **numerical/structural algebra** and **symbolic CAS tree rewriting**.

## 1. 2x2 Matrix Algebra (`$mat2`) & Linear Systems (`solve_2x2`)

`Library/50_algebra.math` defines `$mat2(a:real, b:real, c:real, d:real)`:
- **Arithmetic & Vector Transform**: `m1 + m2`, `m1 - m2`, `m1 * m2`, `s * m`, and `m * vec2(x, y)`.
- **Invariants & Inverse**: `det(m)`, `trace(m)`, `transpose(m)`, `inv_mat2(m)`.
- **Cramer's Rule 2x2 Linear System Solver (`solve_2x2`)**:
  `solve_2x2(2, 3, 8, 5, -1, 3)` solves `2x + 3y = 8, 5x - y = 3` -> `vec2(x=1, y=2)`.

## 2. Quadratic Equation Solvers (Real & Complex Roots)

For `a*x^2 + b*x + c = 0`:
- `quad_disc(a, b, c)`: Discriminant `b^2 - 4*a*c`.
- `quad_roots(a, b, c)`: Returns real roots `vec2(r1, r2)` (`quad_roots(1, -5, 6)` -> `vec2(x=3, y=2)`).
- `quad_croot(a, b, c)`: Returns principal root in `$complex` even when discriminant < 0 (`quad_croot(1, 2, 5)` -> `-1 + 2i`).

## 3. Symbolic & Numerical Polynomial Algebra on `$vec`

A polynomial `P(x) = c_0 + c_1*x + c_2*x^2 + ... + c_n*x^n` is represented by its coefficient vector `[c_0, c_1, ..., c_n]`:
- `poly_val(p, x)`: Horner's method evaluation in O(n) steps.
- `poly_deriv(p)`: Exact symbolic derivative `[c_1, 2*c_2, 3*c_3, ...]`.
- `poly_integ(p, c0)`: Exact symbolic indefinite integral `[c0, c_0/1, c_1/2, ...]`.
- `poly_add(p, q)` & `poly_mul(p, q)`: Exact symbolic polynomial sum and product (Cauchy convolution).
- `poly_newton(p, x0, steps)`: Newton-Raphson algebraic root solver using `poly_deriv(p)`.
