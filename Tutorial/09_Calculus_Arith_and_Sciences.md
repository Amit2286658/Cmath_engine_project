# Chapter 9 — High School Calculus, Commercial Arithmetic & Quantitative Sciences

This chapter documents:
1. **Strict Boolean-Condition Prefix `!`** (`!(x == 5)`, `(x == !(1 | 2))`) and **Approximate Equality `~`** (`5.001 ~ 5`).
2. **High School Calculus (`00_bootstrap.math` & `Library/55_calculus.math`)**: Derivatives, definite integrals, limits (including removable singularities and one-sided limits), discrete sums/products, symbolic polynomial calculus, tangent/normal lines, arc length, and solids of revolution.
3. **Commercial & General Arithmetic (`Library/10_arith.math`)**: Percentages, profit & loss, discounts, simple/compound interest, EMI, work & time, pipes & cisterns, AP/GP/HP progressions, means, and relative speed/boats.
4. **Expanded High School Physics (`70_physics.math`), Chemistry (`80_chemistry.math`), and Biology (`90_biology.math`)**.

---

## 1. Strict Boolean Prefix `!` vs. Approximate Equality `~`

- **Prefix `!` (`"!" (a:real) :80 -> real = @cnot(a)`)**:
  - Exclusively flips a **Boolean condition** (`!(x == 5)`, `!(5.01 ~ 5)`, `!(a > 0 && b > 0)`, `!is_prime(100)`, or distributed `(x == !(1 | 2))` / `(x & !y == 1 | 2)`).
  - Non-boolean operands such as `!5`, `!0`, or `!(2 + 3)` are rejected with an error (`"Operator '!' requires a boolean condition..."`).
  - Postfix `n!` (`5! = 120`) remains factorial because the Pratt parser separates prefix (`nud`) and postfix (`led`) positions.
- **Approximate Equality `~` (`(a:real, T) "~" (b:real, T) :<30 -> real = @approx(a, b)`)**:
  - Returns `1` (true) when $|a - b| \le 0.001$ (at least 3 decimal places close):
    - `5.001 ~ 5` $\implies 1$ (true)
    - `5.000001 ~ 5` $\implies 1$ (true)
    - `4.999 ~ 5` $\implies 1$ (true)
    - `5.01 ~ 5` $\implies 0$ (false)
    - `5.002 ~ 5` $\implies 0$ (false)

---

## 2. High School Calculus (`00_bootstrap.math` & `Library/55_calculus.math`)

### 2.1 Expression-Based Calculus Constructs
- `diff(x, expr, x0)`: First derivative $\frac{d}{dx}(\text{expr})$ at $x = x_0$ (4th-order 5-point stencil).
  - Example: `diff(x, x^3 + 2*x, 3)` $\implies 29$.
- `diff2(x, expr, x0)` and `diff_n(x, expr, x0, order)`: Second and higher-order derivatives at $x = x_0$.
  - Example: `diff2(x, x^4, 2)` $\implies 48$.
- `integrate(x, expr, a, b)` / `ntg(x, expr, a, b)`: Definite integral $\int_a^b \text{expr}\,dx$ via Composite Simpson's 1/3 Rule.
  - Example: `integrate(x, 3 * (x^2), 0, 2)` $\implies 8$, `ntg(x, sin(x), 0, π) ~ 2` $\implies 1$.
- `lim(x, expr, x0)`, `lim_left(x, expr, x0)`, `lim_right(x, expr, x0)`: Two-sided and one-sided limits with Richardson extrapolation (handles removable singularities and rejects jump discontinuities).
  - Examples: `lim(x, sin(x) / x, 0)` $\implies 1$, `lim(x, (x^2 - 4) / (x - 2), 2)` $\implies 4$.
- `sigma(k, expr, lo, hi)` and `prod(k, expr, lo, hi)`: Discrete summation $\sum_{k=\text{lo}}^{\text{hi}}$ and product $\prod_{k=\text{lo}}^{\text{hi}}$.

### 2.2 Polynomial & Geometric Calculus (`[calculus]` in `Library/55_calculus.math`)
- `poly_eval(c, x)`, `poly_diff(c)`, `poly_diff_at(c, x0)`, `poly_diff2_at(c, x0)`, `poly_def_integ(c, a, b)`
- `tangent_slope(c, x0)`, `normal_slope(c, x0)`, `tangent_y(c, x0, x)`, `normal_y(c, x0, x)`, `poly_curvature(c, x0)`
- `quad_critical_x(a, b, c)`, `quad_extremum_y(a, b, c)`, `poly_newton_root(c, x_init)`
- `poly_avg_val(c, a, b)`, `poly_arc_length(c, a, b)`, `poly_vol_disk(c, a, b)`, `poly_vol_shell(c, a, b)`
- `trapz_vec(y, dx)`, `simpson_vec(y, dx)`, `taylor_exp(x, n)`, `taylor_sin(x, n)`, `taylor_cos(x, n)`

---

## 3. Commercial & Broader Arithmetic (`Library/10_arith.math`)

- **Percentages**: `pct_of(part, whole)`, `pct_val(pct, total)`, `pct_change(old_v, new_v)`, `pct_inc(val, pct)`, `pct_dec(val, pct)`, `pct_error(measured, exact)`, `successive_pct(p1, p2)`
- **Profit, Loss & Discounts**: `profit`, `loss`, `profit_pct`, `loss_pct`, `sp_from_profit_pct`, `sp_from_loss_pct`, `cp_from_profit_pct`, `cp_from_loss_pct`, `discount_amt`, `sp_after_discount`, `successive_discount`
- **Interest, Depreciation & EMI**: `simple_interest(p, r, t)`, `si_amount(p, r, t)`, `compound_amount(p, r, t)`, `compound_interest(p, r, t)`, `ci_amount_n(p, r, t, n)`, `ci_si_diff_2yr(p, r)`, `depreciated_val(p0, r, t)`, `emi(principal, annual_rate_pct, months)`
- **Work & Time / Pipes & Cisterns**: `work_rate`, `work_together_2(d1, d2)`, `work_together_3(d1, d2, d3)`, `work_other_person(d_both, d_one)`, `work_rem_after`, `pipe_fill_with_leak(t_fill, t_empty)`, `man_days_solve(m1, d1, w1, m2, w2)`
- **Progressions, Means & Speed**: `ap_nth`, `ap_sum`, `gp_nth`, `gp_sum`, `gp_inf_sum`, `hp_nth`, `arith_mean`, `geom_mean`, `harm_mean`, `fourth_proportional`, `third_proportional`, `mean_proportional`, `alligation_ratio`, `avg_speed_2`, `rel_speed_same`, `rel_speed_opp`, `boat_downstream`, `boat_upstream`, `boat_still_speed`, `stream_speed`, `sum_n`, `sum_sq_n`, `sum_cube_n`

---

## 4. High School Physics, Chemistry & Biology (`70_physics.math`, `80_chemistry.math`, `90_biology.math`)

- **`[physics]` (`Library/70_physics.math`)**: SUVAT kinematics (`suvat_v`, `suvat_s`, `suvat_v2`, `suvat_nth_sec`), friction/inclines/collisions (`elastic_v1`, `elastic_v2`, `inelastic_v`, `recoil_vel`), rotation & SHM (`torque_val`, `rot_ke`, `shm_vel`, `shm_acc`), fluids & elasticity (`hydro_pressure`, `buoyant_force`, `terminal_vel`, `youngs_modulus`), thermodynamics (`sensible_heat`, `latent_heat`, `wien_peak_lambda`, `stefan_boltzmann_p`), acoustics & optics (`doppler_freq`, `mirror_image_v`, `lens_power_dioptre`, `young_fringe_width`, `malus_law`, `brewster_angle`), AC RLC & modern physics (`rlc_impedance`, `rlc_resonant_freq`, `transformer_v2`, `bohr_energy_ev`, `rydberg_wavelength`).
- **`[chemistry]` (`Library/80_chemistry.math`)**: Empirical/stoichiometric yields (`percent_composition`, `percent_yield`, `atom_economy`, `normality_from_molarity`, `mole_fraction`, `ppm_conc`), gas laws (`boyle_p2`, `charles_v2`, `combined_gas_v2`, `dalton_partial_p`, `van_der_waals_p`), colligative properties (`raoult_p_soln`, `bp_elevation`, `fp_depression`, `vant_hoff_osmotic`), weak acid/buffer/Ksp (`weak_acid_ph`, `pka_from_ka`, `solubility_ksp_ab`, `solubility_ksp_ab2`), electrochemistry & solid state (`cell_emf`, `faraday_mass`, `molar_conductivity`, `crystal_density`, `bragg_d_spacing`).
- **`[biology]` (`Library/90_biology.math`)**: Hardy-Weinberg genetics (`hw_homo_dom`, `hw_hetero`, `hw_homo_rec`, `hw_carrier_count`, `recomb_freq`, `chi_sq_term`), molecular biology & PCR (`chargaff_g_from_a`, `dna_length_nm`, `dna_turns`, `pcr_copies`, `pcr_yield_from`), enzyme kinetics & $Q_{10}$ (`michaelis_menten`, `competitive_inh_v`, `noncomp_inh_v`, `q10_rate`, `respiratory_quotient`), plant water relations (`water_potential`, `solute_potential`, `diffusion_pressure_deficit`, `stomatal_index`), microscopy (`total_magnification`, `fov_diameter`, `mitotic_index`), human physiology (`cardiac_output`, `ejection_fraction`, `mean_arterial_pressure`, `alveolar_ventilation`, `renal_clearance`, `bmi_calc`), and ecology (`pop_exp_growth`, `pop_logistic_nt`, `lincoln_petersen_pop`, `trophic_energy_at`, `simpson_diversity_vec`, `shannon_diversity_vec`).
