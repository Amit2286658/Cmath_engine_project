/*
 * lexer.c — Standalone UTF-8 & Dynamic Pattern-Driven Lexer Implementation
 */

#include "lexer.h"
#include <stdlib.h>
#include <string.h>

bool lex_is_ident_start(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0x80;
}

bool lex_is_ident_cont(unsigned char c) {
    return lex_is_ident_start(c) || c == '_' || (c >= '0' && c <= '9');
}

static bool is_ascii_word(const char *s, uint16_t len) {
    if (len == 0) return false;
    unsigned char c0 = (unsigned char)s[0];
    if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z'))) return false;
    for (uint16_t i = 1; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9'))) {
            return false;
        }
    }
    return true;
}

static bool ident_continues_at(const char *src, uint16_t p, uint16_t len) {
    if (p >= len) return false;
    unsigned char c = (unsigned char)src[p];
    if (lex_is_ident_start(c) || (c >= '0' && c <= '9')) return true;
    if (c == '_') {
        if (p + 1 >= len) return false;
        unsigned char nc = (unsigned char)src[p + 1];
        return lex_is_ident_start(nc) || (nc >= '0' && nc <= '9');
    }
    return false;
}

static int match_single_atom(const MemArena *mem, const PatElem *pe,
                             const char *src, uint16_t pos, uint16_t len) {
    if (pos >= len) return -1;
    unsigned char c = (unsigned char)src[pos];
    switch (pe->kind) {
        case PAT_DIGIT:
            return (c >= '0' && c <= '9') ? 1 : -1;
        case PAT_ALPHA:
            return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') ? 1 : -1;
        case PAT_LIT: {
            if (pe->chars.len == 0 || pos + pe->chars.len > len) return -1;
            if (memcmp(src + pos, mem_str_ptr(mem, pe->chars), pe->chars.len) == 0) {
                return (int)pe->chars.len;
            }
            return -1;
        }
        case PAT_NOT_CHARS: {
            const char *excl = mem_str_ptr(mem, pe->chars);
            for (uint16_t i = 0; i < pe->chars.len; ++i) {
                if (src[pos] == excl[i]) return -1;
            }
            int ulen = 1;
            if ((c & 0xE0) == 0xC0) ulen = 2;
            else if ((c & 0xF0) == 0xE0) ulen = 3;
            else if ((c & 0xF8) == 0xF0) ulen = 4;
            return (pos + ulen <= len) ? ulen : 1;
        }
        default:
            return -1;
    }
}

static int match_pat_seq(const MemArena *mem, const SymTable *tab,
                         uint16_t p_idx, uint16_t p_end,
                         const char *src, uint16_t pos, uint16_t len) {
    if (p_idx >= p_end) return (int)pos;
    const PatElem *pe = &tab->pat_elems[p_idx];

    if (pe->kind == PAT_GROUP_START) {
        uint16_t depth = 1;
        uint16_t g_end = p_idx + 1;
        while (g_end < p_end && depth > 0) {
            if (tab->pat_elems[g_end].kind == PAT_GROUP_START) depth++;
            else if (tab->pat_elems[g_end].kind == PAT_GROUP_END) depth--;
            if (depth > 0) g_end++;
        }
        uint8_t quant = (g_end < p_end) ? tab->pat_elems[g_end].quant : QUANT_ONE;
        uint16_t next_p = (g_end < p_end) ? (g_end + 1) : p_end;

        if (quant == QUANT_OPT) {
            int sub_end = match_pat_seq(mem, tab, p_idx + 1, g_end, src, pos, len);
            if (sub_end >= 0) {
                int rem = match_pat_seq(mem, tab, next_p, p_end, src, (uint16_t)sub_end, len);
                if (rem >= 0) return rem;
            }
            return match_pat_seq(mem, tab, next_p, p_end, src, pos, len);
        } else if (quant == QUANT_ONE) {
            int sub_end = match_pat_seq(mem, tab, p_idx + 1, g_end, src, pos, len);
            if (sub_end < 0) return -1;
            return match_pat_seq(mem, tab, next_p, p_end, src, (uint16_t)sub_end, len);
        } else {
            uint16_t ends[32];
            int count = 0;
            if (quant == QUANT_STAR) ends[count++] = pos;
            uint16_t cur = pos;
            while (count < 32) {
                int sub_end = match_pat_seq(mem, tab, p_idx + 1, g_end, src, cur, len);
                if (sub_end <= (int)cur) break;
                cur = (uint16_t)sub_end;
                ends[count++] = cur;
            }
            for (int i = count - 1; i >= 0; --i) {
                int rem = match_pat_seq(mem, tab, next_p, p_end, src, ends[i], len);
                if (rem >= 0) return rem;
            }
            return -1;
        }
    }

    uint8_t quant = pe->quant;
    uint16_t next_p = p_idx + 1;

    if (quant == QUANT_ONE) {
        int step = match_single_atom(mem, pe, src, pos, len);
        if (step < 0) return -1;
        return match_pat_seq(mem, tab, next_p, p_end, src, pos + (uint16_t)step, len);
    } else if (quant == QUANT_OPT) {
        int step = match_single_atom(mem, pe, src, pos, len);
        if (step > 0) {
            int rem = match_pat_seq(mem, tab, next_p, p_end, src, pos + (uint16_t)step, len);
            if (rem >= 0) return rem;
        }
        return match_pat_seq(mem, tab, next_p, p_end, src, pos, len);
    } else {
        uint16_t ends[128];
        int count = 0;
        if (quant == QUANT_STAR) ends[count++] = pos;
        uint16_t cur = pos;
        while (count < 128) {
            int step = match_single_atom(mem, pe, src, cur, len);
            if (step <= 0) break;
            cur += (uint16_t)step;
            ends[count++] = cur;
        }
        for (int i = count - 1; i >= 0; --i) {
            int rem = match_pat_seq(mem, tab, next_p, p_end, src, ends[i], len);
            if (rem >= 0) return rem;
        }
        return -1;
    }
}

static Value construct_consumed_token(MemArena *mem, const SymTable *tab,
                                      uint16_t type_id, const char *src, uint16_t off, uint16_t len) {
    const TypeDef *td = &tab->types[type_id];
    char tmp[64];
    uint16_t copy_len = len < 63 ? len : 63;
    memcpy(tmp, src + off, copy_len);
    tmp[copy_len] = '\0';

    switch (td->parser_intr) {
        case INTR_PARSE_F64: {
            double d = strtod(tmp, NULL);
            return val_make_f64(mem, type_id, d);
        }
        case INTR_PARSE_F64_TRIM1: {
            if (copy_len > 0) tmp[copy_len - 1] = '\0';
            double d = strtod(tmp, NULL);
            return val_make_f64(mem, type_id, d);
        }
        case INTR_PARSE_I64: {
            double d = (double)strtoll(tmp, NULL, 10);
            return val_make_f64(mem, type_id, d);
        }
        case INTR_PARSE_STR_SLICE: {
            uint16_t s_off = off;
            uint16_t s_len = len;
            if (s_len >= 2 && src[s_off] == '"' && src[s_off + s_len - 1] == '"') {
                s_off += 1;
                s_len -= 2;
            }
            StrSlice slice = mem_intern_str(mem, src + s_off, s_len);
            return val_make_str(mem, type_id, slice);
        }
        default:
            return val_void();
    }
}

static bool skip_ws_and_comments(Lexer *lex, bool *out_had_nl) {
    bool saw_nl = false;
    while (lex->pos < lex->len) {
        char c = lex->src[lex->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lex->pos++;
        } else if (c == '\n') {
            saw_nl = true;
            if (lex->stop_at_newline && lex->paren_depth == 0) {
                if (out_had_nl) *out_had_nl = saw_nl;
                return true;
            }
            lex->pos++;
        } else if (c == '#') {
            while (lex->pos < lex->len && lex->src[lex->pos] != '\n') lex->pos++;
        } else {
            break;
        }
    }
    if (out_had_nl) *out_had_nl = saw_nl;
    return false;
}

void lex_init(Lexer *lex, MemArena *mem, SymTable *tab,
              const char *src, uint16_t len, uint16_t start_pos, bool stop_at_newline) {
    memset(lex, 0, sizeof(*lex));
    lex->mem             = mem;
    lex->tab             = tab;
    lex->src             = src;
    lex->len             = len;
    lex->pos             = start_pos;
    lex->stop_at_newline = stop_at_newline;
}

void lex_next(Lexer *lex) {
    MemArena       *mem = lex->mem;
    const SymTable *tab = lex->tab;
    bool had_nl = false;

    if (skip_ws_and_comments(lex, &had_nl)) {
        memset(&lex->cur, 0, sizeof(lex->cur));
        lex->cur.off         = lex->pos;
        lex->cur.kind        = TOK_EOF;
        lex->cur.had_newline = had_nl;
        return;
    }
    memset(&lex->cur, 0, sizeof(lex->cur));
    lex->cur.off         = lex->pos;
    lex->cur.had_newline = had_nl;

    if (lex->pos >= lex->len) {
        lex->cur.kind = TOK_EOF;
        return;
    }

    char c = lex->src[lex->pos];

    /* 1. Private C intrinsic / host extern hook '@' (only valid inside extension bodies) */
    if (c == '@') {
        lex->cur.kind = TOK_AT;
        lex->cur.len  = 1;
        lex->pos++;
        return;
    }

    /* 2. Active Consuming Types with non-empty patterns (Longest Match wins) */
    int best_type_len = 0;
    uint16_t best_type_id = MEM_TYPE_VOID;

    for (uint16_t tid = 1; tid < tab->type_count; ++tid) {
        const TypeDef *td = &tab->types[tid];
        if (!td->is_consuming || td->field_count == 0 || !sym_mod_enabled(tab, td->mod_id)) continue;

        uint16_t p_start = td->start_idx;
        uint16_t p_end   = p_start + td->field_count;

        int match_end = match_pat_seq(mem, tab, p_start, p_end, lex->src, lex->pos, lex->len);
        if (match_end > (int)lex->pos) {
            int mlen = match_end - (int)lex->pos;
            if (mlen > best_type_len) {
                best_type_len = mlen;
                best_type_id  = tid;
            }
        }
    }

    if (best_type_len > 0) {
        lex->cur.kind = TOK_VAL;
        lex->cur.len  = (uint16_t)best_type_len;
        lex->cur.val  = construct_consumed_token(mem, tab, best_type_id, lex->src, lex->pos, (uint16_t)best_type_len);
        lex->pos += (uint16_t)best_type_len;
        return;
    }

    /* 3. Active Syntax Delimiters (`(`, `)`, `{`, `}`, `[`, `]`, `,`) & Active Operators */
    int best_sym_len = 0;
    uint16_t best_sym_id = 0;
    uint8_t best_syntax_tag = 0;

    for (uint16_t sid = 0; sid < tab->sym_count; ++sid) {
        uint8_t syn_tag = sym_get_active_syntax(tab, sid);
        bool    is_op   = sym_has_active_op(tab, sid);
        if (!syn_tag && !is_op) continue;

        StrSlice s = tab->syms[sid];
        if (s.len == 0 || lex->pos + s.len > lex->len) continue;
        const char *s_ptr = mem_str_ptr(mem, s);
        if (memcmp(lex->src + lex->pos, s_ptr, s.len) == 0) {
            if (is_ascii_word(s_ptr, s.len)) {
                if (ident_continues_at(lex->src, (uint16_t)(lex->pos + s.len), lex->len)) {
                    continue;
                }
            }
            if ((int)s.len > best_sym_len) {
                best_sym_len    = (int)s.len;
                best_sym_id     = sid;
                best_syntax_tag = syn_tag;
            }
        }
    }

    if (best_sym_len > 0) {
        lex->cur.len    = (uint16_t)best_sym_len;
        lex->cur.sym_id = best_sym_id;
        lex->pos += (uint16_t)best_sym_len;

        if (best_syntax_tag == INTR_LPAREN) {
            lex->paren_depth++;
            lex->cur.kind = TOK_LPAREN;
        } else if (best_syntax_tag == INTR_RPAREN) {
            if (lex->paren_depth > 0) lex->paren_depth--;
            lex->cur.kind = TOK_RPAREN;
        } else if (best_syntax_tag == INTR_LBRACE) {
            lex->paren_depth++;
            lex->cur.kind = TOK_LBRACE;
        } else if (best_syntax_tag == INTR_RBRACE) {
            if (lex->paren_depth > 0) lex->paren_depth--;
            lex->cur.kind = TOK_RBRACE;
        } else if (best_syntax_tag == INTR_LBRACK) {
            lex->paren_depth++;
            lex->cur.kind = TOK_LBRACK;
        } else if (best_syntax_tag == INTR_RBRACK) {
            if (lex->paren_depth > 0) lex->paren_depth--;
            lex->cur.kind = TOK_RBRACK;
        } else if (best_syntax_tag == INTR_COMMA) {
            lex->cur.kind = TOK_COMMA;
        } else {
            lex->cur.kind = TOK_OP;
        }
        return;
    }

    /* 4. Identifiers (trailing '_' is reserved for the postfix '_' operator) */
    if (lex_is_ident_start((unsigned char)c)) {
        uint16_t start = lex->pos;
        while (lex->pos < lex->len && ident_continues_at(lex->src, lex->pos, lex->len)) {
            if ((unsigned char)lex->src[lex->pos] >= 0x80 && lex->pos > start) {
                bool sym_starts_here = false;
                for (uint16_t sid = 0; sid < tab->sym_count; ++sid) {
                    if (!sym_has_active_op(tab, sid) && !sym_get_active_syntax(tab, sid)) continue;
                    StrSlice s = tab->syms[sid];
                    if (s.len > 0 && lex->pos + s.len <= lex->len &&
                        memcmp(lex->src + lex->pos, mem_str_ptr(mem, s), s.len) == 0) {
                        sym_starts_here = true;
                        break;
                    }
                }
                if (sym_starts_here) break;
            }
            lex->pos++;
        }
        lex->cur.kind = TOK_IDENT;
        lex->cur.len  = lex->pos - start;
        return;
    }

    mem_err(mem, "Unknown token or undefined entity near '%.*s'",
            (int)(lex->len - lex->pos < 12 ? lex->len - lex->pos : 12), lex->src + lex->pos);
    lex->cur.kind = TOK_EOF;
}