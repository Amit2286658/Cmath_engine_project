/*
 * math_engine.h — Top-Level User Interface Facade Tying All Modules Together
 *
 * Architecture Overview:
 *   User Program (`main.c`)
 *        │
 *        ▼
 *   [math_engine.h / math_engine.c]  (Top-Level Facade / Coordinator)
 *        ├──► [mem_arena.h/.c]       (Isolated Memory Management & 8B/12B Value Packing)
 *        ├──► [sym_table.h/.c]       (Dynamic Types, Flat Structs, Modules & Overloads)
 *        ├──► [lexer.h/.c]           (Pattern-Driven Consuming Types & Maximal-Munch Ops)
 *        ├──► [parser.h/.c]          (Top-Down Operator Precedence Pratt Parser)
 *        ├──► [ext_loader.h/.c]      (External `.math` File & `lib/` Folder Bootstrapper)
 *        └──► [evaluator.h/.c]       (Type-Dispatched AST Evaluator & Private `@` Intrinsics)
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

/* Initialize a blank-slate engine (zero built-in types or operations) */
void        eng_init(Engine *eng);

/* Hook and load all `.math` files from a library folder (`lib/00_bootstrap.math`, etc.) */
bool        eng_boot_dir(Engine *eng, const char *lib_dir_path);

/* Load a single `.math` extension file or source string */
bool        eng_load_ext_file(Engine *eng, const char *filepath);
bool        eng_load_ext(Engine *eng, const char *ext_src);

/* Enable or disable any loaded extension module in O(1) via bitmask flip */
bool        eng_set_module(Engine *eng, const char *mod_name, bool enabled);
bool        eng_is_module_active(const Engine *eng, const char *mod_name);

/* Parse and evaluate an expression string using currently active modules */
bool        eng_eval(Engine *eng, const char *expr, Value *out_val);

/* Format any Value (primitive, string, or nested struct) to a C string buffer */
void        eng_format_value(const Engine *eng, Value val, char *buf, size_t buf_cap);

/* Inspect Value & Engine error state */
uint16_t    eng_val_type(const Engine *eng, Value v);
const char *eng_type_name(const Engine *eng, uint16_t type_id, char *buf, size_t cap);
const char *eng_last_error(const Engine *eng);

#ifdef __cplusplus
}
#endif

#endif /* MATH_ENGINE_H */