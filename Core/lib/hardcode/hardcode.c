/*
 * hardcode.c — Hardcoded Language Grammar, Ergonomic Rules & Built-In Conventions
 *
 * Location: Core/lib/hardcode/hardcode.c
 */

#include "hardcode.h"
#include "../delegate/delegate.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * 1. RESERVED KEYWORDS & BUILT-IN TYPE/UNIT WILDCARDS (`T`, `any`, `none`, `real`)
 * ============================================================================ */

/*code section below manages hardcoding reserved control-flow keywords that cannot be used as variable or tag names*/
/*Construct if, or, then, else, default, switch, case, jump, run, break, continue, return*/
bool hardcode_is_reserved_kw(const SymTable *tab, const MemArena *mem,
                             const char *name, uint16_t len) {
    int sid = sym_find(tab, mem, name, len);
    if (sid < 0) return false;
    uint8_t m = sym_active_meta_tag(tab, (uint16_t)sid, FIX_FUNC);
    return (m == INTR_IF            ||
            m == INTR_OR_BRANCH     ||
            m == INTR_ELSE_BRANCH   ||
            m == INTR_SWITCH_BLOCK  ||
            m == INTR_CASE_BRANCH   ||
            m == INTR_JUMP_TAG      ||
            m == INTR_RUN_TAG       ||
            m == INTR_BREAK_LOOP    ||
            m == INTR_CONTINUE_LOOP ||
            m == INTR_RETURN_FN);
}

/*code section below manages hardcoding generic entity/unit wildcards and default real type fallback*/
/*Construct T, any, none, default :real*/
uint16_t hardcode_resolve_wildcard_type(const char *name, uint16_t len) {
    if ((len == 1 && name[0] == 'T') || (len == 3 && memcmp(name, "any", 3) == 0)) {
        return SYM_TYPE_ANY;
    }
    return MEM_TYPE_VOID;
}

uint8_t hardcode_resolve_wildcard_unit(const char *name, uint16_t len) {
    if ((len == 1 && name[0] == 'T') || (len == 3 && memcmp(name, "any", 3) == 0)) {
        return SYM_UNIT_GENERIC;
    }
    if (len == 4 && memcmp(name, "none", 4) == 0) {
        return SYM_UNIT_LOCK_NONE;
    }
    return MEM_UNIT_NONE;
}

uint16_t hardcode_default_real_type(const MemArena *mem) {
    return mem->fast_real_type ? mem->fast_real_type : 1;
}

/* ============================================================================
 * 2. DEFAULT PARAMETER OVERLOAD WRAPPER SYNTHESIS (`param = default_expr`)
 * ============================================================================ */

/*code section below manages hardcoding automatic overload wrapper synthesis for functions with default parameters*/
/*Construct fn(x, param = default_expr)*/
static int find_active_meta_sym_hc(const SymTable *tab, uint8_t target_meta) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->meta_tag == target_meta && sym_mod_enabled(tab, ov->mod_id)) {
            return (int)ov->sym_id;
        }
    }
    return -1;
}

void hardcode_synthesize_default_overloads(MemArena *mem, SymTable *tab,
                                           const char *src, uint16_t sym_id,
                                           uint8_t cur_mod, uint16_t base_ov_idx,
                                           uint8_t pcount, uint8_t req_count,
                                           const StrSlice *pnames,
                                           const uint16_t *ptypes,
                                           const uint8_t  *punits,
                                           const uint16_t *def_start,
                                           const uint16_t *def_end,
                                           uint16_t rtid, uint8_t ruid) {
    int store_sym = find_active_meta_sym_hc(tab, INTR_STORE);
    int seq_sym   = find_active_meta_sym_hc(tab, INTR_SEQ);
    if (store_sym < 0 || seq_sym < 0) {
        mem_err(mem, "Default parameters require active assignment and sequence operators");
        return;
    }

    OpOverload *base_ov  = &tab->overloads[base_ov_idx];
    uint16_t full_body   = base_ov->body_ast;
    uint8_t  full_locals = base_ov->local_count;
    uint16_t def_asts[8];
    memset(def_asts, 0xFF, sizeof(def_asts));

    for (uint8_t k = req_count; k < pcount; ++k) {
        Parser DP;
        parser_init(&DP, mem, tab, src, def_end[k], def_start[k], true, false);
        DP.param_count = pcount;
        for (uint8_t i = 0; i < pcount; ++i) {
            parser_add_local(&DP, mem_str_ptr(mem, pnames[i]), pnames[i].len, ptypes[i]);
        }
        if (DP.local_count < full_locals) {
            DP.local_count = full_locals;
        }
        lex_next(&DP.lex);
        def_asts[k] = parser_parse_expr(&DP, 0);
        if (mem->has_error || def_asts[k] == MEM_NO_IDX) return;
        if (DP.local_count > full_locals) {
            full_locals = DP.local_count;
        }
    }
    base_ov->local_count = full_locals;

    for (uint8_t m = req_count; m < pcount; ++m) {
        if (tab->overload_count >= SYM_MAX_OVERLOADS) {
            mem_err(mem, "Overload table overflow (max %d)", SYM_MAX_OVERLOADS);
            return;
        }
        uint16_t wrapper_ast = full_body;
        for (int k = (int)pcount - 1; k >= (int)m; --k) {
            uint16_t slot_k  = mem_alloc_ast(mem, AST_SLOT_LOAD, (uint8_t)k, 0, 0, 0);
            uint16_t store_k = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_RIGHT,
                                             (uint16_t)store_sym, slot_k, def_asts[k]);
            wrapper_ast      = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                                             (uint16_t)seq_sym, store_k, wrapper_ast);
        }
        if (mem->has_error) return;

        uint16_t dov_idx = tab->overload_count++;
        OpOverload *dov  = &tab->overloads[dov_idx];
        memset(dov, 0, sizeof(*dov));
        dov->sym_id      = sym_id;
        dov->mod_id      = cur_mod;
        dov->fixity      = FIX_FUNC;
        dov->prec        = 0;
        dov->param_count = m;
        dov->local_count = full_locals;
        dov->ret_type    = (uint8_t)rtid;
        dov->ret_unit    = ruid;
        for (uint8_t i = 0; i < m; ++i) {
            dov->param_types[i] = (uint8_t)ptypes[i];
            dov->param_units[i] = punits[i];
        }
        dov->body_ast = wrapper_ast;
        sym_link_overload(tab, dov_idx);
    }
}

/* ============================================================================
 * 3. PARSER HARDCODED ERGONOMICS (NEWLINE `;`, CONTEXT-AWARE `in`, LAMBDAS)
 * ============================================================================ */

static uint16_t skip_raw_ws_hc(const char *src, uint16_t p, uint16_t len) {
    while (p < len) {
        char c = src[p];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            p++;
        } else if (c == '#') {
            while (p < len && src[p] != '\n') p++;
        } else {
            break;
        }
    }
    return p;
}

static uint8_t token_func_meta_hc(const Parser *P, Token t) {
    const SymTable *tab = P->lex.tab;
    const MemArena *mem = P->lex.mem;
    if (t.kind == TOK_OP) {
        return sym_active_meta_tag(tab, t.sym_id, FIX_FUNC);
    }
    if (t.kind == TOK_IDENT) {
        int sid = sym_find(tab, mem, P->lex.src + t.off, t.len);
        if (sid >= 0) {
            return sym_active_meta_tag(tab, (uint16_t)sid, FIX_FUNC);
        }
    }
    return 0;
}

/*code section below manages hardcoding newline as a default ';' statement separator when a new statement begins*/
/*Construct \n (newline statement separator)*/
bool hardcode_can_start_newline_stmt(const Parser *P) {
    const SymTable *tab = P->lex.tab;
    Token t = P->lex.cur;

    if (!t.had_newline) return false;
    if (t.kind == TOK_EOF    ||
        t.kind == TOK_RPAREN ||
        t.kind == TOK_RBRACE ||
        t.kind == TOK_RBRACK ||
        t.kind == TOK_COMMA) {
        return false;
    }

    uint8_t kw = token_func_meta_hc(P, t);
    if (kw == INTR_OR_BRANCH || kw == INTR_ELSE_BRANCH || kw == INTR_CASE_BRANCH) {
        return false;
    }

    if (t.kind == TOK_VAL    ||
        t.kind == TOK_IDENT  ||
        t.kind == TOK_LPAREN ||
        t.kind == TOK_LBRACE ||
        t.kind == TOK_LBRACK ||
        t.kind == TOK_AT) {
        return true;
    }

    if (t.kind == TOK_OP) {
        if (sym_active_meta_tag(tab, t.sym_id, FIX_INFIX_LEFT) == INTR_SEQ) {
            return true;
        }
        if (sym_get_active_prec(tab, t.sym_id, FIX_PREFIX) >= 0) {
            return true;
        }
        if (lex_is_ident_start((unsigned char)P->lex.src[t.off])) {
            return true;
        }
    }
    return false;
}

/*code section below manages hardcoding the check for active postfix physical unit operators*/
/*Construct <expr> <unit> (e.g. 25km, 50m, 90deg)*/
bool hardcode_sym_is_postfix_unit(const SymTable *tab, uint16_t sym_id) {
    if (!tab || sym_id >= tab->sym_count) return false;
    for (uint16_t i = tab->sym_first_ov[sym_id]; i != MEM_NO_IDX; i = tab->ov_next[i]) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->fixity == FIX_POSTFIX && sym_mod_enabled(tab, ov->mod_id) &&
            ov->ret_unit != 0 && ov->ret_unit < SYM_UNIT_LOCK_NONE) {
            return true;
        }
    }
    return false;
}

/*code section below manages hardcoding the detection of the 'in' operator symbol*/
/*Construct <lhs> in <rhs>*/
bool hardcode_is_in_op_sym(const MemArena *mem, const SymTable *tab, uint16_t sym_id) {
    if (!mem || !tab || sym_id >= tab->sym_count) return false;
    for (uint16_t i = tab->sym_first_ov[sym_id]; i != MEM_NO_IDX; i = tab->ov_next[i]) {
        const OpOverload *ov = &tab->overloads[i];
        if ((ov->fixity == FIX_INFIX_LEFT || ov->fixity == FIX_INFIX_RIGHT) &&
            sym_mod_enabled(tab, ov->mod_id) && ov->body_ast != MEM_NO_IDX) {
            const AstNode *b = mem_get_ast(mem, ov->body_ast);
            if (b && b->kind == AST_INTRINSIC && b->sub == INTR_VIN) {
                return true;
            }
        }
    }
    StrSlice s = tab->syms[sym_id];
    return (s.len == 2 && memcmp(mem_str_ptr(mem, s), "in", 2) == 0);
}

/*code section below manages hardcoding the detection of bare unit conversion RHS on 'in' ("25km in m")*/
/*Construct <expr> in <unit>*/
bool hardcode_ast_is_bare_unit_rhs(const MemArena *mem, uint16_t ast_idx) {
    if (!mem || ast_idx == MEM_NO_IDX) return false;
    const AstNode *n = mem_get_ast(mem, ast_idx);
    return (n && n->kind == AST_UNARY && n->sub == (uint8_t)FIX_POSTFIX && n->right == 1);
}

/*code section below manages hardcoding undefined iterator variable validation on the LHS of 'in' when not followed by a loop body*/
/*Construct (i in 1...5) { ... } vs i in 1...5 { ... }*/
bool hardcode_check_undef_in_lhs(MemArena *mem, const SymTable *tab, uint16_t in_ast_idx) {
    if (!mem || !tab || in_ast_idx == MEM_NO_IDX) return true;
    const AstNode *in_node = mem_get_ast(mem, in_ast_idx);
    if (!in_node || in_node->kind != AST_BINARY ||
        !hardcode_is_in_op_sym(mem, tab, in_node->ref)) {
        return true;
    }
    const AstNode *lhs = mem_get_ast(mem, in_node->left);
    if (lhs && lhs->kind == AST_SLOT_LOAD && (lhs->ref & AST_SLOT_UNDEF_ITER_BIT) != 0) {
        uint16_t var_sym = (uint16_t)(lhs->ref & AST_SLOT_SYM_MASK);
        if (var_sym < tab->sym_count) {
            StrSlice vs = tab->syms[var_sym];
            mem_err(mem, "Undefined symbol or constant '%.*s'",
                    (int)vs.len, mem_str_ptr(mem, vs));
        } else {
            mem_err(mem, "Undefined variable on left side of 'in'");
        }
        return false;
    }
    return true;
}

/*code section below manages hardcoding the differentiating "25km in m" when "m" is also declared a variable or vector*/
/*Construct <expr> in <var_or_unit>*/
bool hardcode_should_use_unit_for_in_rhs(const MemArena *mem, const SymTable *tab,
                                         const AstNode *in_node, Value lhs_val,
                                         Value rhs_val, uint16_t *out_unit_sym) {
    if (out_unit_sym) *out_unit_sym = MEM_NO_IDX;
    if (!mem || !tab || !in_node || !hardcode_is_in_op_sym(mem, tab, in_node->ref)) {
        return false;
    }
    const AstNode *rn = mem_get_ast(mem, in_node->right);
    if (!rn || rn->kind != AST_SLOT_LOAD || (rn->ref & AST_SLOT_DUAL_UNIT_BIT) == 0) {
        return false;
    }
    uint16_t unit_sym = (uint16_t)(rn->ref & AST_SLOT_SYM_MASK);
    if (unit_sym >= tab->sym_count) return false;

    uint8_t unit_uid = MEM_UNIT_NONE;
    for (uint16_t i = tab->sym_first_ov[unit_sym]; i != MEM_NO_IDX; i = tab->ov_next[i]) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->fixity == FIX_POSTFIX && sym_mod_enabled(tab, ov->mod_id) &&
            ov->ret_unit != 0 && ov->ret_unit < SYM_UNIT_LOCK_NONE) {
            unit_uid = ov->ret_unit;
            break;
        }
    }
    if (unit_uid == MEM_UNIT_NONE) return false;

    uint8_t lhs_uid = val_get_unit(mem, lhs_val);
    if (lhs_uid == MEM_UNIT_NONE || lhs_uid != unit_uid) {
        return false;
    }

    uint16_t r_tid = val_get_type(mem, rhs_val);
    if (mem->vec_type_id != 0 && r_tid == mem->vec_type_id) {
        uint16_t hdr_idx  = val_get_pool_idx(rhs_val);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
        uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
        uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
        if (len > 0 && elem_uid == lhs_uid) {
            /* `km` is a vector whose elements have the exact same unit as `lhs` -> membership test */
            return false;
        }
        if (out_unit_sym) *out_unit_sym = unit_sym;
        return true;
    }
    if (mem->mat_type_id != 0 && r_tid == mem->mat_type_id) {
        if (val_get_unit(mem, rhs_val) == lhs_uid) {
            return false;
        }
        if (out_unit_sym) *out_unit_sym = unit_sym;
        return true;
    }

    /* Variable holds a scalar (cannot be a collection for membership), so convert to `1 <unit>` */
    if (out_unit_sym) *out_unit_sym = unit_sym;
    return true;
}

/*code section below manages hardcoding lookahead for parenthesized lambda parameter lists*/
/*Construct (x, y) => expr, () => expr*/
bool hardcode_is_paren_lambda(const Parser *P) {
    if (P->lex.cur.kind != TOK_LPAREN) return false;
    const char *src = P->lex.src;
    uint16_t    len = P->lex.len;
    uint16_t    p   = skip_raw_ws_hc(src, P->lex.pos, len);

    if (p < len && src[p] == ')') {
        p = skip_raw_ws_hc(src, p + 1, len);
        return (p + 1 < len && src[p] == '=' && src[p + 1] == '>');
    }

    for (uint8_t k = 0; k < 4; ++k) {
        if (p >= len || !lex_is_ident_start((unsigned char)src[p])) return false;
        while (p < len && lex_is_ident_cont((unsigned char)src[p])) p++;
        p = skip_raw_ws_hc(src, p, len);

        if (p < len && src[p] == ':' && !(p + 1 < len && src[p + 1] == '=')) {
            p = skip_raw_ws_hc(src, p + 1, len);
            if (p >= len || !lex_is_ident_start((unsigned char)src[p])) return false;
            while (p < len && lex_is_ident_cont((unsigned char)src[p])) p++;
            p = skip_raw_ws_hc(src, p, len);
        }

        if (p < len && src[p] == ',') {
            p = skip_raw_ws_hc(src, p + 1, len);
            continue;
        }
        break;
    }

    if (p >= len || src[p] != ')') return false;
    p = skip_raw_ws_hc(src, p + 1, len);
    return (p + 1 < len && src[p] == '=' && src[p + 1] == '>');
}

/*code section below manages hardcoding lookahead for vector comprehensions inside '[ ... ]'*/
/*Construct [expr for var in iterable (if cond)]*/
bool hardcode_scan_comprehension_var(const Parser *P,
                                     const char **out_vname, uint16_t *out_vlen) {
    const char *src = P->lex.src;
    uint16_t    len = P->lex.len;
    uint16_t    p   = P->lex.cur.off;
    int depth = 0;

    while (p < len) {
        char c = src[p];
        if (c == '"') {
            p++;
            while (p < len && src[p] != '"') {
                if (src[p] == '\\' && p + 1 < len) p++;
                p++;
            }
            if (p < len) p++;
            continue;
        }
        if (c == '#') {
            while (p < len && src[p] != '\n') p++;
            continue;
        }
        if (c == '(' || c == '[' || c == '{') {
            depth++;
            p++;
            continue;
        }
        if (c == ')' || c == ']' || c == '}') {
            if (depth == 0) break;
            depth--;
            p++;
            continue;
        }
        if (depth == 0 && (c == ',' || c == ';')) {
            return false;
        }
        if (depth == 0 && lex_is_ident_start((unsigned char)c)) {
            uint16_t id_start = p;
            while (p < len && lex_is_ident_cont((unsigned char)src[p])) p++;
            uint16_t id_len = p - id_start;
            if (id_len == 3 && memcmp(src + id_start, "for", 3) == 0) {
                uint16_t q = skip_raw_ws_hc(src, p, len);
                if (q < len && lex_is_ident_start((unsigned char)src[q])) {
                    uint16_t vstart = q;
                    while (q < len && lex_is_ident_cont((unsigned char)src[q])) q++;
                    uint16_t vlen = q - vstart;
                    q = skip_raw_ws_hc(src, q, len);
                    if (q + 2 <= len && src[q] == 'i' && src[q + 1] == 'n' &&
                        (q + 2 == len || !lex_is_ident_cont((unsigned char)src[q + 2]))) {
                        if (out_vname) *out_vname = src + vstart;
                        if (out_vlen)  *out_vlen  = vlen;
                        return true;
                    }
                }
            }
            continue;
        }
        p++;
    }
    return false;
}

/*code section below manages hardcoding same-line implicit multiplication rules*/
/*Construct 2x, 2π, 3(x + 1), (a + b)(c + d), 2 sin(x)*/
bool hardcode_can_start_implicit_mul_rhs(const Parser *P) {
    const SymTable *tab = P->lex.tab;
    Token t = P->lex.cur;

    /* Never implicitly multiply across a line break */
    if (t.had_newline) return false;

    if (t.kind == TOK_LPAREN) {
        return !hardcode_is_paren_lambda(P);
    }
    if (t.kind == TOK_IDENT) {
        if (t.len == 3 && memcmp(P->lex.src + t.off, "for", 3) == 0) {
            return false;
        }
        uint8_t kw = token_func_meta_hc(P, t);
        if (kw != 0 && (kw < INTR_CALC_DIFF || kw > INTR_CALC_FMAX)) {
            return false;
        }
        /* Do not treat missing semicolon before `y := ...`, `Tag : ...`, `i in ...`, or `x => ...` as implicit mul */
        uint16_t p = skip_raw_ws_hc(P->lex.src, P->lex.pos, P->lex.len);
        if (p < P->lex.len && (P->lex.src[p] == ':' || P->lex.src[p] == '=')) {
            return false;
        }
        if (p + 1 < P->lex.len && P->lex.src[p + 1] == '=' &&
            (P->lex.src[p] == '+' || P->lex.src[p] == '-' || P->lex.src[p] == '*' ||
             P->lex.src[p] == '/' || P->lex.src[p] == '%' || P->lex.src[p] == '^')) {
            return false;
        }
        if (p + 2 <= P->lex.len && P->lex.src[p] == 'i' && P->lex.src[p + 1] == 'n' &&
            (p + 2 == P->lex.len || !lex_is_ident_cont((unsigned char)P->lex.src[p + 2]))) {
            return false;
        }
        return true;
    }
    if (t.kind == TOK_OP) {
        /* Prefix-only non-word operator like `√` in `2 √16` */
        if (sym_get_active_prec(tab, t.sym_id, FIX_POSTFIX) < 0 &&
            sym_get_active_prec(tab, t.sym_id, FIX_INFIX_LEFT) < 0 &&
            sym_get_active_prec(tab, t.sym_id, FIX_INFIX_RIGHT) < 0 &&
            sym_get_active_prec(tab, t.sym_id, FIX_PREFIX) >= 0 &&
            !((P->lex.src[t.off] >= 'a' && P->lex.src[t.off] <= 'z') ||
              (P->lex.src[t.off] >= 'A' && P->lex.src[t.off] <= 'Z') ||
              P->lex.src[t.off] == '_')) {
            return true;
        }
    }
    return false;
}

/*code section below manages hardcoding plain assignment (':=', '=') vs compound assignment ('+=', '-=', etc.) detection*/
/*Construct :=, =, +=, -=, *=, /=, %=, ^=*/
bool hardcode_is_plain_store_sym(const MemArena *mem, const SymTable *tab,
                                 uint16_t sym_id, bool *out_is_colon_eq) {
    StrSlice s = tab->syms[sym_id];
    const char *ptr = mem_str_ptr(mem, s);
    bool colon_eq = (s.len == 2 && memcmp(ptr, ":=", 2) == 0);
    if (out_is_colon_eq) *out_is_colon_eq = colon_eq;
    return colon_eq || (s.len == 1 && ptr[0] == '=');
}

/*code section below manages hardcoding relational comparison operator detection for distributed '&' and '|' comparisons*/
/*Construct (x & y == 1 | 2), case > 10:*/
bool hardcode_is_rel_comparison_op(const MemArena *mem, const SymTable *tab,
                                   uint16_t sym_id) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->param_count == 2 &&
            sym_mod_enabled(tab, ov->mod_id) && ov->body_ast != MEM_NO_IDX) {
            const AstNode *root = mem_get_ast(mem, ov->body_ast);
            if (root->kind == AST_INTRINSIC &&
                root->sub >= INTR_EQ && root->sub <= INTR_APPROX) {
                return true;
            }
        }
    }
    return false;
}

/*code section below manages hardcoding additive and relational unit compatibility checks*/
/*Construct +, -, plus, minus, ==, !=, <, <=, >, >=, ~*/
bool hardcode_is_additive_or_rel_sym(const MemArena *mem, const SymTable *tab,
                                     uint16_t sym_id, OpFixity fixity, bool *out_is_rel) {
    if (out_is_rel) *out_is_rel = false;
    if (fixity != FIX_INFIX_LEFT && fixity != FIX_INFIX_RIGHT) return false;
    StrSlice s = tab->syms[sym_id];
    const char *p = mem_str_ptr(mem, s);
    if ((s.len == 1 && (p[0] == '+' || p[0] == '-')) ||
        (s.len == 4 && memcmp(p, "plus", 4) == 0) ||
        (s.len == 5 && memcmp(p, "minus", 5) == 0)) {
        return true;
    }
    if ((s.len == 1 && (p[0] == '<' || p[0] == '>' || p[0] == '~')) ||
        (s.len == 2 && (memcmp(p, "==", 2) == 0 || memcmp(p, "!=", 2) == 0 ||
                        memcmp(p, "<=", 2) == 0 || memcmp(p, ">=", 2) == 0))) {
        if (out_is_rel) *out_is_rel = true;
        return true;
    }
    return false;
}

/* ============================================================================
 * 4. EVALUATOR HARDCODED ERGONOMICS (GENERIC `vec<T>`/`list<T>`/`stack<T>`/`queue<T>`,
 *    `AST_FOR_IN`, DESTRUCTURING, `switch`)
 * ============================================================================ */

/*code section below manages hardcoding parameterized container type syntax and variadic constructors*/
/*Construct vec<T>(...), list<T>(...), stack<T>(...), queue<T>(...), vec(...), list(...), stack(...), queue(...)*/
static int container_kind_from_name_hc(const char *name, uint16_t len) {
    if (len == 3 && memcmp(name, "vec", 3) == 0)   return 0;
    if (len == 4 && memcmp(name, "list", 4) == 0)  return 1;
    if (len == 5 && memcmp(name, "stack", 5) == 0) return 2;
    if (len == 5 && memcmp(name, "queue", 5) == 0) return 3;
    return -1;
}

static uint16_t resolve_container_elem_type_hc(const SymTable *tab, const MemArena *mem,
                                               const char *name, uint16_t len) {
    if (container_kind_from_name_hc(name, len) >= 0 && mem->vec_type_id != 0) {
        return mem->vec_type_id;
    }
    return sym_find_type(tab, mem, name, len);
}

bool hardcode_try_parse_container_type_arg(Parser *P, const char *id_str, uint16_t id_len,
                                           uint16_t *out_spec) {
    if (out_spec) *out_spec = 0;
    int kind_code = container_kind_from_name_hc(id_str, id_len);
    if (kind_code < 0) return false;

    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;

    if (P->lex.cur.had_newline || P->lex.cur.kind != TOK_OP) return false;
    StrSlice op_s = tab->syms[P->lex.cur.sym_id];
    if (op_s.len != 1 || mem_str_ptr(mem, op_s)[0] != '<') return false;

    lex_next(&P->lex); /* consume '<' */
    if (P->lex.cur.kind != TOK_IDENT &&
        !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
        mem_err(mem, "Expected type or unit name inside '%.*s<...>'", (int)id_len, id_str);
        return false;
    }
    const char *s1_ptr = P->lex.src + P->lex.cur.off;
    uint16_t    s1_len = P->lex.cur.len;
    lex_next(&P->lex);

    uint8_t locked_tid = 0;
    uint8_t locked_uid = 0;

    if (P->lex.cur.kind == TOK_COMMA) {
        lex_next(&P->lex); /* consume ',' */
        if (P->lex.cur.kind != TOK_IDENT &&
            !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
            mem_err(mem, "Expected unit name after ',' in '%.*s<...>'", (int)id_len, id_str);
            return false;
        }
        const char *s2_ptr = P->lex.src + P->lex.cur.off;
        uint16_t    s2_len = P->lex.cur.len;
        lex_next(&P->lex);

        uint16_t tid = resolve_container_elem_type_hc(tab, mem, s1_ptr, s1_len);
        if (tid == MEM_TYPE_VOID) {
            mem_err(mem, "Unknown element type '%.*s' in '%.*s<...>'",
                    (int)s1_len, s1_ptr, (int)id_len, id_str);
            return false;
        }
        uint8_t uid = sym_find_or_add_unit((SymTable *)tab, mem, s2_ptr, s2_len);
        if (mem->has_error) return false;
        locked_tid = (tid == SYM_TYPE_ANY) ? 0 : (uint8_t)tid;
        locked_uid = (uid == SYM_UNIT_GENERIC || uid == SYM_UNIT_OMITTED || uid == SYM_UNIT_LOCK_NONE) ? 0 : uid;
    } else {
        uint16_t tid = resolve_container_elem_type_hc(tab, mem, s1_ptr, s1_len);
        if (tid == SYM_TYPE_ANY) {
            locked_tid = 0;
            locked_uid = 0;
        } else if (tid != MEM_TYPE_VOID) {
            locked_tid = (uint8_t)tid;
            locked_uid = 0;
        } else {
            uint8_t uid = sym_find_unit(tab, mem, s1_ptr, s1_len);
            if (uid == MEM_UNIT_NONE) {
                mem_err(mem, "Unknown element type '%.*s' in '%.*s<...>'",
                        (int)s1_len, s1_ptr, (int)id_len, id_str);
                return false;
            }
            locked_tid = 0;
            locked_uid = (uid == SYM_UNIT_GENERIC || uid == SYM_UNIT_OMITTED || uid == SYM_UNIT_LOCK_NONE) ? 0 : uid;
        }
    }

    if (P->lex.cur.kind != TOK_OP ||
        tab->syms[P->lex.cur.sym_id].len != 1 ||
        mem_str_ptr(mem, tab->syms[P->lex.cur.sym_id])[0] != '>') {
        mem_err(mem, "Expected closing '>' after element type in '%.*s<...>'", (int)id_len, id_str);
        return false;
    }
    lex_next(&P->lex); /* consume '>' */

    if (P->lex.cur.kind != TOK_LPAREN) {
        mem_err(mem, "Expected '(' after '%.*s<...>'", (int)id_len, id_str);
        return false;
    }

    if (out_spec) {
        *out_spec = (uint16_t)(0x8000u |
                               (((uint32_t)kind_code & 0x03u) << 13u) |
                               (((uint32_t)locked_uid & 0x1Fu) << 8u) |
                               (uint32_t)locked_tid);
    }
    return true;
}

bool hardcode_try_eval_variadic_vec(MemArena *mem, const SymTable *tab,
                                    const AstNode *n, uint16_t frame_base,
                                    uint8_t cur_ret_type, Value *out_val) {
    if (n->ref >= tab->sym_count || mem->vec_type_id == 0) return false;
    StrSlice fn_s = tab->syms[n->ref];
    int kind_code = container_kind_from_name_hc(mem_str_ptr(mem, fn_s), fn_s.len);
    if (kind_code < 0) return false;

    bool is_vec_call = (kind_code == 0);
    bool has_spec    = (n->right != 0 && n->right != MEM_NO_IDX && (n->right & 0x8000u) != 0);

    if (!has_spec) {
        if (is_vec_call && n->sub < 3)  return false;
        if (!is_vec_call && n->sub < 2) return false;
    }

    uint8_t  locked_tid = has_spec ? (uint8_t)(n->right & 0xFFu) : 0;
    uint8_t  locked_uid = has_spec ? (uint8_t)((n->right >> 8u) & 0x1Fu) : 0;
    uint16_t hdr0       = (uint16_t)(((uint32_t)locked_uid << 8u) | (uint32_t)locked_tid);

    Value   items[32];
    uint8_t argc = 0;
    uint16_t cur = n->left;
    while (cur != MEM_NO_IDX && argc < 32 && !mem->has_error) {
        const AstNode *link = mem_get_ast(mem, cur);
        items[argc] = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            *out_val = val_void();
            return true;
        }
        argc++;
        cur = link->right;
    }

    /* Special `vec<T>()` / `vec<T>(cap)` / `vec<T>(count, init)` variations */
    if (is_vec_call && has_spec) {
        if (argc == 0) {
            uint16_t hdr_idx = mem_vec_alloc_hdr(mem, hdr0, 0);
            *out_val = mem->has_error ? val_void()
                                      : val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
            return true;
        }
        if (argc == 1 && val_get_type(mem, items[0]) == mem->fast_real_type &&
            val_get_unit(mem, items[0]) == MEM_UNIT_NONE) {
            int cap_in = 0;
            if (!delegate_val_to_exact_int(mem, tab, items[0], "vector capacity", &cap_in)) {
                *out_val = val_void();
                return true;
            }
            if (cap_in < 0 || cap_in > MEM_MAX_F64_POOL) {
                mem_err(mem, "Vector capacity out of range (%d)", cap_in);
                *out_val = val_void();
                return true;
            }
            uint16_t hdr_idx = mem_vec_alloc_hdr(mem, hdr0, (uint16_t)cap_in);
            *out_val = mem->has_error ? val_void()
                                      : val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
            return true;
        }
        if (argc == 2 && val_get_type(mem, items[0]) == mem->fast_real_type &&
            val_get_unit(mem, items[0]) == MEM_UNIT_NONE) {
            int count = 0;
            if (!delegate_val_to_exact_int(mem, tab, items[0], "vector fill count", &count)) {
                *out_val = val_void();
                return true;
            }
            if (count < 0 || count > MEM_MAX_F64_POOL) {
                mem_err(mem, "Vector fill count out of range (%d)", count);
                *out_val = val_void();
                return true;
            }
            uint16_t hdr_idx = mem_vec_alloc_hdr(mem, hdr0, (uint16_t)count);
            if (mem->has_error) {
                *out_val = val_void();
                return true;
            }
            Value vhandle = val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
            for (int i = 0; i < count && !mem->has_error; ++i) {
                (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, vhandle, items[1], val_void());
            }
            *out_val = mem->has_error ? val_void() : vhandle;
            return true;
        }
    }

    uint16_t hdr_idx = mem_vec_alloc_hdr(mem, hdr0, argc);
    if (mem->has_error) {
        *out_val = val_void();
        return true;
    }
    Value vhandle = val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    for (uint8_t i = 0; i < argc && !mem->has_error; ++i) {
        (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, vhandle, items[i], val_void());
    }
    *out_val = mem->has_error ? val_void() : vhandle;
    return true;
}

/*code section below manages hardcoding O(1) range membership evaluation without allocating a range vector*/
/*Construct x in a...b, x in a..b, x in a..<b, x in a<..b*/
static bool is_direct_vrange_binary(const MemArena *mem, const SymTable *tab,
                                    uint16_t ast_idx, int *out_mode) {
    if (out_mode) *out_mode = 0;
    if (ast_idx == MEM_NO_IDX) return false;
    const AstNode *n = mem_get_ast(mem, ast_idx);
    if (n->kind != AST_BINARY) return false;
    uint16_t sid = n->ref;
    if (sid >= tab->sym_count) return false;
    for (uint16_t i = tab->sym_first_ov[sid]; i != MEM_NO_IDX; i = tab->ov_next[i]) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->fixity == n->sub && sym_mod_enabled(tab, ov->mod_id) &&
            ov->body_ast != MEM_NO_IDX) {
            const AstNode *b = mem_get_ast(mem, ov->body_ast);
            if (b->kind == AST_INTRINSIC && b->sub == INTR_VRANGE) {
                if (out_mode && b->ref != 0 && b->ref != MEM_NO_IDX) {
                    const AstNode *mn = mem_get_ast(mem, b->ref);
                    if (mn->kind == AST_LIT) {
                        *out_mode = (int)mem_read_pool(mem, mn->ref);
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool hardcode_eval_in_vrange_fast(MemArena *mem, const SymTable *tab,
                                  const AstNode *n, uint16_t frame_base,
                                  uint8_t cur_ret_type, Value *out_val) {
    int vrange_mode = 0;
    if (!hardcode_is_in_op_sym(mem, tab, n->ref) ||
        !is_direct_vrange_binary(mem, tab, n->right, &vrange_mode)) {
        return false;
    }
    Value item = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) {
        *out_val = val_void();
        return true;
    }
    const AstNode *rn = mem_get_ast(mem, n->right);
    Value va = eval_ast(mem, tab, rn->left, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) {
        *out_val = val_void();
        return true;
    }
    Value vb = eval_ast(mem, tab, rn->right, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) {
        *out_val = val_void();
        return true;
    }
    int lo = 0, hi = 0;
    if (!delegate_val_to_exact_int(mem, tab, va, "range start", &lo) ||
        !delegate_val_to_exact_int(mem, tab, vb, "range end",   &hi)) {
        *out_val = val_void();
        return true;
    }
    uint8_t elem_uid = MEM_UNIT_NONE;
    if (!delegate_check_additive_unit_compat(mem, tab, "..", 2, va, vb, &elem_uid)) {
        *out_val = val_void();
        return true;
    }
    if (val_get_type(mem, item) != mem->fast_real_type || val_get_unit(mem, item) != elem_uid) {
        *out_val = val_make_f64(mem, mem->fast_real_type, 0.0);
        return true;
    }
    double ditem = val_get_f64(mem, item);
    double ritem = round(ditem);
    if (fabs(ditem - ritem) > 1e-9 || ritem < -2147483647.0 || ritem > 2147483647.0) {
        *out_val = val_make_f64(mem, mem->fast_real_type, 0.0);
        return true;
    }
    long long k    = (long long)ritem;
    long long step = (lo <= hi) ? 1LL : -1LL;
    long long llo  = (long long)lo + ((vrange_mode & 2) ? step : 0LL);
    long long lhi  = (long long)hi - ((vrange_mode & 1) ? step : 0LL);
    bool hit = (step > 0) ? (k >= llo && k <= lhi) : (k <= llo && k >= lhi);
    *out_val = val_make_f64(mem, mem->fast_real_type, hit ? 1.0 : 0.0);
    return true;
}

/*code section below manages hardcoding range and collection iteration loops and vector comprehensions*/
/*Construct (i in a...b) { ... }, i in a...b { ... }, (x in vec) { ... }, [expr for x in iter]*/
Value hardcode_eval_for_in_ast(MemArena *mem, const SymTable *tab,
                               const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type) {
    uint8_t  var_slot   = n->sub;
    uint16_t iter_ast   = n->left;
    bool     is_comp    = (n->ref & 0x8000u) != 0;
    uint16_t raw_lbl    = (uint16_t)(n->ref & 0x7FFFu);
    uint8_t  loop_label = (raw_lbl > 0) ? (uint8_t)(raw_lbl - 1) : 0xFFu;

    uint16_t body_ast = n->right;
    uint16_t cond_ast = MEM_NO_IDX;
    Value    comp_vec = val_void();

    if (is_comp) {
        const AstNode *pair = mem_get_ast(mem, n->right);
        body_ast = pair->left;
        cond_ast = pair->right;
        uint16_t hdr_idx = mem_vec_alloc_hdr(mem, 0, 8);
        if (mem->has_error) return val_void();
        comp_vec = val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    Value    last  = is_comp ? comp_vec : val_make_f64(mem, mem->fast_real_type, 0.0);
    uint32_t guard = 0;

    /* Fast path: `(i in a...b) { ... }` or `[expr for i in a...b]` without allocating range vector */
    int vrange_mode = 0;
    if (is_direct_vrange_binary(mem, tab, iter_ast, &vrange_mode)) {
        const AstNode *rn = mem_get_ast(mem, iter_ast);
        Value va = eval_ast(mem, tab, rn->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
        Value vb = eval_ast(mem, tab, rn->right, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();

        int lo = 0, hi = 0;
        if (!delegate_val_to_exact_int(mem, tab, va, "range start", &lo) ||
            !delegate_val_to_exact_int(mem, tab, vb, "range end",   &hi)) {
            return val_void();
        }
        uint8_t u = MEM_UNIT_NONE;
        if (!delegate_check_additive_unit_compat(mem, tab, "..", 2, va, vb, &u)) {
            return val_void();
        }

        long long step = (lo <= hi) ? 1LL : -1LL;
        long long llo  = (long long)lo + ((vrange_mode & 2) ? step : 0LL);
        long long lhi  = (long long)hi - ((vrange_mode & 1) ? step : 0LL);
        for (long long k = llo;
             ((step > 0) ? (k <= lhi) : (k >= lhi)) && !mem->has_error;
             k += step) {
            mem_set_slot(mem, frame_base, var_slot,
                         val_make_f64_unit(mem, mem->fast_real_type, u, (double)k));
            if (cond_ast != MEM_NO_IDX) {
                Value cv = eval_ast(mem, tab, cond_ast, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cv)) <= 1e-15) continue;
            }
            Value item_res = eval_ast(mem, tab, body_ast, frame_base, cur_ret_type);
            if (is_comp) {
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, comp_vec, item_res, val_void());
            } else {
                last = item_res;
                if (mem->jump_active == JUMP_CONTINUE_ANON ||
                    (mem->jump_active == JUMP_CONTINUE_TAG && mem->jump_target_id == loop_label)) {
                    mem->jump_active = JUMP_NONE;
                    continue;
                }
                if (mem->jump_active == JUMP_BREAK_ANON ||
                    (mem->jump_active == JUMP_BREAK_TAG && mem->jump_target_id == loop_label)) {
                    mem->jump_active = JUMP_NONE;
                    if (val_get_type(mem, mem->jump_val) != MEM_TYPE_VOID) {
                        last = mem->jump_val;
                        mem->jump_val = val_void();
                    }
                    break;
                }
                if (mem->jump_active != JUMP_NONE) {
                    break;
                }
            }
            if (++guard > 1000000u) {
                mem_err(mem, "Loop iteration limit exceeded");
                break;
            }
        }
        return is_comp ? comp_vec : last;
    }

    /* Collection path: `(v in vec) { ... }`, `(x in mat) { ... }`, or `[expr for v in coll]` */
    Value coll = eval_ast(mem, tab, iter_ast, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
    uint16_t coll_tid = val_get_type(mem, coll);

    if (coll_tid == mem->vec_type_id && mem->vec_type_id != 0) {
        uint16_t hdr_idx = val_get_pool_idx(coll);
        for (uint16_t idx = 0; !mem->has_error; ++idx) {
            uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
            uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
            uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
            uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                                ? tab->types[elem_tid].flat_size : 1;
            if (idx >= len) break;

            Value item = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid,
                                                (uint16_t)(data_idx + idx * stride));
            if (elem_tid != 0 && elem_tid < tab->type_count &&
                !tab->types[elem_tid].is_consuming &&
                elem_tid != mem->vec_type_id && elem_tid != mem->mat_type_id &&
                elem_tid != mem->fn_type_id) {
                item = val_clone_comp(mem, item, stride);
            }
            mem_set_slot(mem, frame_base, var_slot, item);
            if (cond_ast != MEM_NO_IDX) {
                Value cv = eval_ast(mem, tab, cond_ast, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cv)) <= 1e-15) continue;
            }
            Value item_res = eval_ast(mem, tab, body_ast, frame_base, cur_ret_type);
            if (is_comp) {
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, comp_vec, item_res, val_void());
            } else {
                last = item_res;
                if (mem->jump_active == JUMP_CONTINUE_ANON ||
                    (mem->jump_active == JUMP_CONTINUE_TAG && mem->jump_target_id == loop_label)) {
                    mem->jump_active = JUMP_NONE;
                    continue;
                }
                if (mem->jump_active == JUMP_BREAK_ANON ||
                    (mem->jump_active == JUMP_BREAK_TAG && mem->jump_target_id == loop_label)) {
                    mem->jump_active = JUMP_NONE;
                    if (val_get_type(mem, mem->jump_val) != MEM_TYPE_VOID) {
                        last = mem->jump_val;
                        mem->jump_val = val_void();
                    }
                    break;
                }
                if (mem->jump_active != JUMP_NONE) {
                    break;
                }
            }
            if (++guard > 1000000u) {
                mem_err(mem, "Loop iteration limit exceeded");
                break;
            }
        }
        return is_comp ? comp_vec : last;
    }

    if (coll_tid == mem->mat_type_id && mem->mat_type_id != 0) {
        uint8_t  rank = 0;
        uint16_t tot_len = 0, data_idx = 0, dims[4] = { 0 };
        delegate_mat_get_info(mem, coll, &rank, &tot_len, &data_idx, dims);
        uint8_t u = val_get_unit(mem, coll);
        for (uint16_t idx = 0; idx < tot_len && !mem->has_error; ++idx) {
            Value item = val_make_f64_unit(mem, mem->fast_real_type, u,
                                           mem_read_pool(mem, data_idx + idx));
            mem_set_slot(mem, frame_base, var_slot, item);
            if (cond_ast != MEM_NO_IDX) {
                Value cv = eval_ast(mem, tab, cond_ast, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cv)) <= 1e-15) continue;
            }
            Value item_res = eval_ast(mem, tab, body_ast, frame_base, cur_ret_type);
            if (is_comp) {
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, comp_vec, item_res, val_void());
            } else {
                last = item_res;
                if (mem->jump_active == JUMP_CONTINUE_ANON ||
                    (mem->jump_active == JUMP_CONTINUE_TAG && mem->jump_target_id == loop_label)) {
                    mem->jump_active = JUMP_NONE;
                    continue;
                }
                if (mem->jump_active == JUMP_BREAK_ANON ||
                    (mem->jump_active == JUMP_BREAK_TAG && mem->jump_target_id == loop_label)) {
                    mem->jump_active = JUMP_NONE;
                    if (val_get_type(mem, mem->jump_val) != MEM_TYPE_VOID) {
                        last = mem->jump_val;
                        mem->jump_val = val_void();
                    }
                    break;
                }
                if (mem->jump_active != JUMP_NONE) {
                    break;
                }
            }
        }
        return is_comp ? comp_vec : last;
    }

    mem_err(mem, "Loop target after 'in' must be a range (a..b / a...b), vector ($vec), or matrix ($mat)");
    return val_void();
}

/*code section below manages hardcoding multi-variable scalar broadcast and struct/vector/matrix destructuring*/
/*Construct x, y := 5 ; r1, r2 := quad_roots(1, -5, 6) ; a, b, c := vec(1, 2, 3)*/
Value hardcode_eval_unpack_elem_ast(MemArena *mem, const SymTable *tab,
                                    const AstNode *n, uint16_t frame_base) {
    uint8_t elem_idx = n->sub;
    uint8_t tot_vars = (uint8_t)n->ref;
    uint8_t tmp_slot = (uint8_t)n->left;
    Value   src      = mem_get_slot(mem, frame_base, tmp_slot);
    uint16_t tid     = val_get_type(mem, src);
    uint8_t  uid     = val_get_unit(mem, src);

    if (tid == 0 || tid >= tab->type_count) return val_void();

    /* 1. Consuming scalar (`real`, `iota`, `str`): broadcast to all variables (`x, y := 5`) */
    if (tab->types[tid].is_consuming) {
        return src;
    }

    /* 2. Dynamic vector (`$vec`): destructure element `elem_idx` (`a, b, c := vec(10, 20, 30)`) */
    if (tid == mem->vec_type_id && mem->vec_type_id != 0) {
        uint16_t hdr_idx  = val_get_pool_idx(src);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
        uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
        uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
        uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                            ? tab->types[elem_tid].flat_size : 1;
        if (elem_idx >= len) {
            mem_err(mem, "Cannot destructure %u variables from vector of length %u",
                    (unsigned)tot_vars, (unsigned)len);
            return val_void();
        }
        return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid,
                                      (uint16_t)(data_idx + elem_idx * stride));
    }

    /* 3. Matrix (`$mat`): destructure element `elem_idx` (`a, b, c := [10, 20, 30]`) */
    if (tid == mem->mat_type_id && mem->mat_type_id != 0) {
        uint8_t  rank = 0;
        uint16_t tot_len = 0, data_idx = 0, dims[4] = { 0 };
        delegate_mat_get_info(mem, src, &rank, &tot_len, &data_idx, dims);
        if (elem_idx >= tot_len) {
            mem_err(mem, "Cannot destructure %u variables from matrix of length %u",
                    (unsigned)tot_vars, (unsigned)tot_len);
            return val_void();
        }
        return val_make_f64_unit(mem, mem->fast_real_type, uid,
                                 mem_read_pool(mem, data_idx + elem_idx));
    }

    /* 4. Flat C-style struct (`$vec2`, `$vec3`, `$complex`, `$circle`): destructure field `elem_idx` */
    const TypeDef *td = &tab->types[tid];
    if (elem_idx >= td->field_count) {
        char tbuf[32];
        sym_type_name(tab, mem, tid, tbuf, sizeof(tbuf));
        mem_err(mem, "Cannot destructure %u variables from '%s' (has %u fields)",
                (unsigned)tot_vars, tbuf, (unsigned)td->field_count);
        return val_void();
    }
    const FieldDef *fd = &tab->fields[td->start_idx + elem_idx];
    uint16_t f_pidx    = (uint16_t)(val_get_pool_idx(src) + fd->flat_offset);
    if (!tab->types[fd->type_id].is_consuming) {
        return val_with_unit(mem,
                             val_make_comp(mem, fd->type_id, f_pidx, tab->types[fd->type_id].flat_size),
                             uid);
    }
    return val_make_f64_unit(mem, fd->type_id ? fd->type_id : mem->fast_real_type, uid,
                             mem_read_pool(mem, f_pidx));
}

/*code section below manages hardcoding pattern-matched 'switch' case evaluation (exact values, ranges, struct/vector destructuring)*/
/*Construct switch (expr) { case pat if guard : { ... } else : { ... } }*/
Value hardcode_eval_switch_eq_ast(MemArena *mem, const SymTable *tab,
                                  const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type) {
    /* sub == 2: Pattern variable binding (`slot` in `n->ref`, value expr in `n->left`), returns 1.0 */
    if (n->sub == 2) {
        Value fval = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
        mem_set_slot(mem, frame_base, (uint8_t)n->ref, fval);
        return val_make_f64(mem, mem->fast_real_type, 1.0);
    }

    Value a = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
    uint16_t ta = val_get_type(mem, a);
    uint8_t  ua = val_get_unit(mem, a);

    /* sub == 1: Struct type check (`ta == n->ref`) */
    if (n->sub == 1) {
        return val_make_f64(mem, mem->fast_real_type, (ta == n->ref) ? 1.0 : 0.0);
    }

    /* sub == 3: Vector / 1D matrix length check (`len == n->ref`) */
    if (n->sub == 3) {
        bool ok = false;
        if (ta == mem->vec_type_id && mem->vec_type_id != 0) {
            uint16_t hdr_idx = val_get_pool_idx(a);
            uint16_t len     = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
            ok = (len == n->ref);
        } else if (ta == mem->mat_type_id && mem->mat_type_id != 0) {
            uint8_t  rank = 0;
            uint16_t tot_len = 0, dims[4] = { 0 };
            if (delegate_mat_get_info(mem, a, &rank, &tot_len, NULL, dims) && rank == 1) {
                ok = (tot_len == n->ref);
            }
        }
        return val_make_f64(mem, mem->fast_real_type, ok ? 1.0 : 0.0);
    }

    /* sub == 0: Fast path for range pattern `case a...b:` / `case a..b:` */
    int sw_vrange_mode = 0;
    if (is_direct_vrange_binary(mem, tab, n->right, &sw_vrange_mode)) {
        if (ta == 0 || ta >= tab->type_count || !tab->types[ta].is_consuming || ta == mem->str_type_id) {
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        const AstNode *rn = mem_get_ast(mem, n->right);
        Value vlo = eval_ast(mem, tab, rn->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
        Value vhi = eval_ast(mem, tab, rn->right, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();

        uint8_t ulo = val_get_unit(mem, vlo);
        uint8_t uhi = val_get_unit(mem, vhi);
        if ((ulo != MEM_UNIT_NONE && ua != ulo) || (uhi != MEM_UNIT_NONE && ua != uhi)) {
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        double da  = val_get_f64(mem, a);
        double dlo = val_get_f64(mem, vlo);
        double dhi = val_get_f64(mem, vhi);
        bool in_rng = false;
        if (dlo <= dhi) {
            bool lo_ok = (sw_vrange_mode & 2) ? (da > dlo + 1e-12) : (da >= dlo - 1e-12);
            bool hi_ok = (sw_vrange_mode & 1) ? (da < dhi - 1e-12) : (da <= dhi + 1e-12);
            in_rng = lo_ok && hi_ok;
        } else {
            bool lo_ok = (sw_vrange_mode & 2) ? (da < dlo - 1e-12) : (da <= dlo + 1e-12);
            bool hi_ok = (sw_vrange_mode & 1) ? (da > dhi + 1e-12) : (da >= dhi - 1e-12);
            in_rng = lo_ok && hi_ok;
        }
        return val_make_f64(mem, mem->fast_real_type, in_rng ? 1.0 : 0.0);
    }

    Value b = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
    uint16_t tb = val_get_type(mem, b);
    uint8_t  ub = val_get_unit(mem, b);

    bool eq = false;
    if (ta == mem->str_type_id && tb == mem->str_type_id && mem->str_type_id != 0) {
        eq = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, b));
    } else if (tb == mem->vec_type_id && ta != mem->vec_type_id && mem->vec_type_id != 0) {
        Value vin_res = delegate_eval_vec_intrinsic(mem, tab, INTR_VIN, a, b, val_void());
        eq = (!mem->has_error && fabs(val_get_f64(mem, vin_res)) > 1e-15);
    } else if (ta == tb && ua == ub) {
        if (ta > 0 && ta < tab->type_count && !tab->types[ta].is_consuming) {
            uint16_t sz = tab->types[ta].flat_size;
            uint16_t pa = val_get_pool_idx(a);
            uint16_t pb = val_get_pool_idx(b);
            eq = true;
            for (uint16_t k = 0; k < sz; ++k) {
                if (fabs(mem_read_pool(mem, pa + k) - mem_read_pool(mem, pb + k)) > 1e-12) {
                    eq = false;
                    break;
                }
            }
        } else {
            eq = (fabs(val_get_f64(mem, a) - val_get_f64(mem, b)) <= 1e-12);
        }
    }
    return val_make_f64(mem, mem->fast_real_type, eq ? 1.0 : 0.0);
}
