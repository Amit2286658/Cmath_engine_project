/*
 * parser.c — Standalone Pratt Parser & AST Builder Implementation
 *
 * Supports:
 *   - Lexical `{ ... }` scope blocks (value-returning and void/isolated blocks)
 *   - Multi-branch `if ... or ... else` statements (one `if`, many `or` else-if
 *     branches, optional single `else`, plus legacy `if(c, t, e)`)
 *   - `switch (expr) { case v1 : { ... } ... else : { ... } }` blocks
 *   - Standard right-associative ternary `cond ? x : y`
 *   - Colon-tagged scopes/loops `Tag : { ... }`, `Tag : (cond) ?? { ... }`, and
 *     `Tag : (i in 1...5) { ... }` with `jump(Tag)`, `run(Tag)`, and `break(Tag)`
 *   - Keywordless range/collection loops `(i in 1...5) { ... }`, `(i in 1..5) { ... }`,
 *     `(v in vec) { ... }` as well as `loop (i in ...) { ... }` and `loop (cond) { ... }`
 *   - Implicit multiplication (`2x`, `2π`, `3(x + 1)`, `(x + 1)(x - 1)`, `2 sin(x)`)
 *   - Uniform Function Call / dot-chaining syntax (`v.push(10)`, `M.transpose().det()`)
 *   - Multi-variable declarations (`x, y := 5`), struct/vector/matrix destructuring
 *     (`r1, r2 := quad_roots(1, -5, 6)`), and compound assignments (`+=`, `-=`, etc.)
 *   - Concrete Entity Type & Concrete Unit Type variable annotations
 *   - Reserved keyword guards for variables and scope tags
 */

#include "parser.h"
#include "../lib/hardcode/hardcode.h"
#include <string.h>

static int find_local_idx(const Parser *P, const char *name, uint16_t len) {
    for (int i = (int)P->vis_count - 1; i >= 0; --i) {
        StrSlice s = P->locals[i].name;
        if (s.len == len && memcmp(mem_str_ptr(P->lex.mem, s), name, len) == 0) {
            return i;
        }
    }
    return -1;
}

int parser_add_local(Parser *P, const char *name, uint16_t len, uint16_t type_id) {
    if (sym_is_reserved_kw(P->lex.tab, P->lex.mem, name, len)) {
        mem_err(P->lex.mem, "Cannot use reserved keyword '%.*s' as a variable name",
                (int)len, name);
        return 0;
    }
    int existing = find_local_idx(P, name, len);
    if (existing >= 0 && P->locals[existing].scope_depth == P->scope_depth) {
        return (int)P->locals[existing].slot;
    }
    if (P->vis_count >= PARSER_MAX_LOCALS || P->local_count >= PARSER_MAX_LOCALS) {
        mem_err(P->lex.mem, "Too many local variables (max %d)", PARSER_MAX_LOCALS);
        return 0;
    }
    uint8_t slot = P->local_count++;
    uint8_t vidx = P->vis_count++;
    P->locals[vidx].name        = mem_intern_str(P->lex.mem, name, len);
    P->locals[vidx].type_id     = type_id;
    P->locals[vidx].slot        = slot;
    P->locals[vidx].scope_depth = P->scope_depth;
    P->slot_types[slot]         = type_id;
    return (int)slot;
}

static uint8_t parser_alloc_anon_slot(Parser *P, uint16_t type_id) {
    if (P->local_count >= PARSER_MAX_LOCALS) {
        mem_err(P->lex.mem, "Too many local variables (max %d)", PARSER_MAX_LOCALS);
        return 0;
    }
    uint8_t slot = P->local_count++;
    P->slot_types[slot] = type_id;
    return slot;
}

static int find_label_id(const Parser *P, const char *name, uint16_t len) {
    for (uint8_t i = 0; i < P->label_count; ++i) {
        StrSlice s = P->labels[i].name;
        if (s.len == len && memcmp(mem_str_ptr(P->lex.mem, s), name, len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static int parser_get_or_add_label(Parser *P, const char *name, uint16_t len) {
    if (sym_is_reserved_kw(P->lex.tab, P->lex.mem, name, len)) {
        mem_err(P->lex.mem, "Cannot use reserved keyword '%.*s' as a scope tag",
                (int)len, name);
        return 0;
    }
    int existing = find_label_id(P, name, len);
    if (existing >= 0) return existing;
    if (P->label_count >= MEM_MAX_LABELS) {
        mem_err(P->lex.mem, "Too many scope tags (max %d)", MEM_MAX_LABELS);
        return 0;
    }
    uint8_t id = P->label_count++;
    P->labels[id].name     = mem_intern_str(P->lex.mem, name, len);
    P->labels[id].body_ast = MEM_NO_IDX;
    P->labels[id].defined  = false;
    return (int)id;
}

bool parser_check_labels(Parser *P) {
    if (P->lex.mem->has_error) return false;
    for (uint8_t i = 0; i < P->label_count; ++i) {
        if (!P->labels[i].defined) {
            StrSlice s = P->labels[i].name;
            mem_err(P->lex.mem, "Undefined scope tag '%.*s'",
                    (int)s.len, mem_str_ptr(P->lex.mem, s));
            return false;
        }
    }
    return true;
}

void parser_init(Parser *P, MemArena *mem, const SymTable *tab,
                 const char *src, uint16_t len, uint16_t start_pos,
                 bool in_ext_body, bool stop_at_newline) {
    memset(P, 0, sizeof(*P));
    lex_init(&P->lex, mem, tab, src, len, start_pos, stop_at_newline);
    P->in_ext_body = in_ext_body;
}

/* ============================================================================
 * 1. TOKEN, OPERATOR & META-INTRINSIC INSPECTION HELPERS
 * ============================================================================ */

static uint8_t token_func_meta(const Parser *P, Token t) {
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

static uint8_t cur_op_meta(const Parser *P) {
    if (P->lex.cur.kind != TOK_OP) return 0;
    uint8_t m = sym_active_meta_tag(P->lex.tab, P->lex.cur.sym_id, FIX_INFIX_LEFT);
    if (!m) m = sym_active_meta_tag(P->lex.tab, P->lex.cur.sym_id, FIX_INFIX_RIGHT);
    return m;
}

static uint16_t skip_raw_ws(const char *src, uint16_t p, uint16_t len) {
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

static bool lex_at_is_syntax(const Lexer *lex, uint16_t p, uint8_t target_syn) {
    p = skip_raw_ws(lex->src, p, lex->len);
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

static bool lex_peek_is_lbrace(const Lexer *lex) {
    return lex_at_is_syntax(lex, lex->pos, INTR_LBRACE);
}

static bool lex_peek_is_lparen(const Lexer *lex) {
    return lex_at_is_syntax(lex, lex->pos, INTR_LPAREN);
}

static bool lex_peek_is_branch_kw(const Parser *P) {
    uint16_t p = skip_raw_ws(P->lex.src, P->lex.pos, P->lex.len);
    if (p >= P->lex.len || !lex_is_ident_start((unsigned char)P->lex.src[p])) return false;
    uint16_t start = p;
    while (p < P->lex.len && lex_is_ident_cont((unsigned char)P->lex.src[p])) p++;
    int sid = sym_find(P->lex.tab, P->lex.mem, P->lex.src + start, p - start);
    if (sid < 0) return false;
    uint8_t m = sym_active_meta_tag(P->lex.tab, (uint16_t)sid, FIX_FUNC);
    return (m == INTR_OR_BRANCH || m == INTR_ELSE_BRANCH);
}

static int find_active_meta_sym(const SymTable *tab, uint8_t target_meta) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->meta_tag == target_meta && sym_mod_enabled(tab, ov->mod_id)) {
            return (int)ov->sym_id;
        }
    }
    return -1;
}

/*
 * Check if `(` at `P->lex.cur` is a `(var in iterable) { ... }` loop header.
 * Returns true if inside `(` is `<ident> in <expr>)` followed by `{` (or `??`),
 * or if `allow_any_body` is true (when preceded by the `loop` keyword).
 */

/*
 * Check if `v0, v1, ...` is a multi-variable assignment (`x, y := 5`,
 * `x, y : real, distance := 50m`, or `x, y += 1`).
 */
static bool lex_is_multi_lhs(const Parser *P) {
    const Lexer    *lex = &P->lex;
    const SymTable *tab = lex->tab;
    const MemArena *mem = lex->mem;
    uint16_t p = lex->pos; /* Position right after the first ',' */

    while (p < lex->len) {
        p = skip_raw_ws(lex->src, p, lex->len);
        if (p >= lex->len || !lex_is_ident_start((unsigned char)lex->src[p])) {
            return false;
        }
        while (p < lex->len && lex_is_ident_cont((unsigned char)lex->src[p])) {
            p++;
        }
        p = skip_raw_ws(lex->src, p, lex->len);

        bool matched_comma = false;
        for (uint16_t sid = 0; sid < tab->sym_count; ++sid) {
            if (sym_get_active_syntax(tab, sid) != INTR_COMMA) continue;
            StrSlice s = tab->syms[sid];
            if (s.len > 0 && p + s.len <= lex->len &&
                memcmp(lex->src + p, mem_str_ptr(mem, s), s.len) == 0) {
                p += s.len;
                matched_comma = true;
                break;
            }
        }
        if (!matched_comma) break;
    }

    int best_len = 0;
    uint16_t best_sid = 0;
    for (uint16_t sid = 0; sid < tab->sym_count; ++sid) {
        if (!sym_has_active_op(tab, sid)) continue;
        StrSlice s = tab->syms[sid];
        if (s.len > 0 && p + s.len <= lex->len &&
            memcmp(lex->src + p, mem_str_ptr(mem, s), s.len) == 0) {
            if ((int)s.len > best_len) {
                best_len = (int)s.len;
                best_sid = sid;
            }
        }
    }
    if (best_len == 0) return false;
    uint8_t m = sym_active_meta_tag(tab, best_sid, FIX_INFIX_RIGHT);
    return (m == INTR_STORE || m == INTR_COLON);
}


static bool parse_var_type_unit_annot(Parser *P, uint8_t *out_lock_type, uint8_t *out_lock_unit) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    *out_lock_type = 0;
    *out_lock_unit = 0;

    lex_next(&P->lex); /* consume ':' */
    if (P->lex.cur.kind != TOK_IDENT &&
        !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
        mem_err(mem, "Expected type or unit name after ':' in variable declaration");
        return false;
    }
    const char *s1_ptr = P->lex.src + P->lex.cur.off;
    uint16_t    s1_len = P->lex.cur.len;
    lex_next(&P->lex);

    if (P->lex.cur.kind == TOK_COMMA) {
        lex_next(&P->lex); /* consume ',' */
        if (P->lex.cur.kind != TOK_IDENT &&
            !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
            mem_err(mem, "Expected unit name after ',' in variable type annotation");
            return false;
        }
        const char *s2_ptr = P->lex.src + P->lex.cur.off;
        uint16_t    s2_len = P->lex.cur.len;
        lex_next(&P->lex);

        uint16_t tid = sym_find_type(tab, mem, s1_ptr, s1_len);
        if (tid == MEM_TYPE_VOID) {
            mem_err(mem, "Unknown entity type '%.*s' in variable declaration", (int)s1_len, s1_ptr);
            return false;
        }
        uint8_t uid = sym_find_or_add_unit((SymTable *)tab, mem, s2_ptr, s2_len);
        *out_lock_type = (tid == SYM_TYPE_ANY) ? 0 : (uint8_t)tid;
        *out_lock_unit = (uid == SYM_UNIT_GENERIC) ? 0 : uid;
        return !mem->has_error;
    }

    uint16_t tid = sym_find_type(tab, mem, s1_ptr, s1_len);
    if (tid == SYM_TYPE_ANY) {
        *out_lock_type = 0;
        *out_lock_unit = 0;
        return true;
    }
    if (tid != MEM_TYPE_VOID) {
        *out_lock_type = (uint8_t)tid;
        *out_lock_unit = 0;
        return true;
    }

    uint8_t uid = sym_find_or_add_unit((SymTable *)tab, mem, s1_ptr, s1_len);
    *out_lock_type = 0;
    *out_lock_unit = (uid == SYM_UNIT_GENERIC) ? 0 : uid;
    return !mem->has_error;
}

static uint16_t build_store_or_compound(Parser *P, uint16_t op_sym_id,
                                        uint16_t lhs_ast, uint16_t rhs_ast) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    if (lhs_ast == MEM_NO_IDX || rhs_ast == MEM_NO_IDX) return MEM_NO_IDX;

    bool is_colon_eq = false;
    if (hardcode_is_plain_store_sym(mem, tab, op_sym_id, &is_colon_eq)) {
        const AstNode *ln = mem_get_ast(mem, lhs_ast);
        const AstNode *rn = mem_get_ast(mem, rhs_ast);
        if (ln->kind == AST_SLOT_LOAD && rn->kind == AST_CONSTRUCT && ln->left == 0) {
            P->slot_types[ln->sub] = rn->ref;
        }
        return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_RIGHT, op_sym_id, lhs_ast, rhs_ast);
    }

    StrSlice op_s   = tab->syms[op_sym_id];
    const char *ptr = mem_str_ptr(mem, op_s);
    if (op_s.len < 2 || ptr[op_s.len - 1] != '=') {
        mem_err(mem, "Invalid compound assignment operator '%.*s'", (int)op_s.len, ptr);
        return MEM_NO_IDX;
    }
    int base_sym  = sym_find(tab, mem, ptr, (uint16_t)(op_s.len - 1));
    int store_sym = find_active_meta_sym(tab, INTR_STORE);
    if (base_sym < 0 || store_sym < 0) {
        mem_err(mem, "Base operator for '%.*s' is not active", (int)op_s.len, ptr);
        return MEM_NO_IDX;
    }
    OpFixity base_fix = (sym_get_active_prec(tab, (uint16_t)base_sym, FIX_INFIX_RIGHT) >= 0)
                        ? FIX_INFIX_RIGHT : FIX_INFIX_LEFT;
    uint16_t comb_rhs = mem_alloc_ast(mem, AST_BINARY, (uint8_t)base_fix,
                                      (uint16_t)base_sym, lhs_ast, rhs_ast);
    return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_RIGHT,
                         (uint16_t)store_sym, lhs_ast, comb_rhs);
}


static uint8_t ast_cand_cor_meta(const MemArena *mem, const SymTable *tab,
                                 uint16_t idx, uint16_t *out_sym) {
    if (idx == MEM_NO_IDX) return 0;
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind != AST_BINARY) return 0;
    uint8_t m = sym_active_meta_tag(tab, n->ref, (OpFixity)n->sub);
    if (m == INTR_CAND || m == INTR_COR) {
        if (out_sym) *out_sym = n->ref;
        return m;
    }
    return 0;
}

static bool ast_cnot_meta(const MemArena *mem, const SymTable *tab,
                          uint16_t idx, uint16_t *out_sym) {
    if (idx == MEM_NO_IDX) return false;
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind != AST_UNARY || n->sub != (uint8_t)FIX_PREFIX) return false;
    uint8_t m = sym_active_meta_tag(tab, n->ref, FIX_PREFIX);
    if (m == INTR_CNOT) {
        if (out_sym) *out_sym = n->ref;
        return true;
    }
    return false;
}

static uint16_t build_distributed_comp(Parser *P, OpFixity fix, uint16_t comp_sym,
                                       uint16_t left, uint16_t right) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    uint16_t comb_sym = 0;

    if (ast_cand_cor_meta(mem, tab, left, &comb_sym) != 0) {
        uint16_t ll = mem_get_ast(mem, left)->left;
        uint16_t lr = mem_get_ast(mem, left)->right;
        uint16_t c1 = build_distributed_comp(P, fix, comp_sym, ll, right);
        uint16_t c2 = build_distributed_comp(P, fix, comp_sym, lr, right);
        return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT, comb_sym, c1, c2);
    }
    if (ast_cnot_meta(mem, tab, left, &comb_sym)) {
        uint16_t inner = mem_get_ast(mem, left)->left;
        uint16_t c1 = build_distributed_comp(P, fix, comp_sym, inner, right);
        return mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_PREFIX, comb_sym, c1, MEM_NO_IDX);
    }
    if (ast_cand_cor_meta(mem, tab, right, &comb_sym) != 0) {
        uint16_t rl = mem_get_ast(mem, right)->left;
        uint16_t rr = mem_get_ast(mem, right)->right;
        uint16_t c1 = build_distributed_comp(P, fix, comp_sym, left, rl);
        uint16_t c2 = build_distributed_comp(P, fix, comp_sym, left, rr);
        return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT, comb_sym, c1, c2);
    }
    if (ast_cnot_meta(mem, tab, right, &comb_sym)) {
        uint16_t inner = mem_get_ast(mem, right)->left;
        uint16_t c1 = build_distributed_comp(P, fix, comp_sym, left, inner);
        return mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_PREFIX, comb_sym, c1, MEM_NO_IDX);
    }
    return mem_alloc_ast(mem, AST_BINARY, (uint8_t)fix, comp_sym, left, right);
}

/* ============================================================================
 * 2. STRUCT FIELD RESOLUTION (`.field`) & UFCS METHOD CHAINING (`.fn(...)`)
 * ============================================================================ */

static uint16_t parse_field_step(Parser *P, uint16_t left) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;

    if (P->lex.cur.kind != TOK_IDENT &&
        !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
        mem_err(mem, "Expected struct field or method name after '.'");
        return MEM_NO_IDX;
    }
    const char *fname = P->lex.src + P->lex.cur.off;
    uint16_t flen     = P->lex.cur.len;
    lex_next(&P->lex);

    /* Uniform Function Call Syntax (UFCS): `base.fn(a, b)` -> `fn(base, a, b)` */
    if (P->lex.cur.kind == TOK_LPAREN) {
        lex_next(&P->lex); /* consume '(' */
        uint8_t saved_ternary = P->in_ternary_then;
        P->in_ternary_then = 0;

        uint16_t head = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, left, MEM_NO_IDX);
        uint16_t tail = head;
        uint8_t  argc = 1;

        if (P->lex.cur.kind != TOK_RPAREN) {
            while (!mem->has_error) {
                uint16_t arg  = parser_parse_expr(P, 0);
                uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, arg, MEM_NO_IDX);
                if (head == MEM_NO_IDX) head = link;
                else mem_get_ast_mut(mem, tail)->right = link;
                tail = link;
                argc++;
                if (P->lex.cur.kind == TOK_COMMA) {
                    lex_next(&P->lex);
                } else {
                    break;
                }
            }
        }
        P->in_ternary_then = saved_ternary;
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected ')' after arguments to '.%.*s(...)'", (int)flen, fname);
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);

        int sym_id = sym_find(tab, mem, fname, flen);
        if (sym_id < 0) {
            mem_err(mem, "Undefined method or function '%.*s'", (int)flen, fname);
            return MEM_NO_IDX;
        }
        return mem_alloc_ast(mem, AST_CALL, argc, (uint16_t)sym_id, head, MEM_NO_IDX);
    }

    const AstNode *lnode = mem_get_ast(mem, left);
    uint16_t cur_type    = MEM_TYPE_VOID;
    uint16_t base_ast    = left;
    uint16_t base_slot   = MEM_NO_IDX;
    uint16_t prev_off    = 0;

    if (lnode->kind == AST_SLOT_LOAD) {
        base_slot = lnode->sub;
        cur_type  = P->slot_types[base_slot];
    } else if (lnode->kind == AST_CONSTRUCT) {
        cur_type  = lnode->ref;
    } else if (lnode->kind == AST_FIELD_LOAD) {
        cur_type  = lnode->sub;
        prev_off  = lnode->ref;
        base_ast  = lnode->left;
        base_slot = lnode->right;
    }

    uint16_t ftype = MEM_TYPE_VOID;
    uint16_t foff  = 0;
    bool resolved  = false;

    if (cur_type != MEM_TYPE_VOID && cur_type < tab->type_count &&
        !tab->types[cur_type].is_consuming && tab->types[cur_type].field_count > 0) {
        resolved = sym_resolve_field(tab, mem, cur_type, fname, flen, &ftype, &foff);
    }
    if (!resolved) {
        for (uint16_t tid = 1; tid < tab->type_count; ++tid) {
            if (!tab->types[tid].is_consuming && tab->types[tid].field_count > 0 &&
                sym_mod_enabled(tab, tab->types[tid].mod_id)) {
                if (sym_resolve_field(tab, mem, tid, fname, flen, &ftype, &foff)) {
                    resolved = true;
                    break;
                }
            }
        }
    }
    if (!resolved) {
        mem_err(mem, "Unknown struct field '%.*s'", (int)flen, fname);
        return MEM_NO_IDX;
    }

    if (lnode->kind == AST_FIELD_LOAD) {
        AstNode *mut = mem_get_ast_mut(mem, left);
        mut->sub = (uint8_t)ftype;
        mut->ref = prev_off + foff;
        return left;
    }

    return mem_alloc_ast(mem, AST_FIELD_LOAD, (uint8_t)ftype, prev_off + foff, base_ast, base_slot);
}

/* ============================================================================
 * 2b. MULTIDIMENSIONAL MATRIX LITERAL `[...]` & C-STYLE SUBSCRIPTING `v[i]`
 * ============================================================================ */

static bool cur_is_seq_semicolon(const Parser *P) {
    if (P->lex.cur.kind != TOK_OP) return false;
    return sym_active_meta_tag(P->lex.tab, P->lex.cur.sym_id, FIX_INFIX_LEFT) == INTR_SEQ;
}

static uint16_t parse_mat_literal(Parser *P) {
    MemArena *mem = P->lex.mem;
    lex_next(&P->lex); /* consume '[' */

    uint8_t saved_ternary = P->in_ternary_then;
    P->in_ternary_then = 0;

    if (P->lex.cur.kind == TOK_RBRACK) {
        P->in_ternary_then = saved_ternary;
        lex_next(&P->lex);
        return mem_alloc_ast(mem, AST_MAT_LIT, 0, 0, MEM_NO_IDX, MEM_NO_IDX);
    }

    uint16_t row0_head = MEM_NO_IDX;
    uint16_t row0_tail = MEM_NO_IDX;
    uint8_t  row0_cnt  = 0;

    while (!mem->has_error) {
        uint16_t item = parser_parse_expr(P, 2);
        if (mem->has_error || item == MEM_NO_IDX) {
            P->in_ternary_then = saved_ternary;
            return MEM_NO_IDX;
        }
        uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, item, MEM_NO_IDX);
        if (row0_head == MEM_NO_IDX) row0_head = link;
        else mem_get_ast_mut(mem, row0_tail)->right = link;
        row0_tail = link;
        row0_cnt++;

        if (P->lex.cur.kind == TOK_COMMA) {
            lex_next(&P->lex);
            if (P->lex.cur.kind == TOK_RBRACK) break;
        } else {
            break;
        }
    }

    if (cur_is_seq_semicolon(P)) {
        uint16_t r0_ast    = mem_alloc_ast(mem, AST_MAT_LIT, row0_cnt, 0, row0_head, MEM_NO_IDX);
        uint16_t rows_head = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, r0_ast, MEM_NO_IDX);
        uint16_t rows_tail = rows_head;
        uint8_t  rows_cnt  = 1;

        while (!mem->has_error && cur_is_seq_semicolon(P)) {
            lex_next(&P->lex);
            if (P->lex.cur.kind == TOK_RBRACK) break;

            uint16_t rk_head = MEM_NO_IDX;
            uint16_t rk_tail = MEM_NO_IDX;
            uint8_t  rk_cnt  = 0;
            while (!mem->has_error) {
                uint16_t item = parser_parse_expr(P, 2);
                if (mem->has_error || item == MEM_NO_IDX) {
                    P->in_ternary_then = saved_ternary;
                    return MEM_NO_IDX;
                }
                uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, item, MEM_NO_IDX);
                if (rk_head == MEM_NO_IDX) rk_head = link;
                else mem_get_ast_mut(mem, rk_tail)->right = link;
                rk_tail = link;
                rk_cnt++;

                if (P->lex.cur.kind == TOK_COMMA) {
                    lex_next(&P->lex);
                } else {
                    break;
                }
            }
            uint16_t rk_ast  = mem_alloc_ast(mem, AST_MAT_LIT, rk_cnt, 0, rk_head, MEM_NO_IDX);
            uint16_t rk_link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, rk_ast, MEM_NO_IDX);
            mem_get_ast_mut(mem, rows_tail)->right = rk_link;
            rows_tail = rk_link;
            rows_cnt++;
        }

        P->in_ternary_then = saved_ternary;
        if (P->lex.cur.kind != TOK_RBRACK) {
            mem_err(mem, "Expected closing ']' after matrix literal");
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);
        return mem_alloc_ast(mem, AST_MAT_LIT, rows_cnt, 0, rows_head, MEM_NO_IDX);
    }

    P->in_ternary_then = saved_ternary;
    if (P->lex.cur.kind != TOK_RBRACK) {
        mem_err(mem, "Expected closing ']' after matrix literal");
        return MEM_NO_IDX;
    }
    lex_next(&P->lex);
    return mem_alloc_ast(mem, AST_MAT_LIT, row0_cnt, 0, row0_head, MEM_NO_IDX);
}

static uint16_t parse_index_step(Parser *P, uint16_t left) {
    MemArena *mem = P->lex.mem;
    lex_next(&P->lex); /* consume '[' */

    uint8_t saved_ternary = P->in_ternary_then;
    P->in_ternary_then = 0;

    if (P->lex.cur.kind == TOK_RBRACK) {
        P->in_ternary_then = saved_ternary;
        mem_err(mem, "Empty subscript '[]' is not allowed");
        return MEM_NO_IDX;
    }

    uint16_t idx_head = MEM_NO_IDX;
    uint16_t idx_tail = MEM_NO_IDX;
    uint8_t  argc     = 0;

    while (!mem->has_error) {
        if (argc >= 4) {
            P->in_ternary_then = saved_ternary;
            mem_err(mem, "Maximum 4 subscript indices supported");
            return MEM_NO_IDX;
        }
        uint16_t idx_expr = parser_parse_expr(P, 2);
        if (mem->has_error || idx_expr == MEM_NO_IDX) {
            P->in_ternary_then = saved_ternary;
            return MEM_NO_IDX;
        }
        uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, idx_expr, MEM_NO_IDX);
        if (idx_head == MEM_NO_IDX) idx_head = link;
        else mem_get_ast_mut(mem, idx_tail)->right = link;
        idx_tail = link;
        argc++;

        if (P->lex.cur.kind == TOK_COMMA) {
            lex_next(&P->lex);
        } else {
            break;
        }
    }

    P->in_ternary_then = saved_ternary;
    if (P->lex.cur.kind != TOK_RBRACK) {
        mem_err(mem, "Expected closing ']' after subscript index");
        return MEM_NO_IDX;
    }
    lex_next(&P->lex);
    return mem_alloc_ast(mem, AST_INDEX, argc, 0, left, idx_head);
}

/* ============================================================================
 * 3. LEXICAL SCOPE BLOCKS `{ ... }`, `if/or/else`, `switch`, AND LOOPS
 * ============================================================================ */

static uint16_t parse_scope_block(Parser *P) {
    MemArena *mem = P->lex.mem;
    if (P->lex.cur.kind != TOK_LBRACE) {
        mem_err(mem, "Expected '{' to open local scope block");
        return MEM_NO_IDX;
    }
    lex_next(&P->lex);

    uint8_t saved_vis     = P->vis_count;
    uint8_t saved_ternary = P->in_ternary_then;
    P->scope_depth++;
    P->in_ternary_then = 0;

    while (cur_op_meta(P) == INTR_SEQ) {
        lex_next(&P->lex);
    }

    uint16_t body = MEM_NO_IDX;
    if (P->lex.cur.kind == TOK_RBRACE) {
        body = mem_alloc_ast(mem, AST_LIT, MEM_TYPE_VOID, 0, 0, 0);
    } else {
        body = parser_parse_expr(P, 0);
    }

    while (cur_op_meta(P) == INTR_SEQ) {
        lex_next(&P->lex);
    }

    if (P->lex.cur.kind != TOK_RBRACE) {
        mem_err(mem, "Expected closing '}' for local scope block");
    } else {
        lex_next(&P->lex);
    }

    P->vis_count       = saved_vis;
    P->scope_depth--;
    P->in_ternary_then = saved_ternary;
    P->last_was_block  = true;
    return body;
}

static uint16_t parse_branch_body(Parser *P) {
    if (P->lex.cur.kind == TOK_LBRACE) {
        return parse_scope_block(P);
    }
    return parser_parse_expr(P, 16);
}

/*
 * Parse `(i in 1...5) { ... }`, `(i in 1..5) { ... }`, or `(v in vec) { ... }`.
 */
static uint16_t parse_for_in_loop(Parser *P) {
    MemArena *mem = P->lex.mem;
    if (P->lex.cur.kind != TOK_LPAREN) {
        mem_err(mem, "Expected '(' to start loop header");
        return MEM_NO_IDX;
    }
    lex_next(&P->lex); /* consume '(' */

    if (P->lex.cur.kind != TOK_IDENT &&
        !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
        mem_err(mem, "Expected loop variable name before 'in'");
        return MEM_NO_IDX;
    }
    const char *vname = P->lex.src + P->lex.cur.off;
    uint16_t    vlen  = P->lex.cur.len;
    int var_slot = parser_add_local(P, vname, vlen, mem->fast_real_type);
    if (mem->has_error) return MEM_NO_IDX;
    lex_next(&P->lex);

    if (P->lex.cur.len != 2 || memcmp(P->lex.src + P->lex.cur.off, "in", 2) != 0) {
        mem_err(mem, "Expected 'in' after loop variable '%.*s'", (int)vlen, vname);
        return MEM_NO_IDX;
    }
    lex_next(&P->lex); /* consume 'in' */

    uint8_t saved_ternary = P->in_ternary_then;
    P->in_ternary_then = 0;
    uint16_t iter_ast = parser_parse_expr(P, 0);
    P->in_ternary_then = saved_ternary;
    if (mem->has_error || iter_ast == MEM_NO_IDX) return MEM_NO_IDX;

    if (P->lex.cur.kind != TOK_RPAREN) {
        mem_err(mem, "Expected ')' after loop iterable");
        return MEM_NO_IDX;
    }
    lex_next(&P->lex); /* consume ')' */

    /* Allow optional `??` between `(i in ...)` and `{ ... }` */
    if (cur_op_meta(P) == INTR_WHILE) {
        lex_next(&P->lex);
    }

    uint16_t body_ast = parse_branch_body(P);
    if (mem->has_error || body_ast == MEM_NO_IDX) return MEM_NO_IDX;
    P->last_was_block = true;
    return mem_alloc_ast(mem, AST_FOR_IN, (uint8_t)var_slot, 0, iter_ast, body_ast);
}

/*
 * Parse `loop (i in 1...5) { ... }`, `loop (cond) { ... }`, or `loop { ... }`.
 */
static uint16_t parse_loop_construct(Parser *P) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;

    if (hardcode_is_paren_for_in(P, true)) {
        return parse_for_in_loop(P);
    }

    int while_sym = find_active_meta_sym(tab, INTR_WHILE);
    if (while_sym < 0) {
        mem_err(mem, "'loop' requires active while intrinsic in [boot_control]");
        return MEM_NO_IDX;
    }

    /* `loop { ... }` -> unconditional loop (exited via `break`) */
    if (P->lex.cur.kind == TOK_LBRACE) {
        uint16_t pidx = mem_alloc_pool(mem, 1);
        if (mem->has_error) return MEM_NO_IDX;
        mem_write_pool(mem, pidx, 1.0);
        uint16_t one_ast  = mem_alloc_ast(mem, AST_LIT, mem->fast_real_type, pidx, 0, 0);
        uint16_t body_ast = parse_scope_block(P);
        P->last_was_block = true;
        return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                             (uint16_t)while_sym, one_ast, body_ast);
    }

    /* `loop (cond) { ... }` -> conditional loop */
    uint16_t cond_ast = MEM_NO_IDX;
    if (P->lex.cur.kind == TOK_LPAREN) {
        lex_next(&P->lex);
        cond_ast = parser_parse_expr(P, 0);
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected ')' after 'loop' condition");
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);
    } else {
        cond_ast = parser_parse_expr(P, 16);
    }
    if (cur_op_meta(P) == INTR_WHILE) {
        lex_next(&P->lex);
    }
    uint16_t body_ast = parse_branch_body(P);
    P->last_was_block = true;
    return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                         (uint16_t)while_sym, cond_ast, body_ast);
}

static uint16_t parse_if_construct(Parser *P, uint16_t if_sym_id) {
    MemArena *mem = P->lex.mem;
    uint16_t cond1 = MEM_NO_IDX;

    if (P->lex.cur.kind == TOK_LPAREN) {
        lex_next(&P->lex);
        cond1 = parser_parse_expr(P, 0);
        if (P->lex.cur.kind == TOK_COMMA) {
            lex_next(&P->lex);
            uint16_t then_val = parser_parse_expr(P, 0);
            if (P->lex.cur.kind != TOK_COMMA) {
                mem_err(mem, "Expected ',' before else-branch in 3-argument if(cond, then, else)");
                return MEM_NO_IDX;
            }
            lex_next(&P->lex);
            uint16_t else_val = parser_parse_expr(P, 0);
            if (P->lex.cur.kind != TOK_RPAREN) {
                mem_err(mem, "Expected ')' after if(cond, then, else)");
            } else {
                lex_next(&P->lex);
            }
            uint16_t l3 = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, else_val, MEM_NO_IDX);
            uint16_t l2 = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, then_val, l3);
            uint16_t l1 = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, cond1, l2);
            return mem_alloc_ast(mem, AST_CALL, 3, if_sym_id, l1, MEM_NO_IDX);
        }
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected ')' after 'if' condition");
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);
    } else {
        cond1 = parser_parse_expr(P, 16);
    }

    if (mem->has_error || cond1 == MEM_NO_IDX) return MEM_NO_IDX;

    uint16_t conds[32];
    uint16_t bodies[32];
    uint8_t  branch_cnt = 0;

    conds[branch_cnt]  = cond1;
    bodies[branch_cnt] = parse_branch_body(P);
    branch_cnt++;

    uint16_t else_body = MEM_NO_IDX;
    bool     seen_else = false;

    while (!mem->has_error) {
        if (cur_op_meta(P) == INTR_SEQ && lex_peek_is_branch_kw(P)) {
            lex_next(&P->lex);
        }

        uint8_t kw_meta = token_func_meta(P, P->lex.cur);
        if (kw_meta == INTR_OR_BRANCH) {
            if (seen_else) {
                mem_err(mem, "'or' branch cannot appear after 'else' block");
                return MEM_NO_IDX;
            }
            if (branch_cnt >= 32) {
                mem_err(mem, "Too many 'or' branches in 'if' statement (max 32)");
                return MEM_NO_IDX;
            }
            lex_next(&P->lex);

            if (P->lex.cur.kind == TOK_LBRACE) {
                mem_err(mem, "Expected condition after 'or' (use 'else { ... }' for the fallback branch)");
                return MEM_NO_IDX;
            }

            uint16_t c_next = MEM_NO_IDX;
            if (P->lex.cur.kind == TOK_LPAREN) {
                lex_next(&P->lex);
                c_next = parser_parse_expr(P, 0);
                if (P->lex.cur.kind != TOK_RPAREN) {
                    mem_err(mem, "Expected ')' after 'or' condition");
                    return MEM_NO_IDX;
                }
                lex_next(&P->lex);
            } else {
                c_next = parser_parse_expr(P, 16);
            }

            uint16_t b_next = parse_branch_body(P);
            conds[branch_cnt]  = c_next;
            bodies[branch_cnt] = b_next;
            branch_cnt++;
            continue;
        }

        if (kw_meta == INTR_ELSE_BRANCH) {
            if (seen_else) {
                mem_err(mem, "Only one 'else' block is allowed in an 'if' statement");
                return MEM_NO_IDX;
            }
            lex_next(&P->lex);

            if (token_func_meta(P, P->lex.cur) == INTR_IF) {
                mem_err(mem, "Only one 'if' is allowed per chain; use 'or (cond) { ... }' for else-if branches");
                return MEM_NO_IDX;
            }

            else_body = parse_branch_body(P);
            seen_else = true;
            continue;
        }

        break;
    }

    uint16_t acc = else_body;
    for (int i = (int)branch_cnt - 1; i >= 0; --i) {
        uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, bodies[i], acc);
        acc = mem_alloc_ast(mem, AST_TERNARY, 0, 0, conds[i], link);
    }
    P->last_was_block = true;
    return acc;
}

static uint16_t parse_switch_construct(Parser *P) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;

    int store_sym = find_active_meta_sym(tab, INTR_STORE);
    int seq_sym   = find_active_meta_sym(tab, INTR_SEQ);
    if (store_sym < 0 || seq_sym < 0) {
        mem_err(mem, "Switch requires active assignment and sequence operators in [boot_control]");
        return MEM_NO_IDX;
    }

    uint16_t target_expr = MEM_NO_IDX;
    if (P->lex.cur.kind == TOK_LPAREN) {
        lex_next(&P->lex);
        target_expr = parser_parse_expr(P, 0);
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected ')' after 'switch' expression");
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);
    } else {
        target_expr = parser_parse_expr(P, 2);
    }

    if (P->lex.cur.kind != TOK_LBRACE) {
        mem_err(mem, "Expected '{' to open 'switch' block");
        return MEM_NO_IDX;
    }
    lex_next(&P->lex);

    uint8_t  sw_slot  = parser_alloc_anon_slot(P, mem->fast_real_type);
    uint16_t slot_ast = mem_alloc_ast(mem, AST_SLOT_LOAD, sw_slot, 0, 0, 0);
    uint16_t init_ast = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_RIGHT,
                                      (uint16_t)store_sym, slot_ast, target_expr);

    uint16_t conds[32];
    uint16_t bodies[32];
    uint8_t  case_cnt     = 0;
    uint16_t default_body = MEM_NO_IDX;
    bool     seen_default = false;

    while (!mem->has_error && P->lex.cur.kind != TOK_RBRACE && P->lex.cur.kind != TOK_EOF) {
        while (cur_op_meta(P) == INTR_SEQ || P->lex.cur.kind == TOK_COMMA) {
            lex_next(&P->lex);
        }
        if (P->lex.cur.kind == TOK_RBRACE || P->lex.cur.kind == TOK_EOF) break;

        uint8_t kw_meta = token_func_meta(P, P->lex.cur);
        if (kw_meta == INTR_ELSE_BRANCH) {
            if (seen_default) {
                mem_err(mem, "Only one 'else'/'default' branch is allowed in a 'switch' block");
                return MEM_NO_IDX;
            }
            lex_next(&P->lex);
            if (cur_op_meta(P) == INTR_COLON) {
                lex_next(&P->lex);
            }
            default_body = parse_branch_body(P);
            seen_default = true;
            continue;
        }

        if (kw_meta == INTR_CASE_BRANCH) {
            lex_next(&P->lex);
        }

        uint16_t case_vals[8];
        uint8_t  val_cnt = 0;
        while (!mem->has_error && val_cnt < 8) {
            P->in_ternary_then++;
            uint16_t v_ast = parser_parse_expr(P, 11);
            P->in_ternary_then--;
            if (mem->has_error || v_ast == MEM_NO_IDX) return MEM_NO_IDX;
            case_vals[val_cnt++] = v_ast;
            if (P->lex.cur.kind == TOK_COMMA) {
                lex_next(&P->lex);
            } else {
                break;
            }
        }

        if (cur_op_meta(P) == INTR_COLON) {
            lex_next(&P->lex);
        }

        uint16_t body_ast = parse_branch_body(P);
        for (uint8_t k = 0; k < val_cnt; ++k) {
            if (case_cnt >= 32) {
                mem_err(mem, "Too many cases in 'switch' block (max 32)");
                return MEM_NO_IDX;
            }
            conds[case_cnt]  = mem_alloc_ast(mem, AST_SWITCH_EQ, 0, 0, slot_ast, case_vals[k]);
            bodies[case_cnt] = body_ast;
            case_cnt++;
        }
    }

    if (P->lex.cur.kind != TOK_RBRACE) {
        mem_err(mem, "Expected closing '}' for 'switch' block");
    } else {
        lex_next(&P->lex);
    }

    uint16_t acc = default_body;
    for (int i = (int)case_cnt - 1; i >= 0; --i) {
        uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, bodies[i], acc);
        acc = mem_alloc_ast(mem, AST_TERNARY, 0, 0, conds[i], link);
    }
    if (acc == MEM_NO_IDX) {
        acc = mem_alloc_ast(mem, AST_LIT, MEM_TYPE_VOID, 0, 0, 0);
    }

    P->last_was_block = true;
    return mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                         (uint16_t)seq_sym, init_ast, acc);
}

static uint16_t parse_tag_op(Parser *P, AstKind kind, const char *op_name) {
    MemArena *mem = P->lex.mem;
    if (P->lex.cur.kind != TOK_LPAREN) {
        mem_err(mem, "Expected '(' after '%s'", op_name);
        return MEM_NO_IDX;
    }
    lex_next(&P->lex);
    if (P->lex.cur.kind != TOK_IDENT &&
        !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
        mem_err(mem, "Expected scope tag name inside '%s(...)'", op_name);
        return MEM_NO_IDX;
    }
    const char *tname = P->lex.src + P->lex.cur.off;
    uint16_t    tlen  = P->lex.cur.len;
    int label_id = parser_get_or_add_label(P, tname, tlen);
    if (mem->has_error) return MEM_NO_IDX;
    lex_next(&P->lex);

    if (P->lex.cur.kind != TOK_RPAREN) {
        mem_err(mem, "Expected ')' after scope tag in '%s(%.*s)'", op_name, (int)tlen, tname);
        return MEM_NO_IDX;
    }
    lex_next(&P->lex);
    return mem_alloc_ast(mem, (uint8_t)kind, (uint8_t)label_id, 0, MEM_NO_IDX, MEM_NO_IDX);
}

static uint16_t parse_break_construct(Parser *P) {
    MemArena *mem = P->lex.mem;
    if (P->lex.cur.kind == TOK_LPAREN) {
        lex_next(&P->lex);
        if (P->lex.cur.kind == TOK_RPAREN) {
            lex_next(&P->lex);
            return mem_alloc_ast(mem, AST_BREAK, 0xFFu, 0, MEM_NO_IDX, MEM_NO_IDX);
        }
        if (P->lex.cur.kind != TOK_IDENT &&
            !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
            mem_err(mem, "Expected loop/scope tag name inside 'break(...)'");
            return MEM_NO_IDX;
        }
        const char *tname = P->lex.src + P->lex.cur.off;
        uint16_t    tlen  = P->lex.cur.len;
        int label_id = parser_get_or_add_label(P, tname, tlen);
        if (mem->has_error) return MEM_NO_IDX;
        lex_next(&P->lex);
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected ')' after tag in 'break(%.*s)'", (int)tlen, tname);
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);
        return mem_alloc_ast(mem, AST_BREAK, (uint8_t)label_id, 0, MEM_NO_IDX, MEM_NO_IDX);
    }

    if (P->lex.cur.kind == TOK_IDENT && token_func_meta(P, P->lex.cur) == 0) {
        const char *tname = P->lex.src + P->lex.cur.off;
        uint16_t    tlen  = P->lex.cur.len;
        int label_id = parser_get_or_add_label(P, tname, tlen);
        if (mem->has_error) return MEM_NO_IDX;
        lex_next(&P->lex);
        return mem_alloc_ast(mem, AST_BREAK, (uint8_t)label_id, 0, MEM_NO_IDX, MEM_NO_IDX);
    }

    return mem_alloc_ast(mem, AST_BREAK, 0xFFu, 0, MEM_NO_IDX, MEM_NO_IDX);
}

static uint16_t resolve_var_for_assign(Parser *P, const char *name, uint16_t len,
                                       uint16_t op_sym_id,
                                       uint8_t lock_type, uint8_t lock_unit) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    if (sym_is_reserved_kw(tab, mem, name, len)) {
        mem_err(mem, "Cannot use reserved keyword '%.*s' as a variable name",
                (int)len, name);
        return MEM_NO_IDX;
    }
    int vidx = find_local_idx(P, name, len);
    bool is_colon_eq = false;
    bool is_plain    = hardcode_is_plain_store_sym(mem, tab, op_sym_id, &is_colon_eq);

    if (is_plain) {
        uint8_t slot = 0;
        uint16_t init_tid = lock_type ? lock_type : mem->fast_real_type;
        if (vidx < 0 || (is_colon_eq && P->scope_depth > P->locals[vidx].scope_depth)) {
            slot = (uint8_t)parser_add_local(P, name, len, init_tid);
            if (mem->has_error) return MEM_NO_IDX;
        } else {
            slot = P->locals[vidx].slot;
            if (lock_type != 0) {
                P->slot_types[slot] = lock_type;
            }
        }
        return mem_alloc_ast(mem, AST_SLOT_LOAD, slot, 0, lock_type, lock_unit);
    }

    if (vidx < 0) {
        mem_err(mem, "Cannot apply compound assignment to undeclared variable '%.*s'",
                (int)len, name);
        return MEM_NO_IDX;
    }
    return mem_alloc_ast(mem, AST_SLOT_LOAD, P->locals[vidx].slot, 0, lock_type, lock_unit);
}

/* ============================================================================
 * 4. PRATT PARSER NUD & LED DRIVERS
 * ============================================================================ */

static uint16_t parse_nud(Parser *P) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    Token t = P->lex.cur;
    P->last_was_block = false;

    /* 1. Consumed Literal Value ($real, $str) */
    if (t.kind == TOK_VAL) {
        lex_next(&P->lex);
        uint16_t tid = val_get_type(mem, t.val);
        if (tid == mem->str_type_id && tid != 0) {
            StrSlice s = val_get_str(mem, t.val);
            return mem_alloc_ast(mem, AST_LIT, (uint8_t)tid, 0, s.off, s.len);
        } else {
            uint16_t pidx = mem_alloc_pool(mem, 1);
            if (mem->has_error) return MEM_NO_IDX;
            mem_write_pool(mem, pidx, val_get_f64(mem, t.val));
            return mem_alloc_ast(mem, AST_LIT, (uint8_t)tid, pidx, 0, 0);
        }
    }

    /* 2. Parenthesized Expression `( ... )` OR Keywordless Loop `(i in 1...5) { ... }` */
    if (t.kind == TOK_LPAREN) {
        if (hardcode_is_paren_for_in(P, false)) {
            return parse_for_in_loop(P);
        }
        lex_next(&P->lex);
        uint8_t saved_ternary = P->in_ternary_then;
        P->in_ternary_then = 0;
        uint16_t inner = parser_parse_expr(P, 0);
        P->in_ternary_then = saved_ternary;
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected closing delimiter ')'");
        } else {
            lex_next(&P->lex);
        }
        return inner;
    }

    /* 3. Local Scope Block `{ stmt1; stmt2; expr }` or `{}` */
    if (t.kind == TOK_LBRACE) {
        return parse_scope_block(P);
    }

    /* 3b. Multidimensional Matrix Literal `[1, 2, 3]`, `[[1, 2], [3, 4]]`, or `[1, 2 ; 3, 4]` */
    if (t.kind == TOK_LBRACK) {
        return parse_mat_literal(P);
    }

    /* 4. Private C Intrinsic `@add(a, b)` OR Host Extern Callback `@fx_rate(from, to)` */
    if (t.kind == TOK_AT) {
        if (!P->in_ext_body) {
            mem_err(mem, "Private '@' intrinsics/externs cannot be called outside extensions");
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);
        if (P->lex.cur.kind != TOK_IDENT &&
            !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
            mem_err(mem, "Expected intrinsic or extern name after '@'");
            return MEM_NO_IDX;
        }
        const char *iname = P->lex.src + P->lex.cur.off;
        uint16_t    ilen  = P->lex.cur.len;
        IntrinsicId intr  = sym_lookup_intrinsic(iname, ilen);
        lex_next(&P->lex);

        if (P->lex.cur.kind != TOK_LPAREN) {
            mem_err(mem, "Expected '(' after '@%.*s'", (int)ilen, iname);
            return MEM_NO_IDX;
        }
        lex_next(&P->lex);

        if (intr != INTR_NONE) {
            uint16_t arg1 = MEM_NO_IDX;
            uint16_t arg2 = MEM_NO_IDX;
            uint16_t arg3 = MEM_NO_IDX;
            if (P->lex.cur.kind != TOK_RPAREN) {
                arg1 = parser_parse_expr(P, 0);
                if (P->lex.cur.kind == TOK_COMMA) {
                    lex_next(&P->lex);
                    arg2 = parser_parse_expr(P, 0);
                    if (P->lex.cur.kind == TOK_COMMA) {
                        lex_next(&P->lex);
                        arg3 = parser_parse_expr(P, 0);
                    }
                }
            }
            if (P->lex.cur.kind != TOK_RPAREN) {
                mem_err(mem, "Expected ')' after intrinsic arguments");
            } else {
                lex_next(&P->lex);
            }
            return mem_alloc_ast(mem, AST_INTRINSIC, (uint8_t)intr, arg3, arg1, arg2);
        }

        StrSlice ext_slice  = mem_intern_str(mem, iname, ilen);
        uint16_t ext_sym_id = sym_intern((SymTable *)tab, mem, ext_slice);
        uint16_t head = MEM_NO_IDX;
        uint16_t tail = MEM_NO_IDX;
        uint8_t  argc = 0;

        if (P->lex.cur.kind != TOK_RPAREN) {
            while (1) {
                uint16_t arg  = parser_parse_expr(P, 0);
                uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, arg, MEM_NO_IDX);
                if (head == MEM_NO_IDX) head = link;
                else mem_get_ast_mut(mem, tail)->right = link;
                tail = link;
                argc++;
                if (P->lex.cur.kind == TOK_COMMA) {
                    lex_next(&P->lex);
                } else {
                    break;
                }
            }
        }
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected ')' after arguments to '@%.*s'", (int)ilen, iname);
        } else {
            lex_next(&P->lex);
        }
        return mem_alloc_ast(mem, AST_EXTERN, argc, ext_sym_id, head, MEM_NO_IDX);
    }

    /* 5. Prefix Unary Operator (`-x`, `√x`, `not x`, `!x`, `++x`, `+++x`) */
    if (t.kind == TOK_OP) {
        if (sym_active_meta_tag(tab, t.sym_id, FIX_FUNC) == INTR_OR_BRANCH) {
            t.kind = TOK_IDENT;
        } else {
            int prec = sym_get_active_prec(tab, t.sym_id, FIX_PREFIX);
            bool is_word_local = lex_is_ident_start((unsigned char)P->lex.src[t.off]) &&
                                 (find_local_idx(P, P->lex.src + t.off, t.len) >= 0 || prec < 0);
            if (!is_word_local) {
                if (prec < 0) {
                    StrSlice s = tab->syms[t.sym_id];
                    mem_err(mem, "Operator '%.*s' has no active prefix definition",
                            (int)s.len, mem_str_ptr(mem, s));
                    return MEM_NO_IDX;
                }
                lex_next(&P->lex);
                uint16_t operand = parser_parse_expr(P, prec);
                return mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_PREFIX, t.sym_id, operand, MEM_NO_IDX);
            }
            t.kind = TOK_IDENT;
        }
    }

    /* 6. Identifier, Control Construct, Tagged Scope, Typed Variable Declaration,
          or Multi-Variable Declaration / Destructuring */
    if (t.kind == TOK_IDENT) {
        const char *id_str = P->lex.src + t.off;
        uint16_t id_len    = t.len;
        int sym_id         = sym_find(tab, mem, id_str, id_len);
        uint8_t fn_meta    = (sym_id >= 0) ? sym_active_meta_tag(tab, (uint16_t)sym_id, FIX_FUNC) : 0;

        lex_next(&P->lex);

        /* 6.0 Reserved keyword guard: check if a control-flow keyword is being used
               as a variable name (`if := 5`, `else : real := 5`, `or, x := 5`)
               or as a scope tag (`if : { ... }`, `else : { ... }`) */
        if (sym_is_reserved_kw(tab, mem, id_str, id_len)) {
            if (P->in_ternary_then == 0 && cur_op_meta(P) == INTR_COLON) {
                if (lex_peek_is_lbrace(&P->lex) || lex_peek_is_lparen(&P->lex)) {
                    mem_err(mem, "Cannot use reserved keyword '%.*s' as a scope tag",
                            (int)id_len, id_str);
                } else {
                    mem_err(mem, "Cannot use reserved keyword '%.*s' as a variable name",
                            (int)id_len, id_str);
                }
                return MEM_NO_IDX;
            }
            if ((P->lex.cur.kind == TOK_OP &&
                 sym_active_meta_tag(tab, P->lex.cur.sym_id, FIX_INFIX_RIGHT) == INTR_STORE) ||
                (P->lex.cur.kind == TOK_COMMA && lex_is_multi_lhs(P))) {
                mem_err(mem, "Cannot use reserved keyword '%.*s' as a variable name",
                        (int)id_len, id_str);
                return MEM_NO_IDX;
            }
        }

        /* 6a. Control-flow constructs bound in `00_bootstrap.math` */
        if (fn_meta == INTR_IF) {
            return parse_if_construct(P, (uint16_t)sym_id);
        }
        if (fn_meta == INTR_SWITCH_BLOCK) {
            return parse_switch_construct(P);
        }
        if (fn_meta == INTR_LOOP_KW) {
            return parse_loop_construct(P);
        }
        if (fn_meta == INTR_JUMP_TAG) {
            return parse_tag_op(P, AST_JUMP, "jump");
        }
        if (fn_meta == INTR_RUN_TAG) {
            return parse_tag_op(P, AST_RUN_TAG, "run");
        }
        if (fn_meta == INTR_BREAK_LOOP) {
            return parse_break_construct(P);
        }
        if (fn_meta == INTR_OR_BRANCH) {
            mem_err(mem, "Unexpected 'or'/'then' without preceding 'if' block");
            return MEM_NO_IDX;
        }
        if (fn_meta == INTR_ELSE_BRANCH) {
            mem_err(mem, "Unexpected 'else' without preceding 'if' or 'switch' block");
            return MEM_NO_IDX;
        }
        if (fn_meta == INTR_CASE_BRANCH) {
            mem_err(mem, "Unexpected 'case' outside of 'switch' block");
            return MEM_NO_IDX;
        }

        /* 6b. Colon-Tagged Scope or Loop (`Tag : { ... }`, `Tag : (i in 1...5) { ... }`, `Tag : loop ...`) */
        if (P->in_ternary_then == 0 && cur_op_meta(P) == INTR_COLON &&
            (lex_peek_is_lbrace(&P->lex) || lex_peek_is_lparen(&P->lex))) {
            bool is_brace_scope = lex_peek_is_lbrace(&P->lex);
            lex_next(&P->lex);
            int label_id = parser_get_or_add_label(P, id_str, id_len);
            if (mem->has_error) return MEM_NO_IDX;
            if (P->labels[label_id].defined) {
                mem_err(mem, "Duplicate scope tag '%.*s' in the same function/expression",
                        (int)id_len, id_str);
                return MEM_NO_IDX;
            }
            P->labels[label_id].defined = true;
            uint16_t body = is_brace_scope ? parse_scope_block(P) : parser_parse_expr(P, 2);
            P->labels[label_id].body_ast = body;
            P->last_was_block = true;
            return mem_alloc_ast(mem, AST_TAGGED_SCOPE, (uint8_t)label_id, 0, body, MEM_NO_IDX);
        }

        /* 6c. Single-Variable Concrete Type/Unit Declaration (`x : real := 5`,
               `x : real, distance := 50m`, `x : distance := 50m`) */
        if (P->in_ternary_then == 0 && cur_op_meta(P) == INTR_COLON) {
            uint8_t lock_type = 0, lock_unit = 0;
            if (!parse_var_type_unit_annot(P, &lock_type, &lock_unit)) {
                return MEM_NO_IDX;
            }
            if (P->lex.cur.kind != TOK_OP ||
                !hardcode_is_plain_store_sym(mem, tab, P->lex.cur.sym_id, NULL)) {
                mem_err(mem, "Expected ':=' or '=' after variable type annotation");
                return MEM_NO_IDX;
            }
            return resolve_var_for_assign(P, id_str, id_len, P->lex.cur.sym_id, lock_type, lock_unit);
        }

        /* 6d. Multi-Variable Declaration / Destructuring / Compound Assignment
               (`x, y := 5`, `r1, r2 := quad_roots(1, -5, 6)`, `a, b, c := [10, 20, 30]`, `x, y += 2`) */
        if (P->lex.cur.kind == TOK_COMMA && lex_is_multi_lhs(P)) {
            const char *var_names[8];
            uint16_t    var_lens[8];
            uint8_t     var_cnt = 0;

            var_names[var_cnt] = id_str;
            var_lens[var_cnt]  = id_len;
            var_cnt++;

            while (P->lex.cur.kind == TOK_COMMA && var_cnt < 8) {
                lex_next(&P->lex);
                if (P->lex.cur.kind != TOK_IDENT &&
                    !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
                    mem_err(mem, "Expected variable name after ',' in multi-variable assignment");
                    return MEM_NO_IDX;
                }
                var_names[var_cnt] = P->lex.src + P->lex.cur.off;
                var_lens[var_cnt]  = P->lex.cur.len;
                var_cnt++;
                lex_next(&P->lex);
            }

            uint8_t lock_type = 0, lock_unit = 0;
            if (cur_op_meta(P) == INTR_COLON) {
                if (!parse_var_type_unit_annot(P, &lock_type, &lock_unit)) {
                    return MEM_NO_IDX;
                }
            }

            if (P->lex.cur.kind != TOK_OP ||
                sym_active_meta_tag(tab, P->lex.cur.sym_id, FIX_INFIX_RIGHT) != INTR_STORE) {
                mem_err(mem, "Expected assignment operator after multi-variable list");
                return MEM_NO_IDX;
            }
            uint16_t op_sym_id = P->lex.cur.sym_id;
            int prec = sym_get_active_prec(tab, op_sym_id, FIX_INFIX_RIGHT);
            lex_next(&P->lex);

            uint16_t slot_asts[8];
            for (uint8_t i = 0; i < var_cnt; ++i) {
                slot_asts[i] = resolve_var_for_assign(P, var_names[i], var_lens[i],
                                                      op_sym_id, lock_type, lock_unit);
                if (mem->has_error) return MEM_NO_IDX;
            }

            uint16_t rhs = parser_parse_expr(P, prec >= 0 ? prec : 5);
            if (mem->has_error || rhs == MEM_NO_IDX) return MEM_NO_IDX;

            int seq_sym = find_active_meta_sym(tab, INTR_SEQ);
            if (hardcode_is_plain_store_sym(mem, tab, op_sym_id, NULL) && seq_sym >= 0) {
                uint8_t  tmp_slot = parser_alloc_anon_slot(P, mem->fast_real_type);
                uint16_t tmp_load = mem_alloc_ast(mem, AST_SLOT_LOAD, tmp_slot, 0, 0, 0);
                uint16_t acc      = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_RIGHT,
                                                  op_sym_id, tmp_load, rhs);
                for (uint8_t i = 0; i < var_cnt; ++i) {
                    uint16_t unpack_i = mem_alloc_ast(mem, AST_UNPACK_ELEM, i, var_cnt, tmp_slot, 0);
                    uint16_t store_i  = build_store_or_compound(P, op_sym_id, slot_asts[i], unpack_i);
                    acc = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                                        (uint16_t)seq_sym, acc, store_i);
                }
                return acc;
            } else {
                uint16_t acc = build_store_or_compound(P, op_sym_id, slot_asts[0], rhs);
                for (uint8_t i = 1; i < var_cnt; ++i) {
                    uint16_t step = build_store_or_compound(P, op_sym_id, slot_asts[i], rhs);
                    acc = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                                        (uint16_t)seq_sym, acc, step);
                }
                return acc;
            }
        }

        /* 6e. Single Local Variable or Parameter Slot */
        int vidx = find_local_idx(P, id_str, id_len);
        bool is_assign = (P->lex.cur.kind == TOK_OP &&
                          sym_active_meta_tag(tab, P->lex.cur.sym_id, FIX_INFIX_RIGHT) == INTR_STORE);
        if (is_assign) {
            return resolve_var_for_assign(P, id_str, id_len, P->lex.cur.sym_id, 0, 0);
        }
        if (vidx >= 0) {
            uint8_t slot = P->locals[vidx].slot;
            return mem_alloc_ast(mem, AST_SLOT_LOAD, slot, 0, 0, 0);
        }

        /* 6f. Call syntax `name(arg1, arg2, ...)` -> Struct, Function, or `Tag()` reuse */
        if (P->lex.cur.kind == TOK_LPAREN) {
            lex_next(&P->lex);
            uint8_t saved_ternary = P->in_ternary_then;
            P->in_ternary_then = 0;

            uint16_t head = MEM_NO_IDX;
            uint16_t tail = MEM_NO_IDX;
            uint8_t  argc = 0;

            if (P->lex.cur.kind != TOK_RPAREN) {
                while (1) {
                    uint16_t arg = MEM_NO_IDX;
                    if (argc == 0 && fn_meta >= INTR_CALC_DIFF && fn_meta <= INTR_CALC_FMAX) {
                        if (P->lex.cur.kind != TOK_IDENT &&
                            !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
                            mem_err(mem, "First argument to '%.*s' must be a variable identifier (e.g. x)",
                                    (int)id_len, id_str);
                            return MEM_NO_IDX;
                        }
                        const char *vname = P->lex.src + P->lex.cur.off;
                        uint16_t    vlen  = P->lex.cur.len;
                        int vslot = parser_add_local(P, vname, vlen, mem->fast_real_type);
                        if (mem->has_error) return MEM_NO_IDX;
                        lex_next(&P->lex);
                        arg = mem_alloc_ast(mem, AST_SLOT_LOAD, (uint8_t)vslot, 0, 0, 0);
                    } else {
                        arg = parser_parse_expr(P, 0);
                    }
                    uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, arg, MEM_NO_IDX);
                    if (head == MEM_NO_IDX) head = link;
                    else mem_get_ast_mut(mem, tail)->right = link;
                    tail = link;
                    argc++;
                    if (P->lex.cur.kind == TOK_COMMA) {
                        lex_next(&P->lex);
                    } else {
                        break;
                    }
                }
            }
            P->in_ternary_then = saved_ternary;
            if (P->lex.cur.kind != TOK_RPAREN) {
                mem_err(mem, "Expected ')' after arguments to '%.*s'", (int)id_len, id_str);
            } else {
                lex_next(&P->lex);
            }

            if (argc == 0) {
                int lid = find_label_id(P, id_str, id_len);
                if (lid >= 0) {
                    return mem_alloc_ast(mem, AST_RUN_TAG, (uint8_t)lid, 0, MEM_NO_IDX, MEM_NO_IDX);
                }
            }

            uint16_t st_tid = sym_find_type(tab, mem, id_str, id_len);
            if (st_tid != MEM_TYPE_VOID && st_tid < tab->type_count &&
                !tab->types[st_tid].is_consuming && tab->types[st_tid].parser_intr == INTR_NONE) {
                return mem_alloc_ast(mem, AST_CONSTRUCT, argc, st_tid, head, MEM_NO_IDX);
            }

            if (sym_id < 0) {
                mem_err(mem, "Undefined function or struct '%.*s'", (int)id_len, id_str);
                return MEM_NO_IDX;
            }
            return mem_alloc_ast(mem, AST_CALL, argc, (uint16_t)sym_id, head, MEM_NO_IDX);
        }

        /* 6g. Named Global Constant (`π`, `e`, `g0`, `c0`, etc.) */
        if (sym_id >= 0) {
            for (uint8_t i = 0; i < tab->const_count; ++i) {
                if (tab->consts[i].sym_id == (uint16_t)sym_id &&
                    sym_mod_enabled(tab, tab->consts[i].mod_id)) {
                    return mem_alloc_ast(mem, AST_CONST_SYM, 0, (uint16_t)sym_id, MEM_NO_IDX, MEM_NO_IDX);
                }
            }
        }

        mem_err(mem, "Undefined symbol or constant '%.*s'", (int)id_len, id_str);
        return MEM_NO_IDX;
    }

    mem_err(mem, "Unexpected token in expression");
    return MEM_NO_IDX;
}


static uint16_t parser_parse_expr_inner(Parser *P, int min_prec);

uint16_t parser_parse_expr(Parser *P, int min_prec) {
    if (P->expr_depth >= PARSER_MAX_EXPR_DEPTH) {
        mem_err(P->lex.mem, "Expression nesting depth limit exceeded (max %d)", PARSER_MAX_EXPR_DEPTH);
        return MEM_NO_IDX;
    }
    P->expr_depth++;
    uint16_t res = parser_parse_expr_inner(P, min_prec);
    P->expr_depth--;
    return res;
}

static uint16_t parser_parse_expr_inner(Parser *P, int min_prec) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    uint16_t left = parse_nud(P);
    if (mem->has_error || left == MEM_NO_IDX) return MEM_NO_IDX;

    while (!mem->has_error) {
        if (P->lex.cur.kind == TOK_LBRACK && 95 >= min_prec) {
            left = parse_index_step(P, left);
            P->last_was_block = false;
            continue;
        }
        if (P->lex.cur.kind == TOK_OP) {
            Token t = P->lex.cur;

            int post_prec = sym_get_active_prec(tab, t.sym_id, FIX_POSTFIX);
            int in_l_prec = sym_get_active_prec(tab, t.sym_id, FIX_INFIX_LEFT);
            int in_r_prec = sym_get_active_prec(tab, t.sym_id, FIX_INFIX_RIGHT);

            if (post_prec >= min_prec) {
                bool can_be_infix = (in_l_prec >= 0 || in_r_prec >= 0);
                bool use_postfix  = !can_be_infix;
                if (can_be_infix && post_prec >= in_l_prec && post_prec >= in_r_prec) {
                    use_postfix = true;
                }
                if (use_postfix) {
                    lex_next(&P->lex);
                    P->last_was_block = false;
                    left = mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_POSTFIX, t.sym_id, left, MEM_NO_IDX);
                    continue;
                }
            }

            if (in_l_prec >= min_prec || in_r_prec >= min_prec) {
                OpFixity fix = (in_r_prec > in_l_prec) ? FIX_INFIX_RIGHT : FIX_INFIX_LEFT;
                int prec     = (fix == FIX_INFIX_RIGHT) ? in_r_prec : in_l_prec;
                int next_min = (fix == FIX_INFIX_RIGHT) ? prec : (prec + 1);
                uint8_t mtag = sym_active_meta_tag(tab, t.sym_id, fix);

                if (mtag == INTR_COLON) {
                    break;
                }

                if (mtag == INTR_TERNARY) {
                    lex_next(&P->lex);
                    P->in_ternary_then++;
                    uint16_t then_ast = parser_parse_expr(P, prec);
                    P->in_ternary_then--;
                    if (mem->has_error || then_ast == MEM_NO_IDX) return MEM_NO_IDX;

                    if (cur_op_meta(P) != INTR_COLON) {
                        mem_err(mem, "Expected ':' in ternary '? :' expression");
                        return MEM_NO_IDX;
                    }
                    lex_next(&P->lex);
                    uint16_t else_ast = parser_parse_expr(P, prec);
                    if (mem->has_error || else_ast == MEM_NO_IDX) return MEM_NO_IDX;

                    uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, then_ast, else_ast);
                    left = mem_alloc_ast(mem, AST_TERNARY, 0, 0, left, link);
                    P->last_was_block = false;
                    continue;
                }

                lex_next(&P->lex);
                if (mtag == INTR_FIELD) {
                    left = parse_field_step(P, left);
                    P->last_was_block = false;
                    continue;
                }
                if (mtag == INTR_SEQ &&
                    (P->lex.cur.kind == TOK_RPAREN || P->lex.cur.kind == TOK_RBRACE ||
                     P->lex.cur.kind == TOK_RBRACK || P->lex.cur.kind == TOK_EOF)) {
                    break;
                }

                uint16_t right = parser_parse_expr(P, next_min);
                if (mtag == INTR_STORE) {
                    left = build_store_or_compound(P, t.sym_id, left, right);
                    continue;
                }
                if (hardcode_is_rel_comparison_op(mem, tab, t.sym_id)) {
                    left = build_distributed_comp(P, fix, t.sym_id, left, right);
                    continue;
                }
                left = mem_alloc_ast(mem, AST_BINARY, (uint8_t)fix, t.sym_id, left, right);
                continue;
            }
        }

        /* Implicit multiplication: `2x`, `2π`, `3(x + 1)`, `(x + 1)(x - 1)`, `2 sin(x)`, `2 √16` */
        if (!P->last_was_block && 60 >= min_prec && hardcode_can_start_implicit_mul_rhs(P)) {
            int mul_sym = sym_find(tab, mem, "*", 1);
            if (mul_sym >= 0 && sym_get_active_prec(tab, (uint16_t)mul_sym, FIX_INFIX_LEFT) >= 0) {
                uint16_t right = parser_parse_expr(P, 65);
                if (mem->has_error || right == MEM_NO_IDX) return MEM_NO_IDX;
                left = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                                     (uint16_t)mul_sym, left, right);
                P->last_was_block = false;
                continue;
            }
        }

        if (P->last_was_block && min_prec <= 1 &&
            P->lex.cur.kind != TOK_EOF &&
            P->lex.cur.kind != TOK_RPAREN &&
            P->lex.cur.kind != TOK_RBRACE &&
            P->lex.cur.kind != TOK_RBRACK &&
            P->lex.cur.kind != TOK_COMMA &&
            P->lex.cur.kind != TOK_OP) {
            uint8_t kw = token_func_meta(P, P->lex.cur);
            if (kw != INTR_OR_BRANCH && kw != INTR_ELSE_BRANCH && kw != INTR_CASE_BRANCH) {
                int seq_sym = find_active_meta_sym(tab, INTR_SEQ);
                if (seq_sym >= 0) {
                    P->last_was_block = false;
                    uint16_t right = parser_parse_expr(P, 2);
                    left = mem_alloc_ast(mem, AST_BINARY, (uint8_t)FIX_INFIX_LEFT,
                                         (uint16_t)seq_sym, left, right);
                    continue;
                }
            }
        }

        break;
    }

    return left;
}
