/*
 * main.c — Frontend Host Program Demonstrating:
 *   1. Internal zero-parse `eng_init()` auto-boot from embedded `stdlib_precompiled.h`
 *      (bootstrap, arith, trig, complex_ext, geom_ext, algebra, calculus, units,
 *       conversions, physics, chemistry, biology)
 *   2. Pure in-memory `user_loader.h` workflow (`user_compile_ext` & `user_load_mathc`)
 *      with zero file-extension or filesystem coupling in `Core/`
 *   3. Postfix `"i"` operator (`$iota`) & dimensional unit postfix operators (`deg`, `°`,
 *      `km`, `mph`, `atm`, `kcal`, `eV`, `dB`)
 *   4. Advanced Control Flow, Scopes, Booleans & Assignments:
 *      - Default newline `;` statement separator inside & outside `{ ... }` / `( ... )`
 *        for one-statement-per-line writing, while keeping `;` compulsory when placing
 *        multiple statements on a single line
 *      - `{ ... }` local scope blocks (value-returning `x = { ... }`, isolated `{ ... }`,
 *        and lexical variable isolation/shadowing)
 *      - Multi-branch `if (c1) { b1 } or (c2) { b2 } else { b3 }` (`"or"` for else-if)
 *      - Pattern-matched `switch (expr) { ... }` (values, ranges `1...5`, relational
 *        guards `case > 10:`, struct destructuring `case vec2(0, y):`, vector
 *        destructuring `case [0, y]:`, and `if <cond>` guards)
 *      - Standard right-associative ternary `cond ? x : y`
 *      - Colon-tagged scopes/loops `Tag : { ... }` and `Tag : (cond) ?? { ... }` with
 *        `jump(Tag)`, `run(Tag)`, `break`, `break(OuterTag)`, `break val`,
 *        `continue`, `continue(OuterTag)`, and `return val`
 *      - Keywordless `(i in 1...5) { ... }` / `(v in vec) { ... }` and `loop` constructs
 *      - Vector & matrix slicing (`v[1..3]`, `M[0..1, 1..2]`) & comprehensions
 *        (`[x^2 for x in 1...5]`, `[x^2 for x in 1...10 if x % 2 == 0]`)
 *      - First-class lambdas (`(x) => x^2`, `x => x * 2`, `(a, b) => a + b`, `() => 42`)
 *        and higher-order collection methods (`.map()`, `.filter()`, `.reduce()`,
 *        `.fold()`, `.any()`, `.all()`, `.find()`, `.count_if()`, `call()`, and `f(x)`)
 */

#include "math_engine.h"
#include "user_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Engine g_eng; /* Single static engine context */

/* ============================================================================
 * 1. HOST PLATFORM FILE I/O HELPERS (KEPT STRICTLY OUTSIDE `Core/`)
 * ============================================================================ */

static uint8_t *frontend_read_file(const char *path, size_t *out_len) {
    if (out_len) *out_len = 0;
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 0) { fclose(fp); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)sz + 1);
    if (!buf) { fclose(fp); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    buf[n] = '\0';
    fclose(fp);
    if (out_len) *out_len = n;
    return buf;
}

static bool frontend_write_file(const char *path, const uint8_t *data, size_t len) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;
    bool ok = (fwrite(data, 1, len, fp) == len);
    fclose(fp);
    return ok;
}

/* ============================================================================
 * 2. HOST EXTERN CALLBACK IMPLEMENTATIONS (`eng_register_extern`)
 * ============================================================================ */

typedef struct {
    const char *ccy;
    double      per_usd;
} FxQuote;

typedef struct {
    FxQuote *quotes;
    size_t   count;
} FxRateCache;

static FxQuote g_live_fx_quotes[] = {
    { "USD",   1.00 },
    { "EUR",   0.92 },
    { "GBP",   0.79 },
    { "INR",  84.00 },
    { "JPY", 150.00 }
};

static FxRateCache g_fx_cache = {
    g_live_fx_quotes,
    sizeof(g_live_fx_quotes) / sizeof(g_live_fx_quotes[0])
};

static bool lookup_usd_rate(const FxRateCache *cache, const char *ccy, double *out_per_usd) {
    for (size_t i = 0; i < cache->count; ++i) {
        if (strcmp(cache->quotes[i].ccy, ccy) == 0) {
            *out_per_usd = cache->quotes[i].per_usd;
            return true;
        }
    }
    return false;
}

static bool host_fx_rate_cb(MemArena *mem, const SymTable *tab,
                            const Value *args, uint8_t argc,
                            void *user_ctx, Value *out_val) {
    (void)tab;
    if (argc != 2) {
        mem_err(mem, "@fx_rate expects 2 currency string arguments (from, to)");
        return false;
    }
    char from_ccy[16], to_ccy[16];
    if (!val_as_cstr(mem, args[0], from_ccy, sizeof(from_ccy)) ||
        !val_as_cstr(mem, args[1], to_ccy,   sizeof(to_ccy))) {
        mem_err(mem, "@fx_rate arguments must be currency code strings");
        return false;
    }

    const FxRateCache *cache = (const FxRateCache *)user_ctx;
    double from_per_usd = 0.0, to_per_usd = 0.0;
    if (!lookup_usd_rate(cache, from_ccy, &from_per_usd) ||
        !lookup_usd_rate(cache, to_ccy,   &to_per_usd)) {
        mem_err(mem, "Unsupported currency pair '%s' -> '%s'", from_ccy, to_ccy);
        return false;
    }
    *out_val = val_from_f64(mem, to_per_usd / from_per_usd);
    return true;
}

static bool host_fx_set_rate_cb(MemArena *mem, const SymTable *tab,
                                const Value *args, uint8_t argc,
                                void *user_ctx, Value *out_val) {
    (void)tab;
    if (argc != 2) {
        mem_err(mem, "@fx_set_rate expects (ccy:str, per_usd:real)");
        return false;
    }
    char ccy[16];
    if (!val_as_cstr(mem, args[0], ccy, sizeof(ccy))) {
        mem_err(mem, "@fx_set_rate first argument must be a currency code string");
        return false;
    }
    double new_rate = val_get_f64(mem, args[1]);
    if (new_rate <= 0.0) {
        mem_err(mem, "@fx_set_rate rate must be positive (got %g)", new_rate);
        return false;
    }
    FxRateCache *cache = (FxRateCache *)user_ctx;
    for (size_t i = 0; i < cache->count; ++i) {
        if (strcmp(cache->quotes[i].ccy, ccy) == 0) {
            cache->quotes[i].per_usd = new_rate;
            *out_val = val_from_f64(mem, new_rate);
            return true;
        }
    }
    mem_err(mem, "Cannot update unknown currency %s", ccy);
    return false;
}

static bool host_fx_history_cb(MemArena *mem, const SymTable *tab,
                               const Value *args, uint8_t argc,
                               void *user_ctx, Value *out_val) {
    (void)tab;
    if (argc != 3) {
        mem_err(mem, "@fx_history expects (from:str, to:str, days:real)");
        return false;
    }
    char from_ccy[16], to_ccy[16];
    if (!val_as_cstr(mem, args[0], from_ccy, sizeof(from_ccy)) ||
        !val_as_cstr(mem, args[1], to_ccy,   sizeof(to_ccy))) {
        mem_err(mem, "@fx_history currency codes must be strings");
        return false;
    }
    int days = (int)val_get_f64(mem, args[2]);
    if (days <= 0 || days > 64) {
        mem_err(mem, "@fx_history days must be in 1..64 (got %d)", days);
        return false;
    }

    const FxRateCache *cache = (const FxRateCache *)user_ctx;
    double from_per_usd = 0.0, to_per_usd = 0.0;
    if (!lookup_usd_rate(cache, from_ccy, &from_per_usd) ||
        !lookup_usd_rate(cache, to_ccy,   &to_per_usd)) {
        mem_err(mem, "Unsupported currency pair '%s' -> '%s'", from_ccy, to_ccy);
        return false;
    }
    double base_rate = to_per_usd / from_per_usd;
    double series[64];
    for (int i = 0; i < days; ++i) {
        series[i] = base_rate - (double)(days - 1 - i) * 0.5;
    }
    *out_val = val_from_f64_vec(mem, series, (uint16_t)days);
    return !mem->has_error;
}

typedef struct {
    char   key[32];
    double val;
    bool   used;
} KvSlot;

typedef struct {
    KvSlot slots[16];
} HostKvStore;

static HostKvStore g_kv_store;

static bool host_kv_set_cb(MemArena *mem, const SymTable *tab,
                           const Value *args, uint8_t argc,
                           void *user_ctx, Value *out_val) {
    (void)tab;
    if (argc != 2) {
        mem_err(mem, "@kv_set expects (key:str, val:real)");
        return false;
    }
    char key[32];
    if (!val_as_cstr(mem, args[0], key, sizeof(key))) {
        mem_err(mem, "@kv_set first argument must be a string key");
        return false;
    }
    double v = val_get_f64(mem, args[1]);
    HostKvStore *store = (HostKvStore *)user_ctx;
    int free_idx = -1;
    for (int i = 0; i < 16; ++i) {
        if (store->slots[i].used && strcmp(store->slots[i].key, key) == 0) {
            store->slots[i].val = v;
            *out_val = val_from_f64(mem, v);
            return true;
        }
        if (!store->slots[i].used && free_idx < 0) free_idx = i;
    }
    if (free_idx < 0) {
        mem_err(mem, "Host KV store full");
        return false;
    }
    size_t klen = strlen(key);
    if (klen >= sizeof(store->slots[0].key)) klen = sizeof(store->slots[0].key) - 1;
    memcpy(store->slots[free_idx].key, key, klen);
    store->slots[free_idx].key[klen] = '\0';
    store->slots[free_idx].val  = v;
    store->slots[free_idx].used = true;
    *out_val = val_from_f64(mem, v);
    return true;
}

static bool host_kv_get_cb(MemArena *mem, const SymTable *tab,
                           const Value *args, uint8_t argc,
                           void *user_ctx, Value *out_val) {
    (void)tab;
    if (argc != 1) {
        mem_err(mem, "@kv_get expects (key:str)");
        return false;
    }
    char key[32];
    if (!val_as_cstr(mem, args[0], key, sizeof(key))) {
        mem_err(mem, "@kv_get argument must be a string key");
        return false;
    }
    const HostKvStore *store = (const HostKvStore *)user_ctx;
    for (int i = 0; i < 16; ++i) {
        if (store->slots[i].used && strcmp(store->slots[i].key, key) == 0) {
            *out_val = val_from_f64(mem, store->slots[i].val);
            return true;
        }
    }
    mem_err(mem, "Key '%s' not found in host KV store", key);
    return false;
}

typedef struct {
    const char *sym;
    double      atomic_mass;
} ElemEntry;

static const ElemEntry k_periodic_table[] = {
    { "H",   1.008 }, { "He",  4.0026}, { "Li",  6.94  }, { "Be",  9.0122},
    { "B",  10.81  }, { "C",  12.011 }, { "N",  14.007 }, { "O",  15.999 },
    { "F",  18.998 }, { "Ne", 20.180 }, { "Na", 22.990 }, { "Mg", 24.305 },
    { "Al", 26.982 }, { "Si", 28.085 }, { "P",  30.974 }, { "S",  32.06  },
    { "Cl", 35.45  }, { "K",  39.098 }, { "Ca", 40.078 }, { "Fe", 55.845 },
    { "Cu", 63.546 }, { "Zn", 65.38  }, { "Ag",107.868 }, { "Au",196.967 },
    { NULL,  0.0   }
};

static bool host_elem_mass_cb(MemArena *mem, const SymTable *tab,
                              const Value *args, uint8_t argc,
                              void *user_ctx, Value *out_val) {
    (void)tab;
    (void)user_ctx;
    if (argc != 1) {
        mem_err(mem, "@elem_mass expects 1 element symbol string");
        return false;
    }
    char sym[8];
    if (!val_as_cstr(mem, args[0], sym, sizeof(sym))) {
        mem_err(mem, "@elem_mass argument must be an element symbol string");
        return false;
    }
    for (int i = 0; k_periodic_table[i].sym != NULL; ++i) {
        if (strcmp(k_periodic_table[i].sym, sym) == 0) {
            *out_val = val_from_f64(mem, k_periodic_table[i].atomic_mass);
            return true;
        }
    }
    mem_err(mem, "Unknown chemical element symbol '%s'", sym);
    return false;
}

static void register_all_host_externs(Engine *eng) {
    eng_register_extern(eng, "fx_rate",     host_fx_rate_cb,     (void *)&g_fx_cache);
    eng_register_extern(eng, "fx_set_rate", host_fx_set_rate_cb, (void *)&g_fx_cache);
    eng_register_extern(eng, "fx_history",  host_fx_history_cb,  (void *)&g_fx_cache);
    eng_register_extern(eng, "kv_set",      host_kv_set_cb,      (void *)&g_kv_store);
    eng_register_extern(eng, "kv_get",      host_kv_get_cb,      (void *)&g_kv_store);
    eng_register_extern(eng, "elem_mass",   host_elem_mass_cb,   NULL);
}

/* ============================================================================
 * 3. AUTOMATED TEST SUITE
 * ============================================================================ */

typedef struct {
    const char *expr;
    const char *expected_val;
    const char *expected_type;
    const char *desc;
} TestCase;

static const TestCase k_tests[] = {
    /* A. Core Arithmetic, Combinatorics, Number Theory & Stats (`10_arith.math`) */
    { "2 + 3 * 4",             "14",         "real",    "Infix precedence (* over +)" },
    { "10 - 3 - 2",            "5",          "real",    "Left-associative binary subtraction '-'" },
    { "-10 - --3 minus 1",     "-13",        "real",    "Prefix '-', infix '-', prefix '--', word 'minus'" },
    { "2 ^ 3 ^ 2",             "512",        "real",    "Right-associative infix '^' (2^(3^2))" },
    { "√16 + 5!",              "124",        "real",    "Prefix '√' + postfix '!' (bootstrap ; := ?\? loop)" },
    { "++5",                   "6",          "real",    "Multi-char prefix '++'" },
    { "+++5",                  "7",          "real",    "Multi-char prefix '+++' (Maximal Munch)" },
    { "5++ + +++10",           "18",         "real",    "Combined postfix '++', infix '+', prefix '+++'" },
    { "10 plus 25",            "35",         "real",    "Word operator 'plus' (inlined passthrough to '+')" },
    { "abs_diff(4, 15)",       "11",         "real",    "Bootstrap lazy 'if(cond, then, else)' branch" },
    { "gcd(1071, 462)",        "21",         "real",    "Multi-instruction Euclidean GCD" },
    { "lcm(12, 18)",           "36",         "real",    "Least Common Multiple lcm(12, 18)" },
    { "ncr(10, 3) + npr(5, 2)","140",        "real",    "Combinatorics: 10C3 (120) + 5P2 (20)" },
    { "fib(10)",               "55",         "real",    "Iterative Fibonacci fib(10)" },
    { "is_prime(97) * 10 + is_prime(100)", "10", "real","Primality test is_prime(97)=1, is_prime(100)=0" },
    { "mod_pow(2, 10, 1000)",  "24",         "real",    "Fast modular exponentiation 2^10 mod 1000" },
    { "clamp(15, 0, 10)",      "10",         "real",    "Range clamping clamp(15, 0, 10)" },

    /* B. Trigonometric, Hyperbolic & Logarithmic Functions (`20_trig.math`) */
    { "sin(π / 2)",            "1",          "real",    "UTF-8 constant 'π' + trig intrinsic @sin" },
    { "ln(e ^ 4)",             "4",          "real",    "Constant 'e' + transcendental @ln" },
    { "log2(1024) + cbrt(27)", "13",         "real",    "Base-2 logarithm log2(1024) + cube root cbrt(27)" },
    { "cosh(0) + sinh(0)",     "1",          "real",    "Hyperbolic cosh(0) + sinh(0)" },

    /* C. Postfix `"i"` Operator (`$iota`) & Complex Arithmetic (`30_complex.math`) */
    { "1 + 1i",                "1 + 1i",     "complex", "Postfix 'i' on literal: real + iota -> complex" },
    { "(3 + 4)i",              "7i",         "iota",    "Postfix 'i' on parenthesized expression -> iota" },
    { "2 + sin(π / 2)i",       "2 + 1i",     "complex", "Postfix 'i' on function call sin(π/2)i" },
    { "((3 + 4)i)i",           "-7",         "real",    "Chained postfix 'i' on iota: (7i)i = -7 (i^2 = -1)" },
    { "(1 + 2i)i",             "-2 + 1i",    "complex", "Postfix 'i' on complex: 90-deg rotation in C-plane" },
    { "3i * 4i",               "-12",        "real",    "Imaginary * imaginary: 3i * 4i = -12" },
    { "(5 + 6i) - (2 + 4i)",   "3 + 2i",     "complex", "Overloaded complex - complex subtraction" },
    { "(1 + 2i) * (3 + 4i)",   "-5 + 10i",   "complex", "Overloaded complex * complex" },
    { "(-5 + 10i) / (3 + 4i)", "1 + 2i",     "complex", "Overloaded complex / complex division" },
    { "cabs(3 - 4i)",          "5",          "real",    "Complex magnitude on '3 - 4i'" },

    /* D. 2D/3D Vectors & Geometry (`40_geom.math`) */
    { "\"Euler Formula\"",     "\"Euler Formula\"", "str", "Consuming string type ($str)" },
    { "vec2(5, 7) - vec2(1, 2)", "vec2(x=4, y=5)", "vec2", "C-style struct subtraction overload" },
    { "dot(vec2(3, 4), vec2(2, 1))", "10",   "real",    "Struct field access via bootstrap '.' (@field)" },
    { "move_circle(circle(vec2(1, 2), 5), 10, 20)",
      "circle(center=vec2(x=11, y=22), radius=5)", "circle",
      "Nested struct field mutation via bootstrap '.', '=', and ';'" },
    { "circle(vec2(1, 2), 5).center.y", "2", "real",    "Chained compile-time nested struct offset read" },
    { "vec3(1, 2, 3) ** vec3(4, 5, 6)", "32", "real",   "$vec3 '**' 3D dot product" },
    { "cross3(vec3(1, 0, 0), vec3(0, 1, 0))", "vec3(x=0, y=0, z=1)", "vec3", "3D vector cross product i x j = k" },
    { "vec3_mag(vec3(3, 4, 12))", "13",      "real",    "3D Euclidean norm hypot(hypot(3,4),12)" },
    { "lerp(10, 20, 0.25)",    "12.5",       "real",    "Linear interpolation lerp(10, 20, 0.25)" },

    /* E. Algebra: Matrices, Linear Systems, Quadratics & Polynomials (`50_algebra.math`) */
    { "det(mat2(4, 7, 2, 6))", "10",         "real",    "2x2 matrix determinant (4*6 - 7*2)" },
    { "inv_mat2(mat2(4, 7, 2, 6)) * vec2(10, 10)", "vec2(x=-1, y=2)", "vec2", "Matrix inverse * vector solution" },
    { "solve_2x2(2, 3, 8, 5, -1, 3)", "vec2(x=1, y=2)", "vec2", "Cramer's rule 2x2 system: 2x+3y=8, 5x-y=3" },
    { "quad_roots(1, -5, 6)",  "vec2(x=3, y=2)", "vec2","Real roots of x^2 - 5x + 6 = 0 -> (3, 2)" },
    { "quad_croot(1, 2, 5)",   "-1 + 2i",    "complex", "Complex root of x^2 + 2x + 5 = 0 -> -1 + 2i" },
    { "p := vec() ; push(p, 6) ; push(p, -5) ; push(p, 1) ; poly_val(p, 4)", "2", "real",
      "Horner polynomial evaluation P(x)=x^2-5x+6 at x=4 -> 2" },
    { "p := vec() ; push(p, 6) ; push(p, -5) ; push(p, 1) ; poly_deriv(p)", "[-5, 2]", "vec",
      "Symbolic polynomial derivative d/dx(6 - 5x + x^2) -> [-5, 2]" },
    { "p := vec() ; push(p, -2) ; push(p, 1) ; q := vec() ; push(q, -3) ; push(q, 1) ; poly_mul(p, q)",
      "[6, -5, 1]", "vec",
      "Symbolic polynomial product (x - 2)*(x - 3) -> 6 - 5x + x^2" },
    { "p := vec() ; push(p, -9) ; push(p, 0) ; push(p, 1) ; poly_newton(p, 5, 8)", "3", "real",
      "Newton-Raphson algebraic root finder for x^2 - 9 = 0 -> 3" },

    /* F. Dimensional Unit Conversions & Postfix Suffixes (`60_units.math`) */
    { "sin(30 deg) + cos(60°)", "1",         "real",    "Postfix angle operators '30 deg' & '60°'" },
    { "20 dB",                 "10",         "real",    "Postfix decibel amplitude ratio 20 dB -> 10x" },
    { "1.5 km + 25000 cm + 500 mm", "1750.5","real",    "Postfix length suffixes (km + cm + mm -> meters)" },
    { "m_to_ft(304.8)",        "1000",       "real",    "Meters to feet conversion m_to_ft(304.8)" },
    { "108 kph",               "30",         "real",    "Postfix speed suffix 108 kph -> 30 m/s" },
    { "2 atm + 1 bar",         "302650",     "real",    "Postfix pressure suffixes (2 atm + 1 bar -> Pa)" },
    { "1 kcal + 1 kJ",         "5184",       "real",    "Postfix energy suffixes (1 kcal + 1 kJ -> Joules)" },
    { "c_to_f(100) + f_to_c(32) + c_to_k(0)", "485.15", "real", "Temperature conversions (212 + 0 + 273.15)" },

    /* G. Classical, Relativistic, Circuit & Quantum Physics (`70_physics.math`) */
    { "ke(2 kg, 36 kph)",      "100",        "real",    "Kinetic energy of 2 kg at 36 kph (10 m/s) -> 100 J" },
    { "mech_energy(2, 10, 5)", "198.0665",   "real",    "Total mechanical energy ke(2,10) + pe(2,5)" },
    { "proj_range(9.80665, 45)", "9.80665",  "real",    "Projectile range at 45 deg" },
    { "res_parallel(6, 3) + res_series(4, 4)", "10", "real", "Parallel (6||3 = 2) + series (4+4 = 8) resistors" },
    { "lorentz_gamma(0.6 * c0)", "1.25",     "real",    "Relativistic Lorentz factor gamma at v = 0.6c" },
    { "carnot_eff(300, 600)",  "0.5",        "real",    "Carnot heat engine efficiency (1 - 300/600)" },

    /* H. Chemistry: Gas Laws, Acid-Base pH, Kinetics & Decay (`80_chemistry.math`) */
    { "ph_from_h(0.001) + poh_from_ph(3)", "14", "real","Acid-base pH(10^-3 M) = 3, pOH = 11 -> sum 14" },
    { "henderson_hasselbalch(4.76, 0.2, 0.2)", "4.76", "real", "Buffer pH when [A-] == [HA] equals pKa" },
    { "decay_rem(100, 15, 5)", "12.5",       "real",    "Radioactive decay after 3 half-lives (100 -> 12.5)" },
    { "graham_rate_ratio(4, 16)", "2",       "real",    "Graham effusion ratio sqrt(16 / 4) = 2" },

    /* I. Dynamic Vector (`$vec` of any `T` type) & Statistical Suite */
    { "v := vec() ; push(v, 10) ; push(v, 20) ; push(v, 30) ; v",
      "[10, 20, 30]", "vec",
      "Growable vec<real>: vec(), push(v,x)" },
    { "v := vec(2) ; push(v, 5) ; push(v, 15) ; push(v, 25) ; cap(v)",
      "4", "real",
      "Auto-doubling capacity (cap 2 -> 4 on 3rd push)" },
    { "v := vec(3, 7) ; set(v, 1, 99) ; pop(v) + at(v, 1) + len(v)",
      "108", "real",
      "vec(cnt,init), set(v,i,x), pop(v) -> 7 + 99 + 2" },
    { "pts := vec() ; push(pts, vec2(1, 2)) ; push(pts, vec2(3, 4)) ; at(pts, -1)",
      "vec2(x=3, y=4)", "vec2",
      "Generic vec<vec2> + negative index at(pts, -1)" },
    { "cvec := vec(2, 1 + 2i) ; set(cvec, 1, 3 - 4i) ; cvec",
      "[1 + 2i, 3 - 4i]", "vec",
      "Generic vec<complex> fill & indexed mutation" },
    { "v := vec() ; push(v, 2) ; push(v, 4) ; push(v, 4) ; push(v, 4) ; push(v, 5) ; push(v, 5) ; push(v, 7) ; push(v, 9) ; stddev(v)",
      "2", "real",
      "Standard deviation of [2,4,4,4,5,5,7,9] -> 2" },
    { "p := vec() ; push(p, vec3(0,0,0)) ; push(p, vec3(3,4,0)) ; push(p, vec3(3,4,12)) ; vec3_path_len(p)",
      "17", "real",
      "3D trajectory path length over vec<vec3> (5 + 12)" },

    /* J. Host-Registered Extern Callbacks (`@fx_rate`, `@fx_set_rate`, `@fx_history`, `@kv_set`/`@kv_get`, `@elem_mass`) */
    { "fx_rate(\"USD\", \"INR\")",         "84",   "real", "Host extern '@fx_rate(\"USD\", \"INR\")'" },
    { "convert_fx(250, \"USD\", \"EUR\")", "230",  "real", "User 'convert_fx(250, USD, EUR)'" },
    { "convert_fx(92, \"EUR\", \"INR\")",  "8400", "real", "Cross-currency triangulation EUR -> USD -> INR" },
    { "50 to_inr + (92 to_usd) to_inr",    "12600","real", "Postfix currency ops 'to_inr' and 'to_usd'" },
    { "fx_with_fee(100, \"USD\", \"INR\", 1.5)", "8274", "real",
      "Multi-instruction 'fx_with_fee(100, USD, INR, 1.5%)' -> 8274" },
    { "fx_set_rate(\"INR\", 86.5) ; r := 100 to_inr ; fx_set_rate(\"INR\", 84) ; r", "8650", "real",
      "Realtime rate tick via '@fx_set_rate' updates 100 to_inr -> 8650 live" },
    { "p := vec() ; push(p, 10) ; push(p, 25) ; push(p, 15) ; vec_sum_fx(p, \"INR\")", "4200", "real",
      "USD price vector sum converted to INR via 'vec_sum_fx(p, INR)' -> 4200" },
    { "fx_history(\"USD\", \"INR\", 5)",   "[82, 82.5, 83, 83.5, 84]", "vec",
      "Vector-returning host extern '@fx_history' -> vec<real>" },
    { "fx_sma(\"USD\", \"INR\", 5, 3)",    "83.5", "real",
      "3-day Simple Moving Average over host '@fx_history' vector" },
    { "save_var(\"portfolio_usd\", 150) ; (load_var(\"portfolio_usd\") + 50) to_inr",
      "16800", "real",
      "Host persistent KV store '@kv_set' & '@kv_get' + 'to_inr'" },
    { "molar_mass_h2o() + molar_mass_co2()", "62.024", "real",
      "Host '@elem_mass' Periodic Table: H2O (18.015) + CO2 (44.009)" },

    /* K. Local Scopes `{ ... }`, `if ... or ... else`, `switch`, `? :` Ternary & `Tag : { ... }` */
    { "x = { a := 3 ; b := 4 ; a^2 + b^2 } ; { x = x + 5 ; 9999 } ; x",
      "30", "real",
      "Value-returning 'x = { ... }' + isolated '{ ... ; 9999 }' discarding unused value" },
    { "x := 10 ; y := { x := 99 ; tmp := 5 ; x + tmp } ; x * 1000 + y",
      "10104", "real",
      "Lexical scope shadowing: inner 'x := 99' leaves outer 'x = 10' intact" },
    { "{}",
      "<void>", "void",
      "Empty '{}' local scope block with no return value (<void>)" },
    { "classify_mach(0.5)",    "\"subsonic\"",   "str",  "if/or/else chain: 1st 'if' branch matched" },
    { "classify_mach(1.0)",    "\"transonic\"",  "str",  "if/or/else chain: 1st 'or' (else-if) matched" },
    { "classify_mach(2.5)",    "\"supersonic\"", "str",  "if/or/else chain: 2nd 'or' (else-if) matched" },
    { "classify_mach(6.0)",    "\"hypersonic\"", "str",  "if/or/else chain: final 'else' branch matched" },
    { "cnt := 0 ; r := if (0) { cnt = cnt + 100 ; 1 } or (1) { cnt = cnt + 10 ; 2 } or (1) { cnt = cnt + 1000 ; 3 } else { cnt = cnt + 5000 ; 4 } ; r * 100 + cnt",
      "210", "real",
      "Single-branch evaluation guarantee: only matching 'or' is evaluated" },
    { "planet_g(3) + planet_g(4) + planet_g(99)",
      "13.52765", "real",
      "switch block on numeric cases (Earth 9.80665 + Mars 3.721 + else 0)" },
    { "switch (\"Au\") { case \"Ag\", \"Cu\" : { 10 } case \"Au\" : { 79 } else : { 0 } }",
      "79", "real",
      "switch block on string cases with multi-value 'case \"Ag\", \"Cu\"'" },
    { "(10 > 5) ? 42 : 99",
      "42", "real",
      "Standard ternary operator 'cond ? x : y'" },
    { "s := 85 ; (s >= 90) ? 4 : (s >= 80) ? 3 : (s >= 70) ? 2 : 0",
      "3", "real",
      "Right-associative chained ternary 'c1 ? v1 : c2 ? v2 : c3 ? v3 : v4'" },
    { "k := 1 ; (k == 1) ? (k = k + 10) : (k = k + 1000) ; k",
      "11", "real",
      "Ternary lazy evaluation (untaken ':' branch is not evaluated)" },
    { "i := 0 ; x = Loop : { i = i + 1 ; if (i < 5) { jump(Loop) } ; i * 10 } ; x",
      "50", "real",
      "Assigned tagged scope 'x = Loop : { ... }' with self-jump" },
    { "x := 1 ; StepA : { x = x + 2 ; if (x < 10) { jump(StepB) } } ; StepB : { x = x * 2 ; if (x < 20) { jump(StepA) } } ; x",
      "36", "real",
      "Jumping forward & backward between 'StepA : { ... }' and 'StepB : { ... }'" },
    { "x := 5 ; if (x > 0) { jump(Done) } ; x = 999 ; Done : { x = x + 100 } ; x",
      "105", "real",
      "Forward 'jump(Done)' skipping intermediate assignment 'x = 999'" },
    { "collatz_steps(6)",
      "8", "real",
      "Tagged scopes 'Tick', 'Loop', 'Finish' with 'run(Tick)', 'Tick()', & 'jump'" },

    /* L. Loop `break` & Tagged `break(Tag)`, Chained/Distributed Booleans,
          Multi-Variable Declarations `x, y := 5`, Compound `+=`, and Dynamic Complex Promotion */
    { "s := 0 ; i := 1 ; (i <= 10) ?? { if (i == 5) { break } ; s += i ; i += 1 } ; s",
      "10", "real",
      "Unlabeled 'break' exiting while loop at i == 5 (1+2+3+4 = 10)" },
    { "tot := 0 ; Outer : { i := 1 ; (i <= 5) ?? { j := 1 ; (j <= 5) ?? { if (i == 2 and j == 3) { break(Outer) } ; tot += 10 ; j += 1 } ; i += 1 } ; tot = 9999 } ; tot",
      "70", "real",
      "Tagged 'break(Outer)' breaking out of nested inner & outer loops (7 * 10 = 70)" },
    { "tot := 0 ; i := 1 ; OuterLoop : (i <= 5) ?? { j := 1 ; (j <= 5) ?? { if (i == 2 and j == 2) { break(OuterLoop) } ; tot += 1 ; j += 1 } ; i += 1 } ; tot",
      "6", "real",
      "Directly tagged while loop 'OuterLoop : (i <= 5) ?? { ... break(OuterLoop) }'" },
    { "x := 1 ; y := 9 ; z := 3 ; (x == 1 && y == 2 || z == 3)",
      "1", "real",
      "Chained boolean precedence '(x == 1 && y == 2 || z == 3)' -> 1" },
    { "(1 and not 0) * 100 + (0 nor 0) * 10 + (1 xor 1) + (1 nand 1)",
      "110", "real",
      "Word boolean operators: 'and', 'not', 'nor', 'xor', 'nand'" },
    { "x := 1 ; y := 2 ; (x & y == 1 | 2)",
      "1", "real",
      "Distributed comparison '(x & y == 1 | 2)' when x=1, y=2 -> 1 (true)" },
    { "x := 1 ; y := 3 ; (x & y == 1 | 2)",
      "0", "real",
      "Distributed comparison '(x & y == 1 | 2)' when x=1, y=3 -> 0 (false)" },
    { "x := 2 ; y := 1 ; z := 4 ; ((x & y == 1 | 2) && (z == 3 | 4))",
      "1", "real",
      "Parenthesized chained distributed comparisons '((x & y == 1 | 2) && (z == 3 | 4))'" },
    { "x, y := 5 ; x * 10 + y",
      "55", "real",
      "Multi-variable declaration 'x, y := 5' (both x and y initialized to 5)" },
    { "a, b, c := 7 ; a + b + c",
      "21", "real",
      "3-variable declaration 'a, b, c := 7' -> 21" },
    { "x := 5 ; x += 1 ; x == 6",
      "1", "real",
      "Compound addition assignment 'x := 5 ; x += 1 ; x == 6' -> 1" },
    { "x := 10 ; x += 5 ; x -= 3 ; x *= 2 ; x /= 4 ; x %= 4 ; x ^= 3 ; x",
      "8", "real",
      "Chained compound assignments '+=', '-=', '*=', '/=', '%=', '^=' -> 8" },
    { "x, y := 5 ; x, y += 3 ; x * 10 + y",
      "88", "real",
      "Multi-variable compound assignment 'x, y := 5 ; x, y += 3' -> 88" },
    { "x := 5 ; x = x + 5i ; x",
      "5 + 5i", "complex",
      "Dynamic slot promotion: 'x := 5 ; x = x + 5i' promotes x from real to complex" },
    { "x := 5 ; x += 5i ; x.re * 10 + x.im",
      "55", "real",
      "Dynamic promotion via 'x += 5i' + struct field access 'x.re' and 'x.im'" },

    /* M. Concrete Entity Types, Concrete Unit Types, Complex Unit Promotion & Generic Unit `T` */
    { "x := 5m ; x *= 10m ; x += 10cm ; x",
      "50.1", "real",
      "Postfix unit arithmetic: 5m * 10m + 10cm -> 50.1m (5010cm) in SI meters" },
    { "x := 5m ; x *= 10 ; x += 10cm ; x",
      "50.1", "real",
      "Postfix unit scaled by plain 10: 5m * 10 + 10cm -> 50.1m" },
    { "x := 5m ; x *= 10m ; x += 10cm ; unit_of(x)",
      "\"distance\"", "str",
      "Unit tag preserved across *= and += on 5m -> \"distance\"" },
    { "unit_of(2 + 5)",
      "\"none\"", "str",
      "Simple expression '2 + 5' has generic UnitType T bound to \"none\"" },
    { "x : real := 5 ; x += 10 ; x = 50m ; x",
      "50", "real",
      "Concrete 'x : real := 5' allows real operations & unit changes while type stays real" },
    { "x : real, distance := 50m ; x += 10cm ; x *= 10 ; x",
      "501", "real",
      "Concrete 'x : real, distance := 50m' allows like-unit '10cm' and scalar '*= 10'" },
    { "x : distance := 50m ; x += 50im ; x",
      "50 + 50i", "complex",
      "Unit-locked 'x : distance := 50m' + '50im' promotes entity type to complex (50 + 50i)" },
    { "x : distance := 50m ; x += 50im ; unit_of(x)",
      "\"distance\"", "str",
      "Promoted 'x : distance := 50m ; x += 50im' retains concrete unit \"distance\"" },
    { "x : distance := 50m ; x += 50im ; type_of(x)",
      "\"complex\"", "str",
      "Promoted 'x : distance := 50m ; x += 50im' has entity type \"complex\"" },
    { "x : distance := 50m ; x += 50im ; x += 10cm ; x += 20icm ; x",
      "50.1 + 50.2i", "complex",
      "Complex distance variable accumulates both real '10cm' and imaginary '20icm'" },
    { "x, y : real, distance := 25m ; x += 50cm ; y += 150cm ; x + y",
      "52", "real",
      "Multi-variable concrete 'x, y : real, distance := 25m' declaration & addition" },
    { "100m / 10sec",
      "10", "real",
      "Cross-unit operator '(d:real,distance) / (t:real,time) -> real,speed'" },
    { "unit_of(100m / 10sec)",
      "\"speed\"", "str",
      "100m / 10sec produces concrete unit \"speed\"" },
    { "avg_speed(1.5 km, 50 sec)",
      "30", "real",
      "Unit-typed function 'avg_speed(d:real,distance, t:real,time) -> real,speed'" },
    { "unit_of(avg_speed(1.5 km, 50 sec))",
      "\"speed\"", "str",
      "avg_speed(1.5 km, 50 sec) returns unit \"speed\"" },
    { "scale_unit(50m, 3)",
      "150", "real",
      "Generic UnitType T function 'scale_unit(x:real,T, k:real) -> real,T'" },
    { "unit_of(scale_unit(50m, 3))",
      "\"distance\"", "str",
      "Generic UnitType T function preserves input unit \"distance\"" },
    { "unit_span(100m, 2500cm)",
      "75", "real",
      "Generic UnitType T 'unit_span(a:real,T, b:real,T)' on distance (100m, 2500cm)" },
    { "unit_span(10, 25)",
      "15", "real",
      "Generic UnitType T 'unit_span(a:real,T, b:real,T)' on unitless numbers (10, 25)" },
    { "2 MB + 500 KB", "2609152", "real",
      "User-defined custom unit group 'data_size' (2 MB + 500 KB in bytes)" },
    { "(10 MB) / (2 sec)", "5242880", "real",
      "User-defined cross-group operator: data_size / time -> bandwidth" },
    { "unit_of((10 MB) / (2 sec))", "\"bandwidth\"", "str",
      "Introspect custom user unit group via unit_of((10 MB) / (2 sec))" },
    { "buf : real, data_size := 1 MB ; buf += 512 KB ; buf", "1572864", "real",
      "Variable locked to custom user unit group 'real, data_size'" },

    /* 11. Prefix '!' Logical Negation Overload & Approximate Equality '~' Operator Tests */
    { "!0 * 10 + !1", "10", "real",
      "Prefix '!' operator overload on 0/1: !0 = 1, !1 = 0 -> 10" },
    { "x := 5 ; !(x == 5) * 10 + !(x == 4)", "1", "real",
      "Prefix '!' flips boolean conditions: !(x == 5) is 0, !(x == 4) is 1" },
    { "x := 3 ; (x == !(1 | 2))", "1", "real",
      "Distributed '!' in comparison: (x == !(1 | 2)) -> !((x==1) | (x==2)) = 1" },
    { "x := 2 ; (x == !(1 | 2))", "0", "real",
      "Distributed '!' in comparison when x=2 -> 0 (false)" },
    { "x := 1 ; y := 3 ; (x & !y == 1 | 2)", "1", "real",
      "Combined distributed '&', '|', and '!': (x & !y == 1 | 2) -> 1" },
    { "x := 4 ; (x == !4)", "0", "real",
      "Single distributed '!': (x == !4) -> !(x == 4) = 0" },
    { "!is_prime(100) * 10 + !is_even(5)", "11", "real",
      "Prefix '!' on boolean predicate functions !is_prime(100) and !is_even(5)" },
    { "5.001 ~ 5", "1", "real",
      "Approximate operator '~': 5.001 ~ 5 is true (|diff| = 0.001 <= 0.001)" },
    { "5.000001 ~ 5", "1", "real",
      "Approximate operator '~': 5.000001 ~ 5 is true (|diff| = 1e-6 <= 0.001)" },
    { "4.999 ~ 5", "1", "real",
      "Approximate operator '~': 4.999 ~ 5 is true (|diff| = 0.001 <= 0.001)" },
    { "5.01 ~ 5", "0", "real",
      "Approximate operator '~': 5.01 ~ 5 is false (|diff| = 0.01 > 0.001)" },
    { "5.002 ~ 5", "0", "real",
      "Approximate operator '~': 5.002 ~ 5 is false (|diff| = 0.002 > 0.001)" },
    { "!(5.01 ~ 5)", "1", "real",
      "Prefix '!' flipping approximate condition: !(5.01 ~ 5) -> 1 (true)" },

    /* 12. Standard Same-Unit-Type Conversions (`Library/65_conversions.math`) */
    { "(2.5 km) to_m", "2500", "real",
      "Postfix same-unit conversion: (2.5 km) to_m -> 2500" },
    { "(1.5 m) to_nm", "1500000000", "real",
      "Postfix same-unit conversion: (1.5 m) to_nm -> 1.5e+09" },
    { "(180 deg) to_rad", "3.141592654", "real",
      "Postfix angle conversion: (180 deg) to_rad -> pi" },
    { "(π rad) to_deg", "180", "real",
      "Postfix angle conversion: (pi rad) to_deg -> 180" },
    { "(2.5 km) in (1 m)", "2500", "real",
      "Universal infix 'in' same-unit conversion: (2.5 km) in (1 m) -> 2500" },
    { "(1.5 m) in (1 nm)", "1500000000", "real",
      "Universal infix 'in' same-unit conversion: (1.5 m) in (1 nm) -> 1.5e+09" },
    { "km_to_m(3.2) + m_to_cm(0.8)", "3280", "real",
      "Pairwise same-unit conversion functions km_to_m(3.2) + m_to_cm(0.8)" },
    { "rad_to_deg(deg_to_rad(135))", "135", "real",
      "Round-trip angle conversion rad_to_deg(deg_to_rad(135)) -> 135" },
    { "(2 hr) to_sec + (5000 ms) to_sec", "7205", "real",
      "Postfix time conversions: (2 hr) to_sec + (5000 ms) to_sec -> 7205" },

    /* 13. C-Style Array Notation `v[i]` for Vectors (`$vec2`, `$vec3`, Structs, `$vec`) */
    { "v := vec2(10, 20) ; v[0] * 100 + v[1]", "1020", "real",
      "C-style array indexing on $vec2: v[0]=v.x, v[1]=v.y" },
    { "v := vec3(2, 4, 6) ; v[0] = 10 ; v[1] += 5 ; v[2] *= 3 ; v", "vec3(x=10, y=9, z=18)", "vec3",
      "C-style array mutation & compound assignment on $vec3: v[0]=10, v[1]+=5, v[2]*=3" },
    { "c := circle(vec2(3, 7), 5) ; c[0][1] = 99 ; c[1] += 10 ; c", "circle(center=vec2(x=3, y=99), radius=15)", "circle",
      "Chained C-style array indexing on nested struct $circle: c[0][1] = 99" },
    { "a := vec(3, 10) ; a[0] = 5 ; a[1] += 7 ; a[-1] = 30 ; a[0] + a[1] + a[2]", "52", "real",
      "C-style array read/write/compound/negative index on dynamic $vec (5 + 17 + 30)" },
    { "pts := vec() ; push(pts, vec2(1, 2)) ; push(pts, vec2(3, 4)) ; pts[1][0] += 10 ; pts[1]", "vec2(x=13, y=4)", "vec2",
      "Chained C-style indexing pts[1][0] += 10 on dynamic vec<vec2>" },

    /* 14. Separate Multidimensional Matrix Data Type (`$mat`) Using `[...]` */
    { "M := [[1, 2, 3], [4, 5, 6]] ; M", "[[1, 2, 3], [4, 5, 6]]", "mat",
      "2D multidimensional matrix literal [[1, 2, 3], [4, 5, 6]] -> $mat" },
    { "M := [1, 2, 3 ; 4, 5, 6] ; M", "[[1, 2, 3], [4, 5, 6]]", "mat",
      "2D semicolon row-separated matrix literal [1, 2, 3 ; 4, 5, 6] -> $mat" },
    { "M := [10, 20 ; 30, 40] ; M[0, 1] + M[1][0]", "50", "real",
      "Matrix indexing via both multi-index M[0, 1] and chained M[1][0]" },
    { "M := [[1, 2], [3, 4]] ; M[0, 1] = 9 ; M[1][0] += 10 ; M", "[[1, 9], [13, 4]]", "mat",
      "In-place matrix element mutation via M[0, 1] = 9 and M[1][0] += 10" },
    { "A := [1, 2 ; 3, 4] ; B := [2, 0 ; 1, 2] ; A * B", "[[4, 4], [10, 8]]", "mat",
      "2D matrix multiplication A * B on $mat" },
    { "M := [4, 7 ; 2, 6] ; det(M) * 100 + trace(M)", "1010", "real",
      "det(M) and trace(M) on $mat (det=10, trace=10)" },
    { "M := [4, 7 ; 2, 6] ; inv(M) * [10, 10]", "[-1, 2]", "mat",
      "Gauss-Jordan matrix inverse inv(M) * 1D vector [10, 10] -> [-1, 2]" },
    { "M := [1, 2, 3 ; 4, 5, 6] ; transpose(M)", "[[1, 4], [2, 5], [3, 6]]", "mat",
      "Matrix transpose of 2x3 $mat -> 3x2 $mat" },
    { "T := [[[1, 2], [3, 4]], [[5, 6], [7, 8]]] ; T[1][0][1] += 10 ; T[1, 0, 1] + mat_rank(T)", "19", "real",
      "3D tensor $mat indexing, mutation T[1][0][1] += 10 (16), and mat_rank(T)=3 -> 19" },
    { "M := [[[1, 2], [3, 4]], [[5, 6], [7, 8]]] ; mat_shape(M)", "[2, 2, 2]", "vec",
      "3D tensor shape introspection mat_shape(M) -> [2, 2, 2]" },

    /* 15. High School Calculus Suite (`00_bootstrap.math` & `55_calculus.math`) */
    { "diff(x, x^3 + 2*x, 3)", "29", "real",
      "First derivative d/dx(x^3 + 2x) at x = 3 -> 29" },
    { "diff2(x, x^4, 2)", "48", "real",
      "Second derivative d^2/dx^2(x^4) at x = 2 -> 48" },
    { "diff_n(x, x^4, 2, 3)", "48", "real",
      "Third derivative d^3/dx^3(x^4) at x = 2 -> 48" },
    { "integrate(x, 3 * (x^2), 0, 2)", "8", "real",
      "Definite integral int_0^2 3x^2 dx -> 8" },
    { "ntg(x, sin(x), 0, π) ~ 2", "1", "real",
      "Definite integral int_0^pi sin(x) dx ~ 2" },
    { "lim(x, sin(x) / x, 0)", "1", "real",
      "Removable singularity limit: lim_{x->0} sin(x)/x -> 1" },
    { "lim(x, (x^2 - 4) / (x - 2), 2)", "4", "real",
      "Algebraic limit: lim_{x->2} (x^2 - 4)/(x - 2) -> 4" },
    { "lim_right(x, abs(x) / x, 0) * 10 + lim_left(x, abs(x) / x, 0)", "9", "real",
      "One-sided limits at 0 for |x|/x: right (+1)*10 + left (-1) = 9" },
    { "sigma(k, k^2, 1, 5)", "55", "real",
      "Discrete summation sigma_{k=1..5} k^2 -> 55" },
    { "prod(k, k, 1, 5)", "120", "real",
      "Discrete product prod_{k=1..5} k -> 120" },
    { "c := vec() ; push(c, 1) ; push(c, -3) ; push(c, 2) ; poly_def_integ(c, 0, 3)", "7.5", "real",
      "Exact polynomial definite integral int_0^3 (1 - 3x + 2x^2) dx -> 7.5" },
    { "c := vec() ; push(c, 0) ; push(c, 0) ; push(c, 1) ; tangent_slope(c, 3) * 10 + normal_slope(c, 3) * -6", "61", "real",
      "Tangent slope (6) & normal slope (-1/6) of y = x^2 at x = 3" },
    { "taylor_exp(1, 12) ~ e", "1", "real",
      "Maclaurin series taylor_exp(1, 12) ~ e" },

    /* 16. Expanded Commercial & General Arithmetic (`10_arith.math`) */
    { "pct_of(45, 180) + pct_val(15, 200)", "55", "real",
      "Percentages: pct_of(45, 180)=25% + pct_val(15, 200)=30 -> 55" },
    { "pct_inc(500, 20) + pct_dec(500, 20)", "1000", "real",
      "Percentage increase (600) + decrease (400) -> 1000" },
    { "profit_pct(400, 500) + successive_discount(20, 10)", "53", "real",
      "Profit % (25%) + successive discount (28%) -> 53" },
    { "simple_interest(1000, 10, 2) + compound_interest(1000, 10, 2)", "410", "real",
      "Simple interest (200) + compound interest (210) -> 410" },
    { "ci_si_diff_2yr(5000, 10)", "50", "real",
      "2-year CI - SI difference on 5000 at 10% -> 50" },
    { "work_together_2(6, 12) + work_together_3(6, 12, 4)", "6", "real",
      "Work & time: 2-person (4 days) + 3-person (2 days) -> 6" },
    { "pipe_fill_with_leak(4, 12) + man_days_solve(10, 6, 1, 15, 2)", "14", "real",
      "Pipe cistern with leak (6 hr) + chain rule man-days (8 days) -> 14" },
    { "ap_sum(2, 3, 10) + gp_sum(2, 3, 4)", "235", "real",
      "Arithmetic progression sum (155) + geometric progression sum (80) -> 235" },
    { "arith_mean(4, 16) + geom_mean(4, 16) + harm_mean(4, 16)", "24.4", "real",
      "AM (10) + GM (8) + HM (6.4) of 4 and 16 -> 24.4" },
    { "avg_speed_2(40, 60) + boat_still_speed(15, 9) + stream_speed(15, 9)", "63", "real",
      "Harmonic average speed (48) + boat in still water (12) + stream (3) -> 63" },

    /* 17. Expanded High School Physics, Chemistry & Quantitative Biology */
    { "suvat_v(10, 2, 5) + suvat_s(10, 2, 5) + suvat_nth_sec(10, 2, 5)", "114", "real",
      "SUVAT kinematics: v(20) + s(75) + s_5th(19) -> 114" },
    { "elastic_v1(2, 10, 2, 0) + elastic_v2(2, 10, 2, 0)", "10", "real",
      "1D elastic collision velocity exchange -> 0 + 10 = 10" },
    { "shm_vel(2, 5, 3) + abs(shm_acc(2, 3))", "20", "real",
      "SHM velocity (8) + acceleration magnitude (12) -> 20" },
    { "mirror_image_v(10, 30) + lens_power_dioptre(0.25)", "19", "real",
      "Ray optics mirror image distance (15) + lens power (4 D) -> 19" },
    { "rlc_impedance(30, 60, 20) + transformer_v2(220, 100, 50)", "160", "real",
      "AC RLC impedance (50 ohm) + step-down transformer (110 V) -> 160" },
    { "bohr_energy_ev(2, 1)", "-3.4", "real",
      "Bohr hydrogen n=2 energy level -> -3.4 eV" },
    { "percent_yield(42, 50) + normality_from_molarity(1.5, 2)", "87", "real",
      "Chemistry % yield (84%) + normality (3 N) -> 87" },
    { "boyle_p2(2, 6, 3) + charles_v2(10, 300, 450)", "19", "real",
      "Boyle P2 (4) + Charles V2 (15) -> 19" },
    { "bp_elevation(2, 0.52, 1.5) + fp_depression(2, 1.86, 1.5)", "7.14", "real",
      "Colligative boiling elevation (1.56) + freezing depression (5.58) -> 7.14" },
    { "weak_acid_ph(10 ^ -5, 0.1) + pka_from_ka(10 ^ -5)", "8", "real",
      "Weak acid pH (3) + pKa (5) -> 8" },
    { "cell_emf(0.34, -0.76)", "1.1", "real",
      "Electrochemical Daniell cell EMF (0.34 - (-0.76)) -> 1.1 V" },
    { "hw_homo_dom(0.6) + hw_hetero(0.6) + hw_homo_rec(0.6)", "1", "real",
      "Hardy-Weinberg equilibrium p^2 + 2pq + q^2 = 1" },
    { "hw_carrier_count(0.04, 1000)", "320", "real",
      "Hardy-Weinberg heterozygous carriers (q^2=0.04, N=1000) -> 320" },
    { "chargaff_g_from_a(20) + dna_turns(100) + pcr_copies(10)", "1064", "real",
      "Chargaff %G (30) + B-DNA helical turns (10) + PCR 2^10 (1024) -> 1064" },
    { "michaelis_menten(100, 5, 5) + q10_rate(10, 20, 30, 2)", "70", "real",
      "Michaelis-Menten v0 at S=Km (50) + Q10 rate (20) -> 70" },
    { "water_potential(-8, 3) + diffusion_pressure_deficit(12, 5)", "2", "real",
      "Plant physiology water potential (-5) + DPD (7) -> 2" },
    { "total_magnification(10, 40) + mitotic_index(18, 200)", "409", "real",
      "Microscopy magnification (400x) + mitotic index (9%) -> 409" },
    { "(cardiac_output(72, 70) + mean_arterial_pressure(120, 80)) ~ 98.3733", "1", "real",
      "Cardiac output (5.04 L/min) + MAP (93.3333 mmHg) ~ 98.3733" },
    { "lincoln_petersen_pop(40, 60, 12) + trophic_energy_at(10000, 2)", "300", "real",
      "Mark-recapture population (200) + 10% trophic energy law (100) -> 300" },

    /* 18. All 8 Streamlined Ergonomic Features + Keywordless `(i in 1...5){}` / `(v in vec){}` & `loop` */
    { "tau_const ~ (2 * π)", "1", "real",
      "[Feat 1] Shorthand unquoted constant 'tau_const = 2π'" },
    { "hyp_3d(3, 4) * 100 + hyp_3d(3, 4, 12)", "513", "real",
      "[Feat 1 & 6] Shorthand 'hyp_3d(x, y, z = 0)' with default z=0 (5) and z=12 (13)" },
    { "log_base(1000) * 10 + log_base(8, 2)", "33", "real",
      "[Feat 1 & 6] Shorthand 'log_base(x, base = 10)' with default base=10 (3) and base=2 (3)" },
    { "round(3.6) * 100 + round(3.14159, 2)", "403.14", "real",
      "[Feat 6] Default parameter 'round(x, digits = 0)': round(3.6)=4, round(3.14159, 2)=3.14" },
    { "fx_with_fee(100, \"USD\", \"INR\")", "8274", "real",
      "[Feat 6] Default fee_pct=1.5 in 'fx_with_fee(100, \"USD\", \"INR\")' -> 8274" },
    { "vec(10, 20, 30, 40)", "[10, 20, 30, 40]", "vec",
      "[Feat 2] Variadic 'vec(10, 20, 30, 40)' constructor -> [10, 20, 30, 40]" },
    { "1..5", "[2, 3, 4]", "vec",
      "[Feat 2] Exclusive-exclusive 2-dot range operator '1..5' -> [2, 3, 4]" },
    { "1...5", "[1, 2, 3, 4, 5]", "vec",
      "[Feat 2] Inclusive-inclusive 3-dot range operator '1...5' -> [1, 2, 3, 4, 5]" },
    { "(5 in 1..100) * 1000 + (1 in 1..100) * 100 + (100 in 1...100) * 10 + (v := vec(1,3,5,7) ; 8 in v)", "1010", "real",
      "[Feat 2] Boolean 'in' operator: 5 in 1..100 (1), 1 in 1..100 (0), 100 in 1...100 (1), 8 in v (0)" },
    { "s := 0 ; (i in 1...5) { s += i } ; s", "15", "real",
      "[Feat 3] Context-aware iterator '(i in 1...5) { s += i }' -> 15" },
    { "s := 0 ; (i in 1..5) { s += i^2 } ; s", "29", "real",
      "[Feat 3] Exclusive-exclusive range loop '(i in 1..5) { s += i^2 }' (2^2+3^2+4^2 = 29)" },
    { "arr := vec(10, 20, 30, 40) ; tot := 0 ; (v in arr) { tot += v } ; tot", "100", "real",
      "[Feat 3] Context-aware vector iterator '(v in arr) { tot += v }' -> 100" },
    { "pts := list(vec2(1, 2), vec2(3, 4), vec2(5, 6)) ; sx := 0 ; (p in pts) { sx += p.x * p.y } ; sx", "44", "real",
      "[Feat 3] Struct-vector iterator '(p in pts) { sx += p.x * p.y }' -> 2+12+30 = 44" },
    { "if (5 in 1...100) { (25km in m) } else { 0m }", "25000", "real",
      "[Feat 3] Context-aware '(5 in 1...100)' conditional + '(25km in m)' bare-unit conversion -> 25000" },
    { "unit_of(25km in m)", "\"distance\"", "str",
      "[Feat 3] '(25km in m)' preserves entity type 'real' and unit type 'distance'" },
    { "s := 0 ; i in 1...5 { s += i } ; k in 1..<4 ?? { s += k * 10 } ; s", "75", "real",
      "[Feat 3] Unparenthesized undefined iterator variable 'i in 1...5 { ... }' (15) + 'k in 1..<4 ?\? { ... }' (60)" },
    { "m := 5 ; (25km in m) * m", "125000", "real",
      "[Feat 3] Dual variable/unit disambiguation: scalar 'm := 5' still allows '25km in m' (25000m * 5 = 125000m)" },
    { "km := vec(10, 25, 30) ; 25m in km", "0.025", "real",
      "[Feat 3] Dual 'km := vec(10,25,30)': '25m in km' auto-resolves to unit conversion (0.025 km)" },
    { "km := vec(10, 25, 30) ; (25 in km) * 10 + (25m in (km))", "10", "real",
      "[Feat 3] Dual 'km := vec(10,25,30)': '25 in km' (1) and parenthesized '25m in (km)' forced vec lookup (0)" },
    { "km := vec(10m, 25m, 50m) ; 25m in km", "1", "real",
      "[Feat 3] Dual distance vector 'km := vec(10m,25m,50m)': '25m in km' resolves to membership (1)" },
    { "km := vec(10m, 25m, 50m) ; 25m in 1km", "0.025", "real",
      "[Feat 3] Dual distance vector 'km := vec(10m,25m,50m)': '25m in 1km' explicitly forces unit conversion (0.025)" },
    { "len(1..<5) * 10 + len(1<..5)", "44", "real",
      "[Feat 2] Half-open ranges '1..<5' ([1,2,3,4]) and '1<..5' ([2,3,4,5])" },
    { "k := 0 ; loop : { k += 1 ; if (k == 7) { break(loop, k * 10) } ; jump(loop) }", "70", "real",
      "[Feat 3] 'loop' used freely as user tag 'loop : { ... break(loop, k*10) }' -> 70" },
    { "(φ ^ 2 - φ) ~ 1", "1", "real",
      "[Feat 3] Golden ratio symbol 'φ' identity φ^2 - φ == 1" },
    { "(φ == phi) * 10 + (ϕ == phi)", "11", "real",
      "[Feat 3] Golden ratio symbols 'φ' and 'ϕ' equal 'phi'" },
    { "cnt := 0 ; Outer : (i in 1...5) ?? { (j in 1...5) ?? { if (i * j > 6) { break(Outer) } ; cnt += 1 } } ; cnt", "8", "real",
      "[Feat 3] Tagged nested loop 'Outer : (i in 1...5) ?? { ... break(Outer) }' -> 8" },
    { "sum_squares_to(5) + vec_dot_self(vec(2, 3, 6))", "104", "real",
      "[Feat 1 & 3] User functions 'sum_squares_to(5)' (55) + 'vec_dot_self(vec(2,3,6))' (49) -> 104" },
    { "x := 4 ; 2x^2 + 3x + 1", "45", "real",
      "[Feat 4] Implicit multiplication '2x^2 + 3x + 1' at x=4 -> 32 + 12 + 1 = 45" },
    { "x := 5 ; 3(x + 1) + (x + 1)(x - 1)", "42", "real",
      "[Feat 4] Implicit multiplication '3(x + 1) + (x + 1)(x - 1)' at x=5 -> 18 + 24 = 42" },
    { "2 sin(π / 2) + 2 √16 + (2π / π)", "12", "real",
      "[Feat 4] Implicit multiplication '2 sin(π/2) + 2 √16 + (2π / π)' -> 2 + 8 + 2 = 12" },
    { "v := vec() ; v.push(10).push(20).push(30).vec_sum() + v.len()", "63", "real",
      "[Feat 5] UFCS dot-chaining 'v.push(10).push(20).push(30).vec_sum() + v.len()' -> 63" },
    { "M := [4, 7 ; 2, 6] ; M.transpose().det() + (3 - 4i).cabs()", "15", "real",
      "[Feat 5] UFCS dot-chaining 'M.transpose().det() + (3 - 4i).cabs()' -> 10 + 5 = 15" },
    { "solve(x, x^2 == 9, 2) * 10 + solve(x, 2x + 5 == 17)", "36", "real",
      "[Feat 7] General equation solver 'solve(x, x^2 == 9, 2)' (3) and 'solve(x, 2x + 5 == 17)' (6)" },
    { "solve(x, cos(x) == x, 0.5) ~ 0.739085", "1", "real",
      "[Feat 7] Transcendental equation solver 'solve(x, cos(x) == x, 0.5)' -> Dottie number ~0.739085" },
    { "fmin(x, (x - 3)^2 + 5, 0, 10) * 10 + fmax(x, -(x - 4)^2 + 12, 0, 10)", "34", "real",
      "[Feat 7] Extrema finders 'fmin(x, (x-3)^2+5, 0, 10)' (3) and 'fmax(x, -(x-4)^2+12, 0, 10)' (4)" },
    { "r1, r2 := quad_roots(1, -5, 6) ; r1 * 10 + r2", "32", "real",
      "[Feat 8] Struct destructuring 'r1, r2 := quad_roots(1, -5, 6)' -> r1=3, r2=2" },
    { "a, b, c := vec(10, 20, 30) ; p, q := [100, 200] ; a + b + c + p + q", "360", "real",
      "[Feat 8] Vector & matrix destructuring 'a, b, c := vec(10,20,30)' and 'p, q := [100,200]'" },

    /* 19. All 5 New Language Constructs:
           #1 Default Newline `;` Inside & Outside Scopes (Compulsory `;` on Single Line)
           #2 `continue`, `continue(Tag)`, `return expr`, Value-Returning `break expr`
           #3 Vector & Matrix Slicing (`v[1..3]`, `M[0..1, 1..2]`) & Comprehensions (`[x^2 for x in ...]`)
           #4 First-Class Lambdas (`(x) => x^2`) & Higher-Order `.map()`, `.filter()`, `.reduce()`
           #5 Pattern-Matched `switch` (Ranges, Relational Guards, Struct/Vector Destructuring & `if` Guards) */
    { "x := 10\ny := 20\nx + y", "30", "real",
      "[Construct 1] Top-level newline as default ';' operator outside any scope" },
    { "{\n  a := 5\n  b := 7\n  a * b\n}", "35", "real",
      "[Construct 1] Newline as default ';' operator inside '{ ... }' scope" },
    { "(\n  u := 4\n  v := 6\n  u^2 + v^2\n)", "52", "real",
      "[Construct 1] Newline as default ';' operator inside '( ... )' block" },
    { "s := 0\n(i in 1...6) ?? {\n  if (i % 2 == 0) { continue }\n  s += i\n}\ns", "9", "real",
      "[Construct 1 & 2] Multi-line '(i in 1...6) ?\?' with 'continue' skipping even numbers (1+3+5 = 9)" },
    { "s := 0 ; i := 0 ; (i < 6) ?? { i += 1 ; if (i == 3 || i == 5) { continue } ; s += i } ; s", "13", "real",
      "[Construct 2] 'continue' inside '?\?' while loop (1+2+4+6 = 13)" },
    { "tot := 0 ; Outer : (i in 1...3) ?? { (j in 1...3) ?? { if (j == 2) { continue(Outer) } ; tot += i * 10 + j } } ; tot", "63", "real",
      "[Construct 2] Tagged 'continue(Outer)' jumping to next outer loop iteration (11+21+31 = 63)" },
    { "found := (x in 1...20) ?? { if (x^2 > 50) { break x } }\nfound", "8", "real",
      "[Construct 2] Value-returning 'break x' from range loop (first x with x^2 > 50 is 8)" },
    { "res := Outer : (i in 1...5) ?? { (j in 1...5) ?? { if (i * j == 12) { break(Outer, i * 100 + j) } } } ; res", "304", "real",
      "[Construct 2] Tagged value-returning 'break(Outer, val)' from nested loop (i=3, j=4 -> 304)" },
    { "safe_sqrt_ret(-4) * 100 + safe_sqrt_ret(0) * 10 + safe_sqrt_ret(9)", "-70", "real",
      "[Construct 2] Early 'return expr' inside user function 'safe_sqrt_ret' (-100 + 0 + 30 = -70)" },
    { "x := 10\nreturn x * 5\nx = 999", "50", "real",
      "[Construct 2] Top-level early 'return expr' halting evaluation immediately -> 50" },
    { "v := vec(10, 20, 30, 40, 50) ; v[1...3]", "[20, 30, 40]", "vec",
      "[Construct 3] Inclusive vector range slicing 'v[1...3]' -> [20, 30, 40]" },
    { "v := vec(10, 20, 30, 40, 50) ; v[0..4]", "[20, 30, 40]", "vec",
      "[Construct 3] Exclusive vector range slicing 'v[0..4]' -> [20, 30, 40]" },
    { "v := vec(10, 20, 30, 40, 50) ; v[-3...-1]", "[30, 40, 50]", "vec",
      "[Construct 3] Negative-index inclusive vector slicing 'v[-3...-1]' -> [30, 40, 50]" },
    { "v := vec(1, 2, 3, 4, 5) ; v[1...3] = 99 ; v", "[1, 99, 99, 99, 5]", "vec",
      "[Construct 3] In-place scalar slice assignment 'v[1...3] = 99'" },
    { "M := [1, 2, 3 ; 4, 5, 6 ; 7, 8, 9] ; M[0...1, 1...2]", "[[2, 3], [5, 6]]", "mat",
      "[Construct 3] 2D submatrix slicing 'M[0...1, 1...2]' -> [[2, 3], [5, 6]]" },
    { "M := [1, 2, 3 ; 4, 5, 6 ; 7, 8, 9] ; M[2, 0...1]", "[7, 8]", "mat",
      "[Construct 3] Row-to-1D-slice indexing 'M[2, 0...1]' -> [7, 8]" },
    { "[x^2 for x in 1...5]", "[1, 4, 9, 16, 25]", "vec",
      "[Construct 3] Vector comprehension '[x^2 for x in 1...5]'" },
    { "[x^2 for x in 1...10 if x % 2 == 0]", "[4, 16, 36, 64, 100]", "vec",
      "[Construct 3] Filtered vector comprehension '[x^2 for x in 1...10 if x % 2 == 0]'" },
    { "[p.x + p.y for p in list(vec2(1, 2), vec2(3, 4), vec2(5, 6)) if p.x > 1]", "[7, 11]", "vec",
      "[Construct 3] Comprehension over struct vector with 'if' guard" },
    { "f := (x) => x^2 + 1 ; f(5) + call(f, 3)", "36", "real",
      "[Construct 4] First-class lambda 'f := (x) => x^2 + 1' invoked via 'f(5)' (26) and 'call(f, 3)' (10)" },
    { "add_fn := (a, b) => a + b * 10 ; add_fn(4, 3) + ((x) => x * 3)(7)", "55", "real",
      "[Construct 4] 2-arg lambda 'add_fn(4, 3)' (34) + immediate IIFE '((x) => x * 3)(7)' (21)" },
    { "k := 10 ; scale := x => x * k ; scale(7)", "70", "real",
      "[Construct 4] Single-ident lambda 'x => x * k' capturing lexical variable 'k = 10'" },
    { "(1...5).map(x => x^2).filter(x => x > 5).sum()", "50", "real",
      "[Construct 4] UFCS higher-order pipeline '(1...5).map(x => x^2).filter(x => x > 5).sum()' (9+16+25 = 50)" },
    { "(1...5).reduce((a, b) => a + b) * 1000 + (1...4).fold(10, (acc, x) => acc + x)", "15020", "real",
      "[Construct 4] '.reduce((a, b) => a + b)' (15) and '.fold(10, (acc, x) => acc + x)' (20)" },
    { "v := vec(2, 4, 7, 8) ; v.any(x => x % 2 != 0) * 100 + v.all(x => x > 0) * 10 + v.count_if(x => x >= 4)", "113", "real",
      "[Construct 4] Higher-order predicates '.any()' (1), '.all()' (1), and '.count_if()' (3)" },
    { "v := vec(3, 8, 15, 22) ; v.find(x => x > 10)", "15", "real",
      "[Construct 4] Higher-order '.find(x => x > 10)' returning first matching element (15)" },
    { "M := [1, 2 ; 3, 4] ; M.map(x => x * 10)", "[[10, 20], [30, 40]]", "mat",
      "[Construct 4] Higher-order '.map(x => x * 10)' preserving 2D matrix shape" },
    { "grade_band(95) == \"A\" && grade_band(84) == \"B\" && grade_band(72) == \"C\" && grade_band(50) == \"F\" && grade_band(120) == \"invalid\"", "1", "real",
      "[Construct 5] Pattern-matched 'switch' with relational 'case >= 90:' and range 'case 80..89:'" },
    { "describe_vec2(vec2(0, 0)) == \"origin\" && describe_vec2(vec2(0, 9)) == \"y-axis\" && describe_vec2(vec2(4, 4)) == \"diagonal\" && describe_vec2(vec2(2, 5)) == \"general\"", "1", "real",
      "[Construct 5] Pattern-matched 'switch' with struct destructuring 'case vec2(0, y):' & 'if x == y'" },
    { "p := vec2(0, 7) ; switch (p) { case vec2(0, y) : { y * 10 } case vec2(x, y) : { x + y } else : { 0 } }", "70", "real",
      "[Construct 5] Struct destructuring in 'switch' binding field variable 'y = 7' -> 70" },
    { "arr := [0, 12] ; switch (arr) { case [0, y] if y > 5 : { y * 3 } case [a, b] : { a + b } else : { -1 } }", "36", "real",
      "[Construct 5] Vector/1D-matrix destructuring 'case [0, y] if y > 5:' binding 'y = 12' -> 36" },

    /* 20. Postfix "_" (@tostr), Prefix "_" (@fromstr), "into" Operator, and Parameterized
           Containers `vec<T>()`, `list<T>()`, `stack<T>()`, `queue<T>()` */
    { "42_", "\"42\"", "str",
      "[Op _] Postfix \"_\" on real literal 42_ -> \"42\" ($str)" },
    { "(3 + 4i)_", "\"3 + 4i\"", "str",
      "[Op _] Postfix \"_\" on complex (3 + 4i)_ -> \"3 + 4i\"" },
    { "vec2(3, 4)_", "\"vec2(x=3, y=4)\"", "str",
      "[Op _] Postfix \"_\" on struct vec2(3, 4)_ -> \"vec2(x=3, y=4)\"" },
    { "vec(10, 20, 30)_", "\"[10, 20, 30]\"", "str",
      "[Op _] Postfix \"_\" on dynamic vector vec(10, 20, 30)_ -> \"[10, 20, 30]\"" },
    { "[1, 2 ; 3, 4]_", "\"[[1, 2], [3, 4]]\"", "str",
      "[Op _] Postfix \"_\" on 2D matrix [1, 2 ; 3, 4]_ -> \"[[1, 2], [3, 4]]\"" },
    { "_\"42\" + 8", "50", "real",
      "[Op _] Prefix \"_\" converting \"42\" -> real 42 (+ 8 = 50)" },
    { "_\"3 + 4i\"", "3 + 4i", "complex",
      "[Op _] Prefix \"_\" converting \"3 + 4i\" -> complex 3 + 4i" },
    { "_\"vec2(3, 4)\"", "vec2(x=3, y=4)", "vec2",
      "[Op _] Prefix \"_\" converting \"vec2(3, 4)\" -> struct vec2(x=3, y=4)" },
    { "_\"[10, 20, 30]\"", "[10, 20, 30]", "vec",
      "[Op _] Prefix \"_\" converting \"[10, 20, 30]\" -> dynamic $vec [10, 20, 30]" },
    { "_\"[[1, 2], [3, 4]]\"", "[[1, 2], [3, 4]]", "mat",
      "[Op _] Prefix \"_\" converting \"[[1, 2], [3, 4]]\" -> 2D $mat" },
    { "unit_of(_\"100 km\")", "\"distance\"", "str",
      "[Op _] Prefix \"_\" converting \"100 km\" preserves unit tag \"distance\"" },
    { "_(vec3(1, 2, 3)_)", "vec3(x=1, y=2, z=3)", "vec3",
      "[Op _] Round-trip string conversion _(vec3(1, 2, 3)_) -> vec3(x=1, y=2, z=3)" },
    { "5 into 5i", "5 + 5i", "complex",
      "[Op into] '5 into 5i' combines real and iota -> complex 5 + 5i" },
    { "5i into 5", "5 + 5i", "complex",
      "[Op into] '5i into 5' combines iota and real -> complex 5 + 5i" },
    { "5 into 5", "10", "real",
      "[Op into] Likewise entities/units '5 into 5' returns sum -> 10" },
    { "50m into 20cm", "50.2", "real",
      "[Op into] Likewise entities/units '50m into 20cm' returns sum -> 50.2 [distance]" },
    { "5i into 3i", "8i", "iota",
      "[Op into] Likewise imaginary '5i into 3i' returns sum -> 8i" },
    { "(1 + 2i) into (3 + 4i)", "4 + 6i", "complex",
      "[Op into] Likewise complex '(1 + 2i) into (3 + 4i)' returns sum -> 4 + 6i" },
    { "vec2(1, 2) into vec2(3, 4)", "vec2(x=4, y=6)", "vec2",
      "[Op into] Likewise struct 'vec2(1, 2) into vec2(3, 4)' returns sum -> vec2(x=4, y=6)" },
    { "vec3(1, 2, 3) into vec3(4, 5, 6)", "vec3(x=5, y=7, z=9)", "vec3",
      "[Op into] Likewise struct 'vec3(1, 2, 3) into vec3(4, 5, 6)' returns sum -> vec3(x=5, y=7, z=9)" },
    { "2 into (3 + 4i)", "5 + 4i", "complex",
      "[Op into] '2 into (3 + 4i)' adds real component into complex -> 5 + 4i" },
    { "3i into (3 + 4i)", "3 + 7i", "complex",
      "[Op into] '3i into (3 + 4i)' adds imaginary component into complex -> 3 + 7i" },
    { "7 into vec2(3, 4)", "vec3(x=3, y=4, z=7)", "vec3",
      "[Op into] '7 into vec2(3, 4)' promotes vec2 + z -> vec3(x=3, y=4, z=7)" },
    { "vec2(3, 4) into 7", "vec3(x=3, y=4, z=7)", "vec3",
      "[Op into] 'vec2(3, 4) into 7' promotes vec2 + z -> vec3(x=3, y=4, z=7)" },
    { "40 into vec(10, 20, 30)", "[10, 20, 30, 40]", "vec",
      "[Op into] '40 into vec(10, 20, 30)' pushes element into vector (alternate for vec.push(T))" },
    { "30 into list(10, 20)", "[10, 20, 30]", "vec",
      "[Op into] '30 into list(10, 20)' pushes element into list -> [10, 20, 30]" },
    { "v := vec<real>() ; 10 into v ; 20 into v ; 30 into v ; v", "[10, 20, 30]", "vec",
      "[Op into & vec<T>] Sequential 'T into v' on typed 'vec<real>()' -> [10, 20, 30]" },
    { "10 into 20 into vec<real>()", "[20, 10]", "vec",
      "[Op into & vec<T>] Right-associative chained '10 into 20 into vec<real>()' -> [20, 10]" },
    { "vec<real>(4).cap() * 100 + vec<real>(3, 7).sum()", "421", "real",
      "[vec<T>] Typed capacity 'vec<real>(4)' (cap 4) and fill 'vec<real>(3, 7)' (sum 21)" },
    { "vec<vec2>(vec2(1, 2), vec2(3, 4))", "[vec2(x=1, y=2), vec2(x=3, y=4)]", "vec",
      "[vec<T>] Variadic typed struct vector 'vec<vec2>(vec2(1, 2), vec2(3, 4))'" },
    { "d := vec<real, distance>(1 km, 500 m) ; d[0] + d[1]", "1500", "real",
      "[vec<T, unit>] Typed & unit-locked 'vec<real, distance>(1 km, 500 m)' -> 1500 m" },
    { "list<complex>(1 + 2i, 3 - 4i)", "[1 + 2i, 3 - 4i]", "vec",
      "[list<T>] Parameterized 'list<complex>(1 + 2i, 3 - 4i)'" },
    { "s := stack<real>(10, 20) ; s.push(30) ; s.peek() * 1000 + s.pop() * 10 + s.len()", "30302", "real",
      "[stack<T>] LIFO 'stack<real>(10, 20)' with .push(30), .peek() (30), .pop() (30), .len() (2)" },
    { "q := queue<real>(10, 20) ; q.enqueue(30) ; q.front() * 1000 + q.dequeue() * 10 + q.front()", "10120", "real",
      "[queue<T>] FIFO 'queue<real>(10, 20)' with .enqueue(30), .front() (10), .dequeue() (10), .front() (20)" },
    { "v : vec<vec2> := vec<vec2>() ; vec2(5, 6) into v ; v[0].x + v[0].y", "11", "real",
      "[vec<T> annot] Variable type lock 'v : vec<vec2> := vec<vec2>()' + 'vec2(5, 6) into v' -> 11" },
    { NULL, NULL, NULL, NULL }
};

static void print_architecture_report(const Engine *eng) {
    printf("========================================================================\n");
    printf("  ENGINE MODE : 8-Byte IEEE-754 NaN-Boxing (sizeof(Value) == 8)\n");
    printf("========================================================================\n");
    printf("  sizeof(Value)        : %5zu bytes\n", sizeof(Value));
    printf("  sizeof(AstNode)      : %5zu bytes\n", sizeof(AstNode));
    printf("  sizeof(MathcHeader)  : %5zu bytes (ABI- & OS-neutral binary header)\n",
           sizeof(MathcHeader));
    printf("  sizeof(MemArena)     : %5zu bytes (%.2f KB isolated memory module)\n",
           sizeof(MemArena), (double)sizeof(MemArena) / 1024.0);
    printf("  sizeof(SymTable)     : %5zu bytes (%.2f KB symbol/type/extern registry)\n",
           sizeof(SymTable), (double)sizeof(SymTable) / 1024.0);
    printf("  sizeof(Engine) Total : %5zu bytes (%.2f KB)\n",
           sizeof(Engine), (double)sizeof(Engine) / 1024.0);
    printf("  Hooked Modules       : %u (active mask = 0x%03llX)\n    -> ",
           eng->sym.mod_count, (unsigned long long)eng->sym.active_modules);
    for (uint8_t i = 0; i < eng->sym.mod_count; ++i) {
        StrSlice m = eng->sym.mod_names[i];
        printf("[%.*s]%s", (int)m.len, mem_str_ptr(&eng->mem, m),
               (i + 1 < eng->sym.mod_count) ? " " : "\n");
    }
    printf("  Registered Types     : %u (%u pat elems, %u struct fields)\n",
           eng->sym.type_count - 1, eng->sym.pat_elem_count, eng->sym.field_count);
    printf("  Registered Overloads : %u / %u (%u / %u syms, %u / %u consts)\n",
           eng->sym.overload_count, (unsigned)SYM_MAX_OVERLOADS,
           eng->sym.sym_count, (unsigned)SYM_MAX_SYMS,
           eng->sym.const_count, (unsigned)SYM_MAX_CONSTS);
    printf("  Permanent Arena Used : %u / %u AST nodes, %u / %u f64 pool, %u / %u src bytes\n",
           eng->mem.perm_ast_count, (unsigned)MEM_MAX_AST_NODES,
           eng->mem.perm_pool_top, (unsigned)MEM_MAX_F64_POOL,
           eng->mem.perm_src_len, (unsigned)MEM_SRC_BUF_CAP);
    printf("  Registered Externs   : %u (@fx_rate, @fx_history, @kv_set, @kv_get, @elem_mass)\n",
           eng->sym.extern_count);
    printf("========================================================================\n\n");
}

int main(void) {
    /* 1. Verify Blank-Slate Guarantee (`eng_init_blank`) */
    eng_init_blank(&g_eng);
    Value v;
    bool blank_ok = !eng_eval(&g_eng, "1 + 2", &v);
    printf("[1] Blank-Slate Verification (eng_init_blank -> eval \"1 + 2\"):\n");
    printf("    Result: %s -> \"%s\"\n\n",
           blank_ok ? "EXPECTED REJECTION" : "UNEXPECTED SUCCESS",
           eng_last_error(&g_eng));

    /* 2. Auto-Boot Engine (`eng_init` loads embedded `stdlib_precompiled.h` in memory) */
    if (!eng_init(&g_eng)) {
        fprintf(stderr, "Failed to auto-initialize engine: %s\n", eng_last_error(&g_eng));
        return 1;
    }
    printf("[2] Internal Zero-Parse Auto-Boot (`eng_init(&g_eng)`):\n");
    printf("    Predefined bootstrap & scientific stdlib loaded from memory (%u modules, %u overloads, %u AST nodes).\n\n",
           g_eng.sym.mod_count, g_eng.sym.overload_count, g_eng.mem.perm_ast_count);

    /* 3. Frontend reads raw source text, passes ONLY raw text to `user_compile_ext`,
     *    receives back the compiled binary image, and tests `user_load_mathc`. */
    const char *math_path  = "Test/user_physics.math";
    const char *mathc_path = "Test/user_physics.mathc";
    size_t raw_len = 0;
    char *raw_math_text = (char *)frontend_read_file(math_path, &raw_len);
    if (!raw_math_text) {
        math_path  = "user_physics.math";
        mathc_path = "user_physics.mathc";
        raw_math_text = (char *)frontend_read_file(math_path, &raw_len);
    }
    if (!raw_math_text) {
        fprintf(stderr, "Frontend error: could not read 'user_physics.math'\n");
        return 1;
    }

    const uint8_t *compiled_mathc = NULL;
    size_t compiled_mathc_len = 0;
    if (!user_compile_ext(&g_eng, raw_math_text, &compiled_mathc, &compiled_mathc_len)) {
        fprintf(stderr, "user_compile_ext failed: %s\n", eng_last_error(&g_eng));
        free(raw_math_text);
        return 1;
    }
    free(raw_math_text);

    (void)frontend_write_file(mathc_path, compiled_mathc, compiled_mathc_len);

    printf("[3] In-Memory User Extension API (`user_loader.h`):\n");
    printf("    - Step A (`user_compile_ext`): Compiled raw text (%zu bytes) -> handed back %zu-byte binary image\n",
           raw_len, compiled_mathc_len);

    size_t disk_mathc_len = 0;
    uint8_t *disk_mathc = frontend_read_file(mathc_path, &disk_mathc_len);
    eng_init(&g_eng);
    register_all_host_externs(&g_eng);

    bool load_mathc_ok = (disk_mathc != NULL) &&
                         user_load_mathc(&g_eng, disk_mathc, disk_mathc_len);
    free(disk_mathc);
    if (!load_mathc_ok) {
        fprintf(stderr, "user_load_mathc failed: %s\n", eng_last_error(&g_eng));
        return 1;
    }
    printf("    - Step B (`user_load_mathc`) : Loaded %zu-byte compiled binary image in O(1) `memcpy` time\n\n",
           disk_mathc_len);

    print_architecture_report(&g_eng);

    /* 4. Run Automated Expression Test Suite */
    printf("[4] Running Full Automated Suite (Stdlib + Control Flow + Units + Calculus + 5 New Constructs):\n");
    int passed = 0, total = 0;
    for (int i = 0; k_tests[i].expr != NULL; ++i) {
        total++;
        Value out;
        char val_buf[128], type_buf[32];
        if (!eng_eval(&g_eng, k_tests[i].expr, &out)) {
            printf("  [FAIL] %-48s -> ERROR: %s\n", k_tests[i].expr, eng_last_error(&g_eng));
            continue;
        }
        eng_format_value(&g_eng, out, val_buf, sizeof(val_buf));
        eng_type_name(&g_eng, eng_val_type(&g_eng, out), type_buf, sizeof(type_buf));

        bool ok = (strcmp(val_buf, k_tests[i].expected_val) == 0) &&
                  (strcmp(type_buf, k_tests[i].expected_type) == 0);
        if (ok) passed++;
        printf("  [%s] %-48s => %-26s : %-7s (%s)\n",
               ok ? "PASS" : "FAIL",
               k_tests[i].expr, val_buf, type_buf, k_tests[i].desc);
    }
    printf("\n    Suite Summary: %d / %d passed.\n\n", passed, total);

    /* 5. Verify Compile-Time Guardrails for Scopes, Compulsory Same-Line ';', `if/or/else`, Reserved Keywords, and `vec<T>` */
    bool vec_type_guard = !eng_eval(&g_eng, "v := vec() ; push(v, 10) ; push(v, vec2(1, 2))", &v) &&
                          !eng_eval(&g_eng, "v := vec<vec2>() ; 10 into v", &v) &&
                          !eng_eval(&g_eng, "vec<real>(vec2(1, 2))", &v) &&
                          !eng_eval(&g_eng, "q := queue<real, distance>() ; q.enqueue(60 deg)", &v) &&
                          !eng_eval(&g_eng, "_\"not_a_valid_type_xyz\"", &v);
    printf("[5] Type-Safety, Compulsory Same-Line ';', Reserved-Keyword & Control-Flow Guardrails:\n");
    printf("    - Homogeneous vec<T> guard    : %s -> \"%s\"\n",
           vec_type_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool same_line_semi_guard = !eng_eval(&g_eng, "x := 10 y := 20", &v) &&
                                !eng_eval(&g_eng, "{ a := 10 b := 20 }", &v) &&
                                !eng_eval(&g_eng, "{ a := 10 } a", &v);
    printf("    - Same-line missing ';' guard : %s -> \"%s\"\n",
           same_line_semi_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool scope_isolation_guard = !eng_eval(&g_eng, "{ inner_only := 42 } ; inner_only", &v);
    printf("    - Local '{ ... }' isolation   : %s -> \"%s\"\n",
           scope_isolation_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool multi_else_guard = !eng_eval(&g_eng, "if (1) { 10 } else { 20 } else { 30 }", &v);
    printf("    - Duplicate 'else' rejection  : %s -> \"%s\"\n",
           multi_else_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool else_if_guard = !eng_eval(&g_eng, "if (0) { 10 } else if (1) { 20 }", &v);
    printf("    - 'else if' (2nd if) rejection: %s -> \"%s\"\n",
           else_if_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool kw_var_guard = !eng_eval(&g_eng, "if := 5", &v) &&
                        !eng_eval(&g_eng, "else := 5", &v) &&
                        !eng_eval(&g_eng, "or := 5", &v) &&
                        !eng_eval(&g_eng, "continue := 5", &v) &&
                        !eng_eval(&g_eng, "return := 5", &v) &&
                        !eng_eval(&g_eng, "x, switch := 5", &v) &&
                        !eng_eval(&g_eng, "(5km in m){}", &v) &&
                        !eng_eval(&g_eng, "(d := 5km ; (d in m){})", &v) &&
                        !eng_eval(&g_eng, "undef_var in 1...10", &v) &&
                        !eng_eval(&g_eng, "if (undef_var in 1...10) { 1 }", &v);
    printf("    - Reserved keyword as variable: %s -> \"%s\"\n",
           kw_var_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool kw_tag_guard = !eng_eval(&g_eng, "if : { 10 }", &v) &&
                        !eng_eval(&g_eng, "jump(else)", &v) &&
                        !eng_eval(&g_eng, "break(switch)", &v);
    printf("    - Reserved keyword as tag     : %s -> \"%s\"\n",
           kw_tag_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool unlike_unit_guard = !eng_eval(&g_eng, "y := 100cm ; y += 60°", &v);
    printf("    - 'y := 100cm ; y += 60°'     : %s -> \"%s\"\n",
           unlike_unit_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool concrete_real_guard = !eng_eval(&g_eng, "x : real := 5 ; x += 5i", &v);
    printf("    - 'x : real := 5 ; x += 5i'   : %s -> \"%s\"\n",
           concrete_real_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool concrete_real_dist_unit_guard = !eng_eval(&g_eng, "x : real, distance := 50m ; x += 60°", &v);
    printf("    - 'x:real,distance += 60°'    : %s -> \"%s\"\n",
           concrete_real_dist_unit_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool concrete_real_dist_iota_guard = !eng_eval(&g_eng, "x : real, distance := 50m ; x += 50im", &v);
    printf("    - 'x:real,distance += 50im'   : %s -> \"%s\"\n",
           concrete_real_dist_iota_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool concrete_dist_unitless_iota_guard = !eng_eval(&g_eng, "x : distance := 50m ; x += 5i", &v);
    printf("    - 'x : distance := 50m ; +=5i': %s -> \"%s\"\n",
           concrete_dist_unitless_iota_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool fn_unit_mismatch_guard = !eng_eval(&g_eng, "avg_speed(100m, 60°)", &v);
    printf("    - 'avg_speed(100m, 60°)'      : %s -> \"%s\"\n",
           fn_unit_mismatch_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool gen_T_unit_mismatch_guard = !eng_eval(&g_eng, "unit_span(100m, 60°)", &v);
    printf("    - 'unit_span(100m, 60°)' (T)  : %s -> \"%s\"\n",
           gen_T_unit_mismatch_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool conv_unit_mismatch_guard = !eng_eval(&g_eng, "(100m) in (1 deg)", &v);
    printf("    - '(100m) in (1 deg)'         : %s -> \"%s\"\n",
           conv_unit_mismatch_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool jagged_mat_guard = !eng_eval(&g_eng, "[[1, 2], [3, 4, 5]]", &v);
    printf("    - Jagged '[[1, 2], [3, 4, 5]]': %s -> \"%s\"\n",
           jagged_mat_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool non_bool_not_guard = !eng_eval(&g_eng, "!5", &v) && !eng_eval(&g_eng, "!(2 + 3)", &v);
    printf("    - Out-of-range '!5' / '!(2+3)': %s -> \"%s\"\n",
           non_bool_not_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool not_overload_guard = !eng_eval(&g_eng, "!\"hello\"", &v);
    printf("    - No-overload '!\"hello\"'      : %s -> \"%s\"\n",
           not_overload_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool lim_diverge_guard = !eng_eval(&g_eng, "lim(x, abs(x) / x, 0)", &v);
    printf("    - Jump limit 'lim(|x|/x, 0)'  : %s -> \"%s\"\n",
           lim_diverge_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool custom_err_guard = !eng_eval(&g_eng, "error(\"Custom domain error: x must be 0 or 1\")", &v);
    printf("    - Custom 'error(\"...\")'       : %s -> \"%s\"\n",
           custom_err_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool fx_unknown_ccy_guard = !eng_eval(&g_eng, "convert_fx(100, \"USD\", \"BTC\")", &v);
    printf("    - Unknown FX pair 'USD->BTC'  : %s -> \"%s\"\n",
           fx_unknown_ccy_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    bool fx_neg_amount_guard = !eng_eval(&g_eng, "convert_fx(-50, \"USD\", \"INR\")", &v);
    printf("    - Negative FX amount '-50'    : %s -> \"%s\"\n\n",
           fx_neg_amount_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    /* 6. Verify O(1) Module Toggling (`boot_syntax`, `units`, `user_physics`) */
    printf("[6] Dynamic O(1) Module Toggling ('boot_syntax', 'units', 'user_physics'):\n");
    eng_set_module(&g_eng, "boot_syntax", false);
    bool syn_off = !eng_eval(&g_eng, "{ 1 + 2 }", &v);
    printf("    - Disable [boot_syntax]  -> eval(\"{ 1 + 2 }\") : %s (\"%s\")\n",
           syn_off ? "REJECTED ('{' & '}' disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "boot_syntax", true);

    eng_set_module(&g_eng, "units", false);
    bool units_off = !eng_eval(&g_eng, "100 km", &v);
    printf("    - Disable [units]        -> eval(\"100 km\")    : %s (\"%s\")\n",
           units_off ? "REJECTED ('km' disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "units", true);

    eng_set_module(&g_eng, "user_physics", false);
    bool phys_off = !eng_eval(&g_eng, "convert_fx(100, \"USD\", \"INR\")", &v);
    printf("    - Disable [user_physics] -> eval(\"convert_fx\") : %s (\"%s\")\n\n",
           phys_off ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "user_physics", true);

    /* 7. Verify All 8 Audit Regression Cases */
    printf("[7] Audit Regression Suite (Bugs 1-10 Verification):\n");

    const uint8_t *uconst_mathc = NULL;
    size_t uconst_len = 0;
    bool bug1_ok = user_compile_ext(&g_eng, "[uconst_mod]\n\"gx\" : real, distance = 5m\n", &uconst_mathc, &uconst_len) &&
                   user_load_mathc(&g_eng, uconst_mathc, uconst_len) &&
                   eng_eval(&g_eng, "10 + 20 * 30", &v) &&
                   eng_eval(&g_eng, "gx + 1m", &v) &&
                   (val_get_f64(&g_eng.mem, v) == 6.0) &&
                   (strcmp(eng_unit_name(&g_eng, eng_val_unit(&g_eng, v), (char[32]){0}, 32), "distance") == 0);
    printf("    - [Bug 1] .mathc unit constant (gx + 1m == 6m) : %s\n", bug1_ok ? "PASS" : "FAIL");

    uint16_t pre_ov  = g_eng.sym.overload_count;
    uint16_t pre_ast = g_eng.mem.perm_ast_count;
    bool bug2_fail = !user_compile_ext(&g_eng, "\"leak_fn\"(x:real) -> real = x + 1\n\"bad_fn\"(x:real) -> real = (1 + ", NULL, NULL);
    bool bug2_no_leak = !eng_eval(&g_eng, "leak_fn(5)", &v);
    bool bug2_ok = bug2_fail && bug2_no_leak &&
                   (g_eng.sym.overload_count == pre_ov) &&
                   (g_eng.mem.perm_ast_count == pre_ast);
    printf("    - [Bug 2] Transactional rollback on bad ext    : %s (overloads=%u)\n",
           bug2_ok ? "PASS" : "FAIL", g_eng.sym.overload_count);

    char deep_expr[512];
    memset(deep_expr, '(', 200);
    deep_expr[200] = '1';
    memset(deep_expr + 201, ')', 200);
    deep_expr[401] = '\0';
    bool bug3_ok = !eng_eval(&g_eng, deep_expr, &v);
    printf("    - [Bug 3] Deep nesting guard (200 parens)      : %s (\"%s\")\n",
           bug3_ok ? "PASS" : "FAIL", eng_last_error(&g_eng));

    bool bug4_ok = !user_compile_ext(&g_eng, "$self_node(a:real, next:self_node)", NULL, NULL);
    printf("    - [Bug 4] Self-referential struct rejection    : %s (\"%s\")\n",
           bug4_ok ? "PASS" : "FAIL", eng_last_error(&g_eng));

    char *huge_expr = (char *)malloc(66000);
    bool bug5_ok = false;
    if (huge_expr) {
        memset(huge_expr, ' ', 65540);
        huge_expr[0] = '1';
        huge_expr[1] = '+';
        huge_expr[2] = '1';
        huge_expr[65540] = '@';
        huge_expr[65541] = '\0';
        bug5_ok = !eng_eval(&g_eng, huge_expr, &v);
        free(huge_expr);
    }
    printf("    - [Bug 5] >65534-byte input truncation guard   : %s (\"%s\")\n",
           bug5_ok ? "PASS" : "FAIL", eng_last_error(&g_eng));

    double saved_pool0 = g_eng.mem.f64_pool[0];
    bool bug6_exhaust = !eng_eval(&g_eng, "v := vec(9000, 5)", &v);
    bool bug6_ok = bug6_exhaust && (g_eng.mem.f64_pool[0] == saved_pool0);
    printf("    - [Bug 6] Pool exhaustion f64_pool[0] integrity: %s (pool[0]=%g)\n",
           bug6_ok ? "PASS" : "FAIL", g_eng.mem.f64_pool[0]);

    bool bug7_compile = user_compile_ext(&g_eng, "\"anon_mod_fn\"(x:real) -> real = x * 3\n", NULL, NULL);
    eng_set_module(&g_eng, "biology", false);
    bool bug7_eval = eng_eval(&g_eng, "anon_mod_fn(7)", &v) && (val_get_f64(&g_eng.mem, v) == 21.0);
    eng_set_module(&g_eng, "biology", true);
    bool bug7_ok = bug7_compile && bug7_eval;
    printf("    - [Bug 7] Headerless user ext in [default]     : %s\n", bug7_ok ? "PASS" : "FAIL");

    bool bug8_huge = !eng_eval(&g_eng, "pts := vec() ; push(pts, 10) ; at(pts, -4294967296)", &v);
    bool bug8_frac = !eng_eval(&g_eng, "pts := vec() ; push(pts, 10) ; push(pts, 20) ; at(pts, 0.5)", &v);
    bool bug8_ok   = bug8_huge && bug8_frac;
    printf("    - [Bug 8] Safe float-to-int index guard        : %s (\"%s\")\n",
           bug8_ok ? "PASS" : "FAIL", eng_last_error(&g_eng));

    /* [Bug 9] Transactional .mathc load (corrupted image must not overwrite live Engine) */
    uint8_t *mut_buf = (uint8_t *)malloc(uconst_len);
    bool bug9_ok = false;
    if (mut_buf && uconst_mathc && uconst_len > sizeof(MathcHeader) + 32) {
        uint16_t saved_ov  = g_eng.sym.overload_count;
        uint16_t saved_ast = g_eng.mem.perm_ast_count;
        uint8_t  saved_mod = g_eng.sym.mod_count;

        /* 9a. Flip payload byte without updating checksum -> rejected by checksum */
        memcpy(mut_buf, uconst_mathc, uconst_len);
        mut_buf[sizeof(MathcHeader) + 8] ^= 0xFFu;
        bool reject_csum = !user_load_mathc(&g_eng, mut_buf, uconst_len);

        /* 9b. Corrupt an AST node index in payload AND recompute checksum -> rejected by pre-copy validator */
        memcpy(mut_buf, uconst_mathc, uconst_len);
        MathcHeader mhdr;
        memcpy(&mhdr, mut_buf, sizeof(MathcHeader));
        size_t ast_off = sizeof(MathcHeader) + (size_t)mhdr.perm_src_len;
        if (mhdr.perm_ast_count > 10) {
            AstNode bad_node = { (uint8_t)AST_UNARY, (uint8_t)FIX_PREFIX, 0, 62719u, MEM_NO_IDX };
            memcpy(mut_buf + ast_off + 5 * sizeof(AstNode), &bad_node, sizeof(AstNode));
        }
        mhdr.checksum = ext_calc_mathc_checksum(&mhdr, mut_buf + sizeof(MathcHeader), mhdr.payload_bytes);
        memcpy(mut_buf, &mhdr, sizeof(MathcHeader));
        bool reject_oob = !user_load_mathc(&g_eng, mut_buf, uconst_len);

        char oob_err[128];
        snprintf(oob_err, sizeof(oob_err), "%s", eng_last_error(&g_eng));

        /* Verify live g_eng was never overwritten and still evaluates prior definitions */
        Value v_fn = 0, v_gx = 0;
        bool state_intact = (g_eng.sym.overload_count == saved_ov) &&
                            (g_eng.mem.perm_ast_count == saved_ast) &&
                            (g_eng.sym.mod_count == saved_mod) &&
                            eng_eval(&g_eng, "anon_mod_fn(7)", &v_fn) &&
                            (val_get_f64(&g_eng.mem, v_fn) == 21.0) &&
                            eng_eval(&g_eng, "gx + 1m", &v_gx) &&
                            (val_get_f64(&g_eng.mem, v_gx) == 6.0);
        bug9_ok = reject_csum && reject_oob && state_intact;
        printf("    - [Bug 9] Transactional .mathc load & checksum : %s (\"%s\")\n",
               bug9_ok ? "PASS" : "FAIL", oob_err);
    }

    /* [Bug 10] 3,000-Iteration Corrupted .mathc Image Fuzzer (raw + checksum-recomputed) */
    static Engine s_fuzz_eng;
    bool bug10_ok = (mut_buf != NULL && uconst_mathc != NULL);
    uint32_t rng = 0xC0FFEE12u;
    int rejected_cnt = 0, accepted_cnt = 0;
    if (bug10_ok) {
        eng_init(&s_fuzz_eng);
        for (int iter = 0; iter < 3000; ++iter) {
            memcpy(mut_buf, uconst_mathc, uconst_len);
            int num_muts = 1 + (int)(rng % 4u);
            for (int m = 0; m < num_muts; ++m) {
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                size_t pos = (iter < 1500)
                             ? (size_t)(rng % uconst_len)
                             : (sizeof(MathcHeader) + (size_t)(rng % (uconst_len - sizeof(MathcHeader))));
                rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                mut_buf[pos] ^= (uint8_t)(1u + (rng % 255u));
            }
            if (iter >= 1500) {
                /* Recompute checksum so mutations directly stress validate_mathc_payload & AST bounds */
                MathcHeader fhdr;
                memcpy(&fhdr, mut_buf, sizeof(MathcHeader));
                if (fhdr.payload_bytes == uconst_len - sizeof(MathcHeader)) {
                    fhdr.checksum = ext_calc_mathc_checksum(&fhdr,
                                                            mut_buf + sizeof(MathcHeader),
                                                            fhdr.payload_bytes);
                    memcpy(mut_buf, &fhdr, sizeof(MathcHeader));
                }
            }
            if (user_load_mathc(&s_fuzz_eng, mut_buf, uconst_len)) {
                accepted_cnt++;
                Value fv;
                (void)eng_eval(&s_fuzz_eng, "2 + 3 * 4", &fv);
                (void)eng_eval(&s_fuzz_eng, "sin(0) + cos(0)", &fv);
                (void)eng_eval(&s_fuzz_eng, "vec2(3, 4).x", &fv);
                (void)eng_eval(&s_fuzz_eng, "5!", &fv);
            } else {
                rejected_cnt++;
            }
        }
    }
    free(mut_buf);
    bool null_guard_ok = !eng_eval(NULL, "1 + 1", &v) && !eng_set_module(NULL, "units", false);
    bug10_ok = bug10_ok && null_guard_ok && (rejected_cnt + accepted_cnt == 3000) && (rejected_cnt > 2500);
    printf("    - [Bug 10] 3,000x mutated .mathc fuzz + NULL   : %s (%d rejected, %d safe-eval)\n\n",
           bug10_ok ? "PASS" : "FAIL", rejected_cnt, accepted_cnt);

    /* 8. Verify Separated Compilation & Dependency-Resolved Linking + Duplicate Rejection */
    printf("[8] Separated Compilation & Dependency-Resolved Linking (`user_compile_unit` / `user_link_ext`):\n");
    /* 8a. Compile 3 dependent modules in REVERSE dependency order:
     *     [mod_aero] -> depends on <[mod_thermo] and <[mod_base]
     *     [mod_thermo] -> depends on <[mod_base]
     *     [mod_base] -> depends on <[arith]
     */
    const char *mod_aero_src =
        "[mod_aero]\n"
        "<[mod_thermo]\n"
        "<[mod_base]\n"
        "aero_lift(v) = thermo_scale(v) + base_k\n";
    const char *mod_thermo_src =
        "[mod_thermo]\n"
        "<[mod_base]\n"
        "thermo_scale(x) = base_fn(x) * 3\n";
    const char *mod_base_src =
        "[mod_base]\n"
        "<[arith]\n"
        "base_k = 7\n"
        "base_fn(x) = x + base_k\n";

    UserBuildCtx ubuild;
    user_build_init(&ubuild);
    const uint8_t *linked_mathc = NULL;
    size_t linked_mathc_len = 0;
    bool link_order_ok = user_compile_unit(&g_eng, &ubuild, mod_aero_src) &&
                         user_compile_unit(&g_eng, &ubuild, mod_thermo_src) &&
                         user_compile_unit(&g_eng, &ubuild, mod_base_src) &&
                         user_link_ext(&g_eng, &ubuild, &linked_mathc, &linked_mathc_len) &&
                         (linked_mathc != NULL && linked_mathc_len > sizeof(MathcHeader)) &&
                         eng_eval(&g_eng, "aero_lift(3)", &v) &&
                         (val_get_f64(&g_eng.mem, v) == 37.0);
    printf("    - Out-of-order compile+link ([aero]->[thermo]->[base]) : %s (aero_lift(3) == 37, %zuB .mathc)\n",
           link_order_ok ? "PASS" : "FAIL", linked_mathc_len);

    /* 8b. Reject duplicate named module load (`[mod_base]` already loaded) */
    bool dup_mod_guard = !user_compile_ext(&g_eng, mod_base_src, NULL, NULL);
    printf("    - Duplicate named module load ([mod_base] 2x)          : %s -> \"%s\"\n",
           dup_mod_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));

    /* 8c. Reject duplicate headerless `[default]` declaration load */
    bool dup_def_guard = !user_compile_ext(&g_eng, "\"anon_mod_fn\"(x:real) -> real = x * 3\n", NULL, NULL);
    printf("    - Duplicate [default] raw-text reload                  : %s -> \"%s\"\n",
           dup_def_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));

    /* 8d. Reject missing module dependency (`<[no_such_mod]`) with transactional rollback */
    bool missing_dep_guard = !user_compile_ext(&g_eng, "[mod_orphan]\n<[no_such_mod]\norphan_fn(x) = x\n", NULL, NULL) &&
                             !eng_eval(&g_eng, "orphan_fn(1)", &v);
    printf("    - Missing module dependency (<[no_such_mod])           : %s -> \"%s\"\n",
           missing_dep_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));

    /* 8e. Reject circular module dependency (`[cyc_a] <[cyc_b]` & `[cyc_b] <[cyc_a]`) */
    const char *cyc_srcs[2] = {
        "[cyc_a]\n<[cyc_b]\na_fn(x) = b_fn(x)\n",
        "[cyc_b]\n<[cyc_a]\nb_fn(x) = a_fn(x)\n"
    };
    bool cycle_dep_guard = !user_compile_and_link(&g_eng, cyc_srcs, 2, NULL, NULL);
    printf("    - Circular module dependency ([cyc_a] <-> [cyc_b])     : %s -> \"%s\"\n\n",
           cycle_dep_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));

    /* 9. Verify Independent Parallel In-Memory Sessions & Order-Tagged Raw-Text Snapshots */
    printf("[9] Parallel In-Memory Sessions & Order-Tagged Raw-Text Snapshots (`eng_session_*`):\n");
    int sess_a = eng_session_open(&g_eng);
    int sess_b = eng_session_open(&g_eng);
    bool sess_open_ok = (sess_a >= 0 && sess_b >= 0 && sess_a != sess_b);

    Value va1 = 0, va2 = 0, va3 = 0, vb1 = 0, vb2 = 0;
    /* Interleave evaluations across Session A and Session B */
    bool sa_step1 = eng_session_eval(&g_eng, sess_a,
                                     "dist_x : real, distance := 50m ; factor := 3 ; q := queue<real, distance>(10m, 20m) ; dist_x",
                                     &va1) && (val_get_f64(&g_eng.mem, va1) == 50.0);
    bool sb_step1 = eng_session_eval(&g_eng, sess_b,
                                     "dist_x := 999 ; pts := stack<vec2>(vec2(1, 2), vec2(5, 8)) ; dist_x",
                                     &vb1) && (val_get_f64(&g_eng.mem, vb1) == 999.0);
    bool sa_step2 = eng_session_eval(&g_eng, sess_a,
                                     "scale_fn := a => a * factor ; q.enqueue(30m) ; dist_x += q.dequeue() ; dist_x",
                                     &va2) && (val_get_f64(&g_eng.mem, va2) == 60.0);
    bool sb_step2 = eng_session_eval(&g_eng, sess_b,
                                     "top_pt := pts.pop() ; dist_x += top_pt.y ; dist_x",
                                     &vb2) && (val_get_f64(&g_eng.mem, vb2) == 1007.0);

    /* Load a new user module while sessions are live -> independent of sessions! */
    bool sess_indep_mod = user_compile_ext(&g_eng, "[sess_helper_mod]\n<[arith]\nboost_ten(x) = x + 10\n", NULL, NULL);
    bool sa_step3 = eng_session_eval(&g_eng, sess_a,
                                     "factor = 4 ; boost_ten(scale_fn(5))",
                                     &va3) && (val_get_f64(&g_eng.mem, va3) == 30.0);

    /* Verify stateless `eng_eval` remains isolated from session variables, and type/unit locks persist in Session A */
    bool stateless_isolated = !eng_eval(&g_eng, "dist_x", &v);
    bool sa_lock_enforced   = !eng_session_eval(&g_eng, sess_a, "dist_x += 60 deg", &v);
    bool parallel_sess_ok   = sess_open_ok && sa_step1 && sb_step1 && sa_step2 && sb_step2 &&
                              sess_indep_mod && sa_step3 && stateless_isolated && sa_lock_enforced &&
                              (eng_session_snapshot_count(&g_eng, sess_a) == 3) &&
                              (eng_session_snapshot_count(&g_eng, sess_b) == 2);
    printf("    - Parallel isolated sessions (Session A & Session B)   : %s (sA=30, sB=1007, locks & closures preserved)\n",
           parallel_sess_ok ? "PASS" : "FAIL");

    /* Iteratively extract raw-text order-tagged snapshots from Session A */
    char saved_snaps[3][ENG_SNAPSHOT_MAX_LEN];
    uint16_t iter_cnt = 0;
    const char *raw_snap = NULL;
    while ((raw_snap = eng_session_snapshot_next(&g_eng, sess_a)) != NULL && iter_cnt < 3) {
        snprintf(saved_snaps[iter_cnt], ENG_SNAPSHOT_MAX_LEN, "%s", raw_snap);
        printf("      Snapshot[%u] : %s\n", (unsigned)(iter_cnt + 1), saved_snaps[iter_cnt]);
        iter_cnt++;
    }
    bool snap_iter_ok = (iter_cnt == 3) &&
                        (strncmp(saved_snaps[0], "MSP1|seq:1|", 11) == 0) &&
                        (strncmp(saved_snaps[1], "MSP1|seq:2|", 11) == 0) &&
                        (strncmp(saved_snaps[2], "MSP1|seq:3|", 11) == 0);

    /* Close Session A in memory, then restore it from out-of-order snapshots [seq:3, seq:1, seq:2] */
    (void)eng_session_close(&g_eng, sess_a);
    const char *shuffled_snaps[3] = { saved_snaps[2], saved_snaps[0], saved_snaps[1] };
    int sess_restored = eng_session_restore(&g_eng, shuffled_snaps, 3);
    Value vr1 = 0, vr2 = 0;
    bool r_ok1 = (sess_restored >= 0) &&
                 eng_session_eval(&g_eng, sess_restored, "dist_x + q.dequeue()", &vr1) &&
                 (val_get_f64(&g_eng.mem, vr1) == 80.0) &&
                 (strcmp(eng_unit_name(&g_eng, eng_val_unit(&g_eng, vr1), (char[32]){0}, 32), "distance") == 0);
    bool r_ok2 = r_ok1 &&
                 eng_session_eval(&g_eng, sess_restored, "scale_fn(10) + (q.dequeue() to_m)", &vr2) &&
                 (val_get_f64(&g_eng.mem, vr2) == 70.0) &&
                 (eng_session_snapshot_count(&g_eng, sess_restored) == 5);
    bool restore_continue_ok = r_ok1 && r_ok2;
    printf("    - Iterative snapshot export & out-of-order restore     : %s (continued seq:4->80m, seq:5->70)\n",
           (snap_iter_ok && restore_continue_ok) ? "PASS" : "FAIL");

    /* Verify incomplete/missing/cross-session/tampered snapshots are rejected */
    const char *missing_seq2[2] = { saved_snaps[0], saved_snaps[2] };
    const char *missing_seq1[2] = { saved_snaps[1], saved_snaps[2] };
    const char *cross_sess[2]   = { saved_snaps[0], eng_session_snapshot_at(&g_eng, sess_b, 1) };
    char tampered_snap[ENG_SNAPSHOT_MAX_LEN];
    snprintf(tampered_snap, sizeof(tampered_snap), "%s", saved_snaps[0]);
    tampered_snap[strlen(tampered_snap) - 1] ^= 1;
    const char *tampered_arr[1] = { tampered_snap };

    bool snap_guards_ok = (eng_session_restore(&g_eng, missing_seq2, 2) < 0) &&
                          (eng_session_restore(&g_eng, missing_seq1, 2) < 0) &&
                          (eng_session_restore(&g_eng, cross_sess,   2) < 0) &&
                          (eng_session_restore(&g_eng, tampered_arr, 1) < 0);
    printf("    - Incomplete/gap/cross-session/tampered snapshot guard : %s -> \"%s\"\n\n",
           snap_guards_ok ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));

    (void)eng_session_close(&g_eng, sess_b);
    (void)eng_session_close(&g_eng, sess_restored);

    return (passed == total && blank_ok && load_mathc_ok && vec_type_guard &&
            link_order_ok && dup_mod_guard && dup_def_guard && missing_dep_guard && cycle_dep_guard &&
            parallel_sess_ok && snap_iter_ok && restore_continue_ok && snap_guards_ok &&
            same_line_semi_guard && scope_isolation_guard && multi_else_guard && else_if_guard &&
            kw_var_guard && kw_tag_guard &&
            unlike_unit_guard && concrete_real_guard && concrete_real_dist_unit_guard &&
            concrete_real_dist_iota_guard && concrete_dist_unitless_iota_guard &&
            fn_unit_mismatch_guard && gen_T_unit_mismatch_guard &&
            conv_unit_mismatch_guard && jagged_mat_guard && non_bool_not_guard && not_overload_guard && lim_diverge_guard && custom_err_guard && fx_unknown_ccy_guard && fx_neg_amount_guard &&
            syn_off && units_off && phys_off &&
            bug1_ok && bug2_ok && bug3_ok && bug4_ok && bug5_ok && bug6_ok && bug7_ok && bug8_ok &&
            bug9_ok && bug10_ok) ? 0 : 1;
}