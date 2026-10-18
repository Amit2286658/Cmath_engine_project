/*
 * ext_loader.c — In-Memory `.math` Extension Compiler & `.mathc` Binary Serializer
 *
 * Pure ISO C99 in-memory implementation: zero filesystem, directory, or path
 * dependencies (`<dirent.h>`, `<sys/stat.h>`, and `FILE*` I/O removed).
 */

#include "ext_loader.h"
#include "parser.h"
#include "evaluator.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    MemArena   *mem;
    SymTable   *tab;
    const char *src;
    uint16_t    len;
    uint16_t    pos;
    uint8_t     cur_mod;
} ExtLoader;

static void ext_skip_ws(ExtLoader *L) {
    while (L->pos < L->len) {
        char c = L->src[L->pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') L->pos++;
        else if (c == '#') {
            while (L->pos < L->len && L->src[L->pos] != '\n') L->pos++;
        } else break;
    }
}

static bool ext_match_char(ExtLoader *L, char expected) {
    ext_skip_ws(L);
    if (L->pos < L->len && L->src[L->pos] == expected) {
        L->pos++;
        return true;
    }
    return false;
}

static StrSlice ext_read_ident(ExtLoader *L) {
    ext_skip_ws(L);
    uint16_t start = L->pos;
    while (L->pos < L->len && lex_is_ident_cont((unsigned char)L->src[L->pos])) {
        L->pos++;
    }
    return mem_intern_str(L->mem, L->src + start, L->pos - start);
}

static StrSlice ext_read_quoted(ExtLoader *L) {
    ext_skip_ws(L);
    if (L->pos >= L->len || L->src[L->pos] != '"') {
        mem_err(L->mem, "Expected quoted string symbol in declaration");
        StrSlice empty = { 0, 0 };
        return empty;
    }
    L->pos++;
    char buf[64];
    uint16_t blen = 0;
    while (L->pos < L->len && L->src[L->pos] != '"') {
        if (L->src[L->pos] == '\\' && L->pos + 1 < L->len) {
            L->pos++;
        }
        if (blen < sizeof(buf) - 1) buf[blen++] = L->src[L->pos];
        L->pos++;
    }
    if (L->pos < L->len && L->src[L->pos] == '"') L->pos++;
    return mem_intern_str(L->mem, buf, blen);
}

static void ext_parse_type_decl(ExtLoader *L) {
    MemArena *mem = L->mem;
    SymTable *tab = L->tab;
    L->pos++; /* skip '$' */
    StrSlice tname = ext_read_ident(L);
    if (tab->type_count >= SYM_MAX_TYPES) {
        mem_err(mem, "Type table overflow");
        return;
    }

    uint16_t tid = tab->type_count++;
    TypeDef *td  = &tab->types[tid];
    memset(td, 0, sizeof(*td));
    td->name   = tname;
    td->mod_id = L->cur_mod;

    ext_skip_ws(L);
    if (ext_match_char(L, '~')) {
        td->is_consuming = 1;
        td->flat_size    = 1;
        td->start_idx    = tab->pat_elem_count;
        uint8_t elem_cnt = 0;

        while (L->pos < L->len && !mem->has_error) {
            ext_skip_ws(L);
            if (L->pos + 1 < L->len && L->src[L->pos] == '=' && L->src[L->pos + 1] == '>') {
                L->pos += 2;
                break;
            }
            if (tab->pat_elem_count >= SYM_MAX_PAT_ELEMS) {
                mem_err(mem, "Pattern element pool overflow");
                return;
            }
            PatElem *pe = &tab->pat_elems[tab->pat_elem_count++];
            memset(pe, 0, sizeof(*pe));
            elem_cnt++;

            char c = L->src[L->pos];
            if (c == '\\' && L->pos + 1 < L->len) {
                char esc = L->src[L->pos + 1];
                L->pos += 2;
                pe->kind = (esc == 'd') ? PAT_DIGIT : PAT_ALPHA;
            } else if (c == '"') {
                pe->kind  = PAT_LIT;
                pe->chars = ext_read_quoted(L);
            } else if (c == '[' && L->pos + 1 < L->len && L->src[L->pos + 1] == '^') {
                L->pos += 2;
                char excl[32];
                uint16_t elen = 0;
                while (L->pos < L->len && L->src[L->pos] != ']') {
                    if (L->src[L->pos] == '\\' && L->pos + 1 < L->len) L->pos++;
                    if (elen < sizeof(excl) - 1) excl[elen++] = L->src[L->pos];
                    L->pos++;
                }
                if (L->pos < L->len && L->src[L->pos] == ']') L->pos++;
                pe->kind  = PAT_NOT_CHARS;
                pe->chars = mem_intern_str(mem, excl, elen);
            } else if (c == '(') {
                L->pos++;
                pe->kind = PAT_GROUP_START;
            } else if (c == ')') {
                L->pos++;
                pe->kind = PAT_GROUP_END;
            } else {
                mem_err(mem, "Invalid pattern syntax near '%c'", c);
                return;
            }

            if (pe->kind != PAT_GROUP_START && L->pos < L->len) {
                char q = L->src[L->pos];
                if (q == '+') { pe->quant = QUANT_PLUS; L->pos++; }
                else if (q == '*') { pe->quant = QUANT_STAR; L->pos++; }
                else if (q == '?') { pe->quant = QUANT_OPT;  L->pos++; }
            }
        }
        td->field_count = elem_cnt;

        if (!ext_match_char(L, '@')) {
            mem_err(mem, "Expected '@parser_intrinsic' after '=>' in consuming type");
            return;
        }
        StrSlice iname = ext_read_ident(L);
        td->parser_intr = (uint8_t)sym_lookup_intrinsic(mem_str_ptr(mem, iname), iname.len);
        if (td->parser_intr == INTR_PARSE_F64 && mem->fast_real_type == 0) {
            mem->fast_real_type = (uint8_t)tid;
        } else if (td->parser_intr == INTR_PARSE_STR_SLICE && mem->str_type_id == 0) {
            mem->str_type_id = (uint8_t)tid;
        } else if (td->parser_intr == INTR_VEC_TYPE) {
            td->is_consuming = 0;
            td->flat_size    = MEM_VEC_HDR_SLOTS;
            if (mem->vec_type_id == 0) {
                mem->vec_type_id = (uint8_t)tid;
            }
        }
    } else if (ext_match_char(L, '(')) {
        td->is_consuming = 0;
        td->start_idx    = tab->field_count;
        uint16_t flat_off = 0;
        uint8_t  fcount   = 0;

        while (L->pos < L->len && !mem->has_error) {
            if (ext_match_char(L, ')')) break;
            StrSlice fname = ext_read_ident(L);
            if (!ext_match_char(L, ':')) {
                mem_err(mem, "Expected ':' in struct field declaration");
                return;
            }
            StrSlice ftype_s = ext_read_ident(L);
            uint16_t ftid = sym_find_type(tab, mem, mem_str_ptr(mem, ftype_s), ftype_s.len);
            if (ftid == MEM_TYPE_VOID) {
                mem_err(mem, "Unknown field type '%.*s'", (int)ftype_s.len, mem_str_ptr(mem, ftype_s));
                return;
            }
            if (tab->field_count >= SYM_MAX_FIELDS) {
                mem_err(mem, "Struct field table overflow");
                return;
            }
            FieldDef *fd = &tab->fields[tab->field_count++];
            fd->name        = fname;
            fd->type_id     = ftid;
            fd->flat_offset = flat_off;

            flat_off += tab->types[ftid].flat_size;
            fcount++;
            ext_match_char(L, ',');
        }
        td->field_count = fcount;
        td->flat_size   = flat_off;
    } else {
        mem_err(mem, "Expected '~' (consuming/container) or '(' (struct) in type declaration");
    }
}

/*
 * Resolve a single `type_or_unit` identifier (and optional `, unit` identifier)
 * in parameter or return position:
 *   - `real`            -> type = `real`,        unit = `SYM_UNIT_OMITTED`
 *   - `real, distance`  -> type = `real`,        unit = `distance`
 *   - `real, T`         -> type = `real`,        unit = `SYM_UNIT_GENERIC`
 *   - `distance`        -> type = `SYM_TYPE_ANY`,unit = `distance`
 *   - `T`               -> type = `SYM_TYPE_ANY`,unit = `SYM_UNIT_OMITTED`
 *   - `T, T`            -> type = `SYM_TYPE_ANY`,unit = `SYM_UNIT_GENERIC`
 */
static bool ext_resolve_type_and_unit(ExtLoader *L, StrSlice first_id,
                                      bool has_second, StrSlice second_id,
                                      uint16_t *out_type, uint8_t *out_unit) {
    MemArena *mem = L->mem;
    SymTable *tab = L->tab;
    const char *s1 = mem_str_ptr(mem, first_id);

    if (has_second) {
        uint16_t tid = sym_find_type(tab, mem, s1, first_id.len);
        if (tid == MEM_TYPE_VOID) {
            mem_err(mem, "Unknown entity type '%.*s'", (int)first_id.len, s1);
            return false;
        }
        const char *s2 = mem_str_ptr(mem, second_id);
        uint8_t uid = sym_find_or_add_unit(tab, mem, s2, second_id.len);
        *out_type = tid;
        *out_unit = uid;
        return !mem->has_error;
    }

    uint16_t tid = sym_find_type(tab, mem, s1, first_id.len);
    if (tid != MEM_TYPE_VOID) {
        *out_type = tid;
        *out_unit = SYM_UNIT_OMITTED;
        return true;
    }

    /* Single identifier is not a known entity type -> treat as concrete unit with generic type T */
    uint8_t uid = sym_find_or_add_unit(tab, mem, s1, first_id.len);
    *out_type = SYM_TYPE_ANY;
    *out_unit = uid;
    return !mem->has_error;
}

static bool ext_parse_one_param(ExtLoader *L, StrSlice *out_name,
                                uint16_t *out_type, uint8_t *out_unit) {
    if (!ext_match_char(L, '(')) return false;
    *out_name = ext_read_ident(L);
    if (!ext_match_char(L, ':')) {
        mem_err(L->mem, "Expected ':' in parameter declaration");
        return false;
    }
    StrSlice tname = ext_read_ident(L);
    bool has_unit  = false;
    StrSlice uname = { 0, 0 };
    if (ext_match_char(L, ',')) {
        has_unit = true;
        uname    = ext_read_ident(L);
    }
    if (!ext_resolve_type_and_unit(L, tname, has_unit, uname, out_type, out_unit)) {
        return false;
    }
    if (!ext_match_char(L, ')')) {
        mem_err(L->mem, "Expected ')' after parameter declaration");
        return false;
    }
    return true;
}

/*
 * Check if the next identifier in a function parameter list `(a:real, distance, b:real)`
 * is a unit annotation (`distance`, not followed by `:`) rather than a new parameter (`b:`).
 */
static bool ext_peek_ident_is_unit_annot(ExtLoader *L) {
    ext_skip_ws(L);
    uint16_t p = L->pos;
    if (p >= L->len || !lex_is_ident_start((unsigned char)L->src[p])) return false;
    while (p < L->len && lex_is_ident_cont((unsigned char)L->src[p])) p++;
    while (p < L->len && (L->src[p] == ' ' || L->src[p] == '\t' || L->src[p] == '\r' || L->src[p] == '\n')) p++;
    return (p >= L->len || L->src[p] != ':');
}

static bool ext_parse_return_spec(ExtLoader *L, uint16_t *out_rtype, uint8_t *out_runit) {
    ext_skip_ws(L);
    if (L->pos + 1 < L->len && L->src[L->pos] == '-' && L->src[L->pos + 1] == '>') {
        L->pos += 2;
    }
    StrSlice rtype_s = ext_read_ident(L);
    bool has_unit    = false;
    StrSlice runit_s = { 0, 0 };
    if (ext_match_char(L, ',')) {
        has_unit = true;
        runit_s  = ext_read_ident(L);
    }
    return ext_resolve_type_and_unit(L, rtype_s, has_unit, runit_s, out_rtype, out_runit);
}

static void ext_compile_body(ExtLoader *L, OpOverload *ov, StrSlice *pnames) {
    MemArena *mem = L->mem;
    SymTable *tab = L->tab;
    ext_skip_ws(L);

    Parser P;
    parser_init(&P, mem, tab, L->src, L->len, L->pos, true, true);
    P.param_count = ov->param_count;

    for (uint8_t i = 0; i < ov->param_count; ++i) {
        parser_add_local(&P, mem_str_ptr(mem, pnames[i]), pnames[i].len, ov->param_types[i]);
    }

    lex_next(&P.lex);
    uint16_t body_root = parser_parse_expr(&P, 0);
    (void)parser_check_labels(&P);
    ov->body_ast    = body_root;
    ov->local_count = P.local_count;
    ov->meta_tag    = 0;

    L->pos = (P.lex.cur.kind == TOK_EOF) ? P.lex.pos : P.lex.cur.off;

    if (body_root != MEM_NO_IDX) {
        const AstNode *root = mem_get_ast(mem, body_root);
        if (root->kind == AST_INTRINSIC &&
            root->sub >= INTR_SEQ && root->sub <= INTR_FIELD) {
            ov->meta_tag = root->sub;
            return;
        }
        if (ov->param_count == 2 && root->kind == AST_BINARY &&
            root->left != MEM_NO_IDX && root->right != MEM_NO_IDX &&
            mem_get_ast(mem, root->left)->kind == AST_SLOT_LOAD && mem_get_ast(mem, root->left)->sub == 0 &&
            mem_get_ast(mem, root->right)->kind == AST_SLOT_LOAD && mem_get_ast(mem, root->right)->sub == 1) {
            uint16_t target_sym = root->ref;
            for (uint16_t i = 0; i < tab->overload_count; ++i) {
                const OpOverload *target = &tab->overloads[i];
                if (target != ov && target->sym_id == target_sym &&
                    target->param_count == 2 &&
                    target->param_types[0] == ov->param_types[0] &&
                    target->param_types[1] == ov->param_types[1] &&
                    target->param_units[0] == ov->param_units[0] &&
                    target->param_units[1] == ov->param_units[1] &&
                    target->ret_unit       == ov->ret_unit) {
                    ov->body_ast    = target->body_ast;
                    ov->local_count = target->local_count;
                    ov->meta_tag    = 1;
                    break;
                }
            }
        }
    }
}

bool ext_load_str(MemArena *mem, SymTable *tab, const char *ext_src) {
    mem_clear_err(mem);

    ExtLoader L;
    memset(&L, 0, sizeof(L));
    L.mem = mem;
    L.tab = tab;
    L.src = ext_src;
    L.len = (uint16_t)strlen(ext_src);
    L.pos = 0;

    if (tab->mod_count == 0) {
        StrSlice def_mod = mem_intern_str(mem, "default", 7);
        L.cur_mod = sym_find_or_add_mod(tab, mem, def_mod);
    } else {
        L.cur_mod = tab->mod_count - 1;
    }

    while (L.pos < L.len && !mem->has_error) {
        ext_skip_ws(&L);
        if (L.pos >= L.len) break;

        char c = L.src[L.pos];

        if (c == '[') {
            L.pos++;
            StrSlice mname = ext_read_ident(&L);
            ext_match_char(&L, ']');
            L.cur_mod = sym_find_or_add_mod(tab, mem, mname);
            continue;
        }

        if (c == '<' && L.pos + 1 < L.len && L.src[L.pos + 1] == '[') {
            L.pos += 2;
            StrSlice dep = ext_read_ident(&L);
            ext_match_char(&L, ']');
            uint8_t dep_id = sym_find_or_add_mod(tab, mem, dep);
            tab->active_modules |= (1ULL << dep_id);
            continue;
        }

        if (c == '$') {
            ext_parse_type_decl(&L);
            continue;
        }

        if (c == '"') {
            StrSlice sym    = ext_read_quoted(&L);
            uint16_t sym_id = sym_intern(tab, mem, sym);
            ext_skip_ws(&L);

            /* Syntax delimiter binding (`"(" => @lparen`, `"{" => @lbrace`, `"," => @comma`) */
            if (L.pos + 1 < L.len && L.src[L.pos] == '=' && L.src[L.pos + 1] == '>') {
                L.pos += 2;
                if (!ext_match_char(&L, '@')) {
                    mem_err(mem, "Expected '@syntax_intrinsic' after '=>'");
                    break;
                }
                StrSlice iname = ext_read_ident(&L);
                IntrinsicId intr = sym_lookup_intrinsic(mem_str_ptr(mem, iname), iname.len);
                if (intr != INTR_LPAREN && intr != INTR_RPAREN &&
                    intr != INTR_LBRACE && intr != INTR_RBRACE &&
                    intr != INTR_COMMA) {
                    mem_err(mem, "Unknown syntax delimiter intrinsic '@%.*s'",
                            (int)iname.len, mem_str_ptr(mem, iname));
                    break;
                }
                if (tab->overload_count >= SYM_MAX_OVERLOADS) {
                    mem_err(mem, "Overload table overflow");
                    break;
                }
                OpOverload *ov = &tab->overloads[tab->overload_count++];
                memset(ov, 0, sizeof(*ov));
                ov->sym_id   = sym_id;
                ov->mod_id   = L.cur_mod;
                ov->fixity   = FIX_SYNTAX;
                ov->meta_tag = (uint8_t)intr;
                continue;
            }

            if (ext_match_char(&L, ':')) {
                uint16_t rtid = 0;
                uint8_t  ruid = SYM_UNIT_OMITTED;
                if (!ext_parse_return_spec(&L, &rtid, &ruid)) break;
                if (!ext_match_char(&L, '=')) {
                    mem_err(mem, "Expected '=' in constant definition");
                    break;
                }
                ext_skip_ws(&L);
                Parser P;
                parser_init(&P, mem, tab, L.src, L.len, L.pos, false, true);
                lex_next(&P.lex);
                uint16_t c_ast = parser_parse_expr(&P, 0);
                L.pos = (P.lex.cur.kind == TOK_EOF) ? P.lex.pos : P.lex.cur.off;

                uint16_t c_pool_idx = 0;
                uint8_t  final_unit = (ruid != SYM_UNIT_OMITTED && ruid != SYM_UNIT_GENERIC && ruid != SYM_UNIT_LOCK_NONE)
                                      ? ruid : MEM_UNIT_NONE;
                Value cval = val_void();
                if (c_ast != MEM_NO_IDX) {
                    if (mem_get_ast(mem, c_ast)->kind == AST_LIT) {
                        c_pool_idx = mem_get_ast(mem, c_ast)->ref;
                        cval = val_make_f64_unit(mem, rtid, final_unit, mem_read_pool(mem, c_pool_idx));
                    } else {
                        Value ev = eval_ast(mem, tab, c_ast, 0, (uint8_t)rtid);
                        double d = val_get_f64(mem, ev);
                        if (final_unit == MEM_UNIT_NONE && ruid == SYM_UNIT_OMITTED) {
                            final_unit = val_get_unit(mem, ev);
                        }
                        c_pool_idx = mem_alloc_pool(mem, 1);
                        mem_write_pool(mem, c_pool_idx, d);
                        cval = val_make_f64_unit(mem, rtid, final_unit, d);
                    }
                }
                if (tab->const_count < SYM_MAX_CONSTS) {
                    uint8_t cid = tab->const_count++;
                    tab->consts[cid].sym_id   = sym_id;
                    tab->consts[cid].mod_id   = L.cur_mod;
                    tab->consts[cid].type_id  = (uint8_t)rtid;
                    tab->consts[cid].unit_id  = final_unit;
                    tab->consts[cid].val_idx  = cid;
                    tab->consts[cid].pool_idx = c_pool_idx;
                    tab->const_vals[cid]      = cval;
                }
                continue;
            }

            if (!ext_match_char(&L, '(')) {
                mem_err(mem, "Expected '(', ':', or '=>' after symbol in declaration");
                break;
            }
            StrSlice pnames[8];
            uint16_t ptypes[8];
            uint8_t  punits[8];
            uint8_t  pcount = 0;

            while (L.pos < L.len && !mem->has_error) {
                if (ext_match_char(&L, ')')) break;
                if (pcount >= 8) { mem_err(mem, "Max 8 parameters supported"); break; }
                pnames[pcount] = ext_read_ident(&L);
                ext_match_char(&L, ':');
                StrSlice tn = ext_read_ident(&L);
                bool has_u  = false;
                StrSlice un = { 0, 0 };
                if (ext_match_char(&L, ',')) {
                    if (ext_peek_ident_is_unit_annot(&L)) {
                        has_u = true;
                        un    = ext_read_ident(&L);
                        ext_match_char(&L, ',');
                    }
                }
                if (!ext_resolve_type_and_unit(&L, tn, has_u, un, &ptypes[pcount], &punits[pcount])) {
                    break;
                }
                pcount++;
            }
            if (mem->has_error) break;

            ext_skip_ws(&L);
            uint8_t fixity = FIX_FUNC;
            uint8_t prec   = 0;

            if (ext_match_char(&L, ':')) {
                fixity = FIX_PREFIX;
                char *endp = NULL;
                prec = (uint8_t)strtol(L.src + L.pos, &endp, 10);
                L.pos = (uint16_t)(endp - L.src);
            }

            uint16_t rtid = 0;
            uint8_t  ruid = SYM_UNIT_OMITTED;
            if (!ext_parse_return_spec(&L, &rtid, &ruid)) break;
            ext_match_char(&L, '=');

            if (tab->overload_count >= SYM_MAX_OVERLOADS) {
                mem_err(mem, "Overload table overflow");
                break;
            }
            OpOverload *ov = &tab->overloads[tab->overload_count++];
            memset(ov, 0, sizeof(*ov));
            ov->sym_id      = sym_id;
            ov->mod_id      = L.cur_mod;
            ov->fixity      = fixity;
            ov->prec        = prec;
            ov->param_count = pcount;
            ov->ret_type    = (uint8_t)rtid;
            ov->ret_unit    = ruid;
            for (uint8_t i = 0; i < pcount; ++i) {
                ov->param_types[i] = (uint8_t)ptypes[i];
                ov->param_units[i] = punits[i];
            }

            ext_compile_body(&L, ov, pnames);
            continue;
        }

        if (c == '(') {
            StrSlice pnames[8];
            uint16_t ptypes[8];
            uint8_t  punits[8];
            if (!ext_parse_one_param(&L, &pnames[0], &ptypes[0], &punits[0])) break;

            StrSlice sym    = ext_read_quoted(&L);
            uint16_t sym_id = sym_intern(tab, mem, sym);
            ext_skip_ws(&L);

            uint8_t pcount = 1;
            uint8_t fixity = FIX_POSTFIX;
            uint8_t prec   = 0;

            if (L.pos < L.len && L.src[L.pos] == '(') {
                if (!ext_parse_one_param(&L, &pnames[1], &ptypes[1], &punits[1])) break;
                pcount = 2;
            }

            if (!ext_match_char(&L, ':')) {
                mem_err(mem, "Expected ':prec', ':<prec', or ':>prec' in operator declaration");
                break;
            }
            if (pcount == 2) {
                if (ext_match_char(&L, '<')) fixity = FIX_INFIX_LEFT;
                else if (ext_match_char(&L, '>')) fixity = FIX_INFIX_RIGHT;
                else fixity = FIX_INFIX_LEFT;
            } else {
                fixity = FIX_POSTFIX;
            }

            char *endp = NULL;
            prec = (uint8_t)strtol(L.src + L.pos, &endp, 10);
            L.pos = (uint16_t)(endp - L.src);

            uint16_t rtid = 0;
            uint8_t  ruid = SYM_UNIT_OMITTED;
            if (!ext_parse_return_spec(&L, &rtid, &ruid)) break;
            ext_match_char(&L, '=');

            if (tab->overload_count >= SYM_MAX_OVERLOADS) {
                mem_err(mem, "Overload table overflow");
                break;
            }
            OpOverload *ov = &tab->overloads[tab->overload_count++];
            memset(ov, 0, sizeof(*ov));
            ov->sym_id         = sym_id;
            ov->mod_id         = L.cur_mod;
            ov->fixity         = fixity;
            ov->prec           = prec;
            ov->param_count    = pcount;
            ov->ret_type       = (uint8_t)rtid;
            ov->ret_unit       = ruid;
            ov->param_types[0] = (uint8_t)ptypes[0];
            ov->param_units[0] = punits[0];
            if (pcount == 2) {
                ov->param_types[1] = (uint8_t)ptypes[1];
                ov->param_units[1] = punits[1];
            }

            ext_compile_body(&L, ov, pnames);
            continue;
        }

        mem_err(mem, "Unrecognized declaration in extension near '%.*s'",
                (int)(L.len - L.pos < 16 ? L.len - L.pos : 16), L.src + L.pos);
        break;
    }

    mem_snapshot_perm(mem);
    return !mem->has_error;
}

/* ============================================================================
 * IN-MEMORY `.mathc` BINARY SERIALIZATION & O(1) LOADER IMPLEMENTATION
 * ============================================================================ */

static uint32_t calc_mathc_payload_bytes(const MathcHeader *h) {
    return (uint32_t)h->perm_src_len
         + (uint32_t)h->perm_ast_count  * (uint32_t)sizeof(AstNode)
         + (uint32_t)h->perm_pool_top   * (uint32_t)sizeof(double)
         + (uint32_t)h->mod_count       * (uint32_t)sizeof(StrSlice)
         + (uint32_t)h->type_count      * (uint32_t)sizeof(TypeDef)
         + (uint32_t)h->unit_count      * (uint32_t)sizeof(StrSlice)
         + (uint32_t)h->pat_elem_count  * (uint32_t)sizeof(PatElem)
         + (uint32_t)h->field_count     * (uint32_t)sizeof(FieldDef)
         + (uint32_t)h->sym_count       * (uint32_t)sizeof(StrSlice)
         + (uint32_t)h->overload_count  * (uint32_t)sizeof(OpOverload)
         + (uint32_t)h->const_count     * (uint32_t)sizeof(ConstDef);
}

static void fill_mathc_header(const MemArena *mem, const SymTable *tab, MathcHeader *hdr) {
    memset(hdr, 0, sizeof(*hdr));
    hdr->magic          = MATHC_MAGIC;
    hdr->version        = MATHC_VERSION;
    hdr->fast_real_type = mem->fast_real_type;
    hdr->str_type_id    = mem->str_type_id;
    hdr->vec_type_id    = mem->vec_type_id;
    hdr->mod_count      = tab->mod_count;
    hdr->type_count     = tab->type_count;
    hdr->unit_count     = tab->unit_count;
    hdr->sym_count      = tab->sym_count;
    hdr->const_count    = tab->const_count;
    hdr->perm_src_len   = mem->perm_src_len;
    hdr->perm_ast_count = mem->perm_ast_count;
    hdr->perm_pool_top  = mem->perm_pool_top;
    hdr->pat_elem_count = tab->pat_elem_count;
    hdr->field_count    = tab->field_count;
    hdr->overload_count = tab->overload_count;
    hdr->active_modules = tab->active_modules;
    hdr->payload_bytes  = calc_mathc_payload_bytes(hdr);
}

uint32_t ext_calc_mathc_size(const MemArena *mem, const SymTable *tab) {
    MathcHeader hdr;
    fill_mathc_header(mem, tab, &hdr);
    return (uint32_t)sizeof(MathcHeader) + hdr.payload_bytes;
}

size_t ext_save_mathc_mem(MemArena *mem, const SymTable *tab,
                          uint8_t *out_buf, size_t buf_cap) {
    mem_clear_err(mem);
    MathcHeader hdr;
    fill_mathc_header(mem, tab, &hdr);

    size_t total_bytes = sizeof(MathcHeader) + (size_t)hdr.payload_bytes;
    if (!out_buf || buf_cap < total_bytes) {
        mem_err(mem, "Output buffer too small for .mathc image (%zu < %zu bytes)",
                buf_cap, total_bytes);
        return 0;
    }

    uint8_t *cur = out_buf;

    #define WRITE_SLICE(src_ptr, byte_cnt) do { \
        size_t _n = (size_t)(byte_cnt); \
        if (_n > 0) { \
            memcpy(cur, (src_ptr), _n); \
            cur += _n; \
        } \
    } while (0)

    WRITE_SLICE(&hdr,           sizeof(MathcHeader));
    WRITE_SLICE(mem->src_buf,   hdr.perm_src_len);
    WRITE_SLICE(mem->ast,       hdr.perm_ast_count * sizeof(AstNode));
    WRITE_SLICE(mem->f64_pool,  hdr.perm_pool_top  * sizeof(double));
    WRITE_SLICE(tab->mod_names, hdr.mod_count      * sizeof(StrSlice));
    WRITE_SLICE(tab->types,     hdr.type_count     * sizeof(TypeDef));
    WRITE_SLICE(tab->units,     hdr.unit_count     * sizeof(StrSlice));
    WRITE_SLICE(tab->pat_elems, hdr.pat_elem_count * sizeof(PatElem));
    WRITE_SLICE(tab->fields,    hdr.field_count    * sizeof(FieldDef));
    WRITE_SLICE(tab->syms,      hdr.sym_count      * sizeof(StrSlice));
    WRITE_SLICE(tab->overloads, hdr.overload_count * sizeof(OpOverload));
    WRITE_SLICE(tab->consts,    hdr.const_count    * sizeof(ConstDef));

    #undef WRITE_SLICE

    return total_bytes;
}

bool ext_load_mathc_mem(MemArena *mem, SymTable *tab,
                        const uint8_t *data, size_t len) {
    mem_clear_err(mem);
    if (!data || len < sizeof(MathcHeader)) {
        mem_err(mem, "Invalid or truncated .mathc image");
        return false;
    }

    MathcHeader hdr;
    memcpy(&hdr, data, sizeof(MathcHeader));
    if (hdr.magic != MATHC_MAGIC || hdr.version != MATHC_VERSION) {
        mem_err(mem, "Bad .mathc magic or version mismatch");
        return false;
    }

    uint32_t expected_payload = calc_mathc_payload_bytes(&hdr);
    if (hdr.payload_bytes != expected_payload ||
        sizeof(MathcHeader) + (size_t)expected_payload > len) {
        mem_err(mem, "Corrupted .mathc payload size");
        return false;
    }

    if (hdr.perm_src_len   > MEM_SRC_BUF_CAP   ||
        hdr.perm_ast_count > MEM_MAX_AST_NODES ||
        hdr.perm_pool_top  > MEM_MAX_F64_POOL  ||
        hdr.mod_count      > SYM_MAX_MODULES   ||
        hdr.type_count     > SYM_MAX_TYPES     ||
        hdr.unit_count     > SYM_MAX_UNITS     ||
        hdr.pat_elem_count > SYM_MAX_PAT_ELEMS ||
        hdr.field_count    > SYM_MAX_FIELDS    ||
        hdr.sym_count      > SYM_MAX_SYMS      ||
        hdr.overload_count > SYM_MAX_OVERLOADS ||
        hdr.const_count    > SYM_MAX_CONSTS) {
        mem_err(mem, ".mathc image exceeds compiled arena capacities");
        return false;
    }

    const uint8_t *cur = data + sizeof(MathcHeader);

    #define COPY_SLICE(dst, byte_cnt) do { \
        size_t _n = (size_t)(byte_cnt); \
        if (_n > 0) { \
            memcpy((dst), cur, _n); \
            cur += _n; \
        } \
    } while (0)

    COPY_SLICE(mem->src_buf,   hdr.perm_src_len);
    COPY_SLICE(mem->ast,       hdr.perm_ast_count * sizeof(AstNode));
    COPY_SLICE(mem->f64_pool,  hdr.perm_pool_top  * sizeof(double));
    COPY_SLICE(tab->mod_names, hdr.mod_count      * sizeof(StrSlice));
    COPY_SLICE(tab->types,     hdr.type_count     * sizeof(TypeDef));
    COPY_SLICE(tab->units,     hdr.unit_count     * sizeof(StrSlice));
    COPY_SLICE(tab->pat_elems, hdr.pat_elem_count * sizeof(PatElem));
    COPY_SLICE(tab->fields,    hdr.field_count    * sizeof(FieldDef));
    COPY_SLICE(tab->syms,      hdr.sym_count      * sizeof(StrSlice));
    COPY_SLICE(tab->overloads, hdr.overload_count * sizeof(OpOverload));
    COPY_SLICE(tab->consts,    hdr.const_count    * sizeof(ConstDef));

    #undef COPY_SLICE

    mem->fast_real_type = hdr.fast_real_type;
    mem->str_type_id    = hdr.str_type_id;
    mem->vec_type_id    = hdr.vec_type_id;
    mem->src_len        = hdr.perm_src_len;
    mem->ast_count      = hdr.perm_ast_count;
    mem->pool_top       = hdr.perm_pool_top;

    tab->mod_count      = hdr.mod_count;
    tab->type_count     = hdr.type_count;
    tab->unit_count     = hdr.unit_count ? hdr.unit_count : 1;
    tab->pat_elem_count = hdr.pat_elem_count;
    tab->field_count    = hdr.field_count;
    tab->sym_count      = hdr.sym_count;
    tab->overload_count = hdr.overload_count;
    tab->const_count    = hdr.const_count;
    tab->active_modules = hdr.active_modules;

    /* Reconstruct ABI-specific Value representations for global constants */
    for (uint8_t i = 0; i < tab->const_count; ++i) {
        uint16_t rtid = tab->consts[i].type_id;
        uint8_t  ruid = tab->consts[i].unit_id;
        double   dval = mem_read_pool(mem, tab->consts[i].pool_idx);
        tab->const_vals[i] = val_make_f64_unit(mem, rtid, ruid, dval);
    }
    mem->pool_top = hdr.perm_pool_top;
    mem_snapshot_perm(mem);
    return true;
}
