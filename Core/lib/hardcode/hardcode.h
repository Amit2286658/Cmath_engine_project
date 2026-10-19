/*
 * hardcode.h — Hardcoded Language Grammar, Ergonomic Rules & Built-In Conventions
 *
 * Location: Core/lib/hardcode/hardcode.h
 *
 * Purpose:
 *   Isolates all hardcoded C language rules and ergonomic mechanics so they are
 *   explicitly separated from:
 *     - `Core/lib/delegate/` : `@<intrinsic>` C delegate implementations
 *     - `Library/`           : Extension-defined operators, types, units & functions
 *
 *   Hardcoded responsibilities housed here:
 *     1. Active reserved control-flow keyword checks (`if`, `or`, `then`, `else`,
 *        `default`, `switch`, `case`, `loop`, `jump`, `run`, `break`)
 *     2. Generic type/unit wildcards (`T`, `any`) and unitless lock (`none`),
 *        plus default `:real` / `-> real` type fallback for shorthand declarations
 *     3. Default parameter overload wrapper synthesis (`fn(x, digits = 0)`)
 *     4. Keywordless `(i in 1...5) { ... }` / `(v in vec) { ... }` lookahead
 *        and zero-allocation `..` / `...` range loop detection (`AST_FOR_IN`)
 *     5. Natural implicit multiplication (`2x`, `2π`, `3(x + 1)`, `2 sin(x)`)
 *     6. Distributed relational comparison detection (`(x & y == 1 | 2)`) and
 *        plain store operator detection (`:=` vs `=`)
 *     7. Variadic `vec(a, b, c, ...)` (3+ args) & `list(a, ...)` (1+ args) dispatch
 *     8. Struct, vector, and matrix destructuring (`r1, r2 := quad_roots(...)`)
 */

#ifndef CORE_HARDCODE_H
#define CORE_HARDCODE_H

#include "../../Headers/sym_table.h"
#include "../../Headers/parser.h"
#include "../../Headers/evaluator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1. Reserved Keyword & Built-In Wildcard / Default Type Helpers */
bool     hardcode_is_reserved_kw(const SymTable *tab, const MemArena *mem,
                                 const char *name, uint16_t len);
uint16_t hardcode_resolve_wildcard_type(const char *name, uint16_t len);
uint8_t  hardcode_resolve_wildcard_unit(const char *name, uint16_t len);
uint16_t hardcode_default_real_type(const MemArena *mem);

/* 2. Default Parameter Overload Wrapper Synthesis (`param = default_expr`) */
void     hardcode_synthesize_default_overloads(MemArena *mem, SymTable *tab,
                                               const char *src, uint16_t sym_id,
                                               uint8_t cur_mod, uint16_t base_ov_idx,
                                               uint8_t pcount, uint8_t req_count,
                                               const StrSlice *pnames,
                                               const uint16_t *ptypes,
                                               const uint8_t  *punits,
                                               const uint16_t *def_start,
                                               const uint16_t *def_end,
                                               uint16_t rtid, uint8_t ruid);

/* 3. Parser Hardcoded Ergonomics (Keywordless `(i in ...){}`, Implicit `*`, Operators) */
bool     hardcode_is_paren_for_in(const Parser *P, bool allow_any_body);
bool     hardcode_can_start_implicit_mul_rhs(const Parser *P);
bool     hardcode_is_plain_store_sym(const MemArena *mem, const SymTable *tab,
                                     uint16_t sym_id, bool *out_is_colon_eq);
bool     hardcode_is_rel_comparison_op(const MemArena *mem, const SymTable *tab,
                                       uint16_t sym_id);
bool     hardcode_is_additive_or_rel_sym(const MemArena *mem, const SymTable *tab,
                                         uint16_t sym_id, OpFixity fixity, bool *out_is_rel);

/* 4. Evaluator Hardcoded Ergonomics (Variadic `vec`/`list`, `AST_FOR_IN`, `AST_UNPACK_ELEM`) */
bool     hardcode_try_eval_variadic_vec(MemArena *mem, const SymTable *tab,
                                        const AstNode *n, uint16_t frame_base,
                                        uint8_t cur_ret_type, Value *out_val);
Value    hardcode_eval_for_in_ast(MemArena *mem, const SymTable *tab,
                                  const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type);
Value    hardcode_eval_unpack_elem_ast(MemArena *mem, const SymTable *tab,
                                       const AstNode *n, uint16_t frame_base);

#ifdef __cplusplus
}
#endif

#endif /* CORE_HARDCODE_H */
