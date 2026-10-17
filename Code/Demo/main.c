/*
 * main.c — User Program Interacting Exclusively with the `math_engine.h` Top-Level Interface
 *
 * Build 8-byte IEEE-754 NaN-Boxed Engine (Default):
 *   gcc -O2 -Wall -Wextra -DUSE_TAGGED_12B=0 \
 *       mem_arena.c sym_table.c lexer.c parser.c ext_loader.c evaluator.c math_engine.c main.c \
 *       -lm -o math_8b
 *
 * Build 12-byte Tagged Union Engine:
 *   gcc -O2 -Wall -Wextra -DUSE_TAGGED_12B=1 \
 *       mem_arena.c sym_table.c lexer.c parser.c ext_loader.c evaluator.c math_engine.c main.c \
 *       -lm -o math_12b
 */

#include "../Core/Headers/math_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Engine g_eng; /* Single static engine context */

typedef struct {
    const char *expr;
    const char *expected_val;
    const char *expected_type;
    const char *desc;
} TestCase;

static const TestCase k_tests[] = {
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
    printf("  sizeof(MemArena)     : %5zu bytes (%.2f KB isolated memory module)\n",
           sizeof(MemArena), (double)sizeof(MemArena) / 1024.0);
    printf("  sizeof(SymTable)     : %5zu bytes (%.2f KB symbol/type registry)\n",
           sizeof(SymTable), (double)sizeof(SymTable) / 1024.0);
    printf("  sizeof(Engine) Total : %5zu bytes (%.2f KB)\n",
           sizeof(Engine), (double)sizeof(Engine) / 1024.0);
    printf("  Hooked Modules       : %u (active mask = 0x%02llX)\n    -> ",
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

int main(int argc, char **argv) {
    eng_init(&g_eng);

    /* 1. Verify Blank-Slate Guarantee BEFORE hooking `lib/` */
    Value v;
    bool blank_ok = !eng_eval(&g_eng, "1 + 2", &v);
    printf("[1] Blank-Slate Verification (eval \"1 + 2\" before hooking lib/):\n");
    printf("    Result: %s -> \"%s\"\n\n",
           blank_ok ? "EXPECTED REJECTION" : "UNEXPECTED SUCCESS",
           eng_last_error(&g_eng));

    /* 2. Hook and load the modular `lib/` directory via the top-level interface */
    const char *lib_dir = (argc > 1) ? argv[1] : "Library";
    if (!eng_boot_dir(&g_eng, lib_dir)) {
        fprintf(stderr, "Failed to boot library folder '%s': %s\n", lib_dir, eng_last_error(&g_eng));
        return 1;
    }

    print_architecture_report(&g_eng);

    /* 3. Run Automated Expression Test Suite */
    printf("[2] Running Automated Expression & Bootstrap Operator Suite:\n");
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

    /* 4. Verify O(1) Module Toggling (`boot_syntax`, `boot_control` & `complex_ext`) */
    printf("[3] Dynamic O(1) Module Toggling ('boot_syntax', 'boot_control' & 'complex_ext'):\n");
    eng_set_module(&g_eng, "boot_syntax", false);
    bool syn_off = !eng_eval(&g_eng, "(1 + 2)", &v);
    printf("    - Disable [boot_syntax]  -> eval(\"(1 + 2)\") : %s (\"%s\")\n",
           syn_off ? "REJECTED ('(' & ')' disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "boot_syntax", true);

    eng_set_module(&g_eng, "boot_control", false);
    bool ctrl_off = !eng_eval(&g_eng, "5!", &v);
    printf("    - Disable [boot_control] -> eval(\"5!\")      : %s (\"%s\")\n",
           ctrl_off ? "REJECTED (';', '.', & loop disabled!)" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "boot_control", true);
    bool ctrl_on = eng_eval(&g_eng, "5!", &v);
    char rbuf[64];
    eng_format_value(&g_eng, v, rbuf, sizeof(rbuf));
    printf("    - Re-enable [boot_control] -> eval(\"5!\")    : %s (%s)\n",
           ctrl_on ? "RESTORED" : "FAILED", rbuf);

    eng_set_module(&g_eng, "complex_ext", false);
    bool cplx_off = !eng_eval(&g_eng, "1 + 1i", &v);
    printf("    - Disable [complex_ext]  -> eval(\"1 + 1i\")  : %s (\"%s\")\n",
           cplx_off ? "REJECTED AS EXPECTED" : "UNEXPECTED", eng_last_error(&g_eng));
    eng_set_module(&g_eng, "complex_ext", true);

    return (passed == total && blank_ok && syn_off && ctrl_off && ctrl_on && cplx_off) ? 0 : 1;
}