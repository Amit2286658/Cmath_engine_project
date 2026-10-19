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

bool hardcode_is_reserved_kw(const SymTable *tab, const MemArena *mem,
                             const char *name, uint16_t len) {
    int sid = sym_find(tab, mem, name, len);
    if (sid < 0) return false;
    uint8_t m = sym_active_meta_tag(tab, (uint16_t)sid, FIX_FUNC);
    return (m == INTR_IF           ||
            m == INTR_OR_BRANCH    ||
            m == INTR_ELSE_BRANCH  ||
            m == INTR_SWITCH_BLOCK ||
            m == INTR_CASE_BRANCH  ||
            m == INTR_JUMP_TAG     ||
            m == INTR_RUN_TAG      ||
            m == INTR_BREAK_LOOP   ||
            m == INTR_LOOP_KW);
}

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
 * 3. PARSER HARDCODED ERGONOMICS (`(i in ...){}`, IMPLICIT `*`, OPERATORS)
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

static bool lex_at_is_syntax_hc(const Lexer *lex, uint16_t p, uint8_t target_syn) {
    p = skip_raw_ws_hc(lex->src, p, lex->len);
    for (uint16_t sid = 0; sid < lex->tab->sym_count; ++sid) {
        if (sym_get_active_syntax(lex->tab, sid) != target_syn) continue;
        StrSlice s = lex->tab->syms[sid];
        if (s.len > 0 && p + s.len <= lex->len &&
            memcmp(lex->src + p, mem_str_ptr(lex->mem, s), s.len) == 0) {
            return true;
        }
    }
    return false;
}

bool hardcode_is_paren_for_in(const Parser *P, bool allow_any_body) {
    if (P->lex.cur.kind != TOK_LPAREN) return false;
    const char *src = P->lex.src;
    uint16_t    len = P->lex.len;
    uint16_t    p   = skip_raw_ws_hc(src, P->lex.pos, len);

    /* 1. Loop variable identifier */
    if (p >= len || !lex_is_ident_start((unsigned char)src[p])) return false;
    while (p < len && lex_is_ident_cont((unsigned char)src[p])) p++;

    /* 2. Hardcoded `in` keyword/operator inside `(<ident> in <expr>)` */
    p = skip_raw_ws_hc(src, p, len);
    if (p + 2 > len || src[p] != 'i' || src[p + 1] != 'n') return false;
    if (p + 2 < len && lex_is_ident_cont((unsigned char)src[p + 2])) return false;
    p += 2;

    /* 3. Scan balanced parentheses to matching `)` */
    p = skip_raw_ws_hc(src, p, len);
    if (p >= len || src[p] == ')') return false;
    int depth = 1;
    while (p < len && depth > 0) {
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
        if (c == '(' || c == '[' || c == '{') depth++;
        else if (c == ')' || c == ']' || c == '}') depth--;
        p++;
    }
    if (depth != 0) return false;

    if (allow_any_body) return true;

    /* 4. For keywordless `(i in 1...5) { ... }`, require `{` or `??` after `)` */
    p = skip_raw_ws_hc(src, p, len);
    if (lex_at_is_syntax_hc(&P->lex, p, INTR_LBRACE)) return true;
    if (p + 1 < len && src[p] == '?' && src[p + 1] == '?') return true;
    return false;
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

bool hardcode_can_start_implicit_mul_rhs(const Parser *P) {
    const SymTable *tab = P->lex.tab;
    Token t = P->lex.cur;

    if (t.kind == TOK_LPAREN) {
        return !hardcode_is_paren_for_in(P, false);
    }
    if (t.kind == TOK_IDENT) {
        uint8_t kw = token_func_meta_hc(P, t);
        if (kw != 0 && (kw < INTR_CALC_DIFF || kw > INTR_CALC_FMAX)) {
            return false;
        }
        /* Do not treat missing semicolon before `y := ...` or `Tag : ...` as implicit mul */
        uint16_t p = skip_raw_ws_hc(P->lex.src, P->lex.pos, P->lex.len);
        if (p < P->lex.len && (P->lex.src[p] == ':' || P->lex.src[p] == '=')) {
            return false;
        }
        if (p + 1 < P->lex.len && P->lex.src[p + 1] == '=' &&
            (P->lex.src[p] == '+' || P->lex.src[p] == '-' || P->lex.src[p] == '*' ||
             P->lex.src[p] == '/' || P->lex.src[p] == '%' || P->lex.src[p] == '^')) {
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

bool hardcode_is_plain_store_sym(const MemArena *mem, const SymTable *tab,
                                 uint16_t sym_id, bool *out_is_colon_eq) {
    StrSlice s = tab->syms[sym_id];
    const char *ptr = mem_str_ptr(mem, s);
    bool colon_eq = (s.len == 2 && memcmp(ptr, ":=", 2) == 0);
    if (out_is_colon_eq) *out_is_colon_eq = colon_eq;
    return colon_eq || (s.len == 1 && ptr[0] == '=');
}

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
 * 4. EVALUATOR HARDCODED ERGONOMICS (VARIADIC `vec`/`list`, `AST_FOR_IN`, DESTRUCTURING)
 * ============================================================================ */

bool hardcode_try_eval_variadic_vec(MemArena *mem, const SymTable *tab,
                                    const AstNode *n, uint16_t frame_base,
                                    uint8_t cur_ret_type, Value *out_val) {
    if (n->ref >= tab->sym_count || mem->vec_type_id == 0) return false;
    StrSlice fn_s = tab->syms[n->ref];
    bool is_vec_call  = (fn_s.len == 3 && memcmp(mem_str_ptr(mem, fn_s), "vec", 3) == 0);
    bool is_list_call = (fn_s.len == 4 && memcmp(mem_str_ptr(mem, fn_s), "list", 4) == 0);
    if (!((is_vec_call && n->sub >= 3) || (is_list_call && n->sub >= 1))) {
        return false;
    }

    uint16_t hdr_idx = mem_vec_alloc_hdr(mem, 0, n->sub);
    if (mem->has_error) {
        *out_val = val_void();
        return true;
    }
    Value vhandle = val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    uint16_t cur = n->left;
    while (cur != MEM_NO_IDX && !mem->has_error) {
        const AstNode *link = mem_get_ast(mem, cur);
        Value item = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            *out_val = val_void();
            return true;
        }
        (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, vhandle, item, val_void());
        cur = link->right;
    }
    *out_val = vhandle;
    return true;
}

static bool is_direct_vrange_binary(const MemArena *mem, const SymTable *tab, uint16_t ast_idx) {
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
                return true;
            }
        }
    }
    return false;
}

Value hardcode_eval_for_in_ast(MemArena *mem, const SymTable *tab,
                               const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type) {
    uint8_t  var_slot = n->sub;
    uint16_t iter_ast = n->left;
    uint16_t body_ast = n->right;
    Value    last     = val_make_f64(mem, mem->fast_real_type, 0.0);
    uint32_t guard    = 0;

    /* Fast path: `(i in a..b) { ... }` or `(i in a...b) { ... }` without allocating a vector */
    if (is_direct_vrange_binary(mem, tab, iter_ast)) {
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
        for (long long k = (long long)lo;
             ((step > 0) ? (k <= (long long)hi) : (k >= (long long)hi)) && !mem->has_error;
             k += step) {
            mem_set_slot(mem, frame_base, var_slot,
                          val_make_f64_unit(mem, mem->fast_real_type, u, (double)k));
            last = eval_ast(mem, tab, body_ast, frame_base, cur_ret_type);
            if (mem->jump_active == JUMP_BREAK_ANON) {
                mem->jump_active = JUMP_NONE;
                break;
            }
            if (mem->jump_active != JUMP_NONE) {
                break;
            }
            if (++guard > 1000000u) {
                mem_err(mem, "Loop iteration limit exceeded");
                break;
            }
        }
        return last;
    }

    /* Collection path: `(v in vec) { ... }` or `(x in mat) { ... }` */
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
                elem_tid != mem->vec_type_id && elem_tid != mem->mat_type_id) {
                item = val_clone_comp(mem, item, stride);
            }
            mem_set_slot(mem, frame_base, var_slot, item);
            last = eval_ast(mem, tab, body_ast, frame_base, cur_ret_type);
            if (mem->jump_active == JUMP_BREAK_ANON) {
                mem->jump_active = JUMP_NONE;
                break;
            }
            if (mem->jump_active != JUMP_NONE) {
                break;
            }
            if (++guard > 1000000u) {
                mem_err(mem, "Loop iteration limit exceeded");
                break;
            }
        }
        return last;
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
            last = eval_ast(mem, tab, body_ast, frame_base, cur_ret_type);
            if (mem->jump_active == JUMP_BREAK_ANON) {
                mem->jump_active = JUMP_NONE;
                break;
            }
            if (mem->jump_active != JUMP_NONE) {
                break;
            }
        }
        return last;
    }

    mem_err(mem, "Loop target after 'in' must be a range (a..b / a...b), vector ($vec), or matrix ($mat)");
    return val_void();
}

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
