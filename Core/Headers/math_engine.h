/*
 * math_engine.h — Top-Level Engine Wrapper (Auto-Boots Precompiled Standard Library)
 *
 * Architecture Overview:
 *   Frontend / Host Application (`main.c`)
 *        │   (Handles all platform-specific file I/O, paths & storage)
 *        │
 *        ├──► [math_engine.h / math_engine.c]  (Top-Level Engine Wrapper)
 *        │       │   Automatically boots precompiled bootstrap & standard `.math`
 *        │       │   modules from embedded `stdlib_precompiled.h` in O(1) `memcpy`
 *        │       │   time with zero filesystem or directory access.
 *        │       ├──► [mem_arena.h/.c]   (Isolated Memory Arena & 8B/12B Value Packing)
 *        │       ├──► [sym_table.h/.c]   (Dynamic Types, Flat Structs, Modules & Overloads)
 *        │       ├──► [lexer.h/.c]       (Pattern-Driven Consuming Types & Syntax Tokens)
 *        │       ├──► [parser.h/.c]      (Top-Down Operator Precedence Pratt Parser)
 *        │       ├──► [ext_loader.h/.c]  (In-Memory `.math` Compiler & `.mathc` Serializer)
 *        │       └──► [evaluator.h/.c]   (Type-Dispatched AST Evaluator & `@` Intrinsics)
 *        │
 *        └──► [user_loader.h / user_loader.c]  (In-Memory User Extension Utility)
 *                1. `user_compile_ext`: raw `.math` text -> hooks `Engine` + returns `.mathc` bytes
 *                2. `user_load_mathc` : `.mathc` bytes   -> hooks `Engine` in O(1) `memcpy` time
 */

#ifndef MATH_ENGINE_H
#define MATH_ENGINE_H

#include "mem_arena.h"
#include "sym_table.h"
#include "lexer.h"
#include "parser.h"
#include "ext_loader.h"
#include "evaluator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Unified Engine Context combining the isolated Memory Arena and Symbol Registry */
typedef struct {
    MemArena mem;
    SymTable sym;
} Engine;

/*
 * Initialize the Engine and INTERNALLY load all bootstrap & predefined `.math`
 * modules (`[boot_syntax]`, `[boot_types]`, `[boot_control]`, `[arith_core]`,
 * `[arith_ext]`, `[trig_const]`, `[trig_core]`, `[complex_ext]`, `[geom_ext]`)
 * directly from the embedded `k_stdlib_mathc` image in O(1) `memcpy` time
 * without touching the filesystem.
 */
bool        eng_init(Engine *eng);

/*
 * Initialize a completely blank-slate Engine (zero built-in types or operations).
 * Used internally by `mathc` or for testing the blank-slate guarantee.
 */
void        eng_init_blank(Engine *eng);

/* Enable or disable any loaded module in O(1) via bitmask flip */
bool        eng_set_module(Engine *eng, const char *mod_name, bool enabled);
bool        eng_is_module_active(const Engine *eng, const char *mod_name);

/* Parse and evaluate an expression string using currently active modules */
bool        eng_eval(Engine *eng, const char *expr, Value *out_val);

/* Format any Value (primitive, string, vector, or nested struct) to a C string buffer */
void        eng_format_value(const Engine *eng, Value val, char *buf, size_t buf_cap);

/* Inspect Value & Engine error state */
uint16_t    eng_val_type(const Engine *eng, Value v);
const char *eng_type_name(const Engine *eng, uint16_t type_id, char *buf, size_t cap);
const char *eng_last_error(const Engine *eng);

#ifdef __cplusplus
}
#endif

#endif /* MATH_ENGINE_H */