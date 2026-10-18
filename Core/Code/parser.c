/*
 * parser.c — Standalone Pratt Parser & AST Builder Implementation
 */

#include "parser.h"
#include <string.h>

static int find_local_slot(const Parser *P, const char *name, uint16_t len) {
    for (uint8_t i = 0; i < P->local_count; ++i) {
        StrSlice s = P->locals[i].name;
        if (s.len == len && memcmp(mem_str_ptr(P->lex.mem, s), name, len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int parser_add_local(Parser *P, const char *name, uint16_t len, uint16_t type_id) {
    int existing = find_local_slot(P, name, len);
    if (existing >= 0) return existing;
    if (P->local_count >= PARSER_MAX_LOCALS) {
        mem_err(P->lex.mem, "Too many local variables (max %d)", PARSER_MAX_LOCALS);
        return 0;
    }
    int slot = P->local_count++;
    P->locals[slot].name    = mem_intern_str(P->lex.mem, name, len);
    P->locals[slot].type_id = type_id;
    return slot;
}

void parser_init(Parser *P, MemArena *mem, const SymTable *tab,
                 const char *src, uint16_t len, uint16_t start_pos,
                 bool in_ext_body, bool stop_at_newline) {
    memset(P, 0, sizeof(*P));
    lex_init(&P->lex, mem, tab, src, len, start_pos, stop_at_newline);
    P->in_ext_body = in_ext_body;
}

/* Resolve a single `.field` access step for any operator tagged with `@field` (`INTR_FIELD`) */
static uint16_t parse_field_step(Parser *P, uint16_t left) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;

    if (P->lex.cur.kind != TOK_IDENT &&
        !(P->lex.cur.kind == TOK_OP && lex_is_ident_start((unsigned char)P->lex.src[P->lex.cur.off]))) {
        mem_err(mem, "Expected struct field name after member access operator");
        return MEM_NO_IDX;
    }
    const char *fname = P->lex.src + P->lex.cur.off;
    uint16_t flen     = P->lex.cur.len;
    lex_next(&P->lex);

    const AstNode *lnode = mem_get_ast(mem, left);
    uint16_t cur_type    = MEM_TYPE_VOID;
    uint16_t base_ast    = left;
    uint16_t base_slot   = MEM_NO_IDX;
    uint16_t prev_off    = 0;

    if (lnode->kind == AST_SLOT_LOAD) {
        base_slot = lnode->sub;
        cur_type  = P->locals[base_slot].type_id;
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
    if (cur_type != MEM_TYPE_VOID && cur_type < tab->type_count &&
        !tab->types[cur_type].is_consuming && tab->types[cur_type].field_count > 0) {
        if (!sym_resolve_field(tab, mem, cur_type, fname, flen, &ftype, &foff)) {
            mem_err(mem, "Struct type has no field '%.*s'", (int)flen, fname);
            return MEM_NO_IDX;
        }
    } else {
        /* Dynamic/polymorphic base (e.g. `at(v, i).x` on a `vec` of structs): search active structs */
        bool found = false;
        for (uint16_t tid = 1; tid < tab->type_count; ++tid) {
            if (!tab->types[tid].is_consuming && tab->types[tid].field_count > 0 &&
                sym_mod_enabled(tab, tab->types[tid].mod_id)) {
                if (sym_resolve_field(tab, mem, tid, fname, flen, &ftype, &foff)) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            mem_err(mem, "Unknown struct field '%.*s'", (int)flen, fname);
            return MEM_NO_IDX;
        }
    }

    if (lnode->kind == AST_FIELD_LOAD) {
        AstNode *mut = mem_get_ast_mut(mem, left);
        mut->sub = (uint8_t)ftype;
        mut->ref = prev_off + foff;
        return left;
    }

    return mem_alloc_ast(mem, AST_FIELD_LOAD, (uint8_t)ftype, prev_off + foff, base_ast, base_slot);
}

static uint16_t parse_nud(Parser *P) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    Token t = P->lex.cur;

    /* 1. Consumed Literal Value ($real, $str) */
    if (t.kind == TOK_VAL) {
        lex_next(&P->lex);
        uint16_t tid = val_get_type(mem, t.val);
        if (tid == mem->str_type_id && tid != 0) {
            StrSlice s = val_get_str(mem, t.val);
            return mem_alloc_ast(mem, AST_LIT, (uint8_t)tid, 0, s.off, s.len);
        } else {
            uint16_t pidx = mem_alloc_pool(mem, 1);
            mem_write_pool(mem, pidx, val_get_f64(mem, t.val));
            return mem_alloc_ast(mem, AST_LIT, (uint8_t)tid, pidx, 0, 0);
        }
    }

    /* 2. Parenthesized Expression or Multi-Instruction Sequence `( stmt1; stmt2; expr )` */
    if (t.kind == TOK_LPAREN) {
        lex_next(&P->lex);
        uint16_t inner = parser_parse_expr(P, 0);
        if (P->lex.cur.kind != TOK_RPAREN) {
            mem_err(mem, "Expected closing delimiter ')'");
        } else {
            lex_next(&P->lex);
        }
        return inner;
    }

    /* 3. Private C Intrinsic `@add(a, b)` OR Host Extern Callback `@fx_rate(from, to)` */
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

        /* 3a. Built-in C Intrinsic (`@add`, `@sin`, `@vpush`, etc.) */
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

        /* 3b. Host Extern Callback (`@fx_rate(from, to)`, etc.): 0..4 args via AST_ARG_LINK */
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

    /* 4. Prefix Unary Operator (`-x`, `√x`, `++x`, `+++x`) */
    if (t.kind == TOK_OP) {
        int prec = sym_get_active_prec(tab, t.sym_id, FIX_PREFIX);
        bool is_word_local = lex_is_ident_start((unsigned char)P->lex.src[t.off]) &&
                             (find_local_slot(P, P->lex.src + t.off, t.len) >= 0 || prec < 0);
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
        /* Word operator without prefix fixity (e.g. postfix 'i') in a NUD position
           acts as an identifier (local variable, parameter, constant, or call). */
        t.kind = TOK_IDENT;
    }

    /* 5. Identifier */
    if (t.kind == TOK_IDENT) {
        const char *id_str = P->lex.src + t.off;
        uint16_t id_len    = t.len;
        lex_next(&P->lex);

        /* 5a. Local variable or parameter slot (auto-allocated if followed by an `@store` op) */
        int slot = find_local_slot(P, id_str, id_len);
        if (slot < 0 && P->lex.cur.kind == TOK_OP &&
            sym_active_meta_tag(tab, P->lex.cur.sym_id, FIX_INFIX_RIGHT) == INTR_STORE) {
            slot = parser_add_local(P, id_str, id_len, mem->fast_real_type);
        }
        if (slot >= 0) {
            return mem_alloc_ast(mem, AST_SLOT_LOAD, (uint8_t)slot, 0, MEM_NO_IDX, MEM_NO_IDX);
        }

        /* 5b. Call syntax `name(arg1, arg2, ...)` -> Struct Constructor OR Function Call */
        if (P->lex.cur.kind == TOK_LPAREN) {
            lex_next(&P->lex);
            uint16_t head = MEM_NO_IDX;
            uint16_t tail = MEM_NO_IDX;
            uint8_t  argc = 0;

            if (P->lex.cur.kind != TOK_RPAREN) {
                while (1) {
                    uint16_t arg = parser_parse_expr(P, 0);
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
                mem_err(mem, "Expected ')' after arguments to '%.*s'", (int)id_len, id_str);
            } else {
                lex_next(&P->lex);
            }

            uint16_t st_tid = sym_find_type(tab, mem, id_str, id_len);
            if (st_tid != MEM_TYPE_VOID && st_tid < tab->type_count &&
                !tab->types[st_tid].is_consuming && tab->types[st_tid].parser_intr == INTR_NONE) {
                return mem_alloc_ast(mem, AST_CONSTRUCT, argc, st_tid, head, MEM_NO_IDX);
            }

            int sym_id = sym_find(tab, mem, id_str, id_len);
            if (sym_id < 0) {
                mem_err(mem, "Undefined function or struct '%.*s'", (int)id_len, id_str);
                return MEM_NO_IDX;
            }
            return mem_alloc_ast(mem, AST_CALL, argc, (uint16_t)sym_id, head, MEM_NO_IDX);
        }

        /* 5c. Named Global Constant (`π`, `e`, `g0`) */
        int sym_id = sym_find(tab, mem, id_str, id_len);
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

uint16_t parser_parse_expr(Parser *P, int min_prec) {
    MemArena       *mem = P->lex.mem;
    const SymTable *tab = P->lex.tab;
    uint16_t left = parse_nud(P);
    if (mem->has_error || left == MEM_NO_IDX) return MEM_NO_IDX;

    while (!mem->has_error && P->lex.cur.kind == TOK_OP) {
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
                left = mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_POSTFIX, t.sym_id, left, MEM_NO_IDX);
                continue;
            }
        }

        if (in_l_prec >= min_prec || in_r_prec >= min_prec) {
            OpFixity fix = (in_r_prec > in_l_prec) ? FIX_INFIX_RIGHT : FIX_INFIX_LEFT;
            int prec     = (fix == FIX_INFIX_RIGHT) ? in_r_prec : in_l_prec;
            int next_min = (fix == FIX_INFIX_RIGHT) ? prec : (prec + 1);
            uint8_t mtag = sym_active_meta_tag(tab, t.sym_id, fix);

            lex_next(&P->lex);
            if (mtag == INTR_FIELD) {
                left = parse_field_step(P, left);
                continue;
            }
            if (mtag == INTR_SEQ &&
                (P->lex.cur.kind == TOK_RPAREN || P->lex.cur.kind == TOK_EOF)) {
                break;
            }
            uint16_t right = parser_parse_expr(P, next_min);
            if (mtag == INTR_STORE && left != MEM_NO_IDX && right != MEM_NO_IDX) {
                const AstNode *ln = mem_get_ast(mem, left);
                const AstNode *rn = mem_get_ast(mem, right);
                if (ln->kind == AST_SLOT_LOAD && rn->kind == AST_CONSTRUCT) {
                    P->locals[ln->sub].type_id = rn->ref;
                }
            }
            left = mem_alloc_ast(mem, AST_BINARY, (uint8_t)fix, t.sym_id, left, right);
            continue;
        }

        break;
    }

    return left;
}