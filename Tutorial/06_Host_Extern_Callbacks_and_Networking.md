# Chapter 6 — Host Extern Callbacks & Safe Internet/Storage Integration

This chapter explains how to connect `.math` functions and operators to live internet APIs (such as realtime currency conversion), databases, and hardware peripherals without adding any OS or network dependencies to `Core/`.

## 1. Registering Host C Callbacks (`eng_register_extern`)

Inside a `.math` file, any `@<name>(arg1, ...)` call that is not a built-in C math intrinsic compiles into an 8-byte `AST_EXTERN` node referencing the interned symbol ID of `<name>`. On the host side (`Test/main.c`), bind `@<name>` with:

```c
bool eng_register_extern(Engine *eng, const char *name,
                         EngExternFn fn, void *user_ctx);
```

## 2. Value Helper Functions for Host Callbacks (`mem_arena.h`)

- `double val_get_f64(const MemArena *mem, Value v)` — extract a `$real` or `$iota` float.
- `bool val_as_cstr(const MemArena *mem, Value v, char *buf, size_t cap)` — extract a null-terminated `$str`.
- `Value val_from_f64(MemArena *mem, double d)` — construct a `$real` return Value.
- `Value val_from_cstr(MemArena *mem, const char *cstr)` — construct a `$str` return Value.
- `Value val_from_f64_vec(MemArena *mem, const double *vals, uint16_t count)` — construct and return a `$vec` of `$real` numbers.

## 3. Demonstrated Host Externs in `Test/main.c` & `Test/user_physics.math`

1. **`@fx_rate(from, to)`**: Realtime currency conversion & USD cross-rate triangulation (`fx_rate("USD", "INR")`, `convert_fx(92, "EUR", "INR")`, `50 to_inr`).
2. **`@fx_history(from, to, days)`**: Returns a `$vec` of historical rates so `.math` statistical functions (`fx_sma("USD", "INR", 5, 3)`) can compute moving averages.
3. **`@kv_set(key, val)` & `@kv_get(key)`**: Persistent host key-value store across evaluations (`save_var("portfolio_usd", 150) ; (load_var("portfolio_usd") + 50) to_inr`).
4. **`@elem_mass(sym)`**: IUPAC Periodic Table atomic mass lookup (`molar_mass_h2o()`, `molar_mass_co2()`, `molar_mass_glucose()`).
