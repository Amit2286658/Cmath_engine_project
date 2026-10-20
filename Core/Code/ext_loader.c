/*
 * ext_loader.c — In-Memory Extension Compiler & Binary Image Serializer
 *
 * Pure ISO C99 in-memory implementation: zero filesystem, directory, or file
 * extension dependencies (`<dirent.h>`, `<sys/stat.h>`, and `FILE*` I/O removed).
 *
 * Supports both explicit declarations and streamlined shorthand:
 *   - Optional quotes around function and constant names:
 *       `lerp(a, b, t) = a + (b - a) * t`  or  `"lerp"(a:real, b:real, t:real) -> real = ...`
 *       `g0 = 9.80665`                     or  `"g0" : real = 9.80665`
 *   - Optional `:real` parameter types and optional `-> real` return types
 *   - Default parameter values (`round(x:real, digits:real = 0) -> real = ...`)
 */

#include "ext_loader.h"
#include "parser.h"
#include "../lib/hardcode/hardcode.h"
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
        if (blen >= sizeof(buf) - 1) {
            mem_err(L->mem, "Quoted symbol exceeds maximum length (63 bytes)");
            StrSlice empty = { 0, 0 };
            return empty;
        }
        buf[blen++] = L->src[L->pos];
        L->pos++;
    }
    if (L->pos >= L->len || L->src[L->pos] != '"') {
        mem_err(L->mem, "Unterminated quoted string symbol in declaration");
        StrSlice empty = { 0, 0 };
        return empty;
    }
    L->pos++;
    return mem_intern_str(L->mem, buf, blen);
}

static void ext_parse_type_decl(ExtLoader *L) {
    MemArena *mem = L->mem;
    SymTable *tab = L->tab;
    L->pos++; /* skip '$' */
    StrSlice tname = ext_read_ident(L);
    if (tab->type_count >= SYM_MAX_TYPES) {
        mem_err(mem, "Type table overflow (max %d)", SYM_MAX_TYPES);
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
                if (esc == 'd') {
                    pe->kind = PAT_DIGIT;
                } else if (esc == 'a') {
                    pe->kind = PAT_ALPHA;
                } else {
                    mem_err(mem, "Invalid pattern escape '\\%c' (expected \\d or \\a)", esc);
                    return;
                }
            } else if (c == '"') {
                pe->kind  = PAT_LIT;
                pe->chars = ext_read_quoted(L);
            } else if (c == '[' && L->pos + 1 < L->len && L->src[L->pos + 1] == '^') {
                L->pos += 2;
                char excl[32];
                uint16_t elen = 0;
                while (L->pos < L->len && L->src[L->pos] != ']') {
                    if (L->src[L->pos] == '\\' && L->pos + 1 < L->len) L->pos++;
                    if (elen >= sizeof(excl) - 1) {
                        mem_err(mem, "Negated character class exceeds maximum length (31 bytes)");
                        return;
                    }
                    excl[elen++] = L->src[L->pos];
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
        } else if (td->parser_intr == INTR_MAT_TYPE) {
            td->is_consuming = 0;
            td->flat_size    = MEM_MAT_HDR_SLOTS;
            if (mem->mat_type_id == 0) {
                mem->mat_type_id = (uint8_t)tid;
            }
        } else if (td->parser_intr == INTR_FN_TYPE) {
            td->is_consuming = 0;
            td->flat_size    = MEM_FN_HDR_SLOTS;
            if (mem->fn_type_id == 0) {
                mem->fn_type_id = (uint8_t)tid;
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
            uint16_t ftid  = mem->fast_real_type;
            if (ext_match_char(L, ':')) {
                StrSlice ftype_s = ext_read_ident(L);
                ftid = sym_find_type(tab, mem, mem_str_ptr(mem, ftype_s), ftype_s.len);
                if (ftid == MEM_TYPE_VOID || ftid == SYM_TYPE_ANY) {
                    mem_err(mem, "Unknown field type '%.*s'", (int)ftype_s.len, mem_str_ptr(mem, ftype_s));
                    return;
                }
            }
            if (ftid == tid) {
                mem_err(mem, "Self-referential struct field '%.*s' in '$%.*s' is not allowed",
                        (int)fname.len, mem_str_ptr(mem, fname),
                        (int)tname.len, mem_str_ptr(mem, tname));
                return;
            }
            if (ftid >= tab->type_count || tab->types[ftid].flat_size == 0) {
                mem_err(mem, "Incomplete or invalid field type in '$%.*s'",
                        (int)tname.len, mem_str_ptr(mem, tname));
                return;
            }
            if (tab->field_count >= SYM_MAX_FIELDS) {
                mem_err(mem, "Struct field table overflow (max %d)", SYM_MAX_FIELDS);
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

    uint8_t uid = sym_find_or_add_unit(tab, mem, s1, first_id.len);
    *out_type = SYM_TYPE_ANY;
    *out_unit = uid;
    return !mem->has_error;
}

static bool ext_parse_one_param(ExtLoader *L, StrSlice *out_name,
                                uint16_t *out_type, uint8_t *out_unit) {
    if (!ext_match_char(L, '(')) return false;
    *out_name = ext_read_ident(L);
    if (sym_is_reserved_kw(L->tab, L->mem, mem_str_ptr(L->mem, *out_name), out_name->len)) {
        mem_err(L->mem, "Cannot use reserved keyword '%.*s' as a variable name",
                (int)out_name->len, mem_str_ptr(L->mem, *out_name));
        return false;
    }
    if (!ext_match_char(L, ':')) {
        *out_type = L->mem->fast_real_type ? L->mem->fast_real_type : 1;
        *out_unit = SYM_UNIT_OMITTED;
        if (!ext_match_char(L, ')')) {
            mem_err(L->mem, "Expected ')' after parameter declaration");
            return false;
        }
        return true;
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

static bool ext_peek_ident_is_unit_annot(ExtLoader *L) {
    ext_skip_ws(L);
    uint16_t p = L->pos;
    if (p >= L->len || !lex_is_ident_start((unsigned char)L->src[p])) return false;
    uint16_t id_start = p;
    while (p < L->len && lex_is_ident_cont((unsigned char)L->src[p])) p++;
    uint16_t id_len = p - id_start;
    while (p < L->len && (L->src[p] == ' ' || L->src[p] == '\t' || L->src[p] == '\r' || L->src[p] == '\n')) p++;
    if (p < L->len && (L->src[p] == ':' || L->src[p] == '=')) return false;
    return sym_find_unit(L->tab, L->mem, L->src + id_start, id_len) != MEM_UNIT_NONE;
}

static bool ext_parse_return_spec(ExtLoader *L, uint16_t *out_rtype, uint8_t *out_runit) {
    ext_skip_ws(L);
    if (L->pos < L->len && L->src[L->pos] == '=') {
        *out_rtype = L->mem->fast_real_type ? L->mem->fast_real_type : 1;
        *out_runit = SYM_UNIT_OMITTED;
        return true;
    }
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

static void ext_skip_default_expr(ExtLoader *L, uint16_t *out_start, uint16_t *out_end) {
    ext_skip_ws(L);
    *out_start = L->pos;
    int depth = 0;
    while (L->pos < L->len) {
        char c = L->src[L->pos];
        if (c == '"') {
            L->pos++;
            while (L->pos < L->len && L->src[L->pos] != '"') {
                if (L->src[L->pos] == '\\' && L->pos + 1 < L->len) L->pos++;
                L->pos++;
            }
            if (L->pos < L->len) L->pos++;
            continue;
        }
        if (c == '(' || c == '[' || c == '{') depth++;
        else if (c == ')' || c == ']' || c == '}') {
            if (depth == 0) break;
            depth--;
        } else if (c == ',' && depth == 0) {
            break;
        }
        L->pos++;
    }
    *out_end = L->pos;
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
            if (target_sym < tab->sym_count) {
                for (uint16_t i = tab->sym_first_ov[target_sym]; i != MEM_NO_IDX; i = tab->ov_next[i]) {
                    const OpOverload *target = &tab->overloads[i];
                    if (target != ov &&
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
}


static void ext_parse_sym_decl(ExtLoader *L, uint16_t sym_id) {
    MemArena *mem = L->mem;
    SymTable *tab = L->tab;
    ext_skip_ws(L);

    /* 1. Syntax delimiter binding (`"(" => @lparen`, `"{" => @lbrace`, `"," => @comma`) */
    if (L->pos + 1 < L->len && L->src[L->pos] == '=' && L->src[L->pos + 1] == '>') {
        L->pos += 2;
        if (!ext_match_char(L, '@')) {
            mem_err(mem, "Expected '@syntax_intrinsic' after '=>'");
            return;
        }
        StrSlice iname = ext_read_ident(L);
        IntrinsicId intr = sym_lookup_intrinsic(mem_str_ptr(mem, iname), iname.len);
        if (intr != INTR_LPAREN && intr != INTR_RPAREN &&
            intr != INTR_LBRACE && intr != INTR_RBRACE &&
            intr != INTR_LBRACK && intr != INTR_RBRACK &&
            intr != INTR_COMMA) {
            mem_err(mem, "Unknown syntax delimiter intrinsic '@%.*s'",
                    (int)iname.len, mem_str_ptr(mem, iname));
            return;
        }
        if (tab->overload_count >= SYM_MAX_OVERLOADS) {
            mem_err(mem, "Overload table overflow (max %d)", SYM_MAX_OVERLOADS);
            return;
        }
        uint16_t ov_idx = tab->overload_count++;
        OpOverload *ov  = &tab->overloads[ov_idx];
        memset(ov, 0, sizeof(*ov));
        ov->sym_id   = sym_id;
        ov->mod_id   = L->cur_mod;
        ov->fixity   = FIX_SYNTAX;
        ov->meta_tag = (uint8_t)intr;
        sym_link_overload(tab, ov_idx);
        return;
    }

    /* 2. Constant definition (`"g0" : real = 9.80665`, `g0 : real = 9.80665`, or `g0 = 9.80665`) */
    if (ext_match_char(L, ':') || ext_match_char(L, '=')) {
        bool already_matched_eq = (L->src[L->pos - 1] == '=');
        uint16_t rtid = mem->fast_real_type ? mem->fast_real_type : 1;
        uint8_t  ruid = SYM_UNIT_OMITTED;
        if (!already_matched_eq) {
            if (!ext_parse_return_spec(L, &rtid, &ruid)) return;
            if (rtid == SYM_TYPE_ANY) {
                rtid = mem->fast_real_type ? mem->fast_real_type : 1;
            }
            if (!ext_match_char(L, '=')) {
                mem_err(mem, "Expected '=' in constant definition");
                return;
            }
        }
        if (tab->const_count >= SYM_MAX_CONSTS) {
            mem_err(mem, "Constant table overflow (max %d)", SYM_MAX_CONSTS);
            return;
        }
        ext_skip_ws(L);
        Parser P;
        parser_init(&P, mem, tab, L->src, L->len, L->pos, false, true);
        lex_next(&P.lex);
        uint16_t c_ast = parser_parse_expr(&P, 0);
        L->pos = (P.lex.cur.kind == TOK_EOF) ? P.lex.pos : P.lex.cur.off;
        if (mem->has_error || c_ast == MEM_NO_IDX) return;

        uint16_t c_pool_idx = 0;
        uint8_t  final_unit = (ruid != SYM_UNIT_OMITTED && ruid != SYM_UNIT_GENERIC && ruid != SYM_UNIT_LOCK_NONE)
                              ? ruid : MEM_UNIT_NONE;
        if (mem_get_ast(mem, c_ast)->kind == AST_LIT) {
            c_pool_idx = mem_get_ast(mem, c_ast)->ref;
        } else {
            Value ev = eval_ast(mem, tab, c_ast, 0, (uint8_t)rtid);
            if (mem->has_error) return;
            double d = val_get_f64(mem, ev);
            if (final_unit == MEM_UNIT_NONE && ruid == SYM_UNIT_OMITTED) {
                final_unit = val_get_unit(mem, ev);
            }
            c_pool_idx = mem_alloc_pool(mem, 1);
            if (mem->has_error) return;
            mem_write_pool(mem, c_pool_idx, d);
        }
        Value cval = val_make_f64_at_pool(mem, rtid, final_unit, c_pool_idx);
        uint8_t cid = tab->const_count++;
        tab->consts[cid].sym_id   = sym_id;
        tab->consts[cid].mod_id   = L->cur_mod;
        tab->consts[cid].type_id  = (uint8_t)rtid;
        tab->consts[cid].unit_id  = final_unit;
        tab->consts[cid].val_idx  = cid;
        tab->consts[cid].pool_idx = c_pool_idx;
        tab->const_vals[cid]      = cval;
        return;
    }

    /* 3. Function or Prefix Operator Declaration (`"fn"(...)` or `fn(...)`) */
    if (!ext_match_char(L, '(')) {
        mem_err(mem, "Expected '(', ':', '=', or '=>' after symbol in declaration");
        return;
    }
    StrSlice pnames[8];
    uint16_t ptypes[8];
    uint8_t  punits[8];
    bool     has_def[8];
    uint16_t def_start[8];
    uint16_t def_end[8];
    uint8_t  pcount    = 0;
    uint8_t  req_count = 0;
    bool     seen_def  = false;
    memset(has_def, 0, sizeof(has_def));

    while (L->pos < L->len && !mem->has_error) {
        if (ext_match_char(L, ')')) break;
        if (pcount >= 8) { mem_err(mem, "Max 8 parameters supported"); break; }
        pnames[pcount] = ext_read_ident(L);
        if (sym_is_reserved_kw(tab, mem, mem_str_ptr(mem, pnames[pcount]), pnames[pcount].len)) {
            mem_err(mem, "Cannot use reserved keyword '%.*s' as a variable name",
                    (int)pnames[pcount].len, mem_str_ptr(mem, pnames[pcount]));
            break;
        }
        if (ext_match_char(L, ':')) {
            StrSlice tn = ext_read_ident(L);
            bool has_u  = false;
            StrSlice un = { 0, 0 };
            if (ext_match_char(L, ',')) {
                if (ext_peek_ident_is_unit_annot(L)) {
                    has_u = true;
                    un    = ext_read_ident(L);
                }
            }
            if (!ext_resolve_type_and_unit(L, tn, has_u, un, &ptypes[pcount], &punits[pcount])) {
                break;
            }
        } else {
            ptypes[pcount] = mem->fast_real_type ? mem->fast_real_type : 1;
            punits[pcount] = SYM_UNIT_OMITTED;
        }

        if (ext_match_char(L, '=')) {
            seen_def        = true;
            has_def[pcount] = true;
            ext_skip_default_expr(L, &def_start[pcount], &def_end[pcount]);
        } else {
            if (seen_def) {
                mem_err(mem, "Parameter without default value cannot follow a default parameter");
                break;
            }
            req_count++;
        }
        ext_match_char(L, ',');
        pcount++;
    }
    if (mem->has_error) return;

    ext_skip_ws(L);
    uint8_t fixity = FIX_FUNC;
    uint8_t prec   = 0;

    if (ext_match_char(L, ':')) {
        fixity = FIX_PREFIX;
        char *endp = NULL;
        prec = (uint8_t)strtol(L->src + L->pos, &endp, 10);
        L->pos = (uint16_t)(endp - L->src);
    }

    uint16_t rtid = 0;
    uint8_t  ruid = SYM_UNIT_OMITTED;
    if (!ext_parse_return_spec(L, &rtid, &ruid)) return;
    ext_match_char(L, '=');

    if (tab->overload_count >= SYM_MAX_OVERLOADS) {
        mem_err(mem, "Overload table overflow (max %d)", SYM_MAX_OVERLOADS);
        return;
    }
    uint16_t ov_idx = tab->overload_count++;
    OpOverload *ov  = &tab->overloads[ov_idx];
    memset(ov, 0, sizeof(*ov));
    ov->sym_id      = sym_id;
    ov->mod_id      = L->cur_mod;
    ov->fixity      = fixity;
    ov->prec        = prec;
    ov->param_count = pcount;
    ov->ret_type    = (uint8_t)rtid;
    ov->ret_unit    = ruid;
    for (uint8_t i = 0; i < pcount; ++i) {
        ov->param_types[i] = (uint8_t)ptypes[i];
        ov->param_units[i] = punits[i];
    }
    sym_link_overload(tab, ov_idx);

    ext_compile_body(L, ov, pnames);
    if (mem->has_error) return;

    /* 4. Synthesize lower-arity overload wrappers for trailing default parameters */
    if (seen_def && fixity == FIX_FUNC && req_count < pcount) {
        hardcode_synthesize_default_overloads(mem, tab, L->src, sym_id, L->cur_mod,
                                              ov_idx, pcount, req_count,
                                              pnames, ptypes, punits,
                                              def_start, def_end, rtid, ruid);
    }
}

bool ext_load_str(MemArena *mem, SymTable *tab, const char *ext_src) {
    mem_clear_err(mem);
    if (!ext_src) {
        mem_err(mem, "Null extension source string");
        return false;
    }
    size_t raw_len = strlen(ext_src);
    if (raw_len > 65534u) {
        mem_err(mem, "Extension source length exceeds maximum (65534 bytes)");
        return false;
    }

    /* Save watermarks for transactional rollback on compile error */
    uint16_t saved_src_len        = mem->src_len;
    uint16_t saved_ast_count      = mem->ast_count;
    uint16_t saved_pool_top       = mem->pool_top;
    uint8_t  saved_fast_real      = mem->fast_real_type;
    uint8_t  saved_str_type       = mem->str_type_id;
    uint8_t  saved_vec_type       = mem->vec_type_id;
    uint8_t  saved_mat_type       = mem->mat_type_id;
    uint8_t  saved_fn_type        = mem->fn_type_id;

    uint8_t  saved_mod_count      = tab->mod_count;
    uint64_t saved_active_modules = tab->active_modules;
    uint8_t  saved_type_count     = tab->type_count;
    uint8_t  saved_unit_count     = tab->unit_count;
    uint16_t saved_pat_elem_count = tab->pat_elem_count;
    uint16_t saved_field_count    = tab->field_count;
    uint16_t saved_sym_count      = tab->sym_count;
    uint16_t saved_overload_count = tab->overload_count;
    uint8_t  saved_const_count    = tab->const_count;

    ExtLoader L;
    memset(&L, 0, sizeof(L));
    L.mem = mem;
    L.tab = tab;
    L.src = ext_src;
    L.len = (uint16_t)raw_len;
    L.pos = 0;

    /* Default any extension without an explicit `[module]` header to `[default]` */
    StrSlice def_mod = mem_intern_str(mem, "default", 7);
    L.cur_mod = sym_find_or_add_mod(tab, mem, def_mod);

    while (L.pos < L.len && !mem->has_error) {
        ext_skip_ws(&L);
        if (L.pos >= L.len) break;

        char c = L.src[L.pos];

        if (c == '[') {
            L.pos++;
            StrSlice mname = ext_read_ident(&L);
            if (!ext_match_char(&L, ']')) {
                mem_err(mem, "Expected closing ']' in module header");
                break;
            }
            L.cur_mod = sym_find_or_add_mod(tab, mem, mname);
            continue;
        }

        if (c == '<' && L.pos + 1 < L.len && L.src[L.pos + 1] == '[') {
            L.pos += 2;
            StrSlice dep = ext_read_ident(&L);
            if (!ext_match_char(&L, ']')) {
                mem_err(mem, "Expected closing ']' in module dependency");
                break;
            }
            uint8_t dep_id = sym_find_or_add_mod(tab, mem, dep);
            tab->active_modules |= (1ULL << dep_id);
            continue;
        }

        if (c == '$') {
            ext_parse_type_decl(&L);
            continue;
        }

        if (c == '"') {
            StrSlice sym = ext_read_quoted(&L);
            if (mem->has_error) break;
            uint16_t sym_id = sym_intern(tab, mem, sym);
            if (mem->has_error) break;
            ext_parse_sym_decl(&L, sym_id);
            continue;
        }

        if (lex_is_ident_start((unsigned char)c)) {
            StrSlice sym = ext_read_ident(&L);
            if (mem->has_error || sym.len == 0) break;
            uint16_t sym_id = sym_intern(tab, mem, sym);
            if (mem->has_error) break;
            ext_parse_sym_decl(&L, sym_id);
            continue;
        }

        if (c == '(') {
            StrSlice pnames[8];
            uint16_t ptypes[8];
            uint8_t  punits[8];
            if (!ext_parse_one_param(&L, &pnames[0], &ptypes[0], &punits[0])) break;

            ext_skip_ws(&L);
            StrSlice sym = (L.pos < L.len && L.src[L.pos] == '"')
                           ? ext_read_quoted(&L)
                           : ext_read_ident(&L);
            if (mem->has_error || sym.len == 0) break;
            uint16_t sym_id = sym_intern(tab, mem, sym);
            if (mem->has_error) break;
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
                mem_err(mem, "Overload table overflow (max %d)", SYM_MAX_OVERLOADS);
                break;
            }
            uint16_t ov_idx = tab->overload_count++;
            OpOverload *ov  = &tab->overloads[ov_idx];
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
            sym_link_overload(tab, ov_idx);

            ext_compile_body(&L, ov, pnames);
            continue;
        }

        mem_err(mem, "Unrecognized declaration in extension near '%.*s'",
                (int)(L.len - L.pos < 16 ? L.len - L.pos : 16), L.src + L.pos);
        break;
    }

    if (mem->has_error) {
        /* Transactional rollback: restore all permanent & symbol table counters */
        mem->src_len        = saved_src_len;
        mem->ast_count      = saved_ast_count;
        mem->pool_top       = saved_pool_top;
        mem->fast_real_type = saved_fast_real;
        mem->str_type_id    = saved_str_type;
        mem->vec_type_id    = saved_vec_type;
        mem->mat_type_id    = saved_mat_type;
        mem->fn_type_id     = saved_fn_type;

        tab->mod_count      = saved_mod_count;
        tab->active_modules = saved_active_modules;
        tab->type_count     = saved_type_count;
        tab->unit_count     = saved_unit_count;
        tab->pat_elem_count = saved_pat_elem_count;
        tab->field_count    = saved_field_count;
        tab->sym_count      = saved_sym_count;
        tab->overload_count = saved_overload_count;
        tab->const_count    = saved_const_count;
        sym_rebuild_index(tab);
        return false;
    }

    mem_snapshot_perm(mem);
    return true;
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
    hdr->const_count    = tab->const_count;
    hdr->sym_count      = tab->sym_count;
    hdr->unit_count     = tab->unit_count;
    hdr->mat_type_id    = mem->mat_type_id;
    hdr->perm_src_len   = mem->perm_src_len;
    hdr->perm_ast_count = mem->perm_ast_count;
    hdr->perm_pool_top  = mem->perm_pool_top;
    hdr->pat_elem_count = tab->pat_elem_count;
    hdr->field_count    = tab->field_count;
    hdr->overload_count = tab->overload_count;
    hdr->active_modules = (uint32_t)(tab->active_modules & 0xFFFFFFFFu);
    hdr->payload_bytes  = calc_mathc_payload_bytes(hdr);
    hdr->checksum       = 0;
}

uint32_t ext_calc_mathc_checksum(const MathcHeader *hdr,
                                 const uint8_t *payload, size_t payload_len) {
    if (!hdr || (!payload && payload_len > 0)) return 0;
    MathcHeader clean_hdr = *hdr;
    clean_hdr.checksum = 0;

    uint32_t h = 2166136261u;
    const uint8_t *hb = (const uint8_t *)&clean_hdr;
    for (size_t i = 0; i < sizeof(MathcHeader); ++i) {
        h ^= hb[i];
        h *= 16777619u;
    }
    for (size_t i = 0; i < payload_len; ++i) {
        h ^= payload[i];
        h *= 16777619u;
    }
    return (h == 0) ? 1u : h;
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

    hdr.checksum = ext_calc_mathc_checksum(&hdr,
                                           out_buf + sizeof(MathcHeader),
                                           hdr.payload_bytes);
    memcpy(out_buf, &hdr, sizeof(MathcHeader));

    return total_bytes;
}

static bool validate_mathc_payload(const uint8_t *payload, const MathcHeader *hdr) {
    /* 1. Validate special bootstrap type IDs in header */
    if (hdr->fast_real_type >= hdr->type_count ||
        hdr->str_type_id    >= hdr->type_count ||
        hdr->vec_type_id    >= hdr->type_count ||
        hdr->mat_type_id    >= hdr->type_count) {
        return false;
    }

    const uint8_t *cur        = payload;
    const uint8_t *src_ptr    = cur; cur += hdr->perm_src_len;
    const uint8_t *ast_ptr    = cur; cur += (size_t)hdr->perm_ast_count * sizeof(AstNode);
    const uint8_t *pool_ptr   = cur; cur += (size_t)hdr->perm_pool_top  * sizeof(double);
    const uint8_t *mods_ptr   = cur; cur += (size_t)hdr->mod_count      * sizeof(StrSlice);
    const uint8_t *types_ptr  = cur; cur += (size_t)hdr->type_count     * sizeof(TypeDef);
    const uint8_t *units_ptr  = cur; cur += (size_t)hdr->unit_count     * sizeof(StrSlice);
    const uint8_t *pats_ptr   = cur; cur += (size_t)hdr->pat_elem_count * sizeof(PatElem);
    const uint8_t *fields_ptr = cur; cur += (size_t)hdr->field_count    * sizeof(FieldDef);
    const uint8_t *syms_ptr   = cur; cur += (size_t)hdr->sym_count      * sizeof(StrSlice);
    const uint8_t *ovs_ptr    = cur; cur += (size_t)hdr->overload_count * sizeof(OpOverload);
    const uint8_t *consts_ptr = cur; cur += (size_t)hdr->const_count    * sizeof(ConstDef);
    (void)src_ptr;
    (void)pool_ptr;

    /* 2. Validate module names */
    for (uint8_t i = 0; i < hdr->mod_count; ++i) {
        StrSlice s;
        memcpy(&s, mods_ptr + (size_t)i * sizeof(StrSlice), sizeof(StrSlice));
        if ((uint32_t)s.off + (uint32_t)s.len > hdr->perm_src_len) return false;
    }

    /* 3. Validate types */
    for (uint16_t i = 1; i < hdr->type_count; ++i) {
        TypeDef td;
        memcpy(&td, types_ptr + (size_t)i * sizeof(TypeDef), sizeof(TypeDef));
        if ((uint32_t)td.name.off + (uint32_t)td.name.len > hdr->perm_src_len) return false;
        if (td.mod_id >= hdr->mod_count) return false;
        if (td.parser_intr >= INTR_COUNT) return false;
        if (td.is_consuming) {
            if ((uint32_t)td.start_idx + (uint32_t)td.field_count > hdr->pat_elem_count) return false;
        } else if (td.parser_intr == INTR_NONE) {
            if ((uint32_t)td.start_idx + (uint32_t)td.field_count > hdr->field_count) return false;
            if (td.flat_size == 0 || td.flat_size > 256u) return false;
        }
    }

    /* 4. Validate units */
    for (uint8_t i = 1; i < hdr->unit_count; ++i) {
        StrSlice s;
        memcpy(&s, units_ptr + (size_t)i * sizeof(StrSlice), sizeof(StrSlice));
        if ((uint32_t)s.off + (uint32_t)s.len > hdr->perm_src_len) return false;
    }

    /* 5. Validate pattern elements */
    for (uint16_t i = 0; i < hdr->pat_elem_count; ++i) {
        PatElem pe;
        memcpy(&pe, pats_ptr + (size_t)i * sizeof(PatElem), sizeof(PatElem));
        if (pe.kind < PAT_DIGIT || pe.kind > PAT_GROUP_END) return false;
        if (pe.quant > QUANT_OPT) return false;
        if ((uint32_t)pe.chars.off + (uint32_t)pe.chars.len > hdr->perm_src_len) return false;
        if (pe.kind == PAT_LIT && pe.chars.len == 0) return false;
    }

    /* 6. Validate struct fields */
    for (uint16_t i = 0; i < hdr->field_count; ++i) {
        FieldDef fd;
        memcpy(&fd, fields_ptr + (size_t)i * sizeof(FieldDef), sizeof(FieldDef));
        if ((uint32_t)fd.name.off + (uint32_t)fd.name.len > hdr->perm_src_len) return false;
        if (fd.type_id == 0 || fd.type_id >= hdr->type_count) return false;
        if (fd.flat_offset >= 256u) return false;
    }

    /* 7. Validate struct field ranges against parent struct flat_size */
    for (uint16_t i = 1; i < hdr->type_count; ++i) {
        TypeDef td;
        memcpy(&td, types_ptr + (size_t)i * sizeof(TypeDef), sizeof(TypeDef));
        if (!td.is_consuming && td.parser_intr == INTR_NONE) {
            for (uint8_t f = 0; f < td.field_count; ++f) {
                FieldDef fd;
                memcpy(&fd, fields_ptr + (size_t)(td.start_idx + f) * sizeof(FieldDef), sizeof(FieldDef));
                if (fd.type_id == i) return false;
                TypeDef ftd;
                memcpy(&ftd, types_ptr + (size_t)fd.type_id * sizeof(TypeDef), sizeof(TypeDef));
                uint32_t fsz = ftd.is_consuming ? 1u : (uint32_t)ftd.flat_size;
                if (fsz == 0 || (uint32_t)fd.flat_offset + fsz > (uint32_t)td.flat_size) return false;
            }
        }
    }

    /* 8. Validate interned symbols */
    for (uint16_t i = 0; i < hdr->sym_count; ++i) {
        StrSlice s;
        memcpy(&s, syms_ptr + (size_t)i * sizeof(StrSlice), sizeof(StrSlice));
        if ((uint32_t)s.off + (uint32_t)s.len > hdr->perm_src_len) return false;
    }

    /* 9. Validate operator & function overloads */
    uint8_t eff_units = hdr->unit_count ? hdr->unit_count : 1u;
    for (uint16_t i = 0; i < hdr->overload_count; ++i) {
        OpOverload ov;
        memcpy(&ov, ovs_ptr + (size_t)i * sizeof(OpOverload), sizeof(OpOverload));
        if (ov.sym_id >= hdr->sym_count) return false;
        if (ov.mod_id >= hdr->mod_count) return false;
        if (ov.fixity > FIX_SYNTAX) return false;
        if (ov.param_count > 8u || ov.local_count > PARSER_MAX_LOCALS) return false;
        if (ov.local_count < ov.param_count) return false;
        if (ov.ret_type >= hdr->type_count && ov.ret_type != SYM_TYPE_ANY) return false;
        if (ov.ret_unit < SYM_UNIT_LOCK_NONE && ov.ret_unit >= eff_units) return false;
        for (uint8_t p = 0; p < ov.param_count; ++p) {
            if (ov.param_types[p] >= hdr->type_count && ov.param_types[p] != SYM_TYPE_ANY) return false;
            if (ov.param_units[p] < SYM_UNIT_LOCK_NONE && ov.param_units[p] >= eff_units) return false;
        }
        if (ov.fixity != FIX_SYNTAX) {
            if (ov.body_ast >= hdr->perm_ast_count) return false;
        }
    }

    /* 10. Validate constants */
    for (uint8_t i = 0; i < hdr->const_count; ++i) {
        ConstDef cd;
        memcpy(&cd, consts_ptr + (size_t)i * sizeof(ConstDef), sizeof(ConstDef));
        if (cd.sym_id >= hdr->sym_count) return false;
        if (cd.mod_id >= hdr->mod_count) return false;
        if (cd.type_id == 0 || cd.type_id >= hdr->type_count) return false;
        if (cd.unit_id >= eff_units) return false;
        if (cd.val_idx >= hdr->const_count) return false;
        if (cd.pool_idx >= hdr->perm_pool_top) return false;
    }

    /* 11. Validate every AST node and all child indices (`left`, `right`, `ref`, `sub`) */
    #define AST_OPT_OK(idx) ((idx) == MEM_NO_IDX || (idx) < hdr->perm_ast_count)
    #define AST_REQ_OK(idx) ((idx) < hdr->perm_ast_count)

    for (uint16_t i = 0; i < hdr->perm_ast_count; ++i) {
        AstNode n;
        memcpy(&n, ast_ptr + (size_t)i * sizeof(AstNode), sizeof(AstNode));
        switch ((AstKind)n.kind) {
            case AST_LIT:
                if (n.sub >= hdr->type_count) return false;
                if (n.sub == hdr->str_type_id && hdr->str_type_id != 0) {
                    if ((uint32_t)n.left + (uint32_t)n.right > hdr->perm_src_len) return false;
                } else if (n.sub != MEM_TYPE_VOID) {
                    if (n.ref >= hdr->perm_pool_top) return false;
                }
                break;

            case AST_CONST_SYM:
                if (n.ref >= hdr->sym_count) return false;
                break;

            case AST_SLOT_LOAD:
                if (n.sub >= PARSER_MAX_LOCALS) return false;
                if (n.left != MEM_NO_IDX && n.left != SYM_TYPE_ANY && n.left >= hdr->type_count) return false;
                if (n.right != MEM_NO_IDX && n.right < SYM_UNIT_LOCK_NONE && n.right >= eff_units) return false;
                break;

            case AST_FIELD_LOAD:
                if (n.sub >= hdr->type_count) return false;
                if (n.ref >= 256u) return false;
                if (!AST_REQ_OK(n.left)) return false;
                if (n.right != MEM_NO_IDX && n.right >= PARSER_MAX_LOCALS) return false;
                break;

            case AST_UNARY:
                if (n.sub > FIX_SYNTAX || n.ref >= hdr->sym_count || !AST_REQ_OK(n.left)) return false;
                break;

            case AST_BINARY:
                if (n.sub > FIX_SYNTAX || n.ref >= hdr->sym_count ||
                    !AST_REQ_OK(n.left) || !AST_REQ_OK(n.right)) return false;
                break;

            case AST_CALL:
                if (n.ref >= hdr->sym_count || !AST_OPT_OK(n.left) || !AST_OPT_OK(n.right)) return false;
                break;

            case AST_ARG_LINK:
                if (!AST_OPT_OK(n.left) || !AST_OPT_OK(n.right)) return false;
                break;

            case AST_INTRINSIC:
                if (n.sub == INTR_NONE || n.sub >= INTR_COUNT) return false;
                if (!AST_OPT_OK(n.left) || !AST_OPT_OK(n.right)) return false;
                if (n.ref != 0 && !AST_OPT_OK(n.ref)) return false;
                break;

            case AST_CONSTRUCT:
                if (n.ref == 0 || n.ref >= hdr->type_count || !AST_OPT_OK(n.left)) return false;
                break;

            case AST_EXTERN:
                if (n.ref >= hdr->sym_count || !AST_OPT_OK(n.left)) return false;
                break;

            case AST_TERNARY:
                if (!AST_REQ_OK(n.left) || !AST_REQ_OK(n.right)) return false;
                break;

            case AST_SWITCH_EQ:
                if (n.sub > 3 || !AST_REQ_OK(n.left) || !AST_OPT_OK(n.right)) return false;
                if (n.sub == 0 && !AST_REQ_OK(n.right)) return false;
                if (n.sub == 1 && (n.ref == 0 || n.ref >= hdr->type_count)) return false;
                if (n.sub == 2 && n.ref >= PARSER_MAX_LOCALS) return false;
                break;

            case AST_TAGGED_SCOPE:
                if (n.sub >= MEM_MAX_LABELS || !AST_REQ_OK(n.left)) return false;
                break;

            case AST_JUMP:
            case AST_RUN_TAG:
                if (n.sub >= MEM_MAX_LABELS) return false;
                break;

            case AST_BREAK:
                if ((n.sub != 0xFFu && n.sub >= MEM_MAX_LABELS) || !AST_OPT_OK(n.left)) return false;
                break;

            case AST_CONTINUE:
                if (n.sub != 0xFFu && n.sub >= MEM_MAX_LABELS) return false;
                break;

            case AST_RETURN:
            case AST_MAT_LIT:
                if (!AST_OPT_OK(n.left)) return false;
                break;

            case AST_INDEX:
                if (!AST_REQ_OK(n.left) || !AST_REQ_OK(n.right)) return false;
                break;

            case AST_FOR_IN:
                if (n.sub >= PARSER_MAX_LOCALS || !AST_REQ_OK(n.left) || !AST_REQ_OK(n.right)) return false;
                break;

            case AST_UNPACK_ELEM:
                if (n.left >= PARSER_MAX_LOCALS) return false;
                break;

            case AST_LAMBDA:
                if (n.sub > 4 || !AST_REQ_OK(n.ref)) return false;
                break;

            case AST_FN_CALL:
                if (!AST_REQ_OK(n.left) || !AST_OPT_OK(n.right)) return false;
                break;

            default:
                return false;
        }
    }

    #undef AST_OPT_OK
    #undef AST_REQ_OK
    return true;
}

bool ext_validate_mathc_buf(const uint8_t *data, size_t len) {
    if (!data || len < sizeof(MathcHeader)) return false;
    MathcHeader hdr;
    memcpy(&hdr, data, sizeof(MathcHeader));
    if (hdr.magic != MATHC_MAGIC || hdr.version != MATHC_VERSION) return false;
    uint32_t expected_payload = calc_mathc_payload_bytes(&hdr);
    if (hdr.payload_bytes != expected_payload ||
        sizeof(MathcHeader) + (size_t)expected_payload > len) {
        return false;
    }
    if (hdr.perm_src_len   > MEM_SRC_BUF_CAP   ||
        hdr.perm_ast_count > MEM_MAX_AST_NODES ||
        hdr.perm_pool_top  > MEM_MAX_F64_POOL  ||
        hdr.mod_count      > SYM_MAX_MODULES   ||
        hdr.type_count     > SYM_MAX_TYPES     ||
        hdr.type_count     == 0                ||
        hdr.unit_count     > SYM_MAX_UNITS     ||
        hdr.pat_elem_count > SYM_MAX_PAT_ELEMS ||
        hdr.field_count    > SYM_MAX_FIELDS    ||
        hdr.sym_count      > SYM_MAX_SYMS      ||
        hdr.overload_count > SYM_MAX_OVERLOADS ||
        hdr.const_count    > SYM_MAX_CONSTS) {
        return false;
    }
    uint32_t expected_csum = ext_calc_mathc_checksum(&hdr,
                                                     data + sizeof(MathcHeader),
                                                     hdr.payload_bytes);
    if (hdr.checksum != expected_csum) return false;
    return validate_mathc_payload(data + sizeof(MathcHeader), &hdr);
}

bool ext_load_mathc_mem(MemArena *mem, SymTable *tab,
                        const uint8_t *data, size_t len) {
    if (!mem || !tab) return false;
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
        hdr.type_count     == 0                ||
        hdr.unit_count     > SYM_MAX_UNITS     ||
        hdr.pat_elem_count > SYM_MAX_PAT_ELEMS ||
        hdr.field_count    > SYM_MAX_FIELDS    ||
        hdr.sym_count      > SYM_MAX_SYMS      ||
        hdr.overload_count > SYM_MAX_OVERLOADS ||
        hdr.const_count    > SYM_MAX_CONSTS) {
        mem_err(mem, ".mathc image exceeds compiled arena capacities");
        return false;
    }

    /* Validate all payload table & AST child indices BEFORE touching mem or tab */
    if (!validate_mathc_payload(data + sizeof(MathcHeader), &hdr)) {
        mem_err(mem, "Corrupted .mathc index out of bounds");
        return false;
    }

    uint32_t expected_csum = ext_calc_mathc_checksum(&hdr,
                                                     data + sizeof(MathcHeader),
                                                     hdr.payload_bytes);
    if (hdr.checksum != expected_csum) {
        mem_err(mem, "Corrupted .mathc checksum mismatch");
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
    mem->mat_type_id    = hdr.mat_type_id;
    mem->fn_type_id     = 0;
    for (uint16_t i = 1; i < hdr.type_count; ++i) {
        if (tab->types[i].parser_intr == INTR_FN_TYPE) {
            mem->fn_type_id = (uint8_t)i;
            break;
        }
    }
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
    tab->active_modules = (uint64_t)hdr.active_modules;

    /* Reconstruct ABI-specific Value representations for global constants IN-PLACE
       using their saved `pool_idx` (zero new pool allocations). */
    for (uint8_t i = 0; i < tab->const_count; ++i) {
        uint16_t rtid = tab->consts[i].type_id;
        uint8_t  ruid = tab->consts[i].unit_id;
        uint16_t pidx = tab->consts[i].pool_idx;
        tab->const_vals[i] = val_make_f64_at_pool(mem, rtid, ruid, pidx);
    }

    sym_rebuild_index(tab);
    mem_snapshot_perm(mem);
    return true;
}
