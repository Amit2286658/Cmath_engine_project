/*
 * math_engine.c — Top-Level Engine Wrapper Implementation
 *
 * Internally loads the precompiled bootstrap and standard `.math` modules
 * (`stdlib_precompiled.h`) in O(1) `memcpy` time inside `eng_init()` with
 * zero filesystem or directory dependencies.
 */

#include "math_engine.h"
#include "stdlib_precompiled.h"
#include <stdio.h>
#include <string.h>

void eng_init_blank(Engine *eng) {
    mem_init(&eng->mem);
    sym_init(&eng->sym);
}

bool eng_init(Engine *eng) {
    eng_init_blank(eng);
    return ext_load_mathc_mem(&eng->mem, &eng->sym, k_stdlib_mathc, sizeof(k_stdlib_mathc));
}

bool eng_register_extern(Engine *eng, const char *name,
                         EngExternFn fn, void *user_ctx) {
    if (!eng) return false;
    return sym_register_extern(&eng->sym, &eng->mem, name, fn, user_ctx);
}

bool eng_set_module(Engine *eng, const char *mod_name, bool enabled) {
    return sym_set_module(&eng->sym, &eng->mem, mod_name, enabled);
}

bool eng_is_module_active(const Engine *eng, const char *mod_name) {
    return sym_is_module_active(&eng->sym, &eng->mem, mod_name);
}

bool eng_eval(Engine *eng, const char *expr, Value *out_val) {
    mem_clear_err(&eng->mem);
    mem_rewind_scratch(&eng->mem);

    Parser P;
    parser_init(&P, &eng->mem, &eng->sym, expr, (uint16_t)strlen(expr), 0, false, false);

    lex_next(&P.lex);
    if (P.lex.cur.kind == TOK_EOF && !eng->mem.has_error) {
        mem_err(&eng->mem, "Empty expression");
        return false;
    }

    uint16_t root = parser_parse_expr(&P, 0);
    if (!eng->mem.has_error && P.lex.cur.kind != TOK_EOF) {
        mem_err(&eng->mem, "Unexpected trailing token '%.*s'",
                (int)P.lex.cur.len, P.lex.src + P.lex.cur.off);
    }
    if (eng->mem.has_error || root == MEM_NO_IDX) {
        return false;
    }

    uint16_t frame_base = 0;
    if (P.local_count > 0) {
        frame_base = mem_push_frame(&eng->mem, P.local_count);
        for (uint8_t i = 0; i < P.local_count; ++i) {
            mem_set_slot(&eng->mem, frame_base, i, val_make_f64(&eng->mem, eng->mem.fast_real_type, 0.0));
        }
    }

    Value res = eval_ast(&eng->mem, &eng->sym, root, frame_base, 0);
    if (P.local_count > 0) {
        mem_pop_frame(&eng->mem, frame_base);
    }
    if (eng->mem.has_error) return false;
    if (out_val) *out_val = res;
    return true;
}

uint16_t eng_val_type(const Engine *eng, Value v) {
    return val_get_type(&eng->mem, v);
}

const char *eng_type_name(const Engine *eng, uint16_t type_id, char *buf, size_t cap) {
    return sym_type_name(&eng->sym, &eng->mem, type_id, buf, cap);
}

const char *eng_last_error(const Engine *eng) {
    return eng->mem.err_msg;
}

static void format_struct_at(const Engine *eng, uint16_t type_id, uint16_t pool_idx,
                             char *buf, size_t cap);

static void format_vec_at(const Engine *eng, uint16_t hdr_idx, char *buf, size_t cap) {
    uint16_t elem_tid = (uint16_t)mem_read_pool(&eng->mem, hdr_idx + 0);
    uint16_t len      = (uint16_t)mem_read_pool(&eng->mem, hdr_idx + 1);
    uint16_t data_idx = (uint16_t)mem_read_pool(&eng->mem, hdr_idx + 3);
    uint16_t stride   = (elem_tid > 0 && elem_tid < eng->sym.type_count)
                        ? eng->sym.types[elem_tid].flat_size : 1;

    size_t used = (size_t)snprintf(buf, cap, "[");
    for (uint16_t i = 0; i < len && used < cap; ++i) {
        uint16_t elem_pidx = (uint16_t)(data_idx + i * stride);
        char elem_str[96];
        if (elem_tid == eng->mem.str_type_id && elem_tid != 0) {
            uint32_t packed = (uint32_t)mem_read_pool(&eng->mem, elem_pidx);
            StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
            snprintf(elem_str, sizeof(elem_str), "\"%.*s\"",
                     (int)s.len, mem_str_ptr(&eng->mem, s));
        } else if (elem_tid == eng->mem.vec_type_id && elem_tid != 0) {
            format_vec_at(eng, elem_pidx, elem_str, sizeof(elem_str));
        } else if (elem_tid > 0 && elem_tid < eng->sym.type_count &&
                   !eng->sym.types[elem_tid].is_consuming) {
            format_struct_at(eng, elem_tid, elem_pidx, elem_str, sizeof(elem_str));
        } else {
            snprintf(elem_str, sizeof(elem_str), "%.6g", mem_read_pool(&eng->mem, elem_pidx));
        }
        used += (size_t)snprintf(buf + used, cap - used, "%s%s",
                                 (i > 0 ? ", " : ""), elem_str);
    }
    if (used < cap) snprintf(buf + used, cap - used, "]");
}

static void format_struct_at(const Engine *eng, uint16_t type_id, uint16_t pool_idx,
                             char *buf, size_t cap) {
    if (type_id == eng->mem.vec_type_id && type_id != 0) {
        format_vec_at(eng, pool_idx, buf, cap);
        return;
    }

    const TypeDef *td = &eng->sym.types[type_id];
    char tname[32];
    eng_type_name(eng, type_id, tname, sizeof(tname));

    if (strcmp(tname, "complex") == 0 && td->flat_size == 2) {
        double re = mem_read_pool(&eng->mem, pool_idx);
        double im = mem_read_pool(&eng->mem, pool_idx + 1);
        if (im >= 0.0) {
            snprintf(buf, cap, "%.6g + %.6gi", re, im);
        } else {
            snprintf(buf, cap, "%.6g - %.6gi", re, -im);
        }
        return;
    }

    size_t used = (size_t)snprintf(buf, cap, "%s(", tname);
    for (uint8_t i = 0; i < td->field_count && used < cap; ++i) {
        const FieldDef *fd = &eng->sym.fields[td->start_idx + i];
        char f_str[64];
        if (!eng->sym.types[fd->type_id].is_consuming) {
            format_struct_at(eng, fd->type_id, pool_idx + fd->flat_offset, f_str, sizeof(f_str));
        } else {
            snprintf(f_str, sizeof(f_str), "%.6g", mem_read_pool(&eng->mem, pool_idx + fd->flat_offset));
        }
        used += (size_t)snprintf(buf + used, cap - used, "%s%.*s=%s",
                                 (i > 0 ? ", " : ""),
                                 (int)fd->name.len, mem_str_ptr(&eng->mem, fd->name),
                                 f_str);
    }
    if (used < cap) snprintf(buf + used, cap - used, ")");
}

void eng_format_value(const Engine *eng, Value val, char *buf, size_t buf_cap) {
    uint16_t tid = eng_val_type(eng, val);
    if (tid == 0 || tid >= eng->sym.type_count) {
        snprintf(buf, buf_cap, "<void>");
        return;
    }
    const TypeDef *td = &eng->sym.types[tid];
    if (td->is_consuming) {
        if (td->parser_intr == INTR_PARSE_STR_SLICE) {
            StrSlice s = val_get_str(&eng->mem, val);
            snprintf(buf, buf_cap, "\"%.*s\"", (int)s.len, mem_str_ptr(&eng->mem, s));
        } else if (td->parser_intr == INTR_PARSE_F64_TRIM1) {
            snprintf(buf, buf_cap, "%.6gi", val_get_f64(&eng->mem, val));
        } else {
            snprintf(buf, buf_cap, "%.10g", val_get_f64(&eng->mem, val));
        }
    } else {
        format_struct_at(eng, tid, val_get_pool_idx(val), buf, buf_cap);
    }
}