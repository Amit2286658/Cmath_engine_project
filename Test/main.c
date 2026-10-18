/*
 * main.c — Frontend Host Program Demonstrating:
 *   1. Internal zero-parse `eng_init()` auto-boot from embedded `stdlib_precompiled.h`
 *      (bootstrap, arith, trig, complex_ext, geom_ext, algebra, units, physics, chemistry)
 *   2. Pure in-memory `user_loader.h` workflow (`user_compile_ext` & `user_load_mathc`)
 *   3. Postfix `"i"` operator (`$iota`) & dimensional unit postfix operators (`deg`, `°`,
 *      `km`, `mph`, `atm`, `kcal`, `eV`, `dB`)
 *   4. Algebraic expressions: 2x2 matrices (`$mat2`), linear systems (`solve_2x2`),
 *      real & complex quadratic roots (`quad_roots`, `quad_croot`), and symbolic/numerical
 *      polynomial algebra (`poly_val`, `poly_deriv`, `poly_add`, `poly_mul`, `poly_newton`)
 *   5. Host Extern Callbacks (`eng_register_extern`):
 *      - `@fx_rate(from, to)`          : Realtime currency conversion
 *      - `@fx_history(from, to, days)` : Vector-returning historical FX series (`$vec`)
 *      - `@kv_set(k, v)` / `@kv_get(k)`: Persistent host key-value store across evals
 *      - `@elem_mass(sym)`             : IUPAC Periodic Table atomic mass lookup
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

/* 2a. Realtime FX Rate & Historical Series Provider (`@fx_rate`, `@fx_history`) */
typedef struct {
    const char *ccy;
    double      per_usd;
} FxQuote;

typedef struct {
    const FxQuote *quotes;
    size_t         count;
} FxRateCache;

static const FxQuote k_live_fx_quotes[] = {
    { "USD",   1.00 },
    { "EUR",   0.92 },
    { "GBP",   0.79 },
    { "INR",  84.00 },
    { "JPY", 150.00 }
};

static const FxRateCache k_fx_cache = {
    k_live_fx_quotes,
    sizeof(k_live_fx_quotes) / sizeof(k_live_fx_quotes[0])
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

/* Returns a dynamic `$vec` of daily historical exchange rates */
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
    /* Generate deterministic daily series ending at `base_rate` (step = 0.5) */
    for (int i = 0; i < days; ++i) {
        series[i] = base_rate - (double)(days - 1 - i) * 0.5;
    }
    *out_val = val_from_f64_vec(mem, series, (uint16_t)days);
    return !mem->has_error;
}

/* 2b. Persistent Host Key-Value Store (`@kv_set`, `@kv_get`) */
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

/* 2c. IUPAC Periodic Table Atomic Mass Lookup (`@elem_mass`) */
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
    eng_register_extern(eng, "fx_rate",    host_fx_rate_cb,    (void *)&k_fx_cache);
    eng_register_extern(eng, "fx_history", host_fx_history_cb, (void *)&k_fx_cache);
    eng_register_extern(eng, "kv_set",     host_kv_set_cb,     (void *)&g_kv_store);
    eng_register_extern(eng, "kv_get",     host_kv_get_cb,     (void *)&g_kv_store);
    eng_register_extern(eng, "elem_mass",  host_elem_mass_cb,  NULL);
}

/* ============================================================================
 * 3. AUTOMATED TEST SUITE (75 TESTS ACROSS ALL 11 MODULES & HOST EXTERNS)
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

    /* J. Host-Registered Extern Callbacks (`@fx_rate`, `@fx_history`, `@kv_set`/`@kv_get`, `@elem_mass`) */
    { "fx_rate(\"USD\", \"INR\")",         "84",   "real", "Host extern '@fx_rate(\"USD\", \"INR\")'" },
    { "convert_fx(250, \"USD\", \"EUR\")", "230",  "real", "User 'convert_fx(250, \"USD\", \"EUR\")'" },
    { "convert_fx(92, \"EUR\", \"INR\")",  "8400", "real", "Cross-currency triangulation EUR -> USD -> INR" },
    { "50 to_inr + (92 to_usd) to_inr",    "12600","real", "Postfix currency ops 'to_inr' and 'to_usd'" },
    { "fx_history(\"USD\", \"INR\", 5)",   "[82, 82.5, 83, 83.5, 84]", "vec",
      "Vector-returning host extern '@fx_history' -> vec<real>" },
    { "fx_sma(\"USD\", \"INR\", 5, 3)",    "83.5", "real",
      "3-day Simple Moving Average over host '@fx_history' vector" },
    { "save_var(\"portfolio_usd\", 150) ; (load_var(\"portfolio_usd\") + 50) to_inr",
      "16800", "real",
      "Host persistent KV store '@kv_set' & '@kv_get' + 'to_inr'" },
    { "molar_mass_h2o() + molar_mass_co2()", "62.024", "real",
      "Host '@elem_mass' Periodic Table: H2O (18.015) + CO2 (44.009)" },
    { NULL, NULL, NULL, NULL }
};

static void print_architecture_report(const Engine *eng) {
    printf("========================================================================\n");
#if USE_TAGGED_12B
    printf("  ENGINE MODE : 12-Byte Tagged Union (USE_TAGGED_12B = 1)\n");
#else
    printf("  ENGINE MODE : 8-Byte IEEE-754 NaN-Boxing (USE_TAGGED_12B = 0)\n");
#endif
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

    /* 3. Frontend reads raw `.math` text, passes ONLY raw text to `user_compile_ext`,
     *    receives back the compiled `.mathc` binary image, and tests `user_load_mathc`. */
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

    /* Frontend saves the returned compiled `.mathc` binary image to its own storage */
    (void)frontend_write_file(mathc_path, compiled_mathc, compiled_mathc_len);

    printf("[3] In-Memory User Extension API (`user_loader.h`):\n");
    printf("    - Step A (`user_compile_ext`): Compiled raw text (%zu bytes) -> handed back %zu-byte `.mathc` image\n",
           raw_len, compiled_mathc_len);

    /* Frontend reloads the compiled `.mathc` binary image into a fresh Engine via `user_load_mathc` */
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
    printf("    - Step B (`user_load_mathc`) : Loaded %zu-byte compiled `.mathc` image in O(1) `memcpy` time\n\n",
           disk_mathc_len);

    print_architecture_report(&g_eng);

    /* 4. Run Automated Expression Test Suite */
    printf("[4] Running Full Automated Suite (Stdlib + Algebra + Units + Physics + Chemistry + Host Externs):\n");
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

    /* 5. Verify Homogeneous Type Safety on Dynamic Vectors (`vec<T>`) */
    bool vec_type_guard = !eng_eval(&g_eng, "v := vec() ; push(v, 10) ; push(v, vec2(1, 2))", &v);
    printf("[5] Dynamic Vector Type-Safety Verification (pushing 'vec2' into 'vec<real>'):\n");
    printf("    - Result: %s -> \"%s\"\n\n",
           vec_type_guard ? "REJECTED AS EXPECTED" : "UNEXPECTED",
           eng_last_error(&g_eng));

    /* 6. Verify O(1) Module Toggling (`boot_syntax`, `units`, `user_physics`) */
    printf("[6] Dynamic O(1) Module Toggling ('boot_syntax', 'units', 'user_physics'):\n");
    eng_set_module(&g_eng, "boot_syntax", false);
    bool syn_off = !eng_eval(&g_eng, "(1 + 2)", &v);
    printf("    - Disable [boot_syntax]  -> eval(\"(1 + 2)\")   : %s (\"%s\")\n",
           syn_off ? "REJECTED ('(' & ')' disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "boot_syntax", true);

    eng_set_module(&g_eng, "units", false);
    bool units_off = !eng_eval(&g_eng, "100 km", &v);
    printf("    - Disable [units]        -> eval(\"100 km\")    : %s (\"%s\")\n",
           units_off ? "REJECTED ('km' disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "units", true);

    eng_set_module(&g_eng, "user_physics", false);
    bool phys_off = !eng_eval(&g_eng, "convert_fx(100, \"USD\", \"INR\")", &v);
    printf("    - Disable [user_physics] -> eval(\"convert_fx\") : %s (\"%s\")\n",
           phys_off ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "user_physics", true);

    return (passed == total && blank_ok && load_mathc_ok && vec_type_guard &&
            syn_off && units_off && phys_off) ? 0 : 1;
}