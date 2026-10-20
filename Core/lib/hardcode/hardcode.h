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
 */

#ifndef CORE_HARDCODE_H
#define CORE_HARDCODE_H

#include "../../Headers/sym_table.h"
#include "../../Headers/parser.h"
#include "../../Headers/evaluator.h"

#ifdef __cplusplus
extern "C" {
#endif

/*code section below manages hardcoding reserved control-flow keywords that cannot be used as variable or tag names*/
/*Construct if, or, then, else, default, switch, case, jump, run, break, continue, return*/
bool     hardcode_is_reserved_kw(const SymTable *tab, const MemArena *mem,
                                 const char *name, uint16_t len);

/*code section below manages hardcoding generic entity/unit wildcards and default real type fallback*/
/*Construct T, any, none, default :real*/
uint16_t hardcode_resolve_wildcard_type(const char *name, uint16_t len);
uint8_t  hardcode_resolve_wildcard_unit(const char *name, uint16_t len);
uint16_t hardcode_default_real_type(const MemArena *mem);

/*code section below manages hardcoding automatic overload wrapper synthesis for functions with default parameters*/
/*Construct fn(x, param = default_expr)*/
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

/*code section below manages hardcoding newline as a default ';' statement separator when a new statement begins*/
/*Construct \n (newline statement separator)*/
bool     hardcode_can_start_newline_stmt(const Parser *P);

/*code section below manages hardcoding lookahead for parenthesized lambda parameter lists and vector comprehensions*/
/*Construct (x, y) => expr, () => expr, [expr for var in iterable (if cond)]*/
bool     hardcode_is_paren_lambda(const Parser *P);
bool     hardcode_scan_comprehension_var(const Parser *P,
                                         const char **out_vname, uint16_t *out_vlen);

/*code section below manages hardcoding same-line implicit multiplication rules*/
/*Construct 2x, 2π, 3(x + 1), (a + b)(c + d), 2 sin(x)*/
bool     hardcode_can_start_implicit_mul_rhs(const Parser *P);

/*code section below manages hardcoding assignment (':=', '=') and distributed relational comparison ('&', '|') detection*/
/*Construct :=, =, +=..^=, (x & y == 1 | 2), case > 10:*/
bool     hardcode_is_plain_store_sym(const MemArena *mem, const SymTable *tab,
                                     uint16_t sym_id, bool *out_is_colon_eq);
bool     hardcode_is_rel_comparison_op(const MemArena *mem, const SymTable *tab,
                                       uint16_t sym_id);
bool     hardcode_is_additive_or_rel_sym(const MemArena *mem, const SymTable *tab,
                                         uint16_t sym_id, OpFixity fixity, bool *out_is_rel);

/*code section below manages hardcoding the differentiating "25km in m" when "m" is also declared a variable or vector, and undefined iterator LHS on 'in'*/
/*Construct 25km in m, (i in 1...5) { ... }, i in 1...5 { ... }*/
bool     hardcode_sym_is_postfix_unit(const SymTable *tab, uint16_t sym_id);
bool     hardcode_is_in_op_sym(const MemArena *mem, const SymTable *tab, uint16_t sym_id);
bool     hardcode_ast_is_bare_unit_rhs(const MemArena *mem, uint16_t ast_idx);
bool     hardcode_check_undef_in_lhs(MemArena *mem, const SymTable *tab, uint16_t in_ast_idx);
bool     hardcode_should_use_unit_for_in_rhs(const MemArena *mem, const SymTable *tab,
                                             const AstNode *in_node, Value lhs_val,
                                             Value rhs_val, uint16_t *out_unit_sym);
bool     hardcode_eval_in_vrange_fast(MemArena *mem, const SymTable *tab,
                                      const AstNode *n, uint16_t frame_base,
                                      uint8_t cur_ret_type, Value *out_val);

/*code section below manages hardcoding parameterized container nomenclature and variadic constructors*/
/*Construct vec<T>(...), list<T>(...), stack<T>(...), queue<T>(...), vec(...), list(...), stack(...), queue(...)*/
bool     hardcode_try_parse_container_type_arg(Parser *P, const char *id_str, uint16_t id_len,
                                               uint16_t *out_spec);
bool     hardcode_try_eval_variadic_vec(MemArena *mem, const SymTable *tab,
                                        const AstNode *n, uint16_t frame_base,
                                        uint8_t cur_ret_type, Value *out_val);

/*code section below manages hardcoding range/collection loops, multi-variable destructuring, and pattern-matched 'switch'*/
/*Construct (i in a...b) { ... }, [expr for x in iter], r1, r2 := quad_roots(...), switch (x) { case ... }*/
Value    hardcode_eval_for_in_ast(MemArena *mem, const SymTable *tab,
                                  const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type);
Value    hardcode_eval_unpack_elem_ast(MemArena *mem, const SymTable *tab,
                                       const AstNode *n, uint16_t frame_base);
Value    hardcode_eval_switch_eq_ast(MemArena *mem, const SymTable *tab,
                                     const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type);

#ifdef __cplusplus
}
#endif

#endif /* CORE_HARDCODE_H */
