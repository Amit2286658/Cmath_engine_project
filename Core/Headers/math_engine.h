/*
 * math_engine.h — Top-Level Engine Wrapper (Auto-Boots Precompiled Standard Library)
 *
 * Architecture Overview:
 *   User Program (`Test/main.c`)
 *        │
 *        ├──► [Core/Headers + Core/Code]  (Core Engine Pipeline)
 *        │       ├──► [math_engine.h/.c]  Top-Level Engine Wrapper & Stateless Eval
 *        │       ├──► [mem_arena.h/.c]    Isolated Memory Arena & 8B NaN-Boxed Values
 *        │       ├──► [sym_table.h/.c]    Dynamic Types, Units, Structs, Modules, Overloads
 *        │       ├──► [lexer.h/.c]        Pattern-Driven Consuming Types & Syntax Tokens
 *        │       ├──► [parser.h/.c]       Top-Down Operator Precedence Pratt Parser
 *        │       └──► [evaluator.h/.c]    Type- & Unit-Dispatched AST Evaluator
 *        │
 *        ├──► [Core/compile]              (Compilation, Linking, `.mathc` Binary & User Loader)
 *        │       ├──► [compiler.h/.c]     Module Unit Stager & Declaration Compiler
 *        │       ├──► [linker.h/.c]       Topological `<[dep]` Dependency Linker
 *        │       ├──► [mathc_bin.h/.c]    `.mathc` Binary Serializer, Validator & Loader
 *        │       ├──► [user_loader.h/.c]  Host-Facing User Extension Compile/Link/Load API
 *        │       └──► [ext_loader.h]      Umbrella Header for `compiler`, `linker`, `mathc_bin`
 *        │
 *        ├──► [Core/session]              (Parallel In-Memory Sessions & Snapshots)
 *        │       ├──► [session.h/.c]      Parallel Session Lifecycle & `eng_session_eval`
 *        │       └──► [snapshot.h/.c]     Order-Tagged Raw-Text Snapshots & `eng_session_restore`
 *        │
 *        └──► [Core/lib]                  (Delegates & Hardcoded Grammar Rules)
 *                ├──► [delegate/delegate.h/.c]  C `@<intrinsic>` Implementations
 *                └──► [hardcode/hardcode.h/.c]  Isolated Hardcoded Grammar & Ergonomics
 */

#ifndef MATH_ENGINE_H
#define MATH_ENGINE_H

#include "mem_arena.h"
#include "sym_table.h"
#include "lexer.h"
#include "parser.h"
#include "ext_loader.h"
#include "evaluator.h"
#include "session.h"
#include "snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Unified Engine Context combining Arena, Symbol Registry & Parallel Sessions */
struct Engine {
    MemArena   mem;
    SymTable   sym;
    EngSession sessions[ENG_MAX_SESSIONS];
};

/* ============================================================================
 * 1. CORE ENGINE INITIALIZATION, MODULE TOGGLING & STATELESS EVALUATION
 * ============================================================================ */

/*
 * Initialize the Engine and INTERNALLY load all bootstrap & predefined `.math`
 * modules (`[boot_syntax]`, `[boot_types]`, `[boot_control]`, `[arith]`,
 * `[trig]`, `[complex_ext]`, `[geom_ext]`, `[algebra]`, `[calculus]`, `[units]`,
 * `[conversions]`, `[physics]`, `[chemistry]`, `[biology]`) from the embedded
 * precompiled `.mathc` image in O(1) `memcpy` time with zero filesystem access.
 */
bool        eng_init(Engine *eng);

/*
 * Initialize a completely blank-slate Engine (zero built-in types or operations).
 * Used internally by `mathc` or for testing the blank-slate guarantee.
 */
void        eng_init_blank(Engine *eng);

/*
 * Register a platform-provided C callback (`fn`) under `@<name>(...)` so `.math`
 * extensions can safely invoke host services without coupling `Core/` to OS libraries.
 */
bool        eng_register_extern(Engine *eng, const char *name,
                                EngExternFn fn, void *user_ctx);

/* Enable or disable any loaded module in O(1) via bitmask flip */
bool        eng_set_module(Engine *eng, const char *mod_name, bool enabled);
bool        eng_is_module_active(const Engine *eng, const char *mod_name);

/* Stateless one-shot evaluation: parse and evaluate an expression string */
bool        eng_eval(Engine *eng, const char *expr, Value *out_val);

/* ============================================================================
 * 2. VALUE FORMATTING & TYPE / UNIT / ERROR INTROSPECTION
 * ============================================================================ */

/* Format any Value (primitive, string, vector, matrix, or nested struct) to a C string */
void        eng_format_value(const Engine *eng, Value val, char *buf, size_t buf_cap);

/* Format a multidimensional matrix at `hdr_idx` (shared with `Core/session/snapshot.c`) */
void        eng_format_mat_at(const Engine *eng, uint16_t hdr_idx, bool high_prec,
                              char *buf, size_t cap);

/* Inspect Value Entity Type, Unit Type & Engine error state */
uint16_t    eng_val_type(const Engine *eng, Value v);
uint8_t     eng_val_unit(const Engine *eng, Value v);
const char *eng_type_name(const Engine *eng, uint16_t type_id, char *buf, size_t cap);
const char *eng_unit_name(const Engine *eng, uint8_t unit_id, char *buf, size_t cap);
const char *eng_last_error(const Engine *eng);

#ifdef __cplusplus
}
#endif

#endif /* MATH_ENGINE_H */
