/*
 * main.c — Frontend Host Program Demonstrating:
 *   1. Internal zero-parse `eng_init()` auto-boot from embedded `stdlib_precompiled.h`
 *   2. Pure in-memory `user_loader.h` workflow (host application handles file I/O
 *      and change detection):
 *      - `user_compile_ext(&g_eng, raw_text, &compiled_data, &compiled_len)`
 *        takes raw `.math` text, hooks `Engine`, and hands back compiled `.mathc` bytes
 *      - `user_load_mathc(&g_eng, compiled_data, compiled_len)`
 *        takes in compiled `.mathc` bytes and loads them into `Engine` in O(1) time
 *   3. Dynamic growable/shrinkable vectors (`$vec`) of any `T` type
 *   4. Full 36-test expression suite + O(1) module toggling
 */

#include "math_engine.h"
#include "user_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Engine g_eng; /* Single static engine context */

/* Host helper: read a file from the host platform into a raw byte/text buffer */
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

/* Host helper: write a compiled `.mathc` buffer to host storage */
static bool frontend_write_file(const char *path, const uint8_t *data, size_t len) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;
    bool ok = (fwrite(data, 1, len, fp) == len);
    fclose(fp);
    return ok;
}

typedef struct {
    const char *expr;
    const char *expected_val;
    const char *expected_type;
    const char *desc;
} TestCase;

static const TestCase k_tests[] = {
    /* Predefined Standard Library Tests (Auto-Loaded by `eng_init`) */
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
    { "1 + 1i",                "1 + 1i",     "complex", "Consuming real + iota -> non-consuming complex" },
    { "(5 + 6i) - (2 + 4i)",   "3 + 2i",     "complex", "Overloaded complex - complex subtraction" },
    { "(1 + 2i) * (3 + 4i)",   "-5 + 10i",   "complex", "Overloaded complex * complex" },
    { "cabs(3 - 4i)",          "5",          "real",    "Complex magnitude on '3 - 4i'" },
    { "\"Euler Formula\"",     "\"Euler Formula\"", "str", "Consuming string type ($str)" },
    { "sin(π / 2)",            "1",          "real",    "UTF-8 constant 'π' + trig intrinsic @sin" },
    { "ln(e ^ 4)",             "4",          "real",    "Constant 'e' + transcendental @ln" },
    { "gcd(1071, 462)",        "21",         "real",    "Multi-instruction GCD using bootstrap ';', ':=', ?\?" },
    { "vec2(5, 7) - vec2(1, 2)", "vec2(x=4, y=5)", "vec2", "C-style struct subtraction overload" },
    { "dot(vec2(3, 4), vec2(2, 1))", "10",   "real",    "Struct field access via bootstrap '.' (@field)" },
    { "move_circle(circle(vec2(1, 2), 5), 10, 20)",
      "circle(center=vec2(x=11, y=22), radius=5)", "circle",
      "Nested struct field mutation via bootstrap '.', '=', and ';'" },
    { "circle(vec2(1, 2), 5).center.y", "2", "real",    "Chained compile-time nested struct offset read" },

    /* User-Defined Extension Tests (`Test/user_physics.math` via `user_loader.h`) */
    { "vec3(1, 2, 3) ** vec3(4, 5, 6)", "32", "real",   "User-defined $vec3 struct & '**' 3D dot product" },
    { "ke(2, 10) + g0",        "109.80665",  "real",    "User-defined 'ke(m,v)' function + 'g0' constant" },
    { "mech_energy(2, 10, 5)", "198.0665",   "real",    "User func 'mech_energy' calling user 'ke' & 'pe'" },
    { "proj_range(9.80665, 45)", "9.80665",  "real",    "User func calling user 'deg2rad' & stdlib 'sin'" },
    { "vec3_mag(vec3(3, 4, 12))", "13",      "real",    "User func calling predefined stdlib 'hypot'" },
    { "lerp(10, 20, 0.25)",    "12.5",       "real",    "User-defined 'lerp(a,b,t)' function" },

    /* Dynamic Vector (`$vec` of any `T` type) Tests */
    { "v := vec() ; push(v, 10) ; push(v, 20) ; push(v, 30) ; v",
      "[10, 20, 30]", "vec",
      "Growable vec<real>: vec(), push(v,x)" },
    { "v := vec(2) ; push(v, 5) ; push(v, 15) ; push(v, 25) ; cap(v)",
      "4", "real",
      "Auto-doubling capacity (cap 2 -> 4 on 3rd push)" },
    { "v := vec(3, 7) ; set(v, 1, 99) ; pop(v) + at(v, 1) + len(v)",
      "108", "real",
      "vec(cnt,init), set(v,i,x), pop(v) -> 7 + 99 + 2" },
    { "v := vec() ; push(v, 10) ; push(v, 20) ; resize(v, 4) ; shrink(v) ; v",
      "[10, 20, 0, 0]", "vec",
      "Dynamic resize(v, 4) zero-fill + shrink(v)" },
    { "pts := vec() ; push(pts, vec2(1, 2)) ; push(pts, vec2(3, 4)) ; at(pts, -1)",
      "vec2(x=3, y=4)", "vec2",
      "Generic vec<vec2> + negative index at(pts, -1)" },
    { "cvec := vec(2, 1 + 2i) ; set(cvec, 1, 3 - 4i) ; cvec",
      "[1 + 2i, 3 - 4i]", "vec",
      "Generic vec<complex> fill & indexed mutation" },
    { "v := vec() ; push(v, 4) ; push(v, 6) ; push(v, 10) ; push(v, 22) ; vec_sum(v)",
      "42", "real",
      "User func 'vec_sum(v)' looping over vec<real>" },
    { "p := vec() ; push(p, vec3(0,0,0)) ; push(p, vec3(3,4,0)) ; push(p, vec3(3,4,12)) ; vec3_path_len(p)",
      "17", "real",
      "User func 'vec3_path_len(p)' over vec<vec3> (5 + 12)" },
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
    printf("  sizeof(SymTable)     : %5zu bytes (%.2f KB symbol/type registry)\n",
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
    printf("  Registered Overloads : %u (%u AST nodes, %u f64 pool slots)\n",
           eng->sym.overload_count, eng->mem.perm_ast_count, eng->mem.perm_pool_top);
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
    printf("    Predefined bootstrap & stdlib loaded from memory (%u modules, %u overloads, %u AST nodes).\n\n",
           g_eng.sym.mod_count, g_eng.sym.overload_count, g_eng.mem.perm_ast_count);

    /* 3. Host application reads raw `.math` text (from `Test/user_physics.math` or
     *    `user_physics.math`), passes ONLY raw text to `user_compile_ext`, receives
     *    back the compiled `.mathc` binary image, and tests `user_load_mathc`. */
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
        fprintf(stderr, "Host error: could not read 'Test/user_physics.math'\n");
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

    /* Host application saves the returned compiled `.mathc` binary image */
    (void)frontend_write_file(mathc_path, compiled_mathc, compiled_mathc_len);

    printf("[3] In-Memory User Extension API (`user_loader.h`):\n");
    printf("    - Step A (`user_compile_ext`): Compiled raw text (%zu bytes) -> handed back %zu-byte `.mathc` image\n",
           raw_len, compiled_mathc_len);

    /* Host application reloads the compiled `.mathc` binary image into a fresh Engine via `user_load_mathc` */
    size_t disk_mathc_len = 0;
    uint8_t *disk_mathc = frontend_read_file(mathc_path, &disk_mathc_len);
    eng_init(&g_eng);
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

    /* 4. Run Automated Expression Test Suite (22 stdlib + 6 physics + 8 generic vector tests) */
    printf("[4] Running Automated Stdlib, User Extension & Generic Vector Suite:\n");
    int passed = 0, total = 0;
    for (int i = 0; k_tests[i].expr != NULL; ++i) {
        total++;
        Value out;
        char val_buf[128], type_buf[32];
        if (!eng_eval(&g_eng, k_tests[i].expr, &out)) {
            printf("  [FAIL] %-44s -> ERROR: %s\n", k_tests[i].expr, eng_last_error(&g_eng));
            continue;
        }
        eng_format_value(&g_eng, out, val_buf, sizeof(val_buf));
        eng_type_name(&g_eng, eng_val_type(&g_eng, out), type_buf, sizeof(type_buf));

        bool ok = (strcmp(val_buf, k_tests[i].expected_val) == 0) &&
                  (strcmp(type_buf, k_tests[i].expected_type) == 0);
        if (ok) passed++;
        printf("  [%s] %-44s => %-38s : %-7s (%s)\n",
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

    /* 6. Verify O(1) Module Toggling (`boot_syntax`, `boot_control`, `user_physics`) */
    printf("[6] Dynamic O(1) Module Toggling ('boot_syntax', 'boot_control', 'user_physics'):\n");
    eng_set_module(&g_eng, "boot_syntax", false);
    bool syn_off = !eng_eval(&g_eng, "(1 + 2)", &v);
    printf("    - Disable [boot_syntax]  -> eval(\"(1 + 2)\")   : %s (\"%s\")\n",
           syn_off ? "REJECTED ('(' & ')' disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "boot_syntax", true);

    eng_set_module(&g_eng, "boot_control", false);
    bool ctrl_off = !eng_eval(&g_eng, "5!", &v);
    printf("    - Disable [boot_control] -> eval(\"5!\")        : %s (\"%s\")\n",
           ctrl_off ? "REJECTED (';', '.', & loop disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "boot_control", true);

    eng_set_module(&g_eng, "user_physics", false);
    bool phys_off = !eng_eval(&g_eng, "ke(2, 10)", &v);
    printf("    - Disable [user_physics] -> eval(\"ke(2, 10)\") : %s (\"%s\")\n",
           phys_off ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "user_physics", true);

    return (passed == total && blank_ok && load_mathc_ok && vec_type_guard &&
            syn_off && ctrl_off && phys_off) ? 0 : 1;
}