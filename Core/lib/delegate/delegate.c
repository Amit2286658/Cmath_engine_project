/*
 * delegate.c — Internal C `@` Intrinsic Delegate Registry & Execution Engine
 *
 * Location: Core/lib/delegate/delegate.c
 *
 * Contains the `@<intrinsic>` lookup table and all C implementations of
 * operations dispatched via `@` in extension modules.
 */

#include "delegate.h"
#include "../../Headers/parser.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * 1. `@` INTRINSIC NAME -> OPCODE REGISTRY
 * ============================================================================ */

/*Below section is the C's native lookup table and resolver for operator/function delegates @<intrinsic>*/
/*Construct @f64, @lparen, @seq, @store, @add, @sin, @vnew, @vshift, @vpeek, @mmul, @calc_diff, @tostr, @fromstr*/
typedef struct {
    const char *name;
    IntrinsicId id;
} IntrEntry;

static const IntrEntry k_delegate_intrinsics[] = {
    { "f64",           INTR_PARSE_F64 },
    { "f64_trim1",     INTR_PARSE_F64_TRIM1 },
    { "i64",           INTR_PARSE_I64 },
    { "str_slice",     INTR_PARSE_STR_SLICE },
    { "vec_type",      INTR_VEC_TYPE },
    { "mat_type",      INTR_MAT_TYPE },
    { "fn_type",       INTR_FN_TYPE },
    { "lparen",        INTR_LPAREN },
    { "rparen",        INTR_RPAREN },
    { "lbrace",        INTR_LBRACE },
    { "rbrace",        INTR_RBRACE },
    { "lbrack",        INTR_LBRACK },
    { "rbrack",        INTR_RBRACK },
    { "comma",         INTR_COMMA },
    { "seq",           INTR_SEQ },
    { "store",         INTR_STORE },
    { "while",         INTR_WHILE },
    { "if",            INTR_IF },
    { "or_branch",     INTR_OR_BRANCH },
    { "then_branch",   INTR_OR_BRANCH },
    { "else_branch",   INTR_ELSE_BRANCH },
    { "switch_block",  INTR_SWITCH_BLOCK },
    { "case_branch",   INTR_CASE_BRANCH },
    { "ternary",       INTR_TERNARY },
    { "colon",         INTR_COLON },
    { "jump_tag",      INTR_JUMP_TAG },
    { "run_tag",       INTR_RUN_TAG },
    { "break_loop",    INTR_BREAK_LOOP },
    { "continue_loop", INTR_CONTINUE_LOOP },
    { "return_fn",     INTR_RETURN_FN },
    { "lambda",        INTR_LAMBDA },
    { "cand",          INTR_CAND },
    { "cor",           INTR_COR },
    { "cnot",          INTR_CNOT },
    { "calc_diff",     INTR_CALC_DIFF },
    { "calc_diff2",    INTR_CALC_DIFF2 },
    { "calc_diffn",    INTR_CALC_DIFFN },
    { "calc_integ",    INTR_CALC_INTEG },
    { "calc_lim",      INTR_CALC_LIM },
    { "calc_lim_l",    INTR_CALC_LIM_L },
    { "calc_lim_r",    INTR_CALC_LIM_R },
    { "calc_sigma",    INTR_CALC_SIGMA },
    { "calc_prod",     INTR_CALC_PROD },
    { "calc_solve",    INTR_CALC_SOLVE },
    { "calc_fmin",     INTR_CALC_FMIN },
    { "calc_fmax",     INTR_CALC_FMAX },
    { "field",         INTR_FIELD },
    { "vnew",          INTR_VNEW },
    { "vfill",         INTR_VFILL },
    { "vpush",         INTR_VPUSH },
    { "vpop",          INTR_VPOP },
    { "vshift",        INTR_VSHIFT },
    { "vpeek",         INTR_VPEEK },
    { "vget",          INTR_VGET },
    { "vset",          INTR_VSET },
    { "vlen",          INTR_VLEN },
    { "vcap",          INTR_VCAP },
    { "vresize",       INTR_VRESIZE },
    { "vshrink",       INTR_VSHRINK },
    { "vclear",        INTR_VCLEAR },
    { "vrange",        INTR_VRANGE },
    { "vin",           INTR_VIN },
    { "fcall",         INTR_FCALL },
    { "vmap",          INTR_VMAP },
    { "vfilter",       INTR_VFILTER },
    { "vreduce",       INTR_VREDUCE },
    { "vany",          INTR_VANY },
    { "vall",          INTR_VALL },
    { "vfind",         INTR_VFIND },
    { "vcount",        INTR_VCOUNT },
    { "mzero",         INTR_MZERO },
    { "mones",         INTR_MONES },
    { "meye",          INTR_MEYE },
    { "mfill",         INTR_MFILL },
    { "mrows",         INTR_MROWS },
    { "mcols",         INTR_MCOLS },
    { "mrank",         INTR_MRANK },
    { "mlen",          INTR_MLEN },
    { "mshape",        INTR_MSHAPE },
    { "madd",          INTR_MADD },
    { "msub",          INTR_MSUB },
    { "mscal",         INTR_MSCAL },
    { "mmul",          INTR_MMUL },
    { "mtrans",        INTR_MTRANS },
    { "mtrace",        INTR_MTRACE },
    { "mdet",          INTR_MDET },
    { "minv",          INTR_MINV },
    { "add",           INTR_ADD },
    { "sub",           INTR_SUB },
    { "mul",           INTR_MUL },
    { "div",           INTR_DIV },
    { "mod",           INTR_MOD },
    { "neg",           INTR_NEG },
    { "abs",           INTR_ABS },
    { "floor",         INTR_FLOOR },
    { "ceil",          INTR_CEIL },
    { "pow",           INTR_POW },
    { "sqrt",          INTR_SQRT },
    { "cbrt",          INTR_CBRT },
    { "exp",           INTR_EXP },
    { "ln",            INTR_LN },
    { "log10",         INTR_LOG10 },
    { "sin",           INTR_SIN },
    { "cos",           INTR_COS },
    { "tan",           INTR_TAN },
    { "asin",          INTR_ASIN },
    { "acos",          INTR_ACOS },
    { "atan",          INTR_ATAN },
    { "atan2",         INTR_ATAN2 },
    { "sinh",          INTR_SINH },
    { "cosh",          INTR_COSH },
    { "tanh",          INTR_TANH },
    { "eq",            INTR_EQ },
    { "ne",            INTR_NE },
    { "lt",            INTR_LT },
    { "le",            INTR_LE },
    { "gt",            INTR_GT },
    { "ge",            INTR_GE },
    { "approx",        INTR_APPROX },
    { "land",          INTR_LAND },
    { "lor",           INTR_LOR },
    { "lnot",          INTR_LNOT },
    { "lxor",          INTR_LXOR },
    { "lnor",          INTR_LNOR },
    { "lnand",         INTR_LNAND },
    { "raw",           INTR_RAW },
    { "type_name",     INTR_TYPE_NAME },
    { "unit_name",     INTR_UNIT_NAME },
    { "tostr",         INTR_TOSTR },
    { "fromstr",       INTR_FROMSTR },
    { "err",           INTR_ERR },
    { NULL,            INTR_NONE }
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
 * 2. SCALAR & UNIT VALIDATION HELPERS
 * ============================================================================ */

/*Below section is the C's native unit compatibility and integer validation helpers for operator delegates @add, @sub, @eq..@approx*/
/*Construct unit-aware + / - / comparisons (universal 0 rule) and exact-integer index/dimension validation*/
bool delegate_is_zero_unitless_scalar(const MemArena *mem, const SymTable *tab, Value v) {
    uint16_t tid = val_get_type(mem, v);
    if (tid == 0 || tid >= tab->type_count || !tab->types[tid].is_consuming) return false;
    if (tid == mem->str_type_id) return false;
    if (val_get_unit(mem, v) != MEM_UNIT_NONE) return false;
    return fabs(val_get_f64(mem, v)) <= 1e-15;
}

bool delegate_check_additive_unit_compat(MemArena *mem, const SymTable *tab,
                                         const char *op_name, int op_len,
                                         Value a, Value b, uint8_t *out_unit) {
    uint8_t ua = val_get_unit(mem, a);
    uint8_t ub = val_get_unit(mem, b);
    if (ua == ub) {
        if (out_unit) *out_unit = ua;
        return true;
    }
    if (ua == MEM_UNIT_NONE && delegate_is_zero_unitless_scalar(mem, tab, a)) {
        if (out_unit) *out_unit = ub;
        return true;
    }
    if (ub == MEM_UNIT_NONE && delegate_is_zero_unitless_scalar(mem, tab, b)) {
        if (out_unit) *out_unit = ua;
        return true;
    }

    char t1[32], t2[32], u1[32], u2[32];
    sym_type_name(tab, mem, val_get_type(mem, a), t1, sizeof(t1));
    sym_type_name(tab, mem, val_get_type(mem, b), t2, sizeof(t2));
    sym_unit_name(tab, mem, ua, u1, sizeof(u1));
    sym_unit_name(tab, mem, ub, u2, sizeof(u2));
    mem_err(mem, "Unit mismatch for '%.*s' with (%s [%s], %s [%s])",
            op_len, op_name, t1, u1, t2, u2);
    return false;
}

bool delegate_val_to_exact_int(MemArena *mem, const SymTable *tab, Value v,
                               const char *ctx, int *out_int) {
    uint16_t tid = val_get_type(mem, v);
    if (tid == 0 || tid >= tab->type_count ||
        !tab->types[tid].is_consuming || tid == mem->str_type_id) {
        mem_err(mem, "Invalid non-numeric %s", ctx);
        return false;
    }
    double d = val_get_f64(mem, v);
    if (!isfinite(d) || d < -2147483647.0 || d > 2147483647.0) {
        mem_err(mem, "Out-of-range %s (%.6g)", ctx, d);
        return false;
    }
    double r = floor(d + 0.5);
    if (fabs(d - r) > 1e-9) {
        mem_err(mem, "Non-integer %s (%.6g)", ctx, d);
        return false;
    }
    *out_int = (int)r;
    return true;
}

/* ============================================================================
 * 2b. FIRST-CLASS LAMBDA (`$fn`) INVOCATION DELEGATE
 * ============================================================================ */

/*Below section is the C's native for operator delegate @lambda and function delegate @fcall*/
/*Construct (x) => expr, (a, b) => expr, () => expr, f(x), call(f, x)*/
Value delegate_call_fn(MemArena *mem, SymTable *tab,
                       Value fn_val, uint8_t argc, const Value *args) {
    if (mem->fn_type_id == 0 || val_get_type(mem, fn_val) != mem->fn_type_id) {
        mem_err(mem, "Expected function ($fn) handle");
        return val_void();
    }
    uint16_t hdr_idx    = val_get_pool_idx(fn_val);
    uint16_t body_ast   = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
    uint8_t  param_cnt  = (uint8_t)mem_read_pool(mem, hdr_idx + 1);
    uint32_t packed_slt = (uint32_t)mem_read_pool(mem, hdr_idx + 2);
    uint16_t fbase      = (uint16_t)mem_read_pool(mem, hdr_idx + 3);

    if (argc != param_cnt) {
        mem_err(mem, "Lambda expects %u argument(s), got %u",
                (unsigned)param_cnt, (unsigned)argc);
        return val_void();
    }

    uint8_t slots[4] = {
        (uint8_t)(packed_slt & 0xFFu),
        (uint8_t)((packed_slt >> 8)  & 0xFFu),
        (uint8_t)((packed_slt >> 16) & 0xFFu),
        (uint8_t)((packed_slt >> 24) & 0xFFu)
    };

    uint16_t saved_sp = mem->sp;
    uint16_t max_needed = fbase;
    for (uint8_t k = 0; k < param_cnt; ++k) {
        uint16_t pos = (uint16_t)(fbase + slots[k] + 1);
        if (pos > max_needed) max_needed = pos;
    }
    if (max_needed > MEM_MAX_CALL_STACK) {
        mem_err(mem, "Call stack overflow in lambda invocation");
        return val_void();
    }
    if (mem->sp < max_needed) {
        mem->sp = max_needed;
    }

    Value   saved_vals[4];
    uint8_t saved_ltype[4];
    uint8_t saved_lunit[4];
    for (uint8_t k = 0; k < param_cnt; ++k) {
        uint16_t abs_s = (uint16_t)(fbase + slots[k]);
        saved_vals[k]  = mem->stack[abs_s];
        saved_ltype[k] = mem->slot_locked_type[abs_s];
        saved_lunit[k] = mem->slot_locked_unit[abs_s];
        mem->slot_locked_type[abs_s] = 0;
        mem->slot_locked_unit[abs_s] = 0;
        mem->stack[abs_s]            = args[k];
    }

    Value res = eval_ast(mem, tab, body_ast, fbase, 0);
    if (mem->jump_active == JUMP_RETURN) {
        res = mem->jump_val;
        mem->jump_val    = val_void();
        mem->jump_active = JUMP_NONE;
    }

    for (uint8_t k = 0; k < param_cnt; ++k) {
        uint16_t abs_s = (uint16_t)(fbase + slots[k]);
        mem->stack[abs_s]            = saved_vals[k];
        mem->slot_locked_type[abs_s] = saved_ltype[k];
        mem->slot_locked_unit[abs_s] = saved_lunit[k];
    }
    mem->sp = saved_sp;
    return res;
}

/* ============================================================================
 * 3. DYNAMIC VECTOR, LIST, STACK & QUEUE (`$vec`) DELEGATE INTRINSICS
 * ============================================================================ */

/*Below section is the C's native element read/write helpers for container delegates @vnew..@vcount*/
/*Construct vec<T>, list<T>, stack<T>, queue<T> element storage and retrieval*/
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

static bool seq_coll_len(const MemArena *mem, Value coll, uint16_t *out_len) {
    uint16_t tid = val_get_type(mem, coll);
    if (tid == mem->vec_type_id && mem->vec_type_id != 0) {
        uint16_t hdr_idx = val_get_pool_idx(coll);
        *out_len = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
        return true;
    }
    if (tid == mem->mat_type_id && mem->mat_type_id != 0) {
        uint16_t tot_len = 0, dims[4] = { 0 };
        delegate_mat_get_info(mem, coll, NULL, &tot_len, NULL, dims);
        *out_len = tot_len;
        return true;
    }
    return false;
}

static Value seq_coll_get(MemArena *mem, const SymTable *tab, Value coll, uint16_t idx) {
    uint16_t tid = val_get_type(mem, coll);
    if (tid == mem->vec_type_id && mem->vec_type_id != 0) {
        uint16_t hdr_idx  = val_get_pool_idx(coll);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
        uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
        uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                            ? tab->types[elem_tid].flat_size : 1;
        return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid,
                                      (uint16_t)(data_idx + idx * stride));
    }
    if (tid == mem->mat_type_id && mem->mat_type_id != 0) {
        uint16_t data_idx = 0, dims[4] = { 0 };
        delegate_mat_get_info(mem, coll, NULL, NULL, &data_idx, dims);
        uint8_t u = val_get_unit(mem, coll);
        return val_make_f64_unit(mem, mem->fast_real_type, u, mem_read_pool(mem, data_idx + idx));
    }
    return val_void();
}

/*Below section is the C's native for operator/function delegates @fcall, @vmap, @vfilter, @vreduce, @vany, @vall, @vfind, @vcount, @vnew, @vfill, @vrange, @vin, @vpush, @vpop, @vshift, @vpeek, @vget, @vset, @vlen, @vcap, @vresize, @vshrink, @vclear*/
/*Construct call(f, ...), map, filter, reduce, fold, any, all, find, count_if, vec/list/stack/queue, .., ..., ..<, <.., in, into, push, pop, enqueue, dequeue, shift, peek, top, front, back*/
Value delegate_eval_vec_intrinsic(MemArena *mem, SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c) {
    /*Below section is the C's native for operator delegate @fcall*/
    /*Construct call(f, a) and call(f, a, b)*/
    if (intr == INTR_FCALL) {
        if (mem->fn_type_id == 0 || val_get_type(mem, a) != mem->fn_type_id) {
            mem_err(mem, "First argument to call() must be a function ($fn)");
            return val_void();
        }
        uint16_t hdr_idx   = val_get_pool_idx(a);
        uint8_t  param_cnt = (uint8_t)mem_read_pool(mem, hdr_idx + 1);
        Value args[2] = { b, c };
        return delegate_call_fn(mem, tab, a, param_cnt, args);
    }

    if (mem->vec_type_id == 0) {
        mem_err(mem, "Dynamic vector type ($vec) is not loaded");
        return val_void();
    }

    /*Below section is the C's native for operator delegates @vmap, @vfilter, @vreduce, @vany, @vall, @vfind, @vcount*/
    /*Construct map(v, f), filter(v, f), reduce(v, f), fold(v, init, f), any(v, f), all(v, f), find(v, f), count_if(v, f)*/
    if (intr >= INTR_VMAP && intr <= INTR_VCOUNT) {
        uint16_t len = 0;
        if (!seq_coll_len(mem, a, &len)) {
            mem_err(mem, "Expected vector ($vec) or matrix ($mat) as first argument");
            return val_void();
        }
        if (intr == INTR_VMAP) {
            if (val_get_type(mem, a) == mem->mat_type_id && mem->mat_type_id != 0) {
                uint8_t  rank = 0;
                uint16_t tot_len = 0, dat_in = 0, dims[4] = { 0 };
                delegate_mat_get_info(mem, a, &rank, &tot_len, &dat_in, dims);
                uint16_t out_tot = 0, out_dat = 0;
                uint16_t out_hdr = mem_mat_alloc_hdr(mem, rank, dims, &out_tot, &out_dat);
                if (mem->has_error) return val_void();
                uint8_t out_u = MEM_UNIT_NONE;
                for (uint16_t i = 0; i < tot_len && !mem->has_error; ++i) {
                    Value item   = seq_coll_get(mem, tab, a, i);
                    Value mapped = delegate_call_fn(mem, tab, b, 1, &item);
                    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                    if (i == 0) out_u = val_get_unit(mem, mapped);
                    mem_write_pool(mem, (uint16_t)(out_dat + i), val_get_f64(mem, mapped));
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, out_hdr, MEM_MAT_HDR_SLOTS), out_u);
            }
            uint16_t out_hdr = mem_vec_alloc_hdr(mem, 0, len);
            if (mem->has_error) return val_void();
            Value out_vec = val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
            for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                Value item   = seq_coll_get(mem, tab, a, i);
                Value mapped = delegate_call_fn(mem, tab, b, 1, &item);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, out_vec, mapped, val_void());
            }
            return out_vec;
        }
        if (intr == INTR_VFILTER) {
            uint16_t out_hdr = mem_vec_alloc_hdr(mem, 0, len);
            if (mem->has_error) return val_void();
            Value out_vec = val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
            for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                Value item = seq_coll_get(mem, tab, a, i);
                Value keep = delegate_call_fn(mem, tab, b, 1, &item);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, keep)) > 1e-15) {
                    (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, out_vec, item, val_void());
                }
            }
            return out_vec;
        }
        if (intr == INTR_VREDUCE) {
            bool is_two_arg = (val_get_type(mem, b) == mem->fn_type_id && mem->fn_type_id != 0);
            if (is_two_arg) {
                if (len == 0) {
                    mem_err(mem, "Cannot reduce an empty collection without an initial value");
                    return val_void();
                }
                Value acc = seq_coll_get(mem, tab, a, 0);
                for (uint16_t i = 1; i < len && !mem->has_error; ++i) {
                    Value item = seq_coll_get(mem, tab, a, i);
                    Value call_args[2] = { acc, item };
                    acc = delegate_call_fn(mem, tab, b, 2, call_args);
                    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                }
                return acc;
            } else {
                Value acc = b;
                for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                    Value item = seq_coll_get(mem, tab, a, i);
                    Value call_args[2] = { acc, item };
                    acc = delegate_call_fn(mem, tab, c, 2, call_args);
                    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                }
                return acc;
            }
        }
        if (intr == INTR_VANY) {
            for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                Value item = seq_coll_get(mem, tab, a, i);
                Value cond = delegate_call_fn(mem, tab, b, 1, &item);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cond)) > 1e-15) {
                    return val_make_f64(mem, mem->fast_real_type, 1.0);
                }
            }
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        if (intr == INTR_VALL) {
            for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                Value item = seq_coll_get(mem, tab, a, i);
                Value cond = delegate_call_fn(mem, tab, b, 1, &item);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cond)) <= 1e-15) {
                    return val_make_f64(mem, mem->fast_real_type, 0.0);
                }
            }
            return val_make_f64(mem, mem->fast_real_type, 1.0);
        }
        if (intr == INTR_VFIND) {
            for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                Value item = seq_coll_get(mem, tab, a, i);
                Value cond = delegate_call_fn(mem, tab, b, 1, &item);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cond)) > 1e-15) {
                    return item;
                }
            }
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        if (intr == INTR_VCOUNT) {
            uint16_t hits = 0;
            for (uint16_t i = 0; i < len && !mem->has_error; ++i) {
                Value item = seq_coll_get(mem, tab, a, i);
                Value cond = delegate_call_fn(mem, tab, b, 1, &item);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, cond)) > 1e-15) {
                    hits++;
                }
            }
            return val_make_f64(mem, mem->fast_real_type, (double)hits);
        }
    }

    /*Below section is the C's native for operator delegate @vnew*/
    /*Construct vec(), vec(cap), list(), stack(), queue()*/
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

    /*Below section is the C's native for operator delegate @vfill*/
    /*Construct vec(count, init), list(item), stack(item), queue(item)*/
    if (intr == INTR_VFILL) {
        int count = 0;
        if (!delegate_val_to_exact_int(mem, tab, a, "vector fill count", &count)) return val_void();
        if (count < 0 || count > MEM_MAX_F64_POOL) {
            mem_err(mem, "Vector fill count out of range (%d)", count);
            return val_void();
        }
        uint16_t elem_tid = val_get_type(mem, b);
        uint8_t  elem_uid = val_get_unit(mem, b);
        uint16_t packed_t = (uint16_t)(((uint32_t)elem_uid << 8u) | ((uint32_t)elem_tid & 0xFFu));
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

    /*Below section is the C's native for operator delegate @vrange*/
    /*Construct a...b (inclusive), a..<b (right-open), a<..b (left-open), a..b (open)*/
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
        int mode = (val_get_type(mem, c) != MEM_TYPE_VOID) ? (int)val_get_f64(mem, c) : 0;
        int step = (lo <= hi) ? 1 : -1;
        if (mode & 2) lo += step; /* bit 1: exclude start */
        if (mode & 1) hi -= step; /* bit 0: exclude end   */

        long long cnt_ll = 0;
        if ((step == 1 && lo <= hi) || (step == -1 && lo >= hi)) {
            cnt_ll = (step == 1)
                     ? ((long long)hi - (long long)lo + 1LL)
                     : ((long long)lo - (long long)hi + 1LL);
        }
        if (cnt_ll > (long long)MEM_MAX_F64_POOL) {
            mem_err(mem, "Range length (%lld) exceeds pool capacity", cnt_ll);
            return val_void();
        }
        uint16_t count    = (uint16_t)cnt_ll;
        uint16_t elem_tid = mem->fast_real_type;
        uint16_t packed_t = (uint16_t)(((uint32_t)elem_uid << 8u) | ((uint32_t)elem_tid & 0xFFu));
        uint16_t hdr_idx  = mem_vec_alloc_hdr(mem, packed_t, count);
        if (mem->has_error) return val_void();
        if (count > 0) {
            if (!mem_vec_reserve(mem, hdr_idx, 1, count)) return val_void();
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            for (uint16_t i = 0; i < count; ++i) {
                mem_write_pool(mem, (uint16_t)(data_idx + i), (double)(lo + (int)i * step));
            }
            mem_write_pool(mem, hdr_idx + 1, (double)count);
        }
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    /*Below section is the C's native for operator delegate @vin*/
    /*Construct item in vec, item in mat, item in a...b*/
    if (intr == INTR_VIN) {
        if (val_get_type(mem, b) == mem->mat_type_id && mem->mat_type_id != 0) {
            uint8_t  rank = 0;
            uint16_t tot_len = 0, data_idx = 0, dims[4] = { 0 };
            if (!delegate_mat_get_info(mem, b, &rank, &tot_len, &data_idx, dims)) {
                return val_void();
            }
            uint8_t mat_u  = val_get_unit(mem, b);
            uint8_t item_u = val_get_unit(mem, a);
            if (val_get_type(mem, a) != mem->fast_real_type || mat_u != item_u) {
                return val_make_f64(mem, mem->fast_real_type, 0.0);
            }
            double target = val_get_f64(mem, a);
            for (uint16_t i = 0; i < tot_len; ++i) {
                if (fabs(mem_read_pool(mem, (uint16_t)(data_idx + i)) - target) <= 1e-12) {
                    return val_make_f64(mem, mem->fast_real_type, 1.0);
                }
            }
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        if (val_get_type(mem, b) != mem->vec_type_id) {
            mem_err(mem, "Right operand of 'in' must be a range, vector ($vec), matrix ($mat), or unit");
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
        /*Below section is the C's native for operator delegates @vlen, @vcap, @vclear, @vshrink*/
        /*Construct len(v), cap(v), clear(v), shrink(v)*/
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

        /*Below section is the C's native for operator delegate @vpush*/
        /*Construct push(v, x), enqueue(q, x), and x into v*/
        case INTR_VPUSH: {
            uint16_t item_tid = val_get_type(mem, b);
            uint8_t  item_uid = val_get_unit(mem, b);
            if (elem_tid == 0) {
                if (elem_uid != 0 && elem_uid != item_uid) {
                    char exp_u[32], got_u[32];
                    sym_unit_name(tab, mem, elem_uid, exp_u, sizeof(exp_u));
                    sym_unit_name(tab, mem, item_uid, got_u, sizeof(got_u));
                    mem_err(mem, "Vector unit mismatch: cannot push '[%s]' into vec<[%s]>", got_u, exp_u);
                    return val_void();
                }
                elem_tid = item_tid;
                elem_uid = item_uid;
                stride   = (elem_tid < tab->type_count) ? tab->types[elem_tid].flat_size : 1;
                uint16_t packed_t = (uint16_t)(((uint32_t)elem_uid << 8u) | ((uint32_t)elem_tid & 0xFFu));
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

        /*Below section is the C's native for operator delegate @vpop*/
        /*Construct pop(v) (LIFO stack/vector pop from back)*/
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

        /*Below section is the C's native for operator delegate @vshift*/
        /*Construct dequeue(q), shift(q), pop_front(q) (FIFO queue pop from front)*/
        case INTR_VSHIFT: {
            if (len == 0) {
                mem_err(mem, "Cannot dequeue/shift from an empty vector");
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            Value first = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, data_idx);
            if (elem_tid != 0 && elem_tid < tab->type_count &&
                !tab->types[elem_tid].is_consuming &&
                elem_tid != mem->vec_type_id && elem_tid != mem->mat_type_id &&
                elem_tid != mem->fn_type_id) {
                first = val_clone_comp(mem, first, stride);
                if (mem->has_error) return val_void();
            }
            uint16_t new_len = (uint16_t)(len - 1);
            if (new_len > 0) {
                memmove(&mem->f64_pool[data_idx],
                        &mem->f64_pool[data_idx + stride],
                        (size_t)(new_len * stride) * sizeof(double));
            }
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return first;
        }

        /*Below section is the C's native for operator delegate @vpeek*/
        /*Construct peek(s), top(s), back(s) (mode 0) and front(q) (mode 1)*/
        case INTR_VPEEK: {
            if (len == 0) {
                mem_err(mem, "Cannot peek into an empty vector");
                return val_void();
            }
            int mode = (val_get_type(mem, b) != MEM_TYPE_VOID) ? (int)val_get_f64(mem, b) : 0;
            uint16_t idx = (mode == 1) ? 0u : (uint16_t)(len - 1);
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, (uint16_t)(data_idx + idx * stride));
        }

        /*Below section is the C's native for operator delegates @vget and @vset*/
        /*Construct at(v, i), set(v, i, x)*/
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

        /*Below section is the C's native for operator delegate @vresize*/
        /*Construct resize(v, new_len)*/
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

/*Below section is the C's native for operator/function delegates @mzero, @mones, @meye, @mfill, @mrows, @mcols, @mrank, @mlen, @mshape, @madd, @msub, @mscal, @mmul, @mtrans, @mtrace, @mdet, @minv*/
/*Construct mat_zeros, mat_ones, mat_eye, mat_fill, mat_rows, mat_cols, mat_rank, mat_len, mat_shape, A + B, A - B, s * A, A * B, transpose(A), trace(A), det(A), inv(A)*/
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
        dims[i] = (i < rank) ? (uint16_t)mem_read_pool(mem, (uint16_t)(hdr_idx + 4u + i)) : 0;
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
            uint16_t N          = da[0];
            uint16_t saved_pool = mem->pool_top;
            uint16_t tmp_idx    = mem_alloc_pool(mem, (uint16_t)(N * N));
            if (tmp_idx == MEM_NO_IDX || mem->has_error) return val_void();
            double *tmp = &mem->f64_pool[tmp_idx];
            for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                tmp[i] = mem_read_pool(mem, (uint16_t)(dat_a + i));
            }
            double det_val = 1.0;
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t piv = i;
                double max_v = fabs(tmp[i * N + i]);
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double v = fabs(tmp[r * N + i]);
                    if (v > max_v) { max_v = v; piv = r; }
                }
                if (max_v <= 1e-14) {
                    mem->pool_top = saved_pool;
                    return val_make_f64(mem, mem->fast_real_type, 0.0);
                }
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
            mem->pool_top = saved_pool;
            return val_make_f64(mem, mem->fast_real_type, det_val);
        }
        case INTR_MINV: {
            if (ra != 2 || da[0] != da[1] || da[0] == 0 || da[0] > 16) {
                mem_err(mem, "inv() requires a square 2D matrix (1x1 up to 16x16)");
                return val_void();
            }
            uint16_t N          = da[0];
            uint16_t out_dims[4] = { N, N, 0, 0 };
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
            if (hdr_out == MEM_NO_IDX || mem->has_error) return val_void();

            uint16_t saved_pool = mem->pool_top;
            uint16_t aug_idx    = mem_alloc_pool(mem, (uint16_t)(2u * N * N));
            if (aug_idx == MEM_NO_IDX || mem->has_error) return val_void();
            double *aug = &mem->f64_pool[aug_idx];

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
                    mem->pool_top = saved_pool;
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
            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < N; ++col) {
                    double v = aug[r * (2 * N) + N + col];
                    if (fabs(v) < 1e-15) v = 0.0;
                    mem_write_pool(mem, (uint16_t)(dat_out + r * N + col), v);
                }
            }
            mem->pool_top = saved_pool;
            return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
        }
        default:
            return val_void();
    }
}

/* ============================================================================
 * 4b. STRING CONVERSION DELEGATES `@tostr` (POSTFIX `_`) & `@fromstr` (PREFIX `_`)
 * ============================================================================ */

/*Below section is the C's native for operator delegates @tostr (postfix "_") and @fromstr (prefix "_")*/
/*Construct x_ (plain string representation of any value/type) and _s (convert plain string into most appropriate type)*/
static void delegate_format_struct_at(const MemArena *mem, const SymTable *tab,
                                      uint16_t type_id, uint16_t pool_idx,
                                      uint8_t depth, char *buf, size_t cap);

static void delegate_format_mat_slice(const MemArena *mem, uint16_t data_idx,
                                      const uint16_t *dims, uint8_t rank, uint8_t axis,
                                      char *buf, size_t cap, size_t *used) {
    if (*used < cap) *used += (size_t)snprintf(buf + *used, cap - *used, "[");
    if (axis + 1 >= rank) {
        uint16_t n = dims[axis];
        for (uint16_t i = 0; i < n && *used < cap; ++i) {
            double v = mem_read_pool(mem, (uint16_t)(data_idx + i));
            if (v == 0.0) v = 0.0;
            *used += (size_t)snprintf(buf + *used, cap - *used, "%s%.6g",
                                      (i > 0 ? ", " : ""), v);
        }
    } else {
        uint16_t stride = 1;
        for (uint8_t a = (uint8_t)(axis + 1); a < rank; ++a) {
            stride = (uint16_t)(stride * dims[a]);
        }
        uint16_t n = dims[axis];
        for (uint16_t i = 0; i < n && *used < cap; ++i) {
            if (i > 0 && *used < cap) {
                *used += (size_t)snprintf(buf + *used, cap - *used, ", ");
            }
            delegate_format_mat_slice(mem, (uint16_t)(data_idx + i * stride),
                                      dims, rank, (uint8_t)(axis + 1), buf, cap, used);
        }
    }
    if (*used < cap) *used += (size_t)snprintf(buf + *used, cap - *used, "]");
}

static void delegate_format_vec_at(const MemArena *mem, const SymTable *tab,
                                   uint16_t hdr_idx, uint8_t depth,
                                   char *buf, size_t cap) {
    if (depth > 16) {
        snprintf(buf, cap, "[...]");
        return;
    }
    uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
    uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
    uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
    uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
    uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                        ? tab->types[elem_tid].flat_size : 1;

    size_t used = (size_t)snprintf(buf, cap, "[");
    for (uint16_t i = 0; i < len && used < cap; ++i) {
        uint16_t elem_pidx = (uint16_t)(data_idx + i * stride);
        char elem_str[96];
        if (elem_tid == mem->str_type_id && elem_tid != 0) {
            uint32_t packed = (uint32_t)mem_read_pool(mem, elem_pidx);
            StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
            snprintf(elem_str, sizeof(elem_str), "\"%.*s\"",
                     (int)s.len, mem_str_ptr(mem, s));
        } else if (elem_tid == mem->vec_type_id && elem_tid != 0) {
            delegate_format_vec_at(mem, tab, elem_pidx, (uint8_t)(depth + 1), elem_str, sizeof(elem_str));
        } else if (elem_tid > 0 && elem_tid < tab->type_count &&
                   !tab->types[elem_tid].is_consuming) {
            delegate_format_struct_at(mem, tab, elem_tid, elem_pidx, (uint8_t)(depth + 1), elem_str, sizeof(elem_str));
        } else {
            snprintf(elem_str, sizeof(elem_str), "%.6g", mem_read_pool(mem, elem_pidx));
        }
        used += (size_t)snprintf(buf + used, cap - used, "%s%s",
                                 (i > 0 ? ", " : ""), elem_str);
    }
    if (used < cap) snprintf(buf + used, cap - used, "]");
}

static void delegate_format_struct_at(const MemArena *mem, const SymTable *tab,
                                      uint16_t type_id, uint16_t pool_idx,
                                      uint8_t depth, char *buf, size_t cap) {
    if (depth > 16) {
        snprintf(buf, cap, "<...>");
        return;
    }
    if (type_id == mem->vec_type_id && type_id != 0) {
        delegate_format_vec_at(mem, tab, pool_idx, depth, buf, cap);
        return;
    }
    if (type_id == mem->mat_type_id && type_id != 0) {
        uint8_t  rank     = (uint8_t)mem_read_pool(mem, pool_idx + 0);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, pool_idx + 2);
        uint16_t dims[4]  = { 0, 0, 0, 0 };
        for (uint8_t i = 0; i < 4; ++i) {
            dims[i] = (i < rank) ? (uint16_t)mem_read_pool(mem, (uint16_t)(pool_idx + 4u + i)) : 0;
        }
        if (rank == 0) {
            snprintf(buf, cap, "[]");
            return;
        }
        size_t used = 0;
        delegate_format_mat_slice(mem, data_idx, dims, rank, 0, buf, cap, &used);
        return;
    }
    if (type_id == mem->fn_type_id && type_id != 0) {
        unsigned pcnt = (unsigned)mem_read_pool(mem, pool_idx + 1);
        snprintf(buf, cap, "<fn/%u>", pcnt);
        return;
    }

    const TypeDef *td = &tab->types[type_id];
    char tname[32];
    sym_type_name(tab, mem, type_id, tname, sizeof(tname));

    if (strcmp(tname, "complex") == 0 && td->flat_size == 2) {
        double re = mem_read_pool(mem, pool_idx);
        double im = mem_read_pool(mem, pool_idx + 1);
        if (im >= 0.0) {
            snprintf(buf, cap, "%.6g + %.6gi", re, im);
        } else {
            snprintf(buf, cap, "%.6g - %.6gi", re, -im);
        }
        return;
    }

    size_t used = (size_t)snprintf(buf, cap, "%s(", tname);
    for (uint8_t i = 0; i < td->field_count && used < cap; ++i) {
        const FieldDef *fd = &tab->fields[td->start_idx + i];
        char f_str[64];
        if (!tab->types[fd->type_id].is_consuming) {
            delegate_format_struct_at(mem, tab, fd->type_id, (uint16_t)(pool_idx + fd->flat_offset),
                                      (uint8_t)(depth + 1), f_str, sizeof(f_str));
        } else {
            snprintf(f_str, sizeof(f_str), "%.6g", mem_read_pool(mem, (uint16_t)(pool_idx + fd->flat_offset)));
        }
        used += (size_t)snprintf(buf + used, cap - used, "%s%.*s=%s",
                                 (i > 0 ? ", " : ""),
                                 (int)fd->name.len, mem_str_ptr(mem, fd->name),
                                 f_str);
    }
    if (used < cap) snprintf(buf + used, cap - used, ")");
}

static Value delegate_eval_tostr(MemArena *mem, const SymTable *tab, Value val) {
    uint16_t tid = val_get_type(mem, val);
    if (tid == mem->str_type_id && tid != 0) {
        return val;
    }
    char buf[256];
    if (tid == 0 || tid >= tab->type_count) {
        snprintf(buf, sizeof(buf), "<void>");
        return val_from_cstr(mem, buf);
    }
    const TypeDef *td = &tab->types[tid];
    if (td->is_consuming) {
        if (td->parser_intr == INTR_PARSE_F64_TRIM1) {
            snprintf(buf, sizeof(buf), "%.6gi", val_get_f64(mem, val));
        } else {
            snprintf(buf, sizeof(buf), "%.10g", val_get_f64(mem, val));
        }
    } else {
        delegate_format_struct_at(mem, tab, tid, val_get_pool_idx(val), 0, buf, sizeof(buf));
    }
    return val_from_cstr(mem, buf);
}

static Value delegate_eval_fromstr(MemArena *mem, SymTable *tab, Value str_val) {
    if (mem->str_type_id == 0 || val_get_type(mem, str_val) != mem->str_type_id) {
        mem_err(mem, "No compatible type found");
        return val_void();
    }
    StrSlice raw_slice = val_get_str(mem, str_val);
    char raw_buf[512];
    uint16_t copy_len = (raw_slice.len < sizeof(raw_buf) - 1)
                        ? raw_slice.len : (uint16_t)(sizeof(raw_buf) - 1);
    if (copy_len > 0) {
        memcpy(raw_buf, mem_str_ptr(mem, raw_slice), copy_len);
    }
    raw_buf[copy_len] = '\0';

    /* Trim leading and trailing whitespace */
    uint16_t start = 0;
    while (start < copy_len &&
           (raw_buf[start] == ' ' || raw_buf[start] == '\t' ||
            raw_buf[start] == '\r' || raw_buf[start] == '\n')) {
        start++;
    }
    uint16_t end = copy_len;
    while (end > start &&
           (raw_buf[end - 1] == ' ' || raw_buf[end - 1] == '\t' ||
            raw_buf[end - 1] == '\r' || raw_buf[end - 1] == '\n')) {
        end--;
    }

    const char *trimmed = raw_buf + start;
    uint16_t    tlen    = (uint16_t)(end - start);
    if (tlen == 0 || trimmed[0] == '"') {
        mem_err(mem, "No compatible type found for '%.*s'", (int)copy_len, raw_buf);
        return val_void();
    }
    for (uint16_t i = 0; i < tlen; ++i) {
        if (trimmed[i] == '\n') {
            mem_err(mem, "No compatible type found for '%.*s'", (int)copy_len, raw_buf);
            return val_void();
        }
    }

    /* Normalize struct `field=` labels inside `(...)` (e.g. `vec2(x=3, y=4)` -> `vec2(3, 4)`)
       and convert 1D bracketed lists `[a, b, ...]` (without `;` or nested `[` at depth 1) to `list(...)`
       when they represent vectors. */
    char norm_buf[512];
    uint16_t nlen = 0;
    int  pdepth = 0;
    bool at_arg_start = false;
    bool rewrite_1d_vec = false;

    if (tlen >= 2 && trimmed[0] == '[' && trimmed[tlen - 1] == ']') {
        bool has_semi_or_sub = false;
        int  bdepth = 0;
        for (uint16_t i = 1; i + 1 < tlen; ++i) {
            char ch = trimmed[i];
            if (ch == '(' || ch == '{') bdepth++;
            else if (ch == ')' || ch == '}') bdepth--;
            else if (ch == '[' && bdepth == 0) { has_semi_or_sub = true; break; }
            else if (ch == ';' && bdepth == 0) { has_semi_or_sub = true; break; }
            else if (i + 3 <= tlen && bdepth == 0 && memcmp(trimmed + i, "for", 3) == 0) {
                has_semi_or_sub = true;
                break;
            }
        }
        if (!has_semi_or_sub) {
            rewrite_1d_vec = true;
        }
    }

    uint16_t i = 0;
    uint16_t limit = tlen;
    if (rewrite_1d_vec) {
        /* Empty `[]` -> `vec()`, non-empty `[a, b]` -> `list(a, b)` */
        uint16_t inner_p = 1;
        while (inner_p + 1 < tlen && (trimmed[inner_p] == ' ' || trimmed[inner_p] == '\t')) inner_p++;
        if (inner_p + 1 >= tlen) {
            memcpy(norm_buf, "vec()", 5);
            nlen = 5;
            i = tlen;
        } else {
            memcpy(norm_buf, "list(", 5);
            nlen = 5;
            pdepth = 1;
            at_arg_start = true;
            i = 1;
            limit = (uint16_t)(tlen - 1);
        }
    }

    while (i < limit && (size_t)nlen + 2u < sizeof(norm_buf)) {
        char c = trimmed[i];
        if (c == '(') {
            pdepth++;
            at_arg_start = true;
            norm_buf[nlen++] = c;
            i++;
            continue;
        }
        if (c == ')') {
            if (pdepth > 0) pdepth--;
            at_arg_start = false;
            norm_buf[nlen++] = c;
            i++;
            continue;
        }
        if (c == ',' && pdepth > 0) {
            at_arg_start = true;
            norm_buf[nlen++] = c;
            i++;
            continue;
        }
        if (pdepth > 0 && at_arg_start) {
            while (i < limit && (trimmed[i] == ' ' || trimmed[i] == '\t')) i++;
            if (i >= limit) break;
            unsigned char uc = (unsigned char)trimmed[i];
            if ((uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z')) {
                uint16_t scan = i;
                while (scan < limit) {
                    unsigned char sc = (unsigned char)trimmed[scan];
                    if ((sc >= 'a' && sc <= 'z') || (sc >= 'A' && sc <= 'Z') ||
                        (sc >= '0' && sc <= '9') || sc == '_') {
                        scan++;
                    } else {
                        break;
                    }
                }
                uint16_t ws = scan;
                while (ws < limit && (trimmed[ws] == ' ' || trimmed[ws] == '\t')) ws++;
                if (ws < limit && trimmed[ws] == '=' &&
                    !(ws + 1 < limit && (trimmed[ws + 1] == '=' || trimmed[ws + 1] == '>'))) {
                    i = (uint16_t)(ws + 1);
                    continue;
                }
            }
            at_arg_start = false;
        }
        norm_buf[nlen++] = trimmed[i++];
    }
    if (rewrite_1d_vec && nlen > 0 && norm_buf[0] == 'l' && (size_t)nlen + 1u < sizeof(norm_buf)) {
        norm_buf[nlen++] = ')';
    }
    norm_buf[nlen] = '\0';

    uint16_t saved_sp = mem->sp;
    Parser P;
    parser_init(&P, mem, tab, norm_buf, nlen, 0, false, false);
    lex_next(&P.lex);

    uint16_t root = MEM_NO_IDX;
    if (!mem->has_error && P.lex.cur.kind != TOK_EOF) {
        root = parser_parse_expr(&P, 0);
    }

    if (mem->has_error || root == MEM_NO_IDX ||
        P.lex.cur.kind != TOK_EOF || P.local_count != 0) {
        mem->sp = saved_sp;
        mem_clear_err(mem);
        mem_err(mem, "No compatible type found for '%.*s'", (int)copy_len, raw_buf);
        return val_void();
    }

    Value res = eval_ast(mem, tab, root, saved_sp, 0);
    uint16_t rtid = val_get_type(mem, res);
    if (mem->has_error || mem->jump_active != JUMP_NONE ||
        rtid == MEM_TYPE_VOID || rtid == mem->str_type_id) {
        mem->sp          = saved_sp;
        mem->jump_active = JUMP_NONE;
        mem_clear_err(mem);
        mem_err(mem, "No compatible type found for '%.*s'", (int)copy_len, raw_buf);
        return val_void();
    }
    return res;
}

/* ============================================================================
 * 5. SCALAR ARITHMETIC, BOOLEAN LOGIC & TRANSCENDENTAL DELEGATES
 * ============================================================================ */

/*Below section is the C's native top-level dispatcher for operator/function delegates @add..@err, @tostr, @fromstr*/
/*Construct +, -, *, /, %, ^, √, sin..tanh, ==..~, &&..nand, type_of, unit_of, x_, _s, error*/
Value delegate_eval_intrinsic(MemArena *mem, SymTable *tab,
                              IntrinsicId intr, Value a, Value b, Value c, uint8_t ret_type) {
    if (intr >= INTR_VNEW && intr <= INTR_VCOUNT) {
        return delegate_eval_vec_intrinsic(mem, tab, intr, a, b, c);
    }
    if (intr >= INTR_MZERO && intr <= INTR_MINV) {
        return delegate_eval_mat_intrinsic(mem, tab, intr, a, b, c);
    }

    /*Below section is the C's native for operator delegates @type_name, @unit_name, @tostr, @fromstr, @err*/
    /*Construct type_of(x), unit_of(x), x_ (postfix "_"), _s (prefix "_"), error(msg)*/
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
    if (intr == INTR_TOSTR) {
        return delegate_eval_tostr(mem, tab, a);
    }
    if (intr == INTR_FROMSTR) {
        return delegate_eval_fromstr(mem, tab, a);
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
        /*Below section is the C's native for operator delegates @add, @sub, @mul, @div, @mod, @neg, @abs, @floor, @ceil, @pow, @sqrt, @cbrt, @exp, @ln, @log10*/
        /*Construct +, -, *, /, %, unary -, abs, floor, ceil, ^, √ / sqrt, cbrt, exp, ln, log10*/
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

        /*Below section is the C's native for operator delegates @sin, @cos, @tan, @asin, @acos, @atan, @atan2, @sinh, @cosh, @tanh*/
        /*Construct sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh*/
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

        /*Below section is the C's native for operator delegates @eq, @ne, @lt, @le, @gt, @ge, @approx, @land, @lor, @cnot, @lnot, @lxor, @lnor, @lnand, @raw*/
        /*Construct ==, !=, <, <=, >, >=, ~, && / and, || / or, !, not, xor / ^^, nor, nand, i (iota)*/
        case INTR_EQ:
            if (val_get_type(mem, a) == mem->str_type_id && val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0) {
                res = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, b)) ? 1.0 : 0.0;
                break;
            }
            if (!delegate_check_additive_unit_compat(mem, tab, "==", 2, a, b, NULL)) return val_void();
            res = (fabs(da - db) <= 1e-12) ? 1.0 : 0.0;
            break;
        case INTR_NE:
            if (val_get_type(mem, a) == mem->str_type_id && val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0) {
                res = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, b)) ? 0.0 : 1.0;
                break;
            }
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

/*Below section is the C's native for operator/function delegates @calc_diff, @calc_diff2, @calc_diffn, @calc_integ, @calc_lim, @calc_lim_l, @calc_lim_r, @calc_sigma, @calc_prod, @calc_solve, @calc_fmin, @calc_fmax*/
/*Construct diff, diff2, diff_n, integrate, ntg, lim, lim_left, lim_right, sigma, prod, solve, fmin, fmax*/
static double eval_calc_point(MemArena *mem, SymTable *tab,
                              uint8_t var_slot, uint16_t expr_ast,
                              uint16_t frame_base, uint8_t cur_ret_type, double x_val) {
    mem_set_slot(mem, frame_base, var_slot, val_make_f64(mem, mem->fast_real_type, x_val));
    Value v = eval_ast(mem, tab, expr_ast, frame_base, cur_ret_type);
    return val_get_f64(mem, v);
}

static double eval_solve_residual(MemArena *mem, SymTable *tab,
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

Value delegate_eval_calculus_meta(MemArena *mem, SymTable *tab,
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
