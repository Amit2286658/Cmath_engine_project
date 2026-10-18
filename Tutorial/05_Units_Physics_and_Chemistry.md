# Chapter 5 — Unit Conversions, Physics & Chemistry Modules

This chapter documents the `[units]`, `[physics]`, and `[chemistry]` standard library modules (`Library/60_units.math`, `Library/70_physics.math`, and `Library/80_chemistry.math`).

## 1. Postfix Unit Operators & Conversions (`60_units.math`)

All postfix unit operators have precedence `95` and normalize quantities directly to **SI base units** (`rad`, `m`, `kg`, `s`, `m/s`, `Pa`, `J`, `W`):
- **Angles & Decibels**: `deg`, `°`, `arcmin`, `arcsec`, `pct`, `dB`, `dBm`, `deg2rad(d)`, `rad2deg(r)`
- **Length (-> m)**: `km`, `cm`, `mm`, `um`, `nm`, `mi`, `nmi`, `yd`, `ft`, `inch`, `m_to_km`, `m_to_ft`, `ft_to_m`, `in_to_cm`, `cm_to_in`
- **Mass (-> kg)**: `kg`, `gram`, `mg`, `ton`, `lb`, `oz`, `amu`, `kg_to_lb`, `lb_to_kg`
- **Time (-> s)**: `ms`, `us`, `ns`, `sec`, `min_t`, `hr`, `day`
- **Speed (-> m/s)**: `kph`, `mph`, `knots`, `mps_to_kph`, `mps_to_mph`
- **Pressure (-> Pa)**: `kPa`, `MPa`, `bar`, `atm`, `psi`, `torr`
- **Energy & Power (-> J, W)**: `kJ`, `MJ`, `cal`, `kcal`, `Wh`, `kWh`, `eV`, `kW`, `MW`, `hp`
- **Temperature**: `c_to_k`, `k_to_c`, `c_to_f`, `f_to_c`, `f_to_k`, `k_to_f`

## 2. Physics Suite (`70_physics.math`)

- **CODATA Constants**: `g0`, `c0`, `G_grav`, `h_planck`, `k_B`, `ke_coulomb`, `q_e`, `m_e`, `m_p`, `sigma_sb`.
- **Mechanics & Gravity**: `ke`, `pe`, `mech_energy`, `proj_range`, `proj_max_h`, `proj_time`, `centripetal_acc`, `centripetal_force`, `hooke_force`, `elastic_pe`, `pendulum_period`, `grav_force`, `grav_pe`, `orbital_vel`, `escape_vel`, `schwarzschild_r`, `kepler_period`.
- **Circuits, Optics, Quantum & Relativity**: `coulomb_force`, `ohm_v`, `ohm_i`, `ohm_r`, `elec_power`, `cap_energy`, `ind_energy`, `res_series`, `res_parallel`, `rc_tau`, `photon_energy`, `photon_wavelength`, `de_broglie`, `snell_angle2`, `lens_focal`, `lorentz_gamma`, `rel_energy`, `time_dilation`, `length_contraction`, `carnot_eff`.

## 3. Chemistry Suite (`80_chemistry.math`)

- **Constants**: `N_A`, `R_gas`, `R_atm`, `F_faraday`, `T_stp`, `P_stp`, `kw_water`.
- **Stoichiometry, Gas Laws, pH & Kinetics**: `moles`, `mass_from_moles`, `particles_from_moles`, `molarity`, `dilution_v1`, `molality`, `osmotic_pressure`, `ideal_gas_p`, `ideal_gas_v`, `ideal_gas_n`, `ideal_gas_t`, `rms_mol_speed`, `graham_rate_ratio`, `ph_from_h`, `h_from_ph`, `poh_from_oh`, `oh_from_poh`, `poh_from_ph`, `ph_from_poh`, `ph_from_oh`, `henderson_hasselbalch`, `arrhenius_k`, `half_life_1st`, `decay_rem`, `radiocarbon_age`, `gibbs_free_energy`, `gibbs_from_keq`, `nernst_cell`.

## 4. Orthogonal Entity Types & Concrete Unit Types (`x : real, distance := 50m`)

Every `Value` and variable slot carries two orthogonal tags:
1. **Entity Type (`type_id`)**: `real`, `iota`, `complex`, `vec2`, `vec3`, `mat2`, `vec`, `str` (queried via `type_of(x)` in `.math` or `eng_val_type` / `eng_type_name` in C).
2. **Unit Type (`unit_id`)**: `none`, `distance`, `angle`, `mass`, `time`, `speed`, `pressure`, `energy`, `power`, or any user-defined unit (queried via `unit_of(x)` in `.math` or `eng_val_unit` / `eng_unit_name` in C).

### Four Variable Declaration Modes

| Declaration Syntax | Entity Type Lock | Unit Type Lock | Allowed & Rejected Operations |
| :--- | :--- | :--- | :--- |
| `x := 5` or `x := 50m` | Dynamic (`T`) | Dynamic (`T`) | Can promote to `complex` (`x += 5i`) or change unit (`x = 50m`), while `+`/`-` still forbid mixing incompatible units (`100cm + 60°` is rejected). |
| `x : real := 5` | Concrete (`real`) | Dynamic (`T`) | Allows `x += 10` and `x = 50m`, but **rejects** automatic promotion to `complex` (`x += 5i` -> error). |
| `x : real, distance := 50m` | Concrete (`real`) | Concrete (`distance`) | Allows `x += 10cm` (`50.1m`) and `x *= 10` (`500m`), but **rejects** unlike units (`x += 60°`), unitless assignment (`x = 10`), and complex promotion (`x += 50im`). |
| `x : distance := 50m` | Dynamic (`T`) | Concrete (`distance`) | Locks only the unit to `distance`; `x += 50im` (where `50im` is `iota, distance`) promotes `x` to **`50 + 50i` (`complex, distance`)**, while `x += 60°` or `x += 5i` (`iota, none`) is rejected. |

### Unit-Typed Operators, Functions & Generic Unit `T`

```math
# Concrete unit-typed infix operators
(x:real, distance) "+" (y:real, distance) :<50 -> real, distance = @add(x, y)
(d:real, distance) "/" (t:real, time)     :<60 -> real, speed    = @div(d, t)

# Concrete unit-typed functions
"avg_speed"(d:real, distance, t:real, time) -> real, speed = d / t

# Generic UnitType `T` (preserves any unit `T`, including `"none"` on `2 + 5`)
"scale_unit"(x:real, T, k:real)   -> real, T = x * k
"unit_span"(a:real, T, b:real, T) -> real, T = @abs(a - b)
```
