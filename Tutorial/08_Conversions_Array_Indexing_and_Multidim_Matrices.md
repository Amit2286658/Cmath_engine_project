# Chapter 8 — `~` Distributed NOT, Same-Unit Conversions (`65_conversions.math`), Array Subscripting `v[i]`, & Multidimensional Matrices (`$mat`)

## 1. The `~` Logical & Distributed Comparison NOT Operator (`00_bootstrap.math`)
Declared in `Library/00_bootstrap.math` alongside `&` (`@cand`) and `|` (`@cor`):
```text
"~" (a:real) :80 -> real = @cnot(a)
```
- **Standalone logical NOT**: `~0` -> `1`, `~5` -> `0`, `~(10 > 20)` -> `1`.
- **Distributed comparison NOT**: Inside relational comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), `~` distributes automatically alongside `&` and `|`:
  - `(x == ~4)` expands to `~(x == 4)`
  - `(x == ~(1 | 2))` expands to `~((x == 1) | (x == 2))`
  - `(x & ~y == 1 | 2)` expands to `((x == 1) | (x == 2)) & ~((y == 1) | (y == 2))`

## 2. Standard Same-Unit-Type Conversions (`Library/65_conversions.math`)
`Library/65_conversions.math` (`[conversions]`) provides three ways to convert between units of the same unit group (`distance`, `angle`, `mass`, `time`, `speed`, `pressure`, `energy`, `power`):
1. **Universal Infix `"in"` Operator (`:<55`)**:
   ```text
   (val:real, T) "in" (unit:real, T) :<55 -> real, none = @div(val, unit)
   ```
   - `(2.5 km) in (1 m)` -> `2500`
   - `(1.5 m) in (1 nm)` -> `1500000000`
   - `(180 deg) in (1 rad)` -> `3.141592654`
   - Attempting `(100 m) in (1 deg)` is rejected because `distance` != `angle`.
2. **Unit-Checked Postfix Operators (`to_<unit>` `:90`)**:
   - Distance: `to_km`, `to_m`, `to_cm`, `to_mm`, `to_um`, `to_nm`, `to_mi`, `to_nmi`, `to_yd`, `to_ft`, `to_inch`
   - Angle: `to_rad`, `to_deg`, `to_arcmin`, `to_arcsec`
   - Mass: `to_kg`, `to_gram`, `to_mg`, `to_ton`, `to_lb`, `to_oz`
   - Time: `to_sec`, `to_ms`, `to_us`, `to_ns`, `to_min`, `to_hr`, `to_day`
   - Speed, Pressure, Energy, Power: `to_mps`, `to_kph`, `to_mph`, `to_Pa`, `to_atm`, `to_psi`, `to_J`, `to_kcal`, `to_W`, `to_hp`, etc.
3. **Pairwise Functions (`<u1>_to_<u2>(x)`)**:
   - `km_to_m(x)`, `m_to_nm(x)`, `nm_to_m(x)`, `deg_to_rad(x)`, `rad_to_deg(x)`, `kg_to_g(x)`, `hr_to_sec(x)`, etc.

## 3. C-Style Array Indexing `v[i]` for Vectors & Structs
Instead of only using `.` (`v.x`, `v.y`, `v.z`) or `at(v, i)` / `set(v, i, x)`, you can use C-style `v[i]` subscripting for both reading and in-place mutation (`=`, `+=`, `-=`, `*=`, `/=`):
- **Fixed-size vectors & structs (`$vec2`, `$vec3`, `$complex`, `$circle`, `$mat2`)**:
  - `v := vec3(2, 4, 6) ; v[0] = 10 ; v[1] += 5 ; v[2] *= 3` -> `vec3(x=10, y=9, z=18)`
  - Nested structs: `c := circle(vec2(3, 7), 5) ; c[0][1] = 99 ; c[1] += 10` -> `circle(center=vec2(x=3, y=99), radius=15)`
- **Dynamic vectors (`$vec`)**:
  - `a := vec(3, 10) ; a[0] = 5 ; a[1] += 7 ; a[-1] = 30` -> `[5, 17, 30]`
  - Chained indexing on `vec<vec2>`: `pts[1][0] += 10`

## 4. Separate Multidimensional Matrix Data Type (`$mat`) Using `[...]`
`[` and `]` are registered in `00_bootstrap.math` as `@lbrack` and `@rbrack`, and `$mat ~ => @mat_type` defines a dedicated multidimensional matrix/tensor type (1D up to 4D):
- **2D Matrix Literals (Nested or Semicolon Row-Separated)**:
  - `M := [[1, 2, 3], [4, 5, 6]]` or `M := [1, 2, 3 ; 4, 5, 6]`
- **3D / N-D Tensor Literals**:
  - `T := [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]`
- **Multi-Index `M[r, c]` and Chained `M[r][c]` Read & Write**:
  - `M[0, 1] = 9 ; M[1][0] += 10`
  - `T[1][0][1] += 10 ; T[1, 0, 1]`
- **Matrix Arithmetic & Linear Algebra (`50_algebra.math`)**:
  - `A + B`, `A - B`, `A * B` (2D*2D and 2D*1D), `s * A`, `A / s`, `-A`
  - `det(M)`, `trace(M)`, `transpose(M)`, `inv(M)` (Gauss-Jordan inverse)
  - `mat_zeros(r, c)`, `mat_ones(r, c)`, `mat_eye(n)`, `mat_fill(r, c, val)`
  - `mat_rows(M)`, `mat_cols(M)`, `mat_rank(M)`, `mat_len(M)`, `mat_shape(M)`
