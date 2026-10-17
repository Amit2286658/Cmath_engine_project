/*
 * ext_loader.c — External `.math` Extension Compiler & Directory Bootstrapper Implementation
 */

#include "../Headers/ext_loader.h"
#include "../Headers/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

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
        mem_err(mem, "Expected '~' (consuming) or '(' (struct) in type declaration");
    }
}

static bool ext_parse_one_param(ExtLoader *L, StrSlice *out_name, uint16_t *out_type) {
    if (!ext_match_char(L, '(')) return false;
    *out_name = ext_read_ident(L);
    if (!ext_match_char(L, ':')) {
        mem_err(L->mem, "Expected ':' in parameter declaration");
        return false;
    }
    StrSlice tname = ext_read_ident(L);
    *out_type = sym_find_type(L->tab, L->mem, mem_str_ptr(L->mem, tname), tname.len);
    if (*out_type == MEM_TYPE_VOID) {
        mem_err(L->mem, "Unknown parameter type '%.*s'", (int)tname.len, mem_str_ptr(L->mem, tname));
        return false;
    }
    if (!ext_match_char(L, ')')) {
        mem_err(L->mem, "Expected ')' after parameter declaration");
        return false;
    }
    return true;
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
            uint8_t target_sym = (uint8_t)root->ref;
            for (uint16_t i = 0; i < tab->overload_count; ++i) {
                const OpOverload *target = &tab->overloads[i];
                if (target != ov && target->sym_id == target_sym &&
                    target->param_count == 2 &&
                    target->param_types[0] == ov->param_types[0] &&
                    target->param_types[1] == ov->param_types[1]) {
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
            uint8_t  sym_id = sym_intern(tab, mem, sym);
            ext_skip_ws(&L);

            /* Syntax delimiter binding (`"(" => @lparen`, `")" => @rparen`, `"," => @comma`) */
            if (L.pos + 1 < L.len && L.src[L.pos] == '=' && L.src[L.pos + 1] == '>') {
                L.pos += 2;
                if (!ext_match_char(&L, '@')) {
                    mem_err(mem, "Expected '@syntax_intrinsic' after '=>'");
                    break;
                }
                StrSlice iname = ext_read_ident(&L);
                IntrinsicId intr = sym_lookup_intrinsic(mem_str_ptr(mem, iname), iname.len);
                if (intr != INTR_LPAREN && intr != INTR_RPAREN && intr != INTR_COMMA) {
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
                StrSlice rtype_s = ext_read_ident(&L);
                uint16_t rtid = sym_find_type(tab, mem, mem_str_ptr(mem, rtype_s), rtype_s.len);
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

                Value cval = val_void();
                if (c_ast != MEM_NO_IDX && mem_get_ast(mem, c_ast)->kind == AST_LIT) {
                    cval = val_make_f64(mem, rtid, mem_read_pool(mem, mem_get_ast(mem, c_ast)->ref));
                }
                if (tab->const_count < SYM_MAX_CONSTS) {
                    uint8_t cid = tab->const_count++;
                    tab->consts[cid].sym_id  = sym_id;
                    tab->consts[cid].mod_id  = L.cur_mod;
                    tab->consts[cid].val_idx = cid;
                    tab->const_vals[cid]     = cval;
                }
                continue;
            }

            if (!ext_match_char(&L, '(')) {
                mem_err(mem, "Expected '(', ':', or '=>' after symbol in declaration");
                break;
            }
            StrSlice pnames[4];
            uint16_t ptypes[4];
            uint8_t  pcount = 0;

            while (L.pos < L.len && !mem->has_error) {
                if (ext_match_char(&L, ')')) break;
                if (pcount >= 4) { mem_err(mem, "Max 4 parameters supported"); break; }
                pnames[pcount] = ext_read_ident(&L);
                ext_match_char(&L, ':');
                StrSlice tn = ext_read_ident(&L);
                ptypes[pcount] = sym_find_type(tab, mem, mem_str_ptr(mem, tn), tn.len);
                pcount++;
                ext_match_char(&L, ',');
            }

            ext_skip_ws(&L);
            uint8_t fixity = FIX_FUNC;
            uint8_t prec   = 0;

            if (ext_match_char(&L, ':')) {
                fixity = FIX_PREFIX;
                char *endp = NULL;
                prec = (uint8_t)strtol(L.src + L.pos, &endp, 10);
                L.pos = (uint16_t)(endp - L.src);
            }

            ext_skip_ws(&L);
            if (L.pos + 1 < L.len && L.src[L.pos] == '-' && L.src[L.pos + 1] == '>') L.pos += 2;
            StrSlice rtype_s = ext_read_ident(&L);
            uint16_t rtid = sym_find_type(tab, mem, mem_str_ptr(mem, rtype_s), rtype_s.len);
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
            for (uint8_t i = 0; i < pcount; ++i) ov->param_types[i] = (uint8_t)ptypes[i];

            ext_compile_body(&L, ov, pnames);
            continue;
        }

        if (c == '(') {
            StrSlice pnames[4];
            uint16_t ptypes[4];
            if (!ext_parse_one_param(&L, &pnames[0], &ptypes[0])) break;

            StrSlice sym    = ext_read_quoted(&L);
            uint8_t  sym_id = sym_intern(tab, mem, sym);
            ext_skip_ws(&L);

            uint8_t pcount = 1;
            uint8_t fixity = FIX_POSTFIX;
            uint8_t prec   = 0;

            if (L.pos < L.len && L.src[L.pos] == '(') {
                if (!ext_parse_one_param(&L, &pnames[1], &ptypes[1])) break;
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

            ext_skip_ws(&L);
            if (L.pos + 1 < L.len && L.src[L.pos] == '-' && L.src[L.pos + 1] == '>') L.pos += 2;
            StrSlice rtype_s = ext_read_ident(&L);
            uint16_t rtid = sym_find_type(tab, mem, mem_str_ptr(mem, rtype_s), rtype_s.len);
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
            ov->param_types[0] = (uint8_t)ptypes[0];
            if (pcount == 2) ov->param_types[1] = (uint8_t)ptypes[1];

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

bool ext_load_file(MemArena *mem, SymTable *tab, const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        mem_err(mem, "Cannot open extension file '%s'", filepath);
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsize < 0 || fsize > 65535) {
        fclose(fp);
        mem_err(mem, "Extension file '%s' too large", filepath);
        return false;
    }
    char *buf = (char *)malloc((size_t)fsize + 1);
    if (!buf) {
        fclose(fp);
        mem_err(mem, "Out of memory reading file");
        return false;
    }
    size_t read_bytes = fread(buf, 1, (size_t)fsize, fp);
    buf[read_bytes] = '\0';
    fclose(fp);

    bool ok = ext_load_str(mem, tab, buf);
    free(buf);
    return ok;
}

static int cmp_str(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

bool ext_boot_dir(MemArena *mem, SymTable *tab, const char *lib_dir_path) {
    DIR *d = opendir(lib_dir_path);
    if (!d) {
        mem_err(mem, "Cannot open library directory '%s'", lib_dir_path);
        return false;
    }
    char *files[32];
    int count = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && count < 32) {
        size_t nlen = strlen(ent->d_name);
        if (nlen > 5 && strcmp(ent->d_name + nlen - 5, ".math") == 0) {
            size_t plen = strlen(lib_dir_path) + 1 + nlen + 1;
            char *full = (char *)malloc(plen);
            snprintf(full, plen, "%s/%s", lib_dir_path, ent->d_name);
            files[count++] = full;
        }
    }
    closedir(d);

    qsort(files, (size_t)count, sizeof(char *), cmp_str);

    bool ok = true;
    for (int i = 0; i < count; ++i) {
        if (ok && !ext_load_file(mem, tab, files[i])) {
            ok = false;
        }
        free(files[i]);
    }
    return ok;
}