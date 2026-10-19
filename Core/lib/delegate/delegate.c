/*
 * delegate.c — Internal C `@` Intrinsic Delegate Registry & Execution Engine
 *
 * Location: Core/lib/delegate/delegate.c
 *
 * Contains the `@<intrinsic>` lookup table and all C implementations of
 * operations dispatched via `@` in extension modules.
 */

#include "delegate.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

/* ============================================================================
 * 1. `@` INTRINSIC NAME -> OPCODE REGISTRY
 * ============================================================================ */

typedef struct {
    const char *name;
    IntrinsicId id;
} IntrEntry;

static const IntrEntry k_delegate_intrinsics[] = {
    { "f64",          INTR_PARSE_F64 },
    { "f64_trim1",    INTR_PARSE_F64_TRIM1 },
    { "i64",          INTR_PARSE_I64 },
    { "str_slice",    INTR_PARSE_STR_SLICE },
    { "vec_type",     INTR_VEC_TYPE },
    { "mat_type",     INTR_MAT_TYPE },
    { "lparen",       INTR_LPAREN },
    { "rparen",       INTR_RPAREN },
    { "lbrace",       INTR_LBRACE },
    { "rbrace",       INTR_RBRACE },
    { "lbrack",       INTR_LBRACK },
    { "rbrack",       INTR_RBRACK },
    { "comma",        INTR_COMMA },
    { "seq",          INTR_SEQ },
    { "store",        INTR_STORE },
    { "while",        INTR_WHILE },
    { "if",           INTR_IF },
    { "or_branch",    INTR_OR_BRANCH },
    { "then_branch",  INTR_OR_BRANCH },
    { "else_branch",  INTR_ELSE_BRANCH },
    { "switch_block", INTR_SWITCH_BLOCK },
    { "case_branch",  INTR_CASE_BRANCH },
    { "ternary",      INTR_TERNARY },
    { "colon",        INTR_COLON },
    { "jump_tag",     INTR_JUMP_TAG },
    { "run_tag",      INTR_RUN_TAG },
    { "break_loop",   INTR_BREAK_LOOP },
    { "loop_kw",      INTR_LOOP_KW },
    { "cand",         INTR_CAND },
    { "cor",          INTR_COR },
    { "cnot",         INTR_CNOT },
    { "calc_diff",    INTR_CALC_DIFF },
    { "calc_diff2",   INTR_CALC_DIFF2 },
    { "calc_diffn",   INTR_CALC_DIFFN },
    { "calc_integ",   INTR_CALC_INTEG },
    { "calc_lim",     INTR_CALC_LIM },
    { "calc_lim_l",   INTR_CALC_LIM_L },
    { "calc_lim_r",   INTR_CALC_LIM_R },
    { "calc_sigma",   INTR_CALC_SIGMA },
    { "calc_prod",    INTR_CALC_PROD },
    { "calc_solve",   INTR_CALC_SOLVE },
    { "calc_fmin",    INTR_CALC_FMIN },
    { "calc_fmax",    INTR_CALC_FMAX },
    { "field",        INTR_FIELD },
    { "vnew",         INTR_VNEW },
    { "vfill",        INTR_VFILL },
    { "vpush",        INTR_VPUSH },
    { "vpop",         INTR_VPOP },
    { "vget",         INTR_VGET },
    { "vset",         INTR_VSET },
    { "vlen",         INTR_VLEN },
    { "vcap",         INTR_VCAP },
    { "vresize",      INTR_VRESIZE },
    { "vshrink",      INTR_VSHRINK },
    { "vclear",       INTR_VCLEAR },
    { "vrange",       INTR_VRANGE },
    { "vin",          INTR_VIN },
    { "mzero",        INTR_MZERO },
    { "mones",        INTR_MONES },
    { "meye",         INTR_MEYE },
    { "mfill",        INTR_MFILL },
    { "mrows",        INTR_MROWS },
    { "mcols",        INTR_MCOLS },
    { "mrank",        INTR_MRANK },
    { "mlen",         INTR_MLEN },
    { "mshape",       INTR_MSHAPE },
    { "madd",         INTR_MADD },
    { "msub",         INTR_MSUB },
    { "mscal",        INTR_MSCAL },
    { "mmul",         INTR_MMUL },
    { "mtrans",       INTR_MTRANS },
    { "mtrace",       INTR_MTRACE },
    { "mdet",         INTR_MDET },
    { "minv",         INTR_MINV },
    { "add",          INTR_ADD },
    { "sub",          INTR_SUB },
    { "mul",          INTR_MUL },
    { "div",          INTR_DIV },
    { "mod",          INTR_MOD },
    { "neg",          INTR_NEG },
    { "abs",          INTR_ABS },
    { "floor",        INTR_FLOOR },
    { "ceil",         INTR_CEIL },
    { "pow",          INTR_POW },
    { "sqrt",         INTR_SQRT },
    { "cbrt",         INTR_CBRT },
    { "exp",          INTR_EXP },
    { "ln",           INTR_LN },
    { "log10",        INTR_LOG10 },
    { "sin",          INTR_SIN },
    { "cos",          INTR_COS },
    { "tan",          INTR_TAN },
    { "asin",         INTR_ASIN },
    { "acos",         INTR_ACOS },
    { "atan",         INTR_ATAN },
    { "atan2",        INTR_ATAN2 },
    { "sinh",         INTR_SINH },
    { "cosh",         INTR_COSH },
    { "tanh",         INTR_TANH },
    { "eq",           INTR_EQ },
    { "ne",           INTR_NE },
    { "lt",           INTR_LT },
    { "le",           INTR_LE },
    { "gt",           INTR_GT },
    { "ge",           INTR_GE },
    { "approx",       INTR_APPROX },
    { "land",         INTR_LAND },
    { "lor",          INTR_LOR },
    { "lnot",         INTR_LNOT },
    { "lxor",         INTR_LXOR },
    { "lnor",         INTR_LNOR },
    { "lnand",        INTR_LNAND },
    { "raw",          INTR_RAW },
    { "type_name",    INTR_TYPE_NAME },
    { "unit_name",    INTR_UNIT_NAME },
    { "err",          INTR_ERR },
    { NULL,           INTR_NONE }
};

IntrinsicId delegate_lookup_intrinsic(const char *s, uint16_t len) {
    for (int i = 0; k_delegate_intrinsics[i].name != NULL; ++i) {
        if (strlen(k_delegate_intrinsics[i].name) == len &&
            memcmp(k_delegate_intrinsics[i].name, s, len) == 0) {
            return k_delegate_intrinsics[i].id;
        }
    }
    return INTR_NONE;
}

/* ============================================================================
 * 2. UNIT & INTEGER VALIDATION HELPERS FOR DELEGATES
 * ============================================================================ */

bool delegate_is_zero_unitless_scalar(const MemArena *mem, const SymTable *tab, Value v) {
    if (val_get_unit(mem, v) != MEM_UNIT_NONE) return false;
    uint16_t tid = val_get_type(mem, v);
    if (tid == 0 || tid >= tab->type_count || !tab->types[tid].is_consuming) return false;
    if (tid == mem->str_type_id) return false;
    return fabs(val_get_f64(mem, v)) <= 1e-15;
}

bool delegate_check_additive_unit_compat(MemArena *mem, const SymTable *tab,
                                         const char *op_name, int op_len,
                                         Value a, Value b, uint8_t *out_unit) {
    uint8_t u1 = val_get_unit(mem, a);
    uint8_t u2 = val_get_unit(mem, b);
    if (u1 == u2) {
        if (out_unit) *out_unit = u1;
        return true;
    }
    if (delegate_is_zero_unitless_scalar(mem, tab, a)) {
        if (out_unit) *out_unit = u2;
        return true;
    }
    if (delegate_is_zero_unitless_scalar(mem, tab, b)) {
        if (out_unit) *out_unit = u1;
        return true;
    }
    char u1_buf[32], u2_buf[32];
    sym_unit_name(tab, mem, u1, u1_buf, sizeof(u1_buf));
    sym_unit_name(tab, mem, u2, u2_buf, sizeof(u2_buf));
    mem_err(mem, "Unit mismatch in '%.*s': cannot combine '%s' and '%s'",
            op_len, op_name, u1_buf, u2_buf);
    return false;
}

bool delegate_val_to_exact_int(MemArena *mem, const SymTable *tab, Value v,
                               const char *ctx, int *out_int) {
    uint16_t tid = val_get_type(mem, v);
    if (tid == 0 || tid >= tab->type_count || !tab->types[tid].is_consuming ||
        tid == mem->str_type_id) {
        mem_err(mem, "Expected integer scalar for %s", ctx);
        return false;
    }
    double d = val_get_f64(mem, v);
    if (!isfinite(d) || d < -2147483648.0 || d > 2147483647.0) {
        mem_err(mem, "Out-of-range or non-finite %s (%g)", ctx, d);
        return false;
    }
    double r = floor(d + 0.5);
    if (fabs(d - r) > 1e-9) {
        mem_err(mem, "Non-integer %s (%g)", ctx, d);
        return false;
    }
    *out_int = (int)r;
    return true;
}

/* ============================================================================
 * 3. DYNAMIC VECTOR (`$vec`) DELEGATE INTRINSICS (`@vnew` .. `@vin`)
 * ============================================================================ */

void delegate_vec_write_elem(MemArena *mem, const SymTable *tab,
                             uint16_t elem_tid, uint16_t dst_pidx, Value item) {
    if (elem_tid == mem->str_type_id && elem_tid != 0) {
        StrSlice s = val_get_str(mem, item);
        uint32_t packed = ((uint32_t)s.off << 16) | (uint32_t)s.len;
        mem_write_pool(mem, dst_pidx, (double)packed);
    } else if (elem_tid < tab->type_count && !tab->types[elem_tid].is_consuming) {
        uint16_t stride   = tab->types[elem_tid].flat_size;
        uint16_t src_pidx = val_get_pool_idx(item);
        for (uint16_t k = 0; k < stride; ++k) {
            mem_write_pool(mem, dst_pidx + k, mem_read_pool(mem, src_pidx + k));
        }
    } else {
        mem_write_pool(mem, dst_pidx, val_get_f64(mem, item));
    }
}

Value delegate_vec_read_elem(MemArena *mem, const SymTable *tab,
                             uint16_t elem_tid, uint8_t elem_uid, uint16_t src_pidx) {
    if (elem_tid == mem->str_type_id && elem_tid != 0) {
        uint32_t packed = (uint32_t)mem_read_pool(mem, src_pidx);
        StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
        return val_make_str(mem, elem_tid, s);
    } else if (elem_tid < tab->type_count && !tab->types[elem_tid].is_consuming) {
        uint16_t stride = tab->types[elem_tid].flat_size;
        return val_with_unit(mem, val_make_comp(mem, elem_tid, src_pidx, stride), elem_uid);
    } else {
        return val_make_f64_unit(mem, elem_tid ? elem_tid : mem->fast_real_type,
                                 elem_uid, mem_read_pool(mem, src_pidx));
    }
}

Value delegate_eval_vec_intrinsic(MemArena *mem, const SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c) {
    if (mem->vec_type_id == 0) {
        mem_err(mem, "Dynamic vector type ($vec) is not loaded");
        return val_void();
    }

    if (intr == INTR_VNEW) {
        int cap_in = 0;
        if (val_get_type(mem, a) != MEM_TYPE_VOID) {
            if (!delegate_val_to_exact_int(mem, tab, a, "vector capacity", &cap_in)) return val_void();
        }
        if (cap_in < 0 || cap_in > MEM_MAX_F64_POOL) {
            mem_err(mem, "Vector capacity out of range (%d)", cap_in);
            return val_void();
        }
        uint16_t hdr_idx = mem_vec_alloc_hdr(mem, 0, (uint16_t)cap_in);
        if (mem->has_error) return val_void();
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    if (intr == INTR_VFILL) {
        int count = 0;
        if (!delegate_val_to_exact_int(mem, tab, a, "vector fill count", &count)) return val_void();
        if (count < 0 || count > MEM_MAX_F64_POOL) {
            mem_err(mem, "Vector fill count out of range (%d)", count);
            return val_void();
        }
        uint16_t elem_tid = val_get_type(mem, b);
        uint8_t  elem_uid = val_get_unit(mem, b);
        uint16_t packed_t = (uint16_t)(((uint16_t)elem_uid << 8) | (elem_tid & 0xFFu));
        uint16_t stride   = (elem_tid < tab->type_count) ? tab->types[elem_tid].flat_size : 1;
        uint16_t hdr_idx  = mem_vec_alloc_hdr(mem, packed_t, (uint16_t)count);
        if (mem->has_error) return val_void();
        if (count > 0) {
            if (!mem_vec_reserve(mem, hdr_idx, stride, (uint16_t)count)) return val_void();
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            for (uint16_t i = 0; i < (uint16_t)count; ++i) {
                delegate_vec_write_elem(mem, tab, elem_tid, (uint16_t)(data_idx + i * stride), b);
            }
            mem_write_pool(mem, hdr_idx + 1, (double)count);
        }
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    if (intr == INTR_VRANGE) {
        int lo = 0, hi = 0;
        if (!delegate_val_to_exact_int(mem, tab, a, "range start", &lo) ||
            !delegate_val_to_exact_int(mem, tab, b, "range end",   &hi)) {
            return val_void();
        }
        uint8_t elem_uid = MEM_UNIT_NONE;
        if (!delegate_check_additive_unit_compat(mem, tab, "..", 2, a, b, &elem_uid)) {
            return val_void();
        }
        int step = (lo <= hi) ? 1 : -1;
        long long cnt_ll = (lo <= hi)
                           ? ((long long)hi - (long long)lo + 1LL)
                           : ((long long)lo - (long long)hi + 1LL);
        if (cnt_ll > (long long)MEM_MAX_F64_POOL) {
            mem_err(mem, "Range length (%lld) exceeds pool capacity", cnt_ll);
            return val_void();
        }
        uint16_t count    = (uint16_t)cnt_ll;
        uint16_t elem_tid = mem->fast_real_type;
        uint16_t packed_t = (uint16_t)(((uint16_t)elem_uid << 8) | (elem_tid & 0xFFu));
        uint16_t hdr_idx  = mem_vec_alloc_hdr(mem, packed_t, count);
        if (mem->has_error) return val_void();
        if (!mem_vec_reserve(mem, hdr_idx, 1, count)) return val_void();
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        for (uint16_t i = 0; i < count; ++i) {
            mem_write_pool(mem, data_idx + i, (double)(lo + (int)i * step));
        }
        mem_write_pool(mem, hdr_idx + 1, (double)count);
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    if (intr == INTR_VIN) {
        if (val_get_type(mem, b) != mem->vec_type_id) {
            mem_err(mem, "Right operand of 'in' must be a vector ($vec) or matching unit");
            return val_void();
        }
        uint16_t hdr_idx  = val_get_pool_idx(b);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
        uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
        uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
        uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                            ? tab->types[elem_tid].flat_size : 1;

        uint16_t item_tid = val_get_type(mem, a);
        uint8_t  item_uid = val_get_unit(mem, a);
        if (len == 0 || elem_tid != item_tid || elem_uid != item_uid) {
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        for (uint16_t i = 0; i < len; ++i) {
            Value ev = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, (uint16_t)(data_idx + i * stride));
            bool match = false;
            if (elem_tid == mem->str_type_id && mem->str_type_id != 0) {
                match = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, ev));
            } else if (!tab->types[elem_tid].is_consuming) {
                match = true;
                uint16_t pa = val_get_pool_idx(a);
                uint16_t pe = val_get_pool_idx(ev);
                for (uint16_t k = 0; k < stride; ++k) {
                    if (fabs(mem_read_pool(mem, pa + k) - mem_read_pool(mem, pe + k)) > 1e-12) {
                        match = false;
                        break;
                    }
                }
            } else {
                match = (fabs(val_get_f64(mem, a) - val_get_f64(mem, ev)) <= 1e-12);
            }
            if (match) return val_make_f64(mem, mem->fast_real_type, 1.0);
        }
        return val_make_f64(mem, mem->fast_real_type, 0.0);
    }

    if (val_get_type(mem, a) != mem->vec_type_id) {
        mem_err(mem, "Expected dynamic vector ($vec) argument");
        return val_void();
    }
    uint16_t hdr_idx  = val_get_pool_idx(a);
    uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
    uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
    uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
    uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
    uint16_t cap      = (uint16_t)mem_read_pool(mem, hdr_idx + 2);
    uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                        ? tab->types[elem_tid].flat_size : 1;

    switch (intr) {
        case INTR_VLEN:
            return val_make_f64(mem, mem->fast_real_type, (double)len);
        case INTR_VCAP:
            return val_make_f64(mem, mem->fast_real_type, (double)cap);
        case INTR_VCLEAR:
            mem_write_pool(mem, hdr_idx + 1, 0.0);
            return a;
        case INTR_VSHRINK:
            mem_vec_shrink_to_fit(mem, hdr_idx, stride);
            return a;
        case INTR_VPUSH: {
            uint16_t item_tid = val_get_type(mem, b);
            uint8_t  item_uid = val_get_unit(mem, b);
            if (elem_tid == 0) {
                elem_tid = item_tid;
                elem_uid = item_uid;
                stride   = (elem_tid < tab->type_count) ? tab->types[elem_tid].flat_size : 1;
                uint16_t packed_t = (uint16_t)(((uint16_t)elem_uid << 8) | (elem_tid & 0xFFu));
                mem_write_pool(mem, hdr_idx + 0, (double)packed_t);
            } else if (elem_tid != item_tid || elem_uid != item_uid) {
                char exp_t[32], got_t[32];
                sym_type_name(tab, mem, elem_tid, exp_t, sizeof(exp_t));
                sym_type_name(tab, mem, item_tid, got_t, sizeof(got_t));
                mem_err(mem, "Vector type mismatch: cannot push '%s' into vec<%s>", got_t, exp_t);
                return val_void();
            }
            if (!mem_vec_reserve(mem, hdr_idx, stride, (uint16_t)(len + 1))) {
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            delegate_vec_write_elem(mem, tab, elem_tid, (uint16_t)(data_idx + len * stride), b);
            mem_write_pool(mem, hdr_idx + 1, (double)(len + 1));
            return a;
        }
        case INTR_VPOP: {
            if (len == 0) {
                mem_err(mem, "Cannot pop from an empty vector");
                return val_void();
            }
            uint16_t new_len  = (uint16_t)(len - 1);
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, (uint16_t)(data_idx + new_len * stride));
        }
        case INTR_VGET: {
            int raw_idx = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "vector index", &raw_idx)) return val_void();
            int idx = (raw_idx < 0) ? (raw_idx + (int)len) : raw_idx;
            if (idx < 0 || idx >= (int)len) {
                mem_err(mem, "Vector index %d out of bounds (len=%u)", raw_idx, (unsigned)len);
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, (uint16_t)(data_idx + (uint16_t)idx * stride));
        }
        case INTR_VSET: {
            int raw_idx = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "vector index", &raw_idx)) return val_void();
            int idx = (raw_idx < 0) ? (raw_idx + (int)len) : raw_idx;
            if (idx < 0 || idx >= (int)len) {
                mem_err(mem, "Vector index %d out of bounds (len=%u)", raw_idx, (unsigned)len);
                return val_void();
            }
            uint16_t item_tid = val_get_type(mem, c);
            uint8_t  item_uid = val_get_unit(mem, c);
            if (elem_tid != 0 && (elem_tid != item_tid || elem_uid != item_uid)) {
                char exp_t[32], got_t[32];
                sym_type_name(tab, mem, elem_tid, exp_t, sizeof(exp_t));
                sym_type_name(tab, mem, item_tid, got_t, sizeof(got_t));
                mem_err(mem, "Vector type mismatch: cannot set '%s' in vec<%s>", got_t, exp_t);
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            delegate_vec_write_elem(mem, tab, elem_tid, (uint16_t)(data_idx + (uint16_t)idx * stride), c);
            return a;
        }
        case INTR_VRESIZE: {
            int new_len_in = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "vector resize length", &new_len_in)) return val_void();
            if (new_len_in < 0 || new_len_in > MEM_MAX_F64_POOL) {
                mem_err(mem, "Vector resize length out of range (%d)", new_len_in);
                return val_void();
            }
            uint16_t new_len = (uint16_t)new_len_in;
            if (elem_tid == 0) {
                elem_tid = mem->fast_real_type;
                stride   = 1;
                mem_write_pool(mem, hdr_idx + 0, (double)elem_tid);
            }
            if (new_len > len) {
                if (!mem_vec_reserve(mem, hdr_idx, stride, new_len)) return val_void();
                uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
                memset(&mem->f64_pool[data_idx + len * stride], 0,
                       (size_t)((new_len - len) * stride) * sizeof(double));
            }
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return a;
        }
        default:
            return val_void();
    }
}

/* ============================================================================
 * 4. MULTIDIMENSIONAL MATRIX (`$mat`) DELEGATE INTRINSICS (`@mzero` .. `@minv`)
 * ============================================================================ */

bool delegate_mat_get_info(const MemArena *mem, Value v,
                           uint8_t *out_rank, uint16_t *out_total_len,
                           uint16_t *out_data_idx, uint16_t dims[4]) {
    if (mem->mat_type_id == 0 || val_get_type(mem, v) != mem->mat_type_id) {
        return false;
    }
    uint16_t hdr_idx = val_get_pool_idx(v);
    uint8_t  rank    = (uint8_t)mem_read_pool(mem, hdr_idx + 0);
    uint16_t tot_len = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
    uint16_t dat_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 2);
    for (uint8_t i = 0; i < 4; ++i) {
        dims[i] = (i < rank) ? (uint16_t)mem_read_pool(mem, hdr_idx + 4 + i) : 0;
    }
    if (out_rank)      *out_rank      = rank;
    if (out_total_len) *out_total_len = tot_len;
    if (out_data_idx)  *out_data_idx  = dat_idx;
    return true;
}

Value delegate_eval_mat_intrinsic(MemArena *mem, const SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c) {
    if (mem->mat_type_id == 0) {
        mem_err(mem, "Multidimensional matrix type ($mat) is not loaded");
        return val_void();
    }

    if (intr == INTR_MZERO || intr == INTR_MONES || intr == INTR_MFILL) {
        int r_in = 0, c_in = 0;
        if (!delegate_val_to_exact_int(mem, tab, a, "matrix rows", &r_in) ||
            !delegate_val_to_exact_int(mem, tab, b, "matrix cols", &c_in)) {
            return val_void();
        }
        if (r_in <= 0 || c_in <= 0 || r_in > 4096 || c_in > 4096) {
            mem_err(mem, "Matrix dimensions must be positive (%d x %d)", r_in, c_in);
            return val_void();
        }
        uint16_t dims[4] = { (uint16_t)r_in, (uint16_t)c_in, 0, 0 };
        uint16_t tot_len = 0, data_idx = 0;
        uint16_t hdr_idx = mem_mat_alloc_hdr(mem, 2, dims, &tot_len, &data_idx);
        if (mem->has_error) return val_void();

        double fill_v = (intr == INTR_MONES) ? 1.0 :
                        (intr == INTR_MFILL) ? val_get_f64(mem, c) : 0.0;
        uint8_t uid   = (intr == INTR_MFILL) ? val_get_unit(mem, c) : MEM_UNIT_NONE;
        if (fill_v != 0.0) {
            for (uint16_t i = 0; i < tot_len; ++i) {
                mem_write_pool(mem, data_idx + i, fill_v);
            }
        }
        return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS), uid);
    }

    if (intr == INTR_MEYE) {
        int n_in = 0;
        if (!delegate_val_to_exact_int(mem, tab, a, "identity matrix size", &n_in)) return val_void();
        if (n_in <= 0 || n_in > 4096) {
            mem_err(mem, "Identity matrix size must be positive (%d)", n_in);
            return val_void();
        }
        uint16_t n = (uint16_t)n_in;
        uint16_t dims[4] = { n, n, 0, 0 };
        uint16_t tot_len = 0, data_idx = 0;
        uint16_t hdr_idx = mem_mat_alloc_hdr(mem, 2, dims, &tot_len, &data_idx);
        if (mem->has_error) return val_void();
        for (uint16_t i = 0; i < n; ++i) {
            mem_write_pool(mem, (uint16_t)(data_idx + i * n + i), 1.0);
        }
        return val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS);
    }

    uint8_t  ra = 0;
    uint16_t tot_a = 0, dat_a = 0, da[4] = { 0 };
    if (!delegate_mat_get_info(mem, a, &ra, &tot_a, &dat_a, da)) {
        mem_err(mem, "Expected matrix ($mat) argument");
        return val_void();
    }
    uint8_t ua = val_get_unit(mem, a);

    switch (intr) {
        case INTR_MROWS:
            return val_make_f64(mem, mem->fast_real_type, (double)da[0]);
        case INTR_MCOLS:
            return val_make_f64(mem, mem->fast_real_type, (double)((ra >= 2) ? da[1] : da[0]));
        case INTR_MRANK:
            return val_make_f64(mem, mem->fast_real_type, (double)ra);
        case INTR_MLEN:
            return val_make_f64(mem, mem->fast_real_type, (double)tot_a);
        case INTR_MSHAPE: {
            double shape_d[4];
            for (uint8_t i = 0; i < ra; ++i) shape_d[i] = (double)da[i];
            return val_from_f64_vec(mem, shape_d, ra);
        }
        case INTR_MSCAL: {
            double  s  = val_get_f64(mem, b);
            uint8_t ub = val_get_unit(mem, b);
            uint8_t ru = (ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE);
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, ra, da, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t i = 0; i < tot_a; ++i) {
                mem_write_pool(mem, dat_out + i, mem_read_pool(mem, dat_a + i) * s);
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
        }
        case INTR_MADD:
        case INTR_MSUB: {
            uint8_t  rb = 0;
            uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
            if (!delegate_mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims)) {
                mem_err(mem, "Expected second matrix ($mat) argument");
                return val_void();
            }
            if (ra != rb) {
                mem_err(mem, "Matrix rank mismatch (%uD vs %uD)", (unsigned)ra, (unsigned)rb);
                return val_void();
            }
            for (uint8_t i = 0; i < ra; ++i) {
                if (da[i] != db_dims[i]) {
                    mem_err(mem, "Matrix shape mismatch on axis %u (%u vs %u)",
                            (unsigned)i, (unsigned)da[i], (unsigned)db_dims[i]);
                    return val_void();
                }
            }
            uint8_t ru = MEM_UNIT_NONE;
            if (!delegate_check_additive_unit_compat(mem, tab, (intr == INTR_MADD) ? "+" : "-", 1, a, b, &ru)) {
                return val_void();
            }
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, ra, da, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t i = 0; i < tot_a; ++i) {
                double va = mem_read_pool(mem, dat_a + i);
                double vb = mem_read_pool(mem, dat_b + i);
                mem_write_pool(mem, dat_out + i, (intr == INTR_MADD) ? (va + vb) : (va - vb));
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
        }
        case INTR_MMUL: {
            uint8_t  rb = 0;
            uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
            if (!delegate_mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims)) {
                mem_err(mem, "Expected second matrix ($mat) argument for '*'");
                return val_void();
            }
            uint8_t ub = val_get_unit(mem, b);
            uint8_t ru = (ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE);

            if (ra == 2 && rb == 2) {
                uint16_t M = da[0], K = da[1], Kb = db_dims[0], N = db_dims[1];
                if (K != Kb) {
                    mem_err(mem, "Matrix dimension mismatch in '*': (%ux%u) vs (%ux%u)",
                            (unsigned)M, (unsigned)K, (unsigned)Kb, (unsigned)N);
                    return val_void();
                }
                uint16_t out_dims[4] = { M, N, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t r = 0; r < M; ++r) {
                    for (uint16_t col = 0; col < N; ++col) {
                        double sum = 0.0;
                        for (uint16_t k = 0; k < K; ++k) {
                            sum += mem_read_pool(mem, (uint16_t)(dat_a + r * K + k)) *
                                   mem_read_pool(mem, (uint16_t)(dat_b + k * N + col));
                        }
                        mem_write_pool(mem, (uint16_t)(dat_out + r * N + col), sum);
                    }
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            if (ra == 2 && rb == 1) {
                uint16_t M = da[0], K = da[1], Kb = db_dims[0];
                if (K != Kb) {
                    mem_err(mem, "Matrix-vector dimension mismatch in '*': (%ux%u) vs (%u)",
                            (unsigned)M, (unsigned)K, (unsigned)Kb);
                    return val_void();
                }
                uint16_t out_dims[4] = { M, 0, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 1, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t r = 0; r < M; ++r) {
                    double sum = 0.0;
                    for (uint16_t k = 0; k < K; ++k) {
                        sum += mem_read_pool(mem, (uint16_t)(dat_a + r * K + k)) *
                               mem_read_pool(mem, (uint16_t)(dat_b + k));
                    }
                    mem_write_pool(mem, (uint16_t)(dat_out + r), sum);
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            if (ra == 1 && rb == 1 && da[0] == db_dims[0]) {
                uint16_t out_dims[4] = { da[0], 0, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 1, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < da[0]; ++i) {
                    mem_write_pool(mem, dat_out + i,
                                   mem_read_pool(mem, dat_a + i) * mem_read_pool(mem, dat_b + i));
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            mem_err(mem, "Matrix multiplication '*' requires 2D*2D or 2D*1D compatible dimensions");
            return val_void();
        }
        case INTR_MTRANS: {
            if (ra == 1) {
                uint16_t out_dims[4] = { da[0], 1, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < da[0]; ++i) {
                    mem_write_pool(mem, dat_out + i, mem_read_pool(mem, dat_a + i));
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ua);
            }
            if (ra != 2) {
                mem_err(mem, "transpose() requires a 2D matrix (got %uD)", (unsigned)ra);
                return val_void();
            }
            uint16_t R = da[0], C = da[1];
            uint16_t out_dims[4] = { C, R, 0, 0 };
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t r = 0; r < R; ++r) {
                for (uint16_t col = 0; col < C; ++col) {
                    mem_write_pool(mem, (uint16_t)(dat_out + col * R + r),
                                   mem_read_pool(mem, (uint16_t)(dat_a + r * C + col)));
                }
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ua);
        }
        case INTR_MTRACE: {
            if (ra != 2 || da[0] != da[1]) {
                mem_err(mem, "trace() requires a square 2D matrix");
                return val_void();
            }
            uint16_t N = da[0];
            double tr = 0.0;
            for (uint16_t i = 0; i < N; ++i) {
                tr += mem_read_pool(mem, (uint16_t)(dat_a + i * N + i));
            }
            return val_make_f64_unit(mem, mem->fast_real_type, ua, tr);
        }
        case INTR_MDET: {
            if (ra != 2 || da[0] != da[1] || da[0] == 0 || da[0] > 16) {
                mem_err(mem, "det() requires a square 2D matrix (1x1 up to 16x16)");
                return val_void();
            }
            uint16_t N = da[0];
            double tmp[256];
            for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                tmp[i] = mem_read_pool(mem, dat_a + i);
            }
            double det_val = 1.0;
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t piv = i;
                double max_v = fabs(tmp[i * N + i]);
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double v = fabs(tmp[r * N + i]);
                    if (v > max_v) { max_v = v; piv = r; }
                }
                if (max_v <= 1e-14) return val_make_f64(mem, mem->fast_real_type, 0.0);
                if (piv != i) {
                    for (uint16_t col = i; col < N; ++col) {
                        double t = tmp[i * N + col];
                        tmp[i * N + col] = tmp[piv * N + col];
                        tmp[piv * N + col] = t;
                    }
                    det_val = -det_val;
                }
                double diag = tmp[i * N + i];
                det_val *= diag;
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double factor = tmp[r * N + i] / diag;
                    for (uint16_t col = (uint16_t)(i + 1); col < N; ++col) {
                        tmp[r * N + col] -= factor * tmp[i * N + col];
                    }
                }
            }
            return val_make_f64(mem, mem->fast_real_type, det_val);
        }
        case INTR_MINV: {
            if (ra != 2 || da[0] != da[1] || da[0] == 0 || da[0] > 16) {
                mem_err(mem, "inv() requires a square 2D matrix (1x1 up to 16x16)");
                return val_void();
            }
            uint16_t N = da[0];
            double aug[512];
            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < N; ++col) {
                    aug[r * (2 * N) + col]     = mem_read_pool(mem, (uint16_t)(dat_a + r * N + col));
                    aug[r * (2 * N) + N + col] = (r == col) ? 1.0 : 0.0;
                }
            }
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t piv = i;
                double max_v = fabs(aug[i * (2 * N) + i]);
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double v = fabs(aug[r * (2 * N) + i]);
                    if (v > max_v) { max_v = v; piv = r; }
                }
                if (max_v <= 1e-14) {
                    mem_err(mem, "Singular matrix cannot be inverted (det == 0)");
                    return val_void();
                }
                if (piv != i) {
                    for (uint16_t col = 0; col < (uint16_t)(2 * N); ++col) {
                        double t = aug[i * (2 * N) + col];
                        aug[i * (2 * N) + col] = aug[piv * (2 * N) + col];
                        aug[piv * (2 * N) + col] = t;
                    }
                }
                double diag = aug[i * (2 * N) + i];
                for (uint16_t col = 0; col < (uint16_t)(2 * N); ++col) {
                    aug[i * (2 * N) + col] /= diag;
                }
                for (uint16_t r = 0; r < N; ++r) {
                    if (r == i) continue;
                    double factor = aug[r * (2 * N) + i];
                    if (fabs(factor) > 0.0) {
                        for (uint16_t col = 0; col < (uint16_t)(2 * N); ++col) {
                            aug[r * (2 * N) + col] -= factor * aug[i * (2 * N) + col];
                        }
                    }
                }
            }
            uint16_t out_dims[4] = { N, N, 0, 0 };
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < N; ++col) {
                    double v = aug[r * (2 * N) + N + col];
                    if (fabs(v) < 1e-15) v = 0.0;
                    mem_write_pool(mem, (uint16_t)(dat_out + r * N + col), v);
                }
            }
            return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
        }
        default:
            return val_void();
    }
}

/* ============================================================================
 * 5. SCALAR ARITHMETIC, BOOLEAN LOGIC & TRANSCENDENTAL DELEGATES
 * ============================================================================ */

Value delegate_eval_intrinsic(MemArena *mem, const SymTable *tab,
                              IntrinsicId intr, Value a, Value b, Value c, uint8_t ret_type) {
    if (intr >= INTR_VNEW && intr <= INTR_VIN) {
        return delegate_eval_vec_intrinsic(mem, tab, intr, a, b, c);
    }
    if (intr >= INTR_MZERO && intr <= INTR_MINV) {
        return delegate_eval_mat_intrinsic(mem, tab, intr, a, b, c);
    }

    if (intr == INTR_TYPE_NAME) {
        char tbuf[32];
        sym_type_name(tab, mem, val_get_type(mem, a), tbuf, sizeof(tbuf));
        return val_from_cstr(mem, tbuf);
    }
    if (intr == INTR_UNIT_NAME) {
        char ubuf[32];
        sym_unit_name(tab, mem, val_get_unit(mem, a), ubuf, sizeof(ubuf));
        return val_from_cstr(mem, ubuf);
    }
    if (intr == INTR_ERR) {
        char ebuf[120];
        if (val_as_cstr(mem, a, ebuf, sizeof(ebuf)) && ebuf[0] != '\0') {
            mem_err(mem, "%s", ebuf);
        } else {
            mem_err(mem, "Runtime error raised by error()");
        }
        return val_void();
    }

    uint16_t rtid = (ret_type != 0 && ret_type < tab->type_count && tab->types[ret_type].is_consuming)
                    ? ret_type
                    : mem->fast_real_type;
    uint8_t  ua   = val_get_unit(mem, a);
    uint8_t  ub   = (val_get_type(mem, b) != MEM_TYPE_VOID) ? val_get_unit(mem, b) : MEM_UNIT_NONE;
    uint8_t  ruid = MEM_UNIT_NONE;

    double da  = val_get_f64(mem, a);
    double db  = (val_get_type(mem, b) != MEM_TYPE_VOID) ? val_get_f64(mem, b) : 0.0;
    double res = 0.0;

    switch (intr) {
        case INTR_ADD:
            if (!delegate_check_additive_unit_compat(mem, tab, "+", 1, a, b, &ruid)) return val_void();
            res = da + db;
            break;
        case INTR_SUB:
            if (!delegate_check_additive_unit_compat(mem, tab, "-", 1, a, b, &ruid)) return val_void();
            res = da - db;
            break;
        case INTR_MUL:
            ruid = (ua == ub) ? ua : ((ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE));
            res = da * db;
            break;
        case INTR_DIV:
            if (db == 0.0) { mem_err(mem, "Division by zero"); return val_void(); }
            ruid = (ua == ub) ? ua : ((ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE));
            res = da / db;
            break;
        case INTR_MOD:
            ruid = (ua == ub) ? ua : ((ua != 0 && ub == 0) ? ua : MEM_UNIT_NONE);
            res = fmod(da, db);
            break;
        case INTR_NEG:   ruid = ua; res = -da; break;
        case INTR_ABS:   ruid = ua; res = fabs(da); break;
        case INTR_FLOOR: ruid = ua; res = floor(da); break;
        case INTR_CEIL:  ruid = ua; res = ceil(da); break;
        case INTR_POW:   ruid = (ub == 0) ? ua : MEM_UNIT_NONE; res = pow(da, db); break;
        case INTR_SQRT:  ruid = ua; res = sqrt(da); break;
        case INTR_CBRT:  ruid = ua; res = cbrt(da); break;
        case INTR_EXP:   res = exp(da); break;
        case INTR_LN:    res = log(da); break;
        case INTR_LOG10: res = log10(da); break;
        case INTR_SIN:   res = sin(da); break;
        case INTR_COS:   res = cos(da); break;
        case INTR_TAN:   res = tan(da); break;
        case INTR_ASIN:  res = asin(da); break;
        case INTR_ACOS:  res = acos(da); break;
        case INTR_ATAN:  res = atan(da); break;
        case INTR_ATAN2: res = atan2(da, db); break;
        case INTR_SINH:  res = sinh(da); break;
        case INTR_COSH:  res = cosh(da); break;
        case INTR_TANH:  res = tanh(da); break;
        case INTR_EQ:
            if (!delegate_check_additive_unit_compat(mem, tab, "==", 2, a, b, NULL)) return val_void();
            res = (fabs(da - db) <= 1e-12) ? 1.0 : 0.0;
            break;
        case INTR_NE:
            if (!delegate_check_additive_unit_compat(mem, tab, "!=", 2, a, b, NULL)) return val_void();
            res = (fabs(da - db) >  1e-12) ? 1.0 : 0.0;
            break;
        case INTR_LT:
            if (!delegate_check_additive_unit_compat(mem, tab, "<", 1, a, b, NULL)) return val_void();
            res = (da <  db) ? 1.0 : 0.0;
            break;
        case INTR_LE:
            if (!delegate_check_additive_unit_compat(mem, tab, "<=", 2, a, b, NULL)) return val_void();
            res = (da <= db) ? 1.0 : 0.0;
            break;
        case INTR_GT:
            if (!delegate_check_additive_unit_compat(mem, tab, ">", 1, a, b, NULL)) return val_void();
            res = (da >  db) ? 1.0 : 0.0;
            break;
        case INTR_GE:
            if (!delegate_check_additive_unit_compat(mem, tab, ">=", 2, a, b, NULL)) return val_void();
            res = (da >= db) ? 1.0 : 0.0;
            break;
        case INTR_APPROX:
            if (!delegate_check_additive_unit_compat(mem, tab, "~", 1, a, b, NULL)) return val_void();
            res = (fabs(da - db) <= 0.001 + 1e-12) ? 1.0 : 0.0;
            break;
        case INTR_LAND:  res = (fabs(da) > 1e-15 && fabs(db) > 1e-15) ? 1.0 : 0.0; break;
        case INTR_LOR:   res = (fabs(da) > 1e-15 || fabs(db) > 1e-15) ? 1.0 : 0.0; break;
        case INTR_CNOT:
        case INTR_LNOT:
            if (fabs(da) <= 1e-15) {
                res = 1.0;
            } else if (fabs(da - 1.0) <= 1e-15) {
                res = 0.0;
            } else {
                mem_err(mem, "Operand %.6g out of range for boolean negation '!': expected 0 or 1", da);
                return val_void();
            }
            break;
        case INTR_LXOR:  res = ((fabs(da) > 1e-15) != (fabs(db) > 1e-15)) ? 1.0 : 0.0; break;
        case INTR_LNOR:  res = (fabs(da) <= 1e-15 && fabs(db) <= 1e-15) ? 1.0 : 0.0; break;
        case INTR_LNAND: res = !(fabs(da) > 1e-15 && fabs(db) > 1e-15) ? 1.0 : 0.0; break;
        case INTR_RAW:   ruid = ua; res = da; break;
        default:         mem_err(mem, "Unhandled intrinsic %d", (int)intr); return val_void();
    }
    return val_make_f64_unit(mem, rtid, ruid, res);
}

/* ============================================================================
 * 6. CALCULUS, SOLVER & EXTREMA DELEGATES (`@calc_diff` .. `@calc_fmax`)
 * ============================================================================ */

static double eval_calc_point(MemArena *mem, const SymTable *tab,
                              uint8_t var_slot, uint16_t expr_ast,
                              uint16_t frame_base, uint8_t cur_ret_type, double x_val) {
    mem_set_slot(mem, frame_base, var_slot, val_make_f64(mem, mem->fast_real_type, x_val));
    Value v = eval_ast(mem, tab, expr_ast, frame_base, cur_ret_type);
    return val_get_f64(mem, v);
}

static double eval_solve_residual(MemArena *mem, const SymTable *tab,
                                  uint8_t var_slot, uint16_t expr_ast,
                                  uint16_t frame_base, uint8_t cur_ret_type, double x_val) {
    const AstNode *en = mem_get_ast(mem, expr_ast);
    if (en->kind == AST_BINARY && en->ref < tab->sym_count) {
        StrSlice s = tab->syms[en->ref];
        if ((s.len == 2 && memcmp(mem_str_ptr(mem, s), "==", 2) == 0) ||
            (s.len == 1 && mem_str_ptr(mem, s)[0] == '~')) {
            double lv = eval_calc_point(mem, tab, var_slot, en->left,  frame_base, cur_ret_type, x_val);
            double rv = eval_calc_point(mem, tab, var_slot, en->right, frame_base, cur_ret_type, x_val);
            return lv - rv;
        }
    }
    return eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x_val);
}

static double snap_calc_noise(double res, double tol) {
    double nearest = floor(res + 0.5);
    if (fabs(res - nearest) < tol) return nearest;
    if (fabs(res) < tol * 0.1) return 0.0;
    return res;
}

Value delegate_eval_calculus_meta(MemArena *mem, const SymTable *tab,
                                  uint8_t mtag, const AstNode *n,
                                  uint16_t frame_base, uint8_t cur_ret_type) {
    uint16_t arg_asts[6];
    uint8_t  argc = 0;
    uint16_t cur  = n->left;
    while (cur != MEM_NO_IDX && argc < 6) {
        const AstNode *link = mem_get_ast(mem, cur);
        arg_asts[argc++] = link->left;
        cur = link->right;
    }
    if (argc < 2 || (mtag != INTR_CALC_SOLVE && argc < 3)) {
        mem_err(mem, "Calculus construct requires at least %d arguments",
                (mtag == INTR_CALC_SOLVE) ? 2 : 3);
        return val_void();
    }

    const AstNode *var_node = mem_get_ast(mem, arg_asts[0]);
    if (var_node->kind != AST_SLOT_LOAD) {
        mem_err(mem, "First argument to calculus construct must be a variable name");
        return val_void();
    }
    uint8_t  var_slot = var_node->sub;
    uint16_t expr_ast = arg_asts[1];
    Value    saved_v  = mem_get_slot(mem, frame_base, var_slot);
    double   res      = 0.0;

    if (mtag == INTR_CALC_DIFF || mtag == INTR_CALC_DIFF2 || mtag == INTR_CALC_DIFFN) {
        Value v_x0 = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double x0 = val_get_f64(mem, v_x0);
        int order = 1;
        if (mtag == INTR_CALC_DIFF2) {
            order = 2;
        } else if (mtag == INTR_CALC_DIFFN && argc >= 4) {
            Value v_ord = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                return val_void();
            }
            order = (int)val_get_f64(mem, v_ord);
        }

        double scale = (fabs(x0) > 1.0) ? fabs(x0) : 1.0;
        if (order <= 0) {
            res = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0);
        } else if (order == 1) {
            double h   = 1e-5 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (-fp2 + 8.0 * fp1 - 8.0 * fm1 + fm2) / (12.0 * h);
            res = snap_calc_noise(res, 1e-9);
        } else if (order == 2) {
            double h   = 1e-4 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double f0  = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (-fp2 + 16.0 * fp1 - 30.0 * f0 + 16.0 * fm1 - fm2) / (12.0 * h * h);
            res = snap_calc_noise(res, 1e-7);
        } else if (order == 3) {
            double h   = 1e-3 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (fp2 - 2.0 * fp1 + 2.0 * fm1 - fm2) / (2.0 * h * h * h);
            res = snap_calc_noise(res, 1e-5);
        } else {
            double h   = 2e-3 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double f0  = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (fp2 - 4.0 * fp1 + 6.0 * f0 - 4.0 * fm1 + fm2) / (h * h * h * h);
            res = snap_calc_noise(res, 1e-4);
        }
    } else if (mtag == INTR_CALC_INTEG) {
        if (argc < 4) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            mem_err(mem, "integrate(x, expr, a, b) requires lower and upper bounds");
            return val_void();
        }
        Value va = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        Value vb = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double a = val_get_f64(mem, va);
        double b = val_get_f64(mem, vb);
        int steps = 200;
        if (argc >= 5) {
            Value vn = eval_ast(mem, tab, arg_asts[4], frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                return val_void();
            }
            steps = (int)val_get_f64(mem, vn);
            if (steps < 2) steps = 2;
            if (steps % 2 != 0) steps++;
            if (steps > 10000) steps = 10000;
        }
        double h   = (b - a) / (double)steps;
        double sum = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, a)
                   + eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, b);
        for (int k = 1; k < steps && !mem->has_error; ++k) {
            double xk = a + (double)k * h;
            double fk = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, xk);
            sum += ((k % 2 == 1) ? 4.0 : 2.0) * fk;
        }
        res = (h / 3.0) * sum;
        res = snap_calc_noise(res, 1e-9);
    } else if (mtag == INTR_CALC_LIM || mtag == INTR_CALC_LIM_L || mtag == INTR_CALC_LIM_R) {
        Value v_x0 = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double x0    = val_get_f64(mem, v_x0);
        double scale = (fabs(x0) > 1.0) ? fabs(x0) : 1.0;
        double h1    = 1e-6 * scale;
        double h2    = 1e-7 * scale;

        if (mtag == INTR_CALC_LIM_R) {
            double r1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h1);
            double r2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h2);
            res = snap_calc_noise((10.0 * r2 - r1) / 9.0, 1e-8);
        } else if (mtag == INTR_CALC_LIM_L) {
            double l1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h1);
            double l2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h2);
            res = snap_calc_noise((10.0 * l2 - l1) / 9.0, 1e-8);
        } else {
            double r1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h1);
            double r2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h2);
            double l1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h1);
            double l2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h2);
            double lim_r = (10.0 * r2 - r1) / 9.0;
            double lim_l = (10.0 * l2 - l1) / 9.0;
            double tol   = 1e-4 * (1.0 + fabs(lim_r) + fabs(lim_l));
            if (fabs(lim_r - lim_l) > tol) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                mem_err(mem, "Two-sided limit does not exist (left %.6g != right %.6g)", lim_l, lim_r);
                return val_void();
            }
            res = snap_calc_noise(0.5 * (lim_l + lim_r), 1e-8);
        }
    } else if (mtag == INTR_CALC_SIGMA || mtag == INTR_CALC_PROD) {
        if (argc < 4) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            mem_err(mem, "sigma/prod(k, expr, lo, hi) requires lower and upper bounds");
            return val_void();
        }
        Value vlo = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        Value vhi = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        long long lo = (long long)floor(val_get_f64(mem, vlo) + 0.5);
        long long hi = (long long)floor(val_get_f64(mem, vhi) + 0.5);
        res = (mtag == INTR_CALC_PROD) ? 1.0 : 0.0;
        for (long long k = lo; k <= hi && !mem->has_error; ++k) {
            double vk = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, (double)k);
            if (mtag == INTR_CALC_PROD) res *= vk;
            else                        res += vk;
        }
    } else if (mtag == INTR_CALC_SOLVE) {
        double xr = 1.0;
        if (argc >= 3) {
            Value vg = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                return val_void();
            }
            xr = val_get_f64(mem, vg);
        }
        if (argc >= 4) {
            Value vb = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                return val_void();
            }
            xr = 0.5 * (xr + val_get_f64(mem, vb));
        }

        double fx = 0.0;
        for (int iter = 0; iter < 80 && !mem->has_error; ++iter) {
            fx = eval_solve_residual(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, xr);
            if (fabs(fx) <= 1e-12) break;
            double h   = 1e-6 * ((fabs(xr) > 1.0) ? fabs(xr) : 1.0);
            double fp  = eval_solve_residual(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, xr + h);
            double fm  = eval_solve_residual(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, xr - h);
            double dfx = (fp - fm) / (2.0 * h);
            if (fabs(dfx) <= 1e-12) {
                xr += (xr >= 0.0) ? 0.25 : -0.25;
                continue;
            }
            double step = fx / dfx;
            if (step > 100.0) step = 100.0;
            if (step < -100.0) step = -100.0;
            xr -= step;
        }
        fx = eval_solve_residual(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, xr);
        if (!mem->has_error && (!isfinite(xr) || !isfinite(fx) || fabs(fx) > 1e-4)) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            mem_err(mem, "solve() failed to converge to a real root (residual=%.4g)", fx);
            return val_void();
        }
        res = snap_calc_noise(xr, 1e-8);
    } else if (mtag == INTR_CALC_FMIN || mtag == INTR_CALC_FMAX) {
        if (argc < 4) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            mem_err(mem, "fmin/fmax(x, expr, a, b) requires interval bounds [a, b]");
            return val_void();
        }
        Value va = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        Value vb = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double a = val_get_f64(mem, va);
        double b = val_get_f64(mem, vb);
        if (a > b) { double t = a; a = b; b = t; }

        const double invphi = 0.6180339887498948482; /* (sqrt(5) - 1) / 2 */
        double c = b - invphi * (b - a);
        double d = a + invphi * (b - a);
        double fc = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, c);
        double fd = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, d);

        for (int iter = 0; iter < 75 && !mem->has_error; ++iter) {
            bool pick_left = (mtag == INTR_CALC_FMIN) ? (fc < fd) : (fc > fd);
            if (pick_left) {
                b  = d;
                d  = c;
                fd = fc;
                c  = b - invphi * (b - a);
                fc = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, c);
            } else {
                a  = c;
                c  = d;
                fc = fd;
                d  = a + invphi * (b - a);
                fd = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, d);
            }
        }
        res = snap_calc_noise(0.5 * (a + b), 1e-6);
    }

    mem_set_slot(mem, frame_base, var_slot, saved_v);
    if (mem->has_error) return val_void();
    return val_make_f64(mem, mem->fast_real_type, res);
}
