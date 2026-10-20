/*
 * snapshot.c — Order-Tagged Raw-Text Session Snapshots & Continuation
 *
 * Implements:
 *   - `eng_snapshot_format_canonical_val`: Lossless MEP expression serializer
 *     for scalars, strings, structs, vectors, matrices, and lambda closures.
 *   - `eng_snapshot_record_eval`: Order-tagged (`seq:1..N`), FNV-1a hash-chained
 *     (`prev`/`hash`) raw-text snapshot builder.
 *   - `eng_session_snapshot_count`, `eng_session_snapshot_rewind`,
 *     `eng_session_snapshot_next`, `eng_session_snapshot_at`: Iterative and
 *     indexed snapshot accessors.
 *   - `eng_session_restore`: Verifies full, contiguous, uncorrupted snapshot
 *     history (even if supplied out of order) and reconstructs an active
 *     in-memory session ready to continue evaluation.
 */

#include "snapshot.h"
#include "math_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void format_canonical_struct_at(const Engine *eng, uint16_t type_id, uint16_t pool_idx,
                                       uint8_t depth, char *buf, size_t cap);

static void format_canonical_vec_at(const Engine *eng, uint16_t hdr_idx, uint8_t depth,
                                    char *buf, size_t cap) {
    if (depth > 8 || cap == 0) {
        snprintf(buf, cap, "vec()");
        return;
    }
    uint16_t raw_hdr0 = (uint16_t)mem_read_pool(&eng->mem, hdr_idx + 0);
    uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
    uint16_t len      = (uint16_t)mem_read_pool(&eng->mem, hdr_idx + 1);
    uint16_t data_idx = (uint16_t)mem_read_pool(&eng->mem, hdr_idx + 3);
    uint16_t stride   = (elem_tid > 0 && elem_tid < eng->sym.type_count)
                        ? eng->sym.types[elem_tid].flat_size : 1;

    if (elem_tid == 0 || elem_tid >= eng->sym.type_count) {
        snprintf(buf, cap, "vec()");
        return;
    }

    char etname[32];
    eng_type_name(eng, elem_tid, etname, sizeof(etname));
    size_t used = (size_t)snprintf(buf, cap, "list<%s>(", etname);

    for (uint16_t i = 0; i < len && used < cap; ++i) {
        uint16_t elem_pidx = (uint16_t)(data_idx + i * stride);
        char elem_str[128];
        if (elem_tid == eng->mem.str_type_id && elem_tid != 0) {
            uint32_t packed = (uint32_t)mem_read_pool(&eng->mem, elem_pidx);
            StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
            snprintf(elem_str, sizeof(elem_str), "\"%.*s\"",
                     (int)s.len, mem_str_ptr(&eng->mem, s));
        } else if (elem_tid == eng->mem.vec_type_id && elem_tid != 0) {
            format_canonical_vec_at(eng, elem_pidx, (uint8_t)(depth + 1), elem_str, sizeof(elem_str));
        } else if (!eng->sym.types[elem_tid].is_consuming) {
            format_canonical_struct_at(eng, elem_tid, elem_pidx, (uint8_t)(depth + 1), elem_str, sizeof(elem_str));
        } else if (eng->sym.types[elem_tid].parser_intr == INTR_PARSE_F64_TRIM1) {
            double dv = mem_read_pool(&eng->mem, elem_pidx);
            if (dv == 0.0) dv = 0.0;
            snprintf(elem_str, sizeof(elem_str), "(%.17g)i", dv);
        } else {
            double dv = mem_read_pool(&eng->mem, elem_pidx);
            if (dv == 0.0) dv = 0.0;
            snprintf(elem_str, sizeof(elem_str), "%.17g", dv);
        }
        used += (size_t)snprintf(buf + used, cap - used, "%s%s",
                                 (i > 0 ? ", " : ""), elem_str);
    }
    if (used < cap) snprintf(buf + used, cap - used, ")");
}

static void format_canonical_struct_at(const Engine *eng, uint16_t type_id, uint16_t pool_idx,
                                       uint8_t depth, char *buf, size_t cap) {
    if (depth > 8 || cap == 0) {
        snprintf(buf, cap, "0");
        return;
    }
    if (type_id == eng->mem.vec_type_id && type_id != 0) {
        format_canonical_vec_at(eng, pool_idx, depth, buf, cap);
        return;
    }
    if (type_id == eng->mem.mat_type_id && type_id != 0) {
        eng_format_mat_at(eng, pool_idx, true, buf, cap);
        return;
    }
    if (type_id == eng->mem.fn_type_id && type_id != 0) {
        uint16_t body_ast = (uint16_t)mem_read_pool(&eng->mem, pool_idx + 0);
        for (uint8_t i = 0; i < eng->mem.fn_src_count; ++i) {
            if (eng->mem.fn_body_ast[i] == body_ast) {
                StrSlice s = eng->mem.fn_src_slice[i];
                snprintf(buf, cap, "%.*s", (int)s.len, mem_str_ptr(&eng->mem, s));
                return;
            }
        }
        snprintf(buf, cap, "() => 0");
        return;
    }

    const TypeDef *td = &eng->sym.types[type_id];
    char tname[32];
    eng_type_name(eng, type_id, tname, sizeof(tname));

    size_t used = (size_t)snprintf(buf, cap, "%s(", tname);
    for (uint8_t i = 0; i < td->field_count && used < cap; ++i) {
        const FieldDef *fd = &eng->sym.fields[td->start_idx + i];
        char f_str[96];
        if (!eng->sym.types[fd->type_id].is_consuming) {
            format_canonical_struct_at(eng, fd->type_id, (uint16_t)(pool_idx + fd->flat_offset),
                                       (uint8_t)(depth + 1), f_str, sizeof(f_str));
        } else {
            double dv = mem_read_pool(&eng->mem, (uint16_t)(pool_idx + fd->flat_offset));
            if (dv == 0.0) dv = 0.0;
            snprintf(f_str, sizeof(f_str), "%.17g", dv);
        }
        used += (size_t)snprintf(buf + used, cap - used, "%s%s",
                                  (i > 0 ? ", " : ""), f_str);
    }
    if (used < cap) snprintf(buf + used, cap - used, ")");
}

void eng_snapshot_format_canonical_val(const Engine *eng, Value val, char *buf, size_t cap) {
    if (!eng || !buf || cap == 0) return;
    uint16_t tid = eng_val_type(eng, val);
    if (tid == 0 || tid >= eng->sym.type_count) {
        snprintf(buf, cap, "0");
        return;
    }
    const TypeDef *td = &eng->sym.types[tid];
    if (td->is_consuming) {
        if (td->parser_intr == INTR_PARSE_STR_SLICE) {
            StrSlice s = val_get_str(&eng->mem, val);
            snprintf(buf, cap, "\"%.*s\"", (int)s.len, mem_str_ptr(&eng->mem, s));
        } else if (td->parser_intr == INTR_PARSE_F64_TRIM1) {
            double dv = val_get_f64(&eng->mem, val);
            if (dv == 0.0) dv = 0.0;
            snprintf(buf, cap, "(%.17g)i", dv);
        } else {
            double dv = val_get_f64(&eng->mem, val);
            if (dv == 0.0) dv = 0.0;
            snprintf(buf, cap, "%.17g", dv);
        }
    } else {
        format_canonical_struct_at(eng, tid, val_get_pool_idx(val), 0, buf, cap);
    }
}

static void escape_snap_field(const char *in, char *out, size_t out_cap) {
    size_t w = 0;
    for (size_t i = 0; in[i] != '\0' && w + 2 < out_cap; ++i) {
        char c = in[i];
        if (c == '\\')      { out[w++] = '\\'; out[w++] = '\\'; }
        else if (c == '|')  { out[w++] = '\\'; out[w++] = 'p';  }
        else if (c == ';')  { out[w++] = '\\'; out[w++] = 's';  }
        else if (c == '\n') { out[w++] = '\\'; out[w++] = 'n';  }
        else if (c == '\r') { out[w++] = '\\'; out[w++] = 'r';  }
        else                { out[w++] = c; }
    }
    out[w] = '\0';
}

static void unescape_snap_field(const char *in, size_t in_len, char *out, size_t out_cap) {
    size_t w = 0;
    for (size_t i = 0; i < in_len && w + 1 < out_cap; ++i) {
        if (in[i] == '\\' && i + 1 < in_len) {
            char nxt = in[++i];
            if (nxt == 'p')       out[w++] = '|';
            else if (nxt == 's')  out[w++] = ';';
            else if (nxt == 'n')  out[w++] = '\n';
            else if (nxt == 'r')  out[w++] = '\r';
            else                  out[w++] = nxt;
        } else {
            out[w++] = in[i];
        }
    }
    out[w] = '\0';
}

static uint32_t calc_snapshot_hash(uint16_t seq, uint32_t prev_hash, const char *payload) {
    uint32_t h = 2166136261u;
    h ^= (uint32_t)(seq & 0xFFu);        h *= 16777619u;
    h ^= (uint32_t)((seq >> 8) & 0xFFu); h *= 16777619u;
    for (int b = 0; b < 4; ++b) {
        h ^= (uint32_t)((prev_hash >> (b * 8)) & 0xFFu);
        h *= 16777619u;
    }
    if (payload) {
        for (size_t i = 0; payload[i] != '\0'; ++i) {
            h ^= (uint8_t)payload[i];
            h *= 16777619u;
        }
    }
    return (h == 0) ? 1u : h;
}

void eng_snapshot_record_eval(const Engine *eng, EngSession *S, Value res) {
    if (!eng || !S || S->eval_count >= ENG_SESSION_MAX_EVALS) return;

    char payload[ENG_SNAPSHOT_MAX_LEN - 48];
    size_t p_used = (size_t)snprintf(payload, sizeof(payload), "vars:%u|", (unsigned)S->var_count);
    for (uint8_t i = 0; i < S->var_count && p_used < sizeof(payload); ++i) {
        char esc_val[256];
        escape_snap_field(S->vars[i].val_text, esc_val, sizeof(esc_val));
        p_used += (size_t)snprintf(payload + p_used, sizeof(payload) - p_used,
                                   "%s%s:%s:%s:%s:%s=%s",
                                   (i > 0 ? ";" : ""),
                                   S->vars[i].name,
                                   S->vars[i].type_name,
                                   S->vars[i].unit_name,
                                   S->vars[i].lock_type_name,
                                   S->vars[i].lock_unit_name,
                                   esc_val);
    }

    char res_type[32], res_unit[32], res_raw[128], res_esc[160];
    eng_type_name(eng, val_get_type(&eng->mem, res), res_type, sizeof(res_type));
    eng_unit_name(eng, val_get_unit(&eng->mem, res), res_unit, sizeof(res_unit));
    eng_format_value(eng, res, res_raw, sizeof(res_raw));
    escape_snap_field(res_raw, res_esc, sizeof(res_esc));
    if (p_used < sizeof(payload)) {
        snprintf(payload + p_used, sizeof(payload) - p_used,
                 "|res:%s:%s=%s", res_type, res_unit, res_esc);
    }

    uint16_t seq       = (uint16_t)(S->eval_count + 1);
    uint32_t prev_hash = S->chain_hash;
    uint32_t cur_hash  = calc_snapshot_hash(seq, prev_hash, payload);

    snprintf(S->snapshots[S->eval_count], ENG_SNAPSHOT_MAX_LEN,
             "MSP1|seq:%u|prev:%08X|hash:%08X|%s",
             (unsigned)seq, (unsigned)prev_hash, (unsigned)cur_hash, payload);

    S->eval_count       = seq;
    S->chain_hash       = cur_hash;
    S->snap_iter_cursor = 0;
}

uint16_t eng_session_snapshot_count(const Engine *eng, int session_id) {
    if (!eng || session_id < 0 || session_id >= ENG_MAX_SESSIONS || !eng->sessions[session_id].in_use) {
        return 0;
    }
    return eng->sessions[session_id].eval_count;
}

bool eng_session_snapshot_rewind(Engine *eng, int session_id) {
    if (!eng) return false;
    mem_clear_err(&eng->mem);
    if (session_id < 0 || session_id >= ENG_MAX_SESSIONS || !eng->sessions[session_id].in_use) {
        mem_err(&eng->mem, "Invalid or inactive session ID (%d)", session_id);
        return false;
    }
    eng->sessions[session_id].snap_iter_cursor = 0;
    return true;
}

const char *eng_session_snapshot_next(Engine *eng, int session_id) {
    if (!eng || session_id < 0 || session_id >= ENG_MAX_SESSIONS || !eng->sessions[session_id].in_use) {
        return NULL;
    }
    EngSession *S = &eng->sessions[session_id];
    if (S->snap_iter_cursor >= S->eval_count) {
        S->snap_iter_cursor = 0;
        return NULL;
    }
    return S->snapshots[S->snap_iter_cursor++];
}

const char *eng_session_snapshot_at(const Engine *eng, int session_id, uint16_t idx) {
    if (!eng || session_id < 0 || session_id >= ENG_MAX_SESSIONS || !eng->sessions[session_id].in_use) {
        return NULL;
    }
    const EngSession *S = &eng->sessions[session_id];
    if (idx >= S->eval_count) return NULL;
    return S->snapshots[idx];
}

static bool parse_snapshot_header_and_verify(MemArena *mem, const char *snap,
                                             uint16_t *out_seq, uint32_t *out_prev,
                                             uint32_t *out_hash, const char **out_payload) {
    if (!snap || strncmp(snap, "MSP1|seq:", 9) != 0) {
        mem_err(mem, "Invalid snapshot header (expected 'MSP1|seq:...')");
        return false;
    }
    const char *p = snap + 9;
    char *endp = NULL;
    unsigned long seq_ul = strtoul(p, &endp, 10);
    if (endp == p || seq_ul == 0 || seq_ul > 65534ul || strncmp(endp, "|prev:", 6) != 0) {
        mem_err(mem, "Malformed 'seq' field in session snapshot");
        return false;
    }
    p = endp + 6;
    if (strlen(p) < 8) {
        mem_err(mem, "Truncated 'prev' hash in session snapshot");
        return false;
    }
    char hex_buf[9];
    memcpy(hex_buf, p, 8);
    hex_buf[8] = '\0';
    uint32_t prev_h = (uint32_t)strtoul(hex_buf, &endp, 16);
    if (endp != hex_buf + 8 || strncmp(p + 8, "|hash:", 6) != 0) {
        mem_err(mem, "Malformed 'prev' hash in session snapshot");
        return false;
    }
    p += 14;
    if (strlen(p) < 9 || p[8] != '|') {
        mem_err(mem, "Malformed 'hash' field in session snapshot");
        return false;
    }
    memcpy(hex_buf, p, 8);
    hex_buf[8] = '\0';
    uint32_t cur_h = (uint32_t)strtoul(hex_buf, &endp, 16);
    if (endp != hex_buf + 8) {
        mem_err(mem, "Invalid hex digit in snapshot hash");
        return false;
    }
    const char *payload = p + 9;
    uint32_t expected_h = calc_snapshot_hash((uint16_t)seq_ul, prev_h, payload);
    if (cur_h != expected_h) {
        mem_err(mem, "Corrupted session snapshot checksum mismatch at seq:%lu", seq_ul);
        return false;
    }
    *out_seq     = (uint16_t)seq_ul;
    *out_prev    = prev_h;
    *out_hash    = cur_h;
    *out_payload = payload;
    return true;
}

static bool parse_snapshot_vars(MemArena *mem, const char *payload,
                                EngSessionVar *out_vars, uint8_t *out_var_count) {
    *out_var_count = 0;
    if (strncmp(payload, "vars:", 5) != 0) {
        mem_err(mem, "Malformed snapshot payload (missing 'vars:')");
        return false;
    }
    const char *p = payload + 5;
    char *endp = NULL;
    unsigned long vcnt_ul = strtoul(p, &endp, 10);
    if (endp == p || vcnt_ul > ENG_SESSION_MAX_VARS || *endp != '|') {
        mem_err(mem, "Invalid variable count in snapshot payload");
        return false;
    }
    p = endp + 1;

    /* Find the unescaped `|res:` boundary */
    const char *vars_end = NULL;
    for (const char *scan = p; *scan != '\0'; ++scan) {
        if (*scan == '\\' && scan[1] != '\0') {
            scan++;
            continue;
        }
        if (*scan == '|' && strncmp(scan, "|res:", 5) == 0) {
            vars_end = scan;
            break;
        }
    }
    if (!vars_end) {
        mem_err(mem, "Malformed snapshot payload (missing '|res:' section)");
        return false;
    }

    if (vcnt_ul == 0) {
        if (p != vars_end) {
            mem_err(mem, "Snapshot variable count is 0 but variable entries are present");
            return false;
        }
        return true;
    }

    uint8_t parsed_cnt = 0;
    const char *cur = p;
    while (cur < vars_end && parsed_cnt < (uint8_t)vcnt_ul) {
        /* Find end of this variable entry (next unescaped `;` or `vars_end`) */
        const char *entry_end = vars_end;
        for (const char *scan = cur; scan < vars_end; ++scan) {
            if (*scan == '\\' && scan + 1 < vars_end) {
                scan++;
                continue;
            }
            if (*scan == ';') {
                entry_end = scan;
                break;
            }
        }

        /* Parse 5 colon-separated fields before '=': name:type:unit:lock_type:lock_unit=val */
        const char *fields[5];
        size_t      flens[5];
        const char *fcur = cur;
        for (int f = 0; f < 5; ++f) {
            char sep = (f < 4) ? ':' : '=';
            const char *delim = (const char *)memchr(fcur, sep, (size_t)(entry_end - fcur));
            if (!delim || delim == fcur || (size_t)(delim - fcur) >= 32u) {
                mem_err(mem, "Malformed variable descriptor in session snapshot");
                return false;
            }
            fields[f] = fcur;
            flens[f]  = (size_t)(delim - fcur);
            fcur      = delim + 1;
        }

        EngSessionVar *sv = &out_vars[parsed_cnt++];
        memset(sv, 0, sizeof(*sv));
        memcpy(sv->name,           fields[0], flens[0]); sv->name[flens[0]]           = '\0';
        memcpy(sv->type_name,      fields[1], flens[1]); sv->type_name[flens[1]]      = '\0';
        memcpy(sv->unit_name,      fields[2], flens[2]); sv->unit_name[flens[2]]      = '\0';
        memcpy(sv->lock_type_name, fields[3], flens[3]); sv->lock_type_name[flens[3]] = '\0';
        memcpy(sv->lock_unit_name, fields[4], flens[4]); sv->lock_unit_name[flens[4]] = '\0';

        unescape_snap_field(fcur, (size_t)(entry_end - fcur), sv->val_text, sizeof(sv->val_text));
        if (sv->val_text[0] == '\0') {
            mem_err(mem, "Empty variable value expression in session snapshot");
            return false;
        }

        cur = (entry_end < vars_end) ? (entry_end + 1) : vars_end;
    }

    if (parsed_cnt != (uint8_t)vcnt_ul || cur != vars_end) {
        mem_err(mem, "Snapshot variable count mismatch");
        return false;
    }
    *out_var_count = parsed_cnt;
    return true;
}

int eng_session_restore(Engine *eng, const char *const *snapshots, size_t snapshot_count) {
    if (!eng) return -1;
    mem_clear_err(&eng->mem);

    if (!snapshots || snapshot_count == 0 || snapshot_count > ENG_SESSION_MAX_EVALS) {
        mem_err(&eng->mem, "Invalid snapshot count (%zu) for session restore", snapshot_count);
        return -1;
    }

    int         ordered_idx[ENG_SESSION_MAX_EVALS];
    uint32_t    ordered_prev[ENG_SESSION_MAX_EVALS];
    uint32_t    ordered_hash[ENG_SESSION_MAX_EVALS];
    const char *ordered_payload[ENG_SESSION_MAX_EVALS];

    for (size_t i = 0; i < ENG_SESSION_MAX_EVALS; ++i) {
        ordered_idx[i]     = -1;
        ordered_prev[i]    = 0;
        ordered_hash[i]    = 0;
        ordered_payload[i] = NULL;
    }

    /* 1. Parse each snapshot, verify its individual hash, and place it by its order tag (`seq`) */
    for (size_t i = 0; i < snapshot_count; ++i) {
        if (!snapshots[i] || strlen(snapshots[i]) >= ENG_SNAPSHOT_MAX_LEN) {
            mem_err(&eng->mem, "Null or oversized snapshot at index %zu", i);
            return -1;
        }
        uint16_t    seq     = 0;
        uint32_t    prev_h  = 0;
        uint32_t    cur_h   = 0;
        const char *payload = NULL;
        if (!parse_snapshot_header_and_verify(&eng->mem, snapshots[i],
                                              &seq, &prev_h, &cur_h, &payload)) {
            return -1;
        }
        if (seq < 1 || seq > snapshot_count) {
            mem_err(&eng->mem,
                    "Incomplete session snapshots: order tag seq:%u out of contiguous range 1..%zu",
                    (unsigned)seq, snapshot_count);
            return -1;
        }
        if (ordered_idx[seq - 1] >= 0) {
            mem_err(&eng->mem, "Duplicate evaluation order tag seq:%u in snapshots", (unsigned)seq);
            return -1;
        }
        ordered_idx[seq - 1]     = (int)i;
        ordered_prev[seq - 1]    = prev_h;
        ordered_hash[seq - 1]    = cur_h;
        ordered_payload[seq - 1] = payload;
    }

    /* 2. Verify full contiguous chain (seq:1 .. snapshot_count) and hash linkage */
    uint32_t expected_prev = 0;
    for (size_t k = 0; k < snapshot_count; ++k) {
        if (ordered_idx[k] < 0) {
            mem_err(&eng->mem, "Incomplete session snapshots: missing evaluation seq:%zu", k + 1);
            return -1;
        }
        if (ordered_prev[k] != expected_prev) {
            mem_err(&eng->mem,
                    "Broken session snapshot chain at seq:%zu (prev hash %08X != %08X)",
                    k + 1, (unsigned)ordered_prev[k], (unsigned)expected_prev);
            return -1;
        }
        expected_prev = ordered_hash[k];
    }

    /* 3. Parse and validate all variables from each snapshot in order */
    EngSessionVar final_vars[ENG_SESSION_MAX_VARS];
    uint8_t       final_var_cnt = 0;
    memset(final_vars, 0, sizeof(final_vars));

    for (size_t k = 0; k < snapshot_count; ++k) {
        if (!parse_snapshot_vars(&eng->mem, ordered_payload[k], final_vars, &final_var_cnt)) {
            return -1;
        }
    }

    /* 4. Allocate a new in-memory session and populate its full history and variable state */
    int sid = eng_session_open(eng);
    if (sid < 0) return -1;

    EngSession *S = &eng->sessions[sid];
    S->eval_count       = (uint16_t)snapshot_count;
    S->snap_iter_cursor = 0;
    S->chain_hash       = expected_prev;
    S->var_count        = final_var_cnt;
    memcpy(S->vars, final_vars, sizeof(final_vars));

    for (size_t k = 0; k < snapshot_count; ++k) {
        snprintf(S->snapshots[k], ENG_SNAPSHOT_MAX_LEN, "%s", snapshots[ordered_idx[k]]);
    }

    return sid;
}
