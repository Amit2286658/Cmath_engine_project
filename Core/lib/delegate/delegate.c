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
#include "../hardcode/hardcode.h"
#include <math.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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
    { "map_type",      INTR_MAP_TYPE },
    { "complex_type",  INTR_COMPLEX_TYPE },
    { "any_type",      INTR_ANY_TYPE },
    { "none_unit",     INTR_NONE_UNIT },
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
    { "do_while",      INTR_DO_WHILE },
    { "if",            INTR_IF },
    { "or_branch",     INTR_OR_BRANCH },
    { "then_branch",   INTR_OR_BRANCH },
    { "else_branch",   INTR_ELSE_BRANCH },
    { "switch_block",  INTR_SWITCH_BLOCK },
    { "case_branch",   INTR_CASE_BRANCH },
    { "for_kw",        INTR_FOR_KW },
    { "ternary",       INTR_TERNARY },
    { "colon",         INTR_COLON },
    { "jump_tag",      INTR_JUMP_TAG },
    { "run_tag",       INTR_RUN_TAG },
    { "break_loop",    INTR_BREAK_LOOP },
    { "continue_loop", INTR_CONTINUE_LOOP },
    { "return_fn",     INTR_RETURN_FN },
    { "lambda",        INTR_LAMBDA },
    { "pipe",          INTR_PIPE },
    { "cand",          INTR_CAND },
    { "cor",           INTR_COR },
    { "cnot",          INTR_CNOT },
    { "sym_diff",         INTR_SYM_DIFF },
    { "sym_integ",        INTR_SYM_INTEG },
    { "sym_solve",        INTR_SYM_SOLVE },
    { "sym_simplify",     INTR_SYM_SIMPLIFY },
    { "sym_eval",         INTR_SYM_EVAL },
    { "sym_taylor",       INTR_SYM_TAYLOR },
    { "field",         INTR_FIELD },
    { "vnew",          INTR_VNEW },
    { "vfill",         INTR_VFILL },
    { "vpush",         INTR_VPUSH },
    { "vpop",          INTR_VPOP },
    { "vshift",        INTR_VSHIFT },
    { "vget",          INTR_VGET },
    { "vset",          INTR_VSET },
    { "vlen",          INTR_VLEN },
    { "vcap",          INTR_VCAP },
    { "vresize",       INTR_VRESIZE },
    { "vshrink",       INTR_VSHRINK },
    { "vclear",        INTR_VCLEAR },
    { "vrange",        INTR_VRANGE },
    { "vrange_step",   INTR_VRANGE_STEP },
    { "vin",           INTR_VIN },
    { "fcall",         INTR_FCALL },
    { "vsort",         INTR_VSORT },
    { "map_new",       INTR_MAP_NEW },
    { "map_set",       INTR_MAP_SET },
    { "map_get",       INTR_MAP_GET },
    { "map_has",       INTR_MAP_HAS },
    { "map_del",       INTR_MAP_DEL },
    { "mrank",         INTR_MRANK },
    { "mlen",          INTR_MLEN },
    { "mdim",          INTR_MDIM },
    { "mmul",          INTR_MMUL },
    { "mdet",          INTR_MDET },
    { "minv",          INTR_MINV },
    { "mlinsolve",     INTR_MLINSOLVE },
    { "mreshape",      INTR_MRESHAPE },
    { "meigvals",      INTR_MEIGVALS },
    { "meigvecs",      INTR_MEIGVECS },
    { "msvd",          INTR_MSVD },
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
    { "erf",           INTR_ERF },
    { "erfc",          INTR_ERFC },
    { "gamma",         INTR_GAMMA },
    { "lgamma",        INTR_LGAMMA },
    { "eq",            INTR_EQ },
    { "ne",            INTR_NE },
    { "lt",            INTR_LT },
    { "le",            INTR_LE },
    { "gt",            INTR_GT },
    { "ge",            INTR_GE },
    { "lnot",          INTR_LNOT },
    { "band",          INTR_BAND },
    { "bor",           INTR_BOR },
    { "bxor",          INTR_BXOR },
    { "bnot",          INTR_BNOT },
    { "bshl",          INTR_BSHL },
    { "bshr",          INTR_BSHR },
    { "raw",           INTR_RAW },
    { "type_name",     INTR_TYPE_NAME },
    { "unit_name",     INTR_UNIT_NAME },
    { "tostr",         INTR_TOSTR },
    { "fromstr",       INTR_FROMSTR },
    { "str_fmt",       INTR_STR_FMT },
    { "regex_match",   INTR_REGEX_MATCH },
    { "regex_find",    INTR_REGEX_FIND },
    { "regex_find_all",INTR_REGEX_FINDALL },
    { "regex_replace", INTR_REGEX_REPLACE },
    { "regex_split",   INTR_REGEX_SPLIT },
    { "gc_compact",    INTR_GC_COMPACT },
    { "pool_used",     INTR_POOL_USED },
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

uint32_t delegate_intrinsic_table_hash(void) {
    uint32_t h = 2166136261u;
    for (int i = 0; k_delegate_intrinsics[i].name != NULL; ++i) {
        const char *p = k_delegate_intrinsics[i].name;
        while (*p) {
            h ^= (uint8_t)(*p++);
            h *= 16777619u;
        }
        uint32_t id = (uint32_t)k_delegate_intrinsics[i].id;
        h ^= (id & 0xFFu);         h *= 16777619u;
        h ^= ((id >> 8u) & 0xFFu); h *= 16777619u;
    }
    h ^= (uint32_t)INTR_COUNT;
    h *= 16777619u;
    return (h == 0) ? 1u : h;
}

bool delegate_is_meta_intrinsic(IntrinsicId intr) {
    switch (intr) {
        case INTR_SEQ:
        case INTR_STORE:
        case INTR_WHILE:
        case INTR_DO_WHILE:
        case INTR_IF:
        case INTR_OR_BRANCH:
        case INTR_ELSE_BRANCH:
        case INTR_SWITCH_BLOCK:
        case INTR_CASE_BRANCH:
        case INTR_FOR_KW:
        case INTR_TERNARY:
        case INTR_COLON:
        case INTR_JUMP_TAG:
        case INTR_RUN_TAG:
        case INTR_BREAK_LOOP:
        case INTR_CONTINUE_LOOP:
        case INTR_RETURN_FN:
        case INTR_LAMBDA:
        case INTR_PIPE:
        case INTR_CAND:
        case INTR_COR:
        case INTR_CNOT:
        case INTR_SYM_DIFF:
        case INTR_SYM_INTEG:
        case INTR_SYM_SOLVE:
        case INTR_SYM_SIMPLIFY:
        case INTR_SYM_EVAL:
        case INTR_SYM_TAYLOR:
        case INTR_FIELD:
        case INTR_FCALL:
            return true;
        default:
            return false;
    }
}

bool delegate_is_sym_meta(uint8_t mtag) {
    switch ((IntrinsicId)mtag) {
        case INTR_SYM_DIFF:
        case INTR_SYM_INTEG:
        case INTR_SYM_SOLVE:
        case INTR_SYM_SIMPLIFY:
        case INTR_SYM_EVAL:
        case INTR_SYM_TAYLOR:
            return true;
        default:
            return false;
    }
}

bool delegate_is_rel_intrinsic(IntrinsicId intr) {
    switch (intr) {
        case INTR_EQ:
        case INTR_NE:
        case INTR_LT:
        case INTR_LE:
        case INTR_GT:
        case INTR_GE:
            return true;
        default:
            return false;
    }
}

bool delegate_is_map_intrinsic(IntrinsicId intr) {
    switch (intr) {
        case INTR_MAP_NEW:
        case INTR_MAP_SET:
        case INTR_MAP_GET:
        case INTR_MAP_HAS:
        case INTR_MAP_DEL:
            return true;
        default:
            return false;
    }
}

bool delegate_is_vec_or_map_intrinsic(IntrinsicId intr) {
    switch (intr) {
        case INTR_VNEW:
        case INTR_VFILL:
        case INTR_VPUSH:
        case INTR_VPOP:
        case INTR_VSHIFT:
        case INTR_VGET:
        case INTR_VSET:
        case INTR_VLEN:
        case INTR_VCAP:
        case INTR_VRESIZE:
        case INTR_VSHRINK:
        case INTR_VCLEAR:
        case INTR_VRANGE:
        case INTR_VRANGE_STEP:
        case INTR_VIN:
        case INTR_FCALL:
        case INTR_VSORT:
            return true;
        default:
            return delegate_is_map_intrinsic(intr);
    }
}

bool delegate_is_mat_intrinsic(IntrinsicId intr) {
    switch (intr) {
        case INTR_MRANK:
        case INTR_MLEN:
        case INTR_MDIM:
        case INTR_MMUL:
        case INTR_MDET:
        case INTR_MINV:
        case INTR_MLINSOLVE:
        case INTR_MRESHAPE:
        case INTR_MEIGVALS:
        case INTR_MEIGVECS:
        case INTR_MSVD:
            return true;
        default:
            return false;
    }
}

/*Below section is the C's native token constructor for consuming type delegates @f64, @f64_trim1, @i64, @str_slice*/
/*Construct $real (~ \d+ ("_" \d+)* ... => @f64), $iota (=> @f64_trim1), and $str (=> @str_slice)*/
Value delegate_construct_token(MemArena *mem, const SymTable *tab,
                               uint16_t type_id, const char *src,
                               uint16_t off, uint16_t len) {
    const TypeDef *td = &tab->types[type_id];
    char tmp[512];
    uint16_t copy_len = 0;
    if (td->parser_intr != INTR_PARSE_STR_SLICE) {
        for (uint16_t i = 0; i < len && copy_len < sizeof(tmp) - 1; ++i) {
            char ch = src[off + i];
            if (ch == '_') continue;
            tmp[copy_len++] = ch;
        }
        tmp[copy_len] = '\0';
    }

    switch (td->parser_intr) {
        case INTR_PARSE_F64: {
            if (copy_len >= 3 && tmp[0] == '0' && (tmp[1] == 'x' || tmp[1] == 'X')) {
                unsigned long long u = strtoull(tmp + 2, NULL, 16);
                return val_make_f64(mem, type_id, (double)u);
            }
            if (copy_len >= 3 && tmp[0] == '0' && (tmp[1] == 'b' || tmp[1] == 'B')) {
                unsigned long long u = strtoull(tmp + 2, NULL, 2);
                return val_make_f64(mem, type_id, (double)u);
            }
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
            char stack_ubuf[1024];
            char *ubuf = stack_ubuf;
            size_t ucap = sizeof(stack_ubuf);
            if ((size_t)s_len + 2u > ucap) {
                ucap = (size_t)s_len + 2u;
                ubuf = (char *)malloc(ucap);
                if (!ubuf) {
                    mem_err(mem, "Out of memory constructing string literal");
                    return val_void();
                }
            }
            uint16_t ulen = 0;
            for (uint16_t k = 0; k < s_len && (size_t)ulen + 1u < ucap; ++k) {
                char c = src[s_off + k];
                if (c == '\\' && k + 1 < s_len) {
                    char nx = src[s_off + k + 1];
                    if (nx == '\\') { ubuf[ulen++] = '\\'; k++; continue; }
                    if (nx == 'n')  { ubuf[ulen++] = '\n'; k++; continue; }
                    if (nx == 't')  { ubuf[ulen++] = '\t'; k++; continue; }
                    if (nx == 'r')  { ubuf[ulen++] = '\r'; k++; continue; }
                    if (nx == '"')  { ubuf[ulen++] = '"';  k++; continue; }
                }
                ubuf[ulen++] = c;
            }
            StrSlice slice = mem_intern_str(mem, ubuf, ulen);
            if (ubuf != stack_ubuf) free(ubuf);
            return val_make_str(mem, type_id, slice);
        }
        default:
            return val_void();
    }
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
    return val_get_f64(mem, v) == 0.0;
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

/*Below section is the C's native 7D compound SI dimensional exponent algebra for @mul, @div, @pow, @sqrt, @cbrt (driven by @dim in Library/units.math)*/
/*Construct Compound unit formation & cancellation on *, /, ^, √/sqrt, cbrt across all 7 SI base units (m^L * kg^M * s^T * A^I * K^Th * mol^N * cd^J)*/
static bool find_si_unit_dim(const SymTable *tab, uint8_t uid, UnitDimDef *out_dim) {
    if (uid == MEM_UNIT_NONE) {
        memset(out_dim, 0, sizeof(*out_dim));
        out_dim->has_dim = 1;
        return true;
    }
    if (!tab || uid >= tab->unit_count || !tab->unit_dims[uid].has_dim) return false;
    *out_dim = tab->unit_dims[uid];
    return true;
}

static uint8_t lookup_si_dim_unit_id(const SymTable *tab, const int exps[7]) {
    bool all_zero = true;
    for (int d = 0; d < 7; ++d) {
        if (exps[d] != 0) { all_zero = false; break; }
    }
    if (all_zero) return MEM_UNIT_NONE;
    if (!tab) return MEM_UNIT_NONE;
    for (uint8_t u = 1; u < tab->unit_count; ++u) {
        if (!tab->unit_dims[u].has_dim) continue;
        bool match = true;
        for (int d = 0; d < 7; ++d) {
            if ((int)tab->unit_dims[u].exp[d] != exps[d]) {
                match = false;
                break;
            }
        }
        if (match) return u;
    }
    return MEM_UNIT_NONE;
}

uint8_t delegate_resolve_mul_unit(const MemArena *mem, const SymTable *tab,
                                  uint8_t ua, uint8_t ub) {
    (void)mem;
    if (ua == MEM_UNIT_NONE) return ub;
    if (ub == MEM_UNIT_NONE) return ua;
    UnitDimDef da, db;
    if (find_si_unit_dim(tab, ua, &da) && find_si_unit_dim(tab, ub, &db)) {
        int sum_e[7];
        for (int d = 0; d < 7; ++d) sum_e[d] = (int)da.exp[d] + (int)db.exp[d];
        return lookup_si_dim_unit_id(tab, sum_e);
    }
    return (ua == ub) ? ua : MEM_UNIT_NONE;
}

uint8_t delegate_resolve_div_unit(const MemArena *mem, const SymTable *tab,
                                  uint8_t ua, uint8_t ub) {
    (void)mem;
    if (ub == MEM_UNIT_NONE) return ua;
    if (ua == ub) return MEM_UNIT_NONE;
    UnitDimDef da, db;
    if (find_si_unit_dim(tab, ua, &da) && find_si_unit_dim(tab, ub, &db)) {
        int diff_e[7];
        for (int d = 0; d < 7; ++d) diff_e[d] = (int)da.exp[d] - (int)db.exp[d];
        return lookup_si_dim_unit_id(tab, diff_e);
    }
    return MEM_UNIT_NONE;
}

uint8_t delegate_resolve_pow_unit(const MemArena *mem, const SymTable *tab,
                                  uint8_t ua, uint8_t ub, double exp_val) {
    (void)mem;
    if (ub != MEM_UNIT_NONE || ua == MEM_UNIT_NONE) return MEM_UNIT_NONE;
    if (fabs(exp_val) <= 1e-12) return MEM_UNIT_NONE;
    if (fabs(exp_val - 1.0) <= 1e-12) return ua;
    UnitDimDef da;
    if (find_si_unit_dim(tab, ua, &da)) {
        double r = floor(exp_val + 0.5);
        if (fabs(exp_val - r) <= 1e-9 && r >= -4.0 && r <= 4.0) {
            int k = (int)r;
            int scaled[7];
            for (int d = 0; d < 7; ++d) scaled[d] = (int)da.exp[d] * k;
            return lookup_si_dim_unit_id(tab, scaled);
        }
        if (fabs(exp_val - 0.5) <= 1e-9 || fabs(exp_val - (1.0 / 3.0)) <= 1e-9) {
            int root_n = (fabs(exp_val - 0.5) <= 1e-9) ? 2 : 3;
            int scaled[7];
            for (int d = 0; d < 7; ++d) {
                if ((int)da.exp[d] % root_n != 0) return MEM_UNIT_NONE;
                scaled[d] = (int)da.exp[d] / root_n;
            }
            return lookup_si_dim_unit_id(tab, scaled);
        }
        return MEM_UNIT_NONE;
    }
    return ua;
}

uint8_t delegate_resolve_root_unit(const MemArena *mem, const SymTable *tab,
                                   uint8_t ua, int root_n) {
    (void)mem;
    if (ua == MEM_UNIT_NONE || root_n <= 0) return MEM_UNIT_NONE;
    UnitDimDef da;
    if (find_si_unit_dim(tab, ua, &da)) {
        int scaled[7];
        for (int d = 0; d < 7; ++d) {
            if ((int)da.exp[d] % root_n != 0) return MEM_UNIT_NONE;
            scaled[d] = (int)da.exp[d] / root_n;
        }
        return lookup_si_dim_unit_id(tab, scaled);
    }
    return ua;
}

bool delegate_val_to_exact_int(MemArena *mem, const SymTable *tab, Value v,
                               const char *ctx, int *out_int) {
    uint16_t tid = val_get_type(mem, v);
    if (tid == 0 || tid >= tab->type_count ||
        !tab->types[tid].is_consuming || tid == mem->str_type_id) {
        mem_err(mem, "Invalid non-numeric %s", ctx);
        return false;
    }
    if (val_get_unit(mem, v) != MEM_UNIT_NONE &&
        strcmp(ctx, "range start") != 0 &&
        strcmp(ctx, "range end") != 0) {
        mem_err(mem, "Expected unitless integer for %s", ctx);
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

Value delegate_eval_range_step(MemArena *mem, const SymTable *tab,
                               Value va, Value vb, Value vstep, int mode) {
    uint16_t ta = val_get_type(mem, va);
    uint16_t tb = val_get_type(mem, vb);
    if (ta == 0 || ta >= tab->type_count || !tab->types[ta].is_consuming || ta == mem->str_type_id ||
        tb == 0 || tb >= tab->type_count || !tab->types[tb].is_consuming || tb == mem->str_type_id) {
        mem_err(mem, "Range bounds must be numeric scalars");
        return val_void();
    }
    uint8_t elem_uid = MEM_UNIT_NONE;
    if (!delegate_check_additive_unit_compat(mem, tab, "..", 2, va, vb, &elem_uid)) {
        return val_void();
    }
    double da = val_get_f64(mem, va);
    double db = val_get_f64(mem, vb);
    double step = (da <= db) ? 1.0 : -1.0;

    if (val_get_type(mem, vstep) != MEM_TYPE_VOID) {
        uint16_t ts = val_get_type(mem, vstep);
        if (ts == 0 || ts >= tab->type_count || !tab->types[ts].is_consuming || ts == mem->str_type_id) {
            mem_err(mem, "Range step must be a numeric scalar");
            return val_void();
        }
        uint8_t us = val_get_unit(mem, vstep);
        if (us != MEM_UNIT_NONE && us != elem_uid) {
            mem_err(mem, "Unit mismatch in stepped range 'by'");
            return val_void();
        }
        step = val_get_f64(mem, vstep);
        if (!isfinite(step) || fabs(step) <= 1e-15) {
            mem_err(mem, "Range step cannot be zero");
            return val_void();
        }
    } else {
        int lo_i = 0, hi_i = 0;
        if (!delegate_val_to_exact_int(mem, tab, va, "range start", &lo_i) ||
            !delegate_val_to_exact_int(mem, tab, vb, "range end",   &hi_i)) {
            return val_void();
        }
        da = (double)lo_i;
        db = (double)hi_i;
    }

    double eps = 1e-11 * (1.0 + fabs(da) + fabs(db) + fabs(step));
    long long start_k = (mode & 2) ? 1LL : 0LL;
    double first_val  = da + (double)start_k * step;
    long long cnt_ll  = 0;

    if (step > 0.0) {
        bool ok = (mode & 1) ? (first_val < db - eps) : (first_val <= db + eps);
        if (ok) {
            cnt_ll = (long long)floor(((db - first_val) + ((mode & 1) ? -eps : eps)) / step) + 1LL;
        }
    } else {
        bool ok = (mode & 1) ? (first_val > db + eps) : (first_val >= db - eps);
        if (ok) {
            cnt_ll = (long long)floor(((first_val - db) + ((mode & 1) ? -eps : eps)) / (-step)) + 1LL;
        }
    }
    if (cnt_ll < 0) cnt_ll = 0;
    if (cnt_ll > (long long)MEM_MAX_HEAP_SLOTS) {
        mem_err(mem, "Range length (%lld) exceeds maximum capacity", cnt_ll);
        return val_void();
    }

    uint32_t count    = (uint32_t)cnt_ll;
    uint16_t elem_tid = mem->fast_real_type;
    uint16_t packed_t = (uint16_t)(((uint32_t)elem_uid << 8u) | ((uint32_t)elem_tid & 0xFFu));
    uint16_t hdr_idx  = mem_vec_alloc_hdr(mem, packed_t, count);
    if (mem->has_error) return val_void();
    if (count > 0) {
        if (!mem_vec_reserve(mem, hdr_idx, 1, count)) return val_void();
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        double  *dptr     = mem_data_ptr(mem, data_idx, 0);
        if (!dptr) return val_void();
        for (uint32_t i = 0; i < count; ++i) {
            double v = da + (double)(start_k + (long long)i) * step;
            if (fabs(v) < 1e-15) v = 0.0;
            dptr[i] = v;
        }
        mem_write_pool(mem, hdr_idx + 1, (double)count);
    }
    if (val_get_type(mem, vstep) == MEM_TYPE_VOID && hdr_idx != 0) {
        uint8_t slot = (uint8_t)(mem->vrange_cnt++ & 3u);
        mem->vrange_hdr[slot]  = hdr_idx;
        mem->vrange_mode[slot] = (uint8_t)mode;
        mem->vrange_unit[slot] = elem_uid;
        mem->vrange_a[slot]    = da;
        mem->vrange_b[slot]    = db;
    }
    return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
}

bool delegate_extract_slice_indices(MemArena *mem, const SymTable *tab,
                                    Value idx_val, int dim_len,
                                    uint16_t *out_indices, uint16_t *out_count,
                                    uint16_t max_cap) {
    uint16_t tid = val_get_type(mem, idx_val);
    if (tid == mem->vec_type_id && mem->vec_type_id != 0) {
        uint16_t hdr_idx = val_get_pool_idx(idx_val);
        uint32_t len32   = (uint32_t)mem_read_pool(mem, hdr_idx + 1);
        uint16_t dat_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        if (len32 > (uint32_t)max_cap) {
            mem_err(mem, "Slice length (%u) exceeds maximum capacity (%u)", (unsigned)len32, (unsigned)max_cap);
            return false;
        }
        uint16_t len = (uint16_t)len32;
        for (uint16_t i = 0; i < len; ++i) {
            Value ev = val_make_f64(mem, mem->fast_real_type, mem_data_read(mem, dat_idx, i));
            int k = 0;
            if (!delegate_val_to_exact_int(mem, tab, ev, "slice index", &k)) return false;
            if (k < 0) k += dim_len;
            if (k < 0 || k >= dim_len) {
                mem_err(mem, "Slice index %d out of bounds (size=%d)", k, dim_len);
                return false;
            }
            out_indices[i] = (uint16_t)k;
        }
        *out_count = len;
        return true;
    }
    if (tid == mem->mat_type_id && mem->mat_type_id != 0) {
        uint8_t  rank = 0;
        uint16_t tot_len = 0, dat_idx = 0, dims[4] = { 0 };
        delegate_mat_get_info(mem, idx_val, &rank, &tot_len, &dat_idx, dims);
        if (rank != 1 || tot_len > max_cap) {
            mem_err(mem, "Matrix slice index must be a 1D index vector");
            return false;
        }
        for (uint16_t i = 0; i < tot_len; ++i) {
            Value ev = val_make_f64(mem, mem->fast_real_type, mem_data_read(mem, dat_idx, i));
            int k = 0;
            if (!delegate_val_to_exact_int(mem, tab, ev, "slice index", &k)) return false;
            if (k < 0) k += dim_len;
            if (k < 0 || k >= dim_len) {
                mem_err(mem, "Slice index %d out of bounds (size=%d)", k, dim_len);
                return false;
            }
            out_indices[i] = (uint16_t)k;
        }
        *out_count = tot_len;
        return true;
    }
    return false;
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
    if (mem->call_depth > 128u) {
        mem_err(mem, "Maximum function recursion depth (128) exceeded");
        return val_void();
    }
    uint16_t hdr_idx     = val_get_pool_idx(fn_val);
    uint16_t body_ast    = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
    uint8_t  param_cnt   = (uint8_t)((uint8_t)mem_read_pool(mem, hdr_idx + 1) & 0x3Fu);
    uint32_t packed_slt  = (uint32_t)mem_read_pool(mem, hdr_idx + 2);
    uint16_t slot_range  = (uint16_t)(packed_slt & 0xFFFFu);
    uint16_t def_head    = (uint16_t)((packed_slt >> 16u) & 0xFFFFu);
    uint8_t  slot_start  = (uint8_t)(slot_range & 0xFFu);
    uint8_t  total_slots = (uint8_t)((slot_range >> 8u) & 0xFFu);
    uint16_t fbase       = (uint16_t)mem_read_pool(mem, hdr_idx + 3);

    if (total_slots < param_cnt) total_slots = param_cnt;
    if (total_slots > 64u) total_slots = 64u;

    /* Count minimum required parameters (those without default ASTs in def_head) */
    uint8_t min_req = param_cnt;
    if (def_head != MEM_NO_IDX && def_head != 0) {
        const AstNode *dl = mem_get_ast(mem, def_head);
        if (dl->kind == AST_ARG_LINK && dl->sub < min_req) {
            min_req = dl->sub;
        }
    }
    if (argc < min_req || argc > param_cnt) {
        if (min_req == param_cnt) {
            mem_err(mem, "Lambda expects %u argument(s), got %u",
                    (unsigned)param_cnt, (unsigned)argc);
        } else {
            mem_err(mem, "Function expects %u..%u argument(s), got %u",
                    (unsigned)min_req, (unsigned)param_cnt, (unsigned)argc);
        }
        return val_void();
    }

    uint16_t saved_sp   = mem->sp;
    uint16_t max_needed = (uint16_t)(fbase + slot_start + total_slots);
    if (max_needed > MEM_MAX_CALL_STACK) {
        mem_err(mem, "Call stack overflow in function invocation");
        return val_void();
    }
    if (mem->sp < max_needed) {
        mem->sp = max_needed;
    }

    /* Fast path for 0/1/2-slot lambdas with no default arguments (hot calculus/vec loops) */
    if ((def_head == MEM_NO_IDX || def_head == 0) && argc == param_cnt && total_slots <= 2u) {
        uint16_t s0 = (uint16_t)(fbase + slot_start);
        Value    sv0 = val_void(), sv1 = val_void();
        uint8_t  lt0 = 0, lt1 = 0, lu0 = 0, lu1 = 0;
        if (total_slots >= 1u) {
            sv0 = mem->stack[s0];
            lt0 = mem->slot_locked_type[s0];
            lu0 = mem->slot_locked_unit[s0];
            mem->slot_locked_type[s0] = 0;
            mem->slot_locked_unit[s0] = 0;
            if (argc >= 1u) mem->stack[s0] = args[0];
        }
        if (total_slots == 2u) {
            uint16_t s1 = (uint16_t)(s0 + 1u);
            sv1 = mem->stack[s1];
            lt1 = mem->slot_locked_type[s1];
            lu1 = mem->slot_locked_unit[s1];
            mem->slot_locked_type[s1] = 0;
            mem->slot_locked_unit[s1] = 0;
            if (argc >= 2u) mem->stack[s1] = args[1];
        }
        Value res = val_void();
        if (!mem->has_error && mem->jump_active == JUMP_NONE) {
            mem->call_depth++;
            res = eval_ast(mem, tab, body_ast, fbase, 0);
            if (mem->call_depth > 0) mem->call_depth--;
            if (mem->jump_active == JUMP_RETURN) {
                res = mem->jump_val;
                mem->jump_val    = val_void();
                mem->jump_active = JUMP_NONE;
            }
        }
        if (total_slots >= 1u) {
            mem->stack[s0]            = sv0;
            mem->slot_locked_type[s0] = lt0;
            mem->slot_locked_unit[s0] = lu0;
        }
        if (total_slots == 2u) {
            uint16_t s1 = (uint16_t)(s0 + 1u);
            mem->stack[s1]            = sv1;
            mem->slot_locked_type[s1] = lt1;
            mem->slot_locked_unit[s1] = lu1;
        }
        mem->sp = saved_sp;
        if (!mem->has_error && (mem->pool_top > (uint16_t)(MEM_MAX_F64_POOL - 1024u) ||
                                mem->heap_active_count >= 64u)) {
            (void)mem_compact_scratch(mem, &res, 1);
        }
        return res;
    }

    Value   saved_vals[64];
    uint8_t saved_ltype[64];
    uint8_t saved_lunit[64];
    for (uint8_t k = 0; k < total_slots; ++k) {
        uint16_t abs_s = (uint16_t)(fbase + slot_start + k);
        saved_vals[k]  = mem->stack[abs_s];
        saved_ltype[k] = mem->slot_locked_type[abs_s];
        saved_lunit[k] = mem->slot_locked_unit[abs_s];
        mem->slot_locked_type[abs_s] = 0;
        mem->slot_locked_unit[abs_s] = 0;
        if (k < argc) {
            mem->stack[abs_s] = args[k];
        }
    }

    /* Evaluate default parameter expressions for omitted trailing parameters */
    if (argc < param_cnt && def_head != MEM_NO_IDX && def_head != 0) {
        uint16_t cur_d = def_head;
        while (cur_d != MEM_NO_IDX && !mem->has_error) {
            const AstNode *dl = mem_get_ast(mem, cur_d);
            uint8_t p_idx = dl->sub;
            if (p_idx >= argc && p_idx < param_cnt) {
                Value dval = eval_ast(mem, tab, dl->left, fbase, 0);
                if (mem->has_error || mem->jump_active != JUMP_NONE) break;
                mem->stack[fbase + slot_start + p_idx] = dval;
            }
            cur_d = dl->right;
        }
    }

    Value res = val_void();
    if (!mem->has_error && mem->jump_active == JUMP_NONE) {
        mem->call_depth++;
        res = eval_ast(mem, tab, body_ast, fbase, 0);
        if (mem->call_depth > 0) mem->call_depth--;
        if (mem->jump_active == JUMP_RETURN) {
            res = mem->jump_val;
            mem->jump_val    = val_void();
            mem->jump_active = JUMP_NONE;
        }
    }

    for (uint8_t k = 0; k < total_slots; ++k) {
        uint16_t abs_s = (uint16_t)(fbase + slot_start + k);
        mem->stack[abs_s]            = saved_vals[k];
        mem->slot_locked_type[abs_s] = saved_ltype[k];
        mem->slot_locked_unit[abs_s] = saved_lunit[k];
    }
    mem->sp = saved_sp;
    if (!mem->has_error && (mem->pool_top > (uint16_t)(MEM_MAX_F64_POOL - 1024u) ||
                            mem->heap_active_count >= 64u)) {
        (void)mem_compact_scratch(mem, &res, 1);
    }
    return res;
}

/* ============================================================================
 * 3. DYNAMIC VECTOR, LIST, STACK & QUEUE (`$vec`) DELEGATE INTRINSICS
 * ============================================================================ */

/*Below section is the C's native element read/write helpers for container delegates @vnew..@vcount*/
/*Construct vec<T>, list<T>, stack<T>, queue<T> element storage and retrieval*/
void delegate_vec_write_elem(MemArena *mem, const SymTable *tab,
                             uint16_t elem_tid, uint16_t data_idx, uint32_t offset, Value item) {
    if (elem_tid == mem->str_type_id && elem_tid != 0) {
        StrSlice s = val_get_str(mem, item);
        uint32_t packed = ((uint32_t)s.off << 16) | (uint32_t)s.len;
        mem_data_write(mem, data_idx, offset, (double)packed);
    } else if (elem_tid < tab->type_count && !tab->types[elem_tid].is_consuming) {
        uint16_t stride   = tab->types[elem_tid].flat_size;
        uint16_t src_pidx = val_get_pool_idx(item);
        for (uint16_t k = 0; k < stride; ++k) {
            mem_data_write(mem, data_idx, offset + k, mem_read_pool(mem, (uint16_t)(src_pidx + k)));
        }
    } else {
        mem_data_write(mem, data_idx, offset, val_get_f64(mem, item));
    }
}

Value delegate_vec_read_elem(MemArena *mem, const SymTable *tab,
                             uint16_t elem_tid, uint8_t elem_uid, uint16_t data_idx, uint32_t offset) {
    if (elem_tid == mem->str_type_id && elem_tid != 0) {
        uint32_t packed = (uint32_t)mem_data_read(mem, data_idx, offset);
        StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
        return val_make_str(mem, elem_tid, s);
    } else if (elem_tid < tab->type_count && !tab->types[elem_tid].is_consuming) {
        uint16_t stride = tab->types[elem_tid].flat_size;
        uint16_t src_pidx = (uint16_t)(data_idx + offset);
        return val_with_unit(mem, val_make_comp(mem, elem_tid, src_pidx, stride), elem_uid);
    } else {
        return val_make_f64_unit(mem, elem_tid ? elem_tid : mem->fast_real_type,
                                 elem_uid, mem_data_read(mem, data_idx, offset));
    }
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
        uint8_t  param_cnt = (uint8_t)((uint8_t)mem_read_pool(mem, hdr_idx + 1) & 0x3Fu);
        Value args[2] = { b, c };
        return delegate_call_fn(mem, tab, a, param_cnt, args);
    }

    /*Below section is the C's native for operator/function delegates @map_new..@map_entries and @vlen/@vclear/@vin on $map*/
    /*Construct map(), dict(), m[k], m[k] = v, k in m, map_set, map_get, map_has, map_del, keys(m), vals(m), entries(m)*/
    if (delegate_is_map_intrinsic(intr)) {
        if (mem->map_type_id == 0) {
            mem_err(mem, "Map type ($map) is not loaded");
            return val_void();
        }
        if (intr == INTR_MAP_NEW) {
            int cap_in = 0;
            if (val_get_type(mem, a) != MEM_TYPE_VOID) {
                if (!delegate_val_to_exact_int(mem, tab, a, "map capacity", &cap_in)) return val_void();
            }
            if (cap_in < 0 || cap_in > MEM_MAX_F64_POOL / 2) {
                mem_err(mem, "Map capacity out of range (%d)", cap_in);
                return val_void();
            }
            uint16_t hdr_idx = mem_map_alloc_hdr(mem, 0, (uint16_t)cap_in);
            if (mem->has_error) return val_void();
            return val_make_comp(mem, mem->map_type_id, hdr_idx, MEM_MAP_HDR_SLOTS);
        }
        if (val_get_type(mem, a) != mem->map_type_id) {
            mem_err(mem, "Expected map ($map) as first argument");
            return val_void();
        }
        uint16_t hdr_idx  = val_get_pool_idx(a);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
        uint16_t lock_tid = (uint16_t)(raw_hdr0 & 0xFFu);
        uint8_t  lock_uid = (uint8_t)((raw_hdr0 >> 8u) & 0xFFu);
        uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);

        /* Convert key `b` to StrSlice (either direct $str or stringified scalar) */
        StrSlice key_s = { 0, 0 };
        if (val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0) {
            key_s = val_get_str(mem, b);
        } else {
            Value sb = delegate_eval_intrinsic(mem, tab, INTR_TOSTR, b, val_void(), val_void(), 0);
            if (mem->has_error) return val_void();
            key_s = val_get_str(mem, sb);
        }

        int found_idx = -1;
        for (uint16_t i = 0; i < len; ++i) {
            StrSlice ks = mem_map_entry_key(mem, data_idx, i);
            if (mem_slice_eq(mem, ks, key_s)) {
                found_idx = (int)i;
                break;
            }
        }

        if (intr == INTR_MAP_HAS) {
            return val_make_f64(mem, mem->fast_real_type, (found_idx >= 0) ? 1.0 : 0.0);
        }
        if (intr == INTR_MAP_GET) {
            if (found_idx >= 0) {
                return mem_map_entry_val(mem, data_idx, (uint16_t)found_idx);
            }
            if (val_get_type(mem, c) != MEM_TYPE_VOID) {
                return c; /* get_or(m, key, default_val) */
            }
            mem_err(mem, "Key '%.*s' not found in map", (int)key_s.len, mem_str_ptr(mem, key_s));
            return val_void();
        }
        if (intr == INTR_MAP_DEL) {
            if (found_idx >= 0) {
                uint16_t new_len = (uint16_t)(len - 1);
                for (uint16_t i = (uint16_t)found_idx; i < new_len; ++i) {
                    StrSlice nk = mem_map_entry_key(mem, data_idx, (uint16_t)(i + 1));
                    Value    nv = mem_map_entry_val(mem, data_idx, (uint16_t)(i + 1));
                    mem_map_set_entry(mem, data_idx, i, nk, nv);
                }
                mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            }
            return a;
        }
        if (intr == INTR_MAP_SET) {
            uint16_t v_tid = val_get_type(mem, c);
            uint8_t  v_uid = val_get_unit(mem, c);
            if (lock_tid != 0 && lock_tid != v_tid) {
                char exp_t[32], got_t[32];
                sym_type_name(tab, mem, lock_tid, exp_t, sizeof(exp_t));
                sym_type_name(tab, mem, v_tid, got_t, sizeof(got_t));
                mem_err(mem, "Map value type mismatch: expected '%s', got '%s'", exp_t, got_t);
                return val_void();
            }
            if (lock_uid != 0 && lock_uid != v_uid) {
                mem_err(mem, "Map value unit mismatch");
                return val_void();
            }
            Value stored_v = c;
            if (v_tid != 0 && v_tid < tab->type_count && !tab->types[v_tid].is_consuming &&
                v_tid != mem->vec_type_id && v_tid != mem->mat_type_id &&
                v_tid != mem->fn_type_id && v_tid != mem->map_type_id) {
                stored_v = val_clone_comp(mem, c, tab->types[v_tid].flat_size);
                if (mem->has_error) return val_void();
            }
            if (found_idx >= 0) {
                mem_map_set_entry(mem, data_idx, (uint16_t)found_idx, key_s, stored_v);
                return a;
            }
            if (!mem_map_reserve(mem, hdr_idx, (uint16_t)(len + 1))) {
                return val_void();
            }
            data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            mem_map_set_entry(mem, data_idx, len, key_s, stored_v);
            mem_write_pool(mem, hdr_idx + 1, (double)(len + 1));
            return a;
        }
    }

    if (mem->map_type_id != 0 && val_get_type(mem, a) == mem->map_type_id) {
        uint16_t hdr_idx = val_get_pool_idx(a);
        if (intr == INTR_VLEN) {
            return val_make_f64(mem, mem->fast_real_type, mem_read_pool(mem, hdr_idx + 1));
        }
        if (intr == INTR_VCLEAR) {
            mem_write_pool(mem, hdr_idx + 1, 0.0);
            return a;
        }
    }
    if (intr == INTR_VIN && mem->map_type_id != 0 && val_get_type(mem, b) == mem->map_type_id) {
        return delegate_eval_vec_intrinsic(mem, tab, INTR_MAP_HAS, b, a, val_void());
    }

    if (mem->vec_type_id == 0) {
        mem_err(mem, "Dynamic vector type ($vec) is not loaded");
        return val_void();
    }

    /*Below section is the C's native for operator delegate @vnew*/
    /*Construct vec(), vec(cap), list(), stack(), queue()*/
    if (intr == INTR_VNEW) {
        int cap_in = 0;
        if (val_get_type(mem, a) != MEM_TYPE_VOID) {
            if (!delegate_val_to_exact_int(mem, tab, a, "vector capacity", &cap_in)) return val_void();
        }
        if (cap_in < 0 || (uint32_t)cap_in > MEM_MAX_HEAP_SLOTS) {
            mem_err(mem, "Vector capacity out of range (%d)", cap_in);
            return val_void();
        }
        uint16_t hdr_idx = mem_vec_alloc_hdr(mem, 0, (uint32_t)cap_in);
        if (mem->has_error) return val_void();
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    /*Below section is the C's native for operator delegate @vfill*/
    /*Construct vec(count, init), list(item), stack(item), queue(item)*/
    if (intr == INTR_VFILL) {
        int count = 0;
        if (!delegate_val_to_exact_int(mem, tab, a, "vector fill count", &count)) return val_void();
        if (count < 0 || (uint32_t)count > MEM_MAX_HEAP_SLOTS) {
            mem_err(mem, "Vector fill count out of range (%d)", count);
            return val_void();
        }
        uint16_t elem_tid = val_get_type(mem, b);
        uint8_t  elem_uid = val_get_unit(mem, b);
        uint16_t packed_t = (uint16_t)(((uint32_t)elem_uid << 8u) | ((uint32_t)elem_tid & 0xFFu));
        uint16_t stride   = (elem_tid < tab->type_count) ? tab->types[elem_tid].flat_size : 1;
        uint16_t hdr_idx  = mem_vec_alloc_hdr(mem, packed_t, (uint32_t)count);
        if (mem->has_error) return val_void();
        if (count > 0) {
            if (!mem_vec_reserve(mem, hdr_idx, stride, (uint32_t)count)) return val_void();
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            for (uint32_t i = 0; i < (uint32_t)count; ++i) {
                delegate_vec_write_elem(mem, tab, elem_tid, data_idx, i * (uint32_t)stride, b);
            }
            mem_write_pool(mem, hdr_idx + 1, (double)count);
        }
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    /*Below section is the C's native for operator delegates @vlen and @vget on $str*/
    /*Construct len(s:str), at(s:str, i), s[i], s[a...b], and s[a...b by step]*/
    if (val_get_type(mem, a) == mem->str_type_id && mem->str_type_id != 0) {
        if (intr == INTR_VLEN) {
            return val_make_f64(mem, mem->fast_real_type, (double)val_get_str(mem, a).len);
        }
        if (intr == INTR_VGET) {
            StrSlice str_s = val_get_str(mem, a);
            uint16_t b_tid = val_get_type(mem, b);
            if ((b_tid == mem->vec_type_id && mem->vec_type_id != 0) ||
                (b_tid == mem->mat_type_id && mem->mat_type_id != 0)) {
                uint16_t slice_idx[128];
                uint16_t slice_cnt = 0;
                if (!delegate_extract_slice_indices(mem, tab, b, (int)str_s.len,
                                                    slice_idx, &slice_cnt, 128)) {
                    return val_void();
                }
                char buf[256];
                uint16_t out_len = (slice_cnt < (uint16_t)(sizeof(buf) - 1))
                                   ? slice_cnt : (uint16_t)(sizeof(buf) - 1);
                const char *sp = mem_str_ptr(mem, str_s);
                for (uint16_t i = 0; i < out_len; ++i) {
                    buf[i] = sp[slice_idx[i]];
                }
                StrSlice out_s = mem_intern_str(mem, buf, out_len);
                if (mem->has_error) return val_void();
                return val_make_str(mem, mem->str_type_id, out_s);
            } else {
                int k = 0;
                if (!delegate_val_to_exact_int(mem, tab, b, "string index", &k)) {
                    return val_void();
                }
                if (k < 0) k += (int)str_s.len;
                if (k < 0 || k >= (int)str_s.len) {
                    mem_err(mem, "String index %d out of bounds (len=%u)", k, (unsigned)str_s.len);
                    return val_void();
                }
                const char *sp = mem_str_ptr(mem, str_s);
                StrSlice ch_s = mem_intern_str(mem, sp + k, 1);
                if (mem->has_error) return val_void();
                return val_make_str(mem, mem->str_type_id, ch_s);
            }
        }
    }

    /*Below section is the C's native for operator delegates @vrange and @vrange_step*/
    /*Construct a...b, a..<b, a<..b, a..b, and stepped ranges (a...b by step) / strided vectors (v by step)*/
    if (intr == INTR_VRANGE) {
        int mode = (val_get_type(mem, c) != MEM_TYPE_VOID) ? (int)val_get_f64(mem, c) : 0;
        return delegate_eval_range_step(mem, tab, a, b, val_void(), mode);
    }
    if (intr == INTR_VRANGE_STEP) {
        if (val_get_type(mem, a) != mem->vec_type_id) {
            return delegate_eval_range_step(mem, tab, a, b, c, 0);
        }
        uint16_t rng_hdr = val_get_pool_idx(a);
        for (uint8_t k = 0; k < 4; ++k) {
            if (rng_hdr != 0 && mem->vrange_hdr[k] == rng_hdr) {
                Value va = val_make_f64_unit(mem, mem->fast_real_type, mem->vrange_unit[k], mem->vrange_a[k]);
                Value vb = val_make_f64_unit(mem, mem->fast_real_type, mem->vrange_unit[k], mem->vrange_b[k]);
                return delegate_eval_range_step(mem, tab, va, vb, b, (int)mem->vrange_mode[k]);
            }
        }
        int step_k = 0;
        if (!delegate_val_to_exact_int(mem, tab, b, "vector stride step", &step_k) || step_k == 0) {
            if (!mem->has_error) mem_err(mem, "Range or vector step cannot be zero");
            return val_void();
        }
        uint16_t src_hdr  = val_get_pool_idx(a);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, src_hdr + 0);
        uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
        uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
        uint32_t src_len  = (uint32_t)mem_read_pool(mem, src_hdr + 1);
        uint16_t src_dat  = (uint16_t)mem_read_pool(mem, src_hdr + 3);
        uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                            ? tab->types[elem_tid].flat_size : 1;

        uint32_t abs_step = (uint32_t)(step_k > 0 ? step_k : -step_k);
        uint16_t out_hdr  = mem_vec_alloc_hdr(mem, raw_hdr0, (src_len / abs_step) + 1u);
        if (mem->has_error) return val_void();
        Value out_vec = val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
        if (src_len > 0) {
            if (step_k > 0) {
                for (int idx = 0; idx < (int)src_len && !mem->has_error; idx += step_k) {
                    Value item = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid,
                                                        src_dat, (uint32_t)idx * (uint32_t)stride);
                    (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, out_vec, item, val_void());
                }
            } else {
                for (int idx = (int)src_len - 1; idx >= 0 && !mem->has_error; idx += step_k) {
                    Value item = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid,
                                                        src_dat, (uint32_t)idx * (uint32_t)stride);
                    (void)delegate_eval_vec_intrinsic(mem, tab, INTR_VPUSH, out_vec, item, val_void());
                }
            }
        }
        return out_vec;
    }

    /*Below section is the C's native for operator delegate @vin*/
    /*Construct item in vec, item in mat, item in a...b*/
    if (intr == INTR_VIN) {
        if (val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0 &&
            val_get_type(mem, a) == mem->str_type_id) {
            StrSlice needle = val_get_str(mem, a);
            StrSlice hay    = val_get_str(mem, b);
            if (needle.len == 0) return val_make_f64(mem, mem->fast_real_type, 1.0);
            if (needle.len > hay.len) return val_make_f64(mem, mem->fast_real_type, 0.0);
            const char *np = mem_str_ptr(mem, needle);
            const char *hp = mem_str_ptr(mem, hay);
            for (uint16_t i = 0; i + needle.len <= hay.len; ++i) {
                if (memcmp(hp + i, np, needle.len) == 0) {
                    return val_make_f64(mem, mem->fast_real_type, 1.0);
                }
            }
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
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
                if (fabs(mem_data_read(mem, data_idx, i) - target) <= 1e-12) {
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
        uint32_t len      = (uint32_t)mem_read_pool(mem, hdr_idx + 1);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                            ? tab->types[elem_tid].flat_size : 1;

        uint16_t item_tid = val_get_type(mem, a);
        uint8_t  item_uid = val_get_unit(mem, a);
        if (len == 0 || elem_tid != item_tid || elem_uid != item_uid) {
            return val_make_f64(mem, mem->fast_real_type, 0.0);
        }
        for (uint32_t i = 0; i < len; ++i) {
            Value ev = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, data_idx, i * (uint32_t)stride);
            bool match = false;
            if (elem_tid == mem->str_type_id && mem->str_type_id != 0) {
                match = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, ev));
            } else if (!tab->types[elem_tid].is_consuming) {
                match = true;
                uint16_t pa = val_get_pool_idx(a);
                uint16_t pe = val_get_pool_idx(ev);
                for (uint16_t k = 0; k < stride; ++k) {
                    if (fabs(mem_read_pool(mem, (uint16_t)(pa + k)) - mem_read_pool(mem, (uint16_t)(pe + k))) > 1e-12) {
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
    uint32_t len      = (uint32_t)mem_read_pool(mem, hdr_idx + 1);
    uint32_t cap      = (uint32_t)mem_read_pool(mem, hdr_idx + 2);
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
            if (!mem_vec_reserve(mem, hdr_idx, stride, len + 1u)) {
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            delegate_vec_write_elem(mem, tab, elem_tid, data_idx, len * (uint32_t)stride, b);
            mem_write_pool(mem, hdr_idx + 1, (double)(len + 1u));
            return a;
        }

        /*Below section is the C's native for operator delegate @vpop*/
        /*Construct pop(v) (LIFO stack/vector pop from back)*/
        case INTR_VPOP: {
            if (len == 0) {
                mem_err(mem, "Cannot pop from an empty vector");
                return val_void();
            }
            uint32_t new_len  = len - 1u;
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, data_idx, new_len * (uint32_t)stride);
        }

        /*Below section is the C's native for operator delegate @vshift*/
        /*Construct dequeue(q), shift(q), pop_front(q) (FIFO queue pop from front)*/
        case INTR_VSHIFT: {
            if (len == 0) {
                mem_err(mem, "Cannot dequeue/shift from an empty vector");
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            Value first = delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, data_idx, 0);
            if (elem_tid != 0 && elem_tid < tab->type_count &&
                !tab->types[elem_tid].is_consuming &&
                elem_tid != mem->vec_type_id && elem_tid != mem->mat_type_id &&
                elem_tid != mem->fn_type_id && elem_tid != mem->map_type_id) {
                first = val_clone_comp(mem, first, stride);
                if (mem->has_error) return val_void();
            }
            uint32_t new_len = len - 1u;
            if (new_len > 0) {
                double *dptr = mem_data_ptr(mem, data_idx, 0);
                if (dptr) {
                    memmove(dptr, dptr + stride, (size_t)(new_len * (uint32_t)stride) * sizeof(double));
                }
            }
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return first;
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
            return delegate_vec_read_elem(mem, tab, elem_tid, elem_uid, data_idx, (uint32_t)idx * (uint32_t)stride);
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
            delegate_vec_write_elem(mem, tab, elem_tid, data_idx, (uint32_t)idx * (uint32_t)stride, c);
            return a;
        }

        /*Below section is the C's native for operator delegate @vresize*/
        /*Construct resize(v, new_len)*/
        case INTR_VRESIZE: {
            int new_len_in = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "vector resize length", &new_len_in)) return val_void();
            if (new_len_in < 0 || (uint32_t)new_len_in > MEM_MAX_HEAP_SLOTS) {
                mem_err(mem, "Vector resize length out of range (%d)", new_len_in);
                return val_void();
            }
            uint32_t new_len = (uint32_t)new_len_in;
            if (elem_tid == 0) {
                elem_tid = mem->fast_real_type;
                stride   = 1;
                mem_write_pool(mem, hdr_idx + 0, (double)elem_tid);
            }
            if (new_len > len) {
                if (!mem_vec_reserve(mem, hdr_idx, stride, new_len)) return val_void();
                uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
                double *dptr = mem_data_ptr(mem, data_idx, len * (uint32_t)stride);
                if (dptr) {
                    memset(dptr, 0, (size_t)((new_len - len) * (uint32_t)stride) * sizeof(double));
                }
            }
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return a;
        }
        /*Below section is the C's native for operator delegate @vsort*/
        /*Construct sort(v) (mode 0 ascending) and sort_desc(v) (mode 1 descending)*/
        case INTR_VSORT: {
            int desc = (val_get_type(mem, b) != MEM_TYPE_VOID && fabs(val_get_f64(mem, b)) > 1e-15) ? 1 : 0;
            uint16_t out_hdr = mem_vec_alloc_hdr(mem, elem_tid, len);
            if (mem->has_error) return val_void();
            mem_write_pool(mem, out_hdr + 0, (double)raw_hdr0);
            if (len == 0) {
                return val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
            }
            if (elem_tid == 0 || elem_tid >= tab->type_count || !tab->types[elem_tid].is_consuming) {
                mem_err(mem, "sort() requires a vector of scalar or string elements");
                return val_void();
            }
            if (!mem_vec_reserve(mem, out_hdr, 1, len)) return val_void();
            uint16_t src_data = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            uint16_t dst_data = (uint16_t)mem_read_pool(mem, out_hdr + 3);
            const double *sptr = mem_data_ptr_ro(mem, src_data, 0);
            double       *dptr = mem_data_ptr(mem, dst_data, 0);
            if (!sptr || !dptr) return val_void();
            memcpy(dptr, sptr, (size_t)len * sizeof(double));
            mem_write_pool(mem, out_hdr + 1, (double)len);

            bool is_str = (elem_tid == mem->str_type_id && mem->str_type_id != 0);
            for (uint32_t gap = len / 2u; gap > 0u; gap /= 2u) {
                for (uint32_t i = gap; i < len; ++i) {
                    double temp = dptr[i];
                    uint32_t j = i;
                    while (j >= gap) {
                        double prev = dptr[j - gap];
                        bool swap_needed = false;
                        if (is_str) {
                            uint32_t pa = (uint32_t)prev;
                            uint32_t pb = (uint32_t)temp;
                            StrSlice sa = { (uint16_t)(pa >> 16), (uint16_t)(pa & 0xFFFFu) };
                            StrSlice sb = { (uint16_t)(pb >> 16), (uint16_t)(pb & 0xFFFFu) };
                            uint16_t min_l = (sa.len < sb.len) ? sa.len : sb.len;
                            int cmp = (min_l > 0) ? memcmp(mem_str_ptr(mem, sa), mem_str_ptr(mem, sb), min_l) : 0;
                            if (cmp == 0) cmp = (int)sa.len - (int)sb.len;
                            swap_needed = desc ? (cmp < 0) : (cmp > 0);
                        } else {
                            swap_needed = desc ? (prev < temp) : (prev > temp);
                        }
                        if (!swap_needed) break;
                        dptr[j] = prev;
                        j -= gap;
                    }
                    dptr[j] = temp;
                }
            }
            return val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
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

    if (intr == INTR_MRESHAPE) {
        uint16_t src_len = 0, src_dat = 0;
        uint8_t  src_uid = MEM_UNIT_NONE;
        uint8_t  ra_tmp  = 0;
        uint16_t da_tmp[4] = { 0 };
        if (delegate_mat_get_info(mem, a, &ra_tmp, &src_len, &src_dat, da_tmp)) {
            src_uid = val_get_unit(mem, a);
        } else if (mem->vec_type_id != 0 && val_get_type(mem, a) == mem->vec_type_id) {
            uint16_t hdr_a    = val_get_pool_idx(a);
            uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_a + 0);
            uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
            src_uid = (uint8_t)((raw_hdr0 >> 8u) & 0xFFu);
            src_len = (uint16_t)mem_read_pool(mem, hdr_a + 1);
            src_dat = (uint16_t)mem_read_pool(mem, hdr_a + 3);
            if (src_len > 0 && elem_tid != mem->fast_real_type) {
                mem_err(mem, "reshape() requires a real scalar vector or matrix");
                return val_void();
            }
        } else {
            mem_err(mem, "reshape() expects a matrix ($mat) or vector ($vec)");
            return val_void();
        }

        uint8_t  out_rank = 0;
        uint16_t dims[4]  = { 0, 0, 0, 0 };
        uint16_t donor_tot = 0, donor_dat = 0;
        if (delegate_mat_get_info(mem, b, &out_rank, &donor_tot, &donor_dat, dims)) {
            if (donor_tot != src_len) {
                mem_err(mem, "Cannot reshape %u element(s) to target matrix of %u element(s)",
                        (unsigned)src_len, (unsigned)donor_tot);
                return val_void();
            }
        } else {
            int r_in = 0, c_in = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "reshape rows", &r_in) ||
                !delegate_val_to_exact_int(mem, tab, c, "reshape cols", &c_in)) {
                return val_void();
            }
            if (r_in <= 0 || c_in < 0 || r_in > 4096 || c_in > 4096) {
                mem_err(mem, "Reshape dimensions must be positive (%d x %d)", r_in, c_in);
                return val_void();
            }
            uint32_t eff_cols = (c_in == 0) ? 1u : (uint32_t)c_in;
            if ((uint32_t)r_in * eff_cols != (uint32_t)src_len) {
                mem_err(mem, "Cannot reshape %u element(s) into %d x %d matrix",
                        (unsigned)src_len, r_in, c_in);
                return val_void();
            }
            out_rank = (c_in == 0) ? 1u : 2u;
            dims[0]  = (uint16_t)r_in;
            dims[1]  = (uint16_t)c_in;
        }
        uint16_t tot_out = 0, dat_out = 0;
        uint16_t hdr_out = mem_mat_alloc_hdr(mem, out_rank, dims, &tot_out, &dat_out);
        if (hdr_out == MEM_NO_IDX || mem->has_error) return val_void();
        if (src_len > 0) {
            const double *sptr = mem_data_ptr_ro(mem, src_dat, 0);
            double       *dptr = mem_data_ptr(mem, dat_out, 0);
            if (sptr && dptr) memcpy(dptr, sptr, (size_t)src_len * sizeof(double));
        }
        return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), src_uid);
    }

    if (intr == INTR_MMUL) {
        bool a_is_vec = (mem->vec_type_id != 0 && val_get_type(mem, a) == mem->vec_type_id);
        bool b_is_vec = (mem->vec_type_id != 0 && val_get_type(mem, b) == mem->vec_type_id);
        if (a_is_vec || b_is_vec) {
            if (a_is_vec && !b_is_vec) {
                uint8_t rb = 0;
                uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
                if (!delegate_mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims) || rb != 2) {
                    mem_err(mem, "vec * mat requires a 2D matrix on the right");
                    return val_void();
                }
                uint16_t hdr_a = val_get_pool_idx(a);
                uint16_t raw0  = (uint16_t)mem_read_pool(mem, hdr_a + 0);
                uint16_t len_a = (uint16_t)mem_read_pool(mem, hdr_a + 1);
                uint16_t dat_v = (uint16_t)mem_read_pool(mem, hdr_a + 3);
                uint8_t  u_a   = (uint8_t)((raw0 >> 8u) & 0xFFu);
                uint8_t  u_b   = val_get_unit(mem, b);
                uint8_t  r_u   = delegate_resolve_mul_unit(mem, tab, u_a, u_b);
                uint16_t M = db_dims[0], N = db_dims[1];
                if (len_a != M) {
                    mem_err(mem, "vec * mat dimension mismatch: (%u) vs (%ux%u)",
                            (unsigned)len_a, (unsigned)M, (unsigned)N);
                    return val_void();
                }
                uint16_t out_hdr = mem_vec_alloc_hdr(mem, (uint16_t)(mem->fast_real_type | ((uint16_t)r_u << 8u)), N);
                if (mem->has_error || !mem_vec_reserve(mem, out_hdr, 1, N)) return val_void();
                uint16_t out_dat = (uint16_t)mem_read_pool(mem, out_hdr + 3);
                for (uint16_t col = 0; col < N; ++col) {
                    double sum = 0.0;
                    for (uint16_t r = 0; r < M; ++r) {
                        sum += mem_data_read(mem, dat_v, r) *
                               mem_data_read(mem, dat_b, (uint32_t)r * (uint32_t)N + col);
                    }
                    mem_data_write(mem, out_dat, col, sum);
                }
                mem_write_pool(mem, out_hdr + 1, (double)N);
                return val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
            }
            if (!a_is_vec && b_is_vec) {
                uint8_t ra_m = 0;
                uint16_t tot_m = 0, dat_m = 0, dm_dims[4] = { 0 };
                if (!delegate_mat_get_info(mem, a, &ra_m, &tot_m, &dat_m, dm_dims) || ra_m != 2) {
                    mem_err(mem, "mat * vec requires a 2D matrix on the left");
                    return val_void();
                }
                uint16_t hdr_b = val_get_pool_idx(b);
                uint16_t raw0  = (uint16_t)mem_read_pool(mem, hdr_b + 0);
                uint16_t len_b = (uint16_t)mem_read_pool(mem, hdr_b + 1);
                uint16_t dat_v = (uint16_t)mem_read_pool(mem, hdr_b + 3);
                uint8_t  u_a   = val_get_unit(mem, a);
                uint8_t  u_b   = (uint8_t)((raw0 >> 8u) & 0xFFu);
                uint8_t  r_u   = delegate_resolve_mul_unit(mem, tab, u_a, u_b);
                uint16_t M = dm_dims[0], K = dm_dims[1];
                if (K != len_b) {
                    mem_err(mem, "mat * vec dimension mismatch: (%ux%u) vs (%u)",
                            (unsigned)M, (unsigned)K, (unsigned)len_b);
                    return val_void();
                }
                uint16_t out_hdr = mem_vec_alloc_hdr(mem, (uint16_t)(mem->fast_real_type | ((uint16_t)r_u << 8u)), M);
                if (mem->has_error || !mem_vec_reserve(mem, out_hdr, 1, M)) return val_void();
                uint16_t out_dat = (uint16_t)mem_read_pool(mem, out_hdr + 3);
                for (uint16_t r = 0; r < M; ++r) {
                    double sum = 0.0;
                    for (uint16_t k = 0; k < K; ++k) {
                        sum += mem_data_read(mem, dat_m, (uint32_t)r * (uint32_t)K + k) *
                               mem_data_read(mem, dat_v, k);
                    }
                    mem_data_write(mem, out_dat, r, sum);
                }
                mem_write_pool(mem, out_hdr + 1, (double)M);
                return val_make_comp(mem, mem->vec_type_id, out_hdr, MEM_VEC_HDR_SLOTS);
            }
        }
    }

    uint8_t  ra = 0;
    uint16_t tot_a = 0, dat_a = 0, da[4] = { 0 };
    if (!delegate_mat_get_info(mem, a, &ra, &tot_a, &dat_a, da)) {
        mem_err(mem, "Expected matrix ($mat) argument");
        return val_void();
    }
    uint8_t ua = val_get_unit(mem, a);

    switch (intr) {
        case INTR_MRANK:
            return val_make_f64(mem, mem->fast_real_type, (double)ra);
        case INTR_MLEN:
            return val_make_f64(mem, mem->fast_real_type, (double)tot_a);
        case INTR_MDIM: {
            int ax = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "matrix dimension axis", &ax)) {
                return val_void();
            }
            if (ax < 0) ax += (int)ra;
            if (ax < 0 || ax >= (int)ra) {
                mem_err(mem, "Matrix axis %d out of bounds for rank %u", ax, (unsigned)ra);
                return val_void();
            }
            return val_make_f64(mem, mem->fast_real_type, (double)da[ax]);
        }
        case INTR_MMUL: {
            uint8_t  rb = 0;
            uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
            if (!delegate_mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims)) {
                mem_err(mem, "Expected second matrix ($mat) argument for '*'");
                return val_void();
            }
            uint8_t ub = val_get_unit(mem, b);
            uint8_t ru = delegate_resolve_mul_unit(mem, tab, ua, ub);

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
                            sum += mem_data_read(mem, dat_a, (uint32_t)r * (uint32_t)K + k) *
                                   mem_data_read(mem, dat_b, (uint32_t)k * (uint32_t)N + col);
                        }
                        mem_data_write(mem, dat_out, (uint32_t)r * (uint32_t)N + col, sum);
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
                        sum += mem_data_read(mem, dat_a, (uint32_t)r * (uint32_t)K + k) *
                               mem_data_read(mem, dat_b, k);
                    }
                    mem_data_write(mem, dat_out, r, sum);
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            if (ra == 1 && rb == 1 && da[0] == db_dims[0]) {
                uint16_t out_dims[4] = { da[0], 0, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 1, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < da[0]; ++i) {
                    mem_data_write(mem, dat_out, i,
                                   mem_data_read(mem, dat_a, i) * mem_data_read(mem, dat_b, i));
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            mem_err(mem, "Matrix multiplication '*' requires 2D*2D or 2D*1D compatible dimensions");
            return val_void();
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
                tmp[i] = mem_data_read(mem, dat_a, i);
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
                    aug[r * (2 * N) + col]     = mem_data_read(mem, dat_a, (uint32_t)r * (uint32_t)N + col);
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
                    mem_data_write(mem, dat_out, (uint32_t)r * (uint32_t)N + col, v);
                }
            }
            mem->pool_top = saved_pool;
            return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
        }
        case INTR_MLINSOLVE: {
            if (ra != 2 || da[0] != da[1] || da[0] == 0 || da[0] > 16) {
                mem_err(mem, "linsolve(A, b) requires a square 2D matrix A (1x1 up to 16x16)");
                return val_void();
            }
            bool     b_is_vec = (val_get_type(mem, b) == mem->vec_type_id && mem->vec_type_id != 0);
            uint8_t  rb = 0;
            uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
            uint8_t  ub = val_get_unit(mem, b);
            if (b_is_vec) {
                uint16_t hdr_b  = val_get_pool_idx(b);
                uint16_t raw_b0 = (uint16_t)mem_read_pool(mem, hdr_b + 0);
                ub         = (uint8_t)((raw_b0 >> 8) & 0xFFu);
                rb         = 1;
                db_dims[0] = (uint16_t)mem_read_pool(mem, hdr_b + 1);
                tot_b      = db_dims[0];
                dat_b      = (uint16_t)mem_read_pool(mem, hdr_b + 3);
                if (db_dims[0] != da[0]) {
                    mem_err(mem, "linsolve(A, b) dimension mismatch: b must have %u elements", (unsigned)da[0]);
                    return val_void();
                }
            } else if (!delegate_mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims) ||
                       (rb != 1 && rb != 2) || db_dims[0] != da[0]) {
                mem_err(mem, "linsolve(A, b) dimension mismatch: b must have %u rows", (unsigned)da[0]);
                return val_void();
            }
            uint16_t N    = da[0];
            uint16_t K    = (rb == 2) ? db_dims[1] : 1u;
            uint16_t cols = (uint16_t)(N + K);
            uint8_t  ru   = delegate_resolve_div_unit(mem, tab, ub, ua);

            uint16_t out_dims[4] = { N, (rb == 2) ? K : 0u, 0, 0 };
            uint16_t tot_out = 0, dat_out = 0, hdr_out = 0;
            if (b_is_vec) {
                uint16_t packed_t = (uint16_t)(((uint32_t)ru << 8u) | ((uint32_t)mem->fast_real_type & 0xFFu));
                hdr_out = mem_vec_alloc_hdr(mem, packed_t, N);
                if (mem->has_error || !mem_vec_reserve(mem, hdr_out, 1, N)) return val_void();
                dat_out = (uint16_t)mem_read_pool(mem, hdr_out + 3);
                mem_write_pool(mem, hdr_out + 1, (double)N);
            } else {
                hdr_out = mem_mat_alloc_hdr(mem, rb, out_dims, &tot_out, &dat_out);
                if (hdr_out == MEM_NO_IDX || mem->has_error) return val_void();
            }

            uint16_t saved_pool = mem->pool_top;
            uint16_t aug_idx    = mem_alloc_pool(mem, (uint16_t)(N * cols));
            if (aug_idx == MEM_NO_IDX || mem->has_error) return val_void();
            double *aug = &mem->f64_pool[aug_idx];

            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < N; ++col) {
                    aug[r * cols + col] = mem_data_read(mem, dat_a, (uint32_t)r * (uint32_t)N + col);
                }
                for (uint16_t col = 0; col < K; ++col) {
                    aug[r * cols + N + col] = mem_data_read(mem, dat_b, (uint32_t)r * (uint32_t)K + col);
                }
            }
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t piv = i;
                double max_v = fabs(aug[i * cols + i]);
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double v = fabs(aug[r * cols + i]);
                    if (v > max_v) { max_v = v; piv = r; }
                }
                if (max_v <= 1e-14) {
                    mem->pool_top = saved_pool;
                    mem_err(mem, "Singular matrix in linsolve(A, b) (det == 0)");
                    return val_void();
                }
                if (piv != i) {
                    for (uint16_t col = 0; col < cols; ++col) {
                        double t = aug[i * cols + col];
                        aug[i * cols + col] = aug[piv * cols + col];
                        aug[piv * cols + col] = t;
                    }
                }
                double diag = aug[i * cols + i];
                for (uint16_t col = i; col < cols; ++col) {
                    aug[i * cols + col] /= diag;
                }
                for (uint16_t r = 0; r < N; ++r) {
                    if (r == i) continue;
                    double factor = aug[r * cols + i];
                    if (fabs(factor) > 0.0) {
                        for (uint16_t col = i; col < cols; ++col) {
                            aug[r * cols + col] -= factor * aug[i * cols + col];
                        }
                    }
                }
            }
            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < K; ++col) {
                    double v = aug[r * cols + N + col];
                    if (fabs(v) < 1e-15) v = 0.0;
                    mem_data_write(mem, dat_out, (uint32_t)r * (uint32_t)K + col, v);
                }
            }
            mem->pool_top = saved_pool;
            if (b_is_vec) {
                return val_make_comp(mem, mem->vec_type_id, hdr_out, MEM_VEC_HDR_SLOTS);
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
        }
        case INTR_MEIGVALS:
        case INTR_MEIGVECS: {
            if (ra != 2 || da[0] != da[1]) {
                mem_err(mem, "eigvals/eigvecs requires a square 2D matrix");
                return val_void();
            }
            uint16_t N = da[0];
            if (N == 2 && intr == INTR_MEIGVALS) {
                /* Exact characteristic polynomial for any 2x2 real matrix with real eigenvalues */
                double a00 = mem_data_read(mem, dat_a, 0);
                double a01 = mem_data_read(mem, dat_a, 1);
                double a10 = mem_data_read(mem, dat_a, 2);
                double a11 = mem_data_read(mem, dat_a, 3);
                double tr  = a00 + a11;
                double dt  = a00 * a11 - a01 * a10;
                double dsc = tr * tr - 4.0 * dt;
                if (dsc < -1e-11) {
                    mem_err(mem, "eigvals() matrix has complex eigenvalues (disc=%.6g)", dsc);
                    return val_void();
                }
                if (dsc < 0.0) dsc = 0.0;
                double sd = sqrt(dsc);
                double ev[2] = { 0.5 * (tr + sd), 0.5 * (tr - sd) };
                if (fabs(ev[0]) < 1e-14) ev[0] = 0.0;
                if (fabs(ev[1]) < 1e-14) ev[1] = 0.0;
                return val_from_f64_vec(mem, ev, 2);
            }
            if (N > 32) {
                mem_err(mem, "eigvals/eigvecs supports up to 32x32 matrices");
                return val_void();
            }
            /* Cyclic Jacobi eigenvalue/eigenvector algorithm for symmetric NxN matrices */
            uint16_t saved_pool = mem->pool_top;
            uint16_t a_idx = mem_alloc_pool(mem, (uint16_t)(N * N));
            uint16_t v_idx = mem_alloc_pool(mem, (uint16_t)(N * N));
            if (mem->has_error) { mem->pool_top = saved_pool; return val_void(); }
            double *A = &mem->f64_pool[a_idx];
            double *V = &mem->f64_pool[v_idx];
            const double *src_A = mem_data_ptr_ro(mem, dat_a, 0);
            if (!src_A) { mem->pool_top = saved_pool; return val_void(); }
            memcpy(A, src_A, (size_t)(N * N) * sizeof(double));
            memset(V, 0, (size_t)(N * N) * sizeof(double));
            for (uint16_t i = 0; i < N; ++i) V[i * N + i] = 1.0;

            for (int sweep = 0; sweep < 60; ++sweep) {
                double off_norm = 0.0;
                for (uint16_t p = 0; p < N; ++p) {
                    for (uint16_t q = (uint16_t)(p + 1); q < N; ++q) {
                        off_norm += A[p * N + q] * A[p * N + q];
                    }
                }
                if (off_norm < 1e-24) break;
                for (uint16_t p = 0; p < N; ++p) {
                    for (uint16_t q = (uint16_t)(p + 1); q < N; ++q) {
                        double apq = A[p * N + q];
                        if (fabs(apq) < 1e-15) continue;
                        double app = A[p * N + p];
                        double aqq = A[q * N + q];
                        double tau = (aqq - app) / (2.0 * apq);
                        double t   = (tau >= 0.0)
                                     ? (1.0 / (tau + sqrt(1.0 + tau * tau)))
                                     : (-1.0 / (-tau + sqrt(1.0 + tau * tau)));
                        double c_r = 1.0 / sqrt(1.0 + t * t);
                        double s_r = t * c_r;
                        A[p * N + p] = app - t * apq;
                        A[q * N + q] = aqq + t * apq;
                        A[p * N + q] = 0.0;
                        A[q * N + p] = 0.0;
                        for (uint16_t r = 0; r < N; ++r) {
                            if (r != p && r != q) {
                                double arp = A[r * N + p];
                                double arq = A[r * N + q];
                                A[r * N + p] = c_r * arp - s_r * arq;
                                A[p * N + r] = A[r * N + p];
                                A[r * N + q] = s_r * arp + c_r * arq;
                                A[q * N + r] = A[r * N + q];
                            }
                            double vrp = V[r * N + p];
                            double vrq = V[r * N + q];
                            V[r * N + p] = c_r * vrp - s_r * vrq;
                            V[r * N + q] = s_r * vrp + c_r * vrq;
                        }
                    }
                }
            }
            /* Sort eigenvalues descending and permute eigenvector columns accordingly */
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t best = i;
                for (uint16_t j = (uint16_t)(i + 1); j < N; ++j) {
                    if (A[j * N + j] > A[best * N + best]) best = j;
                }
                if (best != i) {
                    double tmp = A[i * N + i];
                    A[i * N + i] = A[best * N + best];
                    A[best * N + best] = tmp;
                    for (uint16_t r = 0; r < N; ++r) {
                        double tv = V[r * N + i];
                        V[r * N + i] = V[r * N + best];
                        V[r * N + best] = tv;
                    }
                }
            }
            if (intr == INTR_MEIGVALS) {
                double evals[32];
                for (uint16_t i = 0; i < N; ++i) {
                    double v = A[i * N + i];
                    if (fabs(v) < 1e-13) v = 0.0;
                    evals[i] = v;
                }
                mem->pool_top = saved_pool;
                return val_from_f64_vec(mem, evals, N);
            } else {
                double v_tmp[1024];
                for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                    v_tmp[i] = (fabs(V[i]) < 1e-14) ? 0.0 : V[i];
                }
                mem->pool_top = saved_pool;
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, da, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                    mem_data_write(mem, dat_out, i, v_tmp[i]);
                }
                return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
            }
        }
        case INTR_MSVD: {
            if (ra != 2) {
                mem_err(mem, "SVD / pinv / lstsq requires a 2D matrix");
                return val_void();
            }
            uint16_t M = da[0], N = da[1];
            if (M > 32 || N > 32) {
                mem_err(mem, "SVD / pinv / lstsq supports up to 32x32 matrices");
                return val_void();
            }
            uint16_t saved_pool = mem->pool_top;
            uint16_t ata_idx = mem_alloc_pool(mem, (uint16_t)(N * N));
            uint16_t v_idx   = mem_alloc_pool(mem, (uint16_t)(N * N));
            uint16_t u_idx   = mem_alloc_pool(mem, (uint16_t)(M * N));
            if (mem->has_error) { mem->pool_top = saved_pool; return val_void(); }
            double *ATA = &mem->f64_pool[ata_idx];
            double *V   = &mem->f64_pool[v_idx];
            double *U   = &mem->f64_pool[u_idx];
            const double *A = mem_data_ptr_ro(mem, dat_a, 0);
            if (!A) { mem->pool_top = saved_pool; return val_void(); }

            for (uint16_t i = 0; i < N; ++i) {
                for (uint16_t j = 0; j < N; ++j) {
                    double sum = 0.0;
                    for (uint16_t r = 0; r < M; ++r) sum += A[r * N + i] * A[r * N + j];
                    ATA[i * N + j] = sum;
                }
            }
            memset(V, 0, (size_t)(N * N) * sizeof(double));
            for (uint16_t i = 0; i < N; ++i) V[i * N + i] = 1.0;

            for (int sweep = 0; sweep < 60; ++sweep) {
                double off_norm = 0.0;
                for (uint16_t p = 0; p < N; ++p)
                    for (uint16_t q = (uint16_t)(p + 1); q < N; ++q)
                        off_norm += ATA[p * N + q] * ATA[p * N + q];
                if (off_norm < 1e-24) break;
                for (uint16_t p = 0; p < N; ++p) {
                    for (uint16_t q = (uint16_t)(p + 1); q < N; ++q) {
                        double apq = ATA[p * N + q];
                        if (fabs(apq) < 1e-15) continue;
                        double app = ATA[p * N + p], aqq = ATA[q * N + q];
                        double tau = (aqq - app) / (2.0 * apq);
                        double t   = (tau >= 0.0)
                                     ? (1.0 / (tau + sqrt(1.0 + tau * tau)))
                                     : (-1.0 / (-tau + sqrt(1.0 + tau * tau)));
                        double c_r = 1.0 / sqrt(1.0 + t * t), s_r = t * c_r;
                        ATA[p * N + p] = app - t * apq;
                        ATA[q * N + q] = aqq + t * apq;
                        ATA[p * N + q] = 0.0;
                        ATA[q * N + p] = 0.0;
                        for (uint16_t r = 0; r < N; ++r) {
                            if (r != p && r != q) {
                                double arp = ATA[r * N + p], arq = ATA[r * N + q];
                                ATA[r * N + p] = c_r * arp - s_r * arq;
                                ATA[p * N + r] = ATA[r * N + p];
                                ATA[r * N + q] = s_r * arp + c_r * arq;
                                ATA[q * N + r] = ATA[r * N + q];
                            }
                            double vrp = V[r * N + p], vrq = V[r * N + q];
                            V[r * N + p] = c_r * vrp - s_r * vrq;
                            V[r * N + q] = s_r * vrp + c_r * vrq;
                        }
                    }
                }
            }
            double svals[32];
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t best = i;
                for (uint16_t j = (uint16_t)(i + 1); j < N; ++j) {
                    if (ATA[j * N + j] > ATA[best * N + best]) best = j;
                }
                if (best != i) {
                    double tmp = ATA[i * N + i];
                    ATA[i * N + i] = ATA[best * N + best];
                    ATA[best * N + best] = tmp;
                    for (uint16_t r = 0; r < N; ++r) {
                        double tv = V[r * N + i];
                        V[r * N + i] = V[r * N + best];
                        V[r * N + best] = tv;
                    }
                }
                double lam = ATA[i * N + i];
                svals[i] = (lam > 1e-24) ? sqrt(lam) : 0.0;
            }
            /* Compute U = A * V * Sigma^-1 */
            memset(U, 0, (size_t)(M * N) * sizeof(double));
            for (uint16_t j = 0; j < N; ++j) {
                if (svals[j] > 1e-12) {
                    for (uint16_t r = 0; r < M; ++r) {
                        double sum = 0.0;
                        for (uint16_t k = 0; k < N; ++k) sum += A[r * N + k] * V[k * N + j];
                        U[r * N + j] = sum / svals[j];
                    }
                }
            }

            int part = (val_get_type(mem, b) != MEM_TYPE_VOID) ? (int)val_get_f64(mem, b) : 0;
            if (part == 0) {
                mem->pool_top = saved_pool;
                uint16_t K = (M < N) ? M : N;
                return val_from_f64_vec(mem, svals, K);
            } else if (part == 1) {
                double u_tmp[1024];
                for (uint16_t i = 0; i < (uint16_t)(M * N); ++i) {
                    u_tmp[i] = (fabs(U[i]) < 1e-14) ? 0.0 : U[i];
                }
                mem->pool_top = saved_pool;
                uint16_t dims[4] = { M, N, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < (uint16_t)(M * N); ++i) {
                    mem_data_write(mem, dat_out, i, u_tmp[i]);
                }
                return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
            } else {
                double v_tmp[1024];
                for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                    v_tmp[i] = (fabs(V[i]) < 1e-14) ? 0.0 : V[i];
                }
                mem->pool_top = saved_pool;
                uint16_t dims[4] = { N, N, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                    mem_data_write(mem, dat_out, i, v_tmp[i]);
                }
                return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
            }
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
static void delegate_buf_append(char *buf, size_t cap, size_t *used, const char *fmt, ...) {
    if (!buf || cap == 0 || *used + 1 >= cap) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + *used, cap - *used, fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    if ((size_t)n >= cap - *used) {
        *used = cap - 1;
    } else {
        *used += (size_t)n;
    }
}

static void delegate_format_struct_at(const MemArena *mem, const SymTable *tab,
                                      uint16_t type_id, uint16_t pool_idx,
                                      uint8_t depth, char *buf, size_t cap);

static void delegate_format_mat_slice(const MemArena *mem, uint16_t data_idx, uint32_t base_off,
                                      const uint16_t *dims, uint8_t rank, uint8_t axis,
                                      char *buf, size_t cap, size_t *used) {
    delegate_buf_append(buf, cap, used, "[");
    if (axis + 1 >= rank) {
        uint16_t n = dims[axis];
        for (uint16_t i = 0; i < n && *used + 1 < cap; ++i) {
            double v = mem_data_read(mem, data_idx, base_off + i);
            if (v == 0.0) v = 0.0;
            delegate_buf_append(buf, cap, used, "%s%.6g", (i > 0 ? ", " : ""), v);
        }
    } else {
        uint32_t stride = 1;
        for (uint8_t a = (uint8_t)(axis + 1); a < rank; ++a) {
            stride *= (uint32_t)dims[a];
        }
        uint16_t n = dims[axis];
        for (uint16_t i = 0; i < n && *used + 1 < cap; ++i) {
            if (i > 0) {
                delegate_buf_append(buf, cap, used, ", ");
            }
            delegate_format_mat_slice(mem, data_idx, base_off + (uint32_t)i * stride,
                                      dims, rank, (uint8_t)(axis + 1), buf, cap, used);
        }
    }
    delegate_buf_append(buf, cap, used, "]");
}

static void delegate_format_vec_at(const MemArena *mem, const SymTable *tab,
                                   uint16_t hdr_idx, uint8_t depth,
                                   char *buf, size_t cap) {
    if (depth > 16 || cap == 0) {
        if (cap > 0) snprintf(buf, cap, "[...]");
        return;
    }
    uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
    uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
    uint32_t len      = (uint32_t)mem_read_pool(mem, hdr_idx + 1);
    uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
    uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                        ? tab->types[elem_tid].flat_size : 1;

    size_t used = 0;
    buf[0] = '\0';
    delegate_buf_append(buf, cap, &used, "[");
    for (uint32_t i = 0; i < len && used + 1 < cap; ++i) {
        uint32_t off = i * (uint32_t)stride;
        char elem_str[96];
        if (elem_tid == mem->str_type_id && elem_tid != 0) {
            uint32_t packed = (uint32_t)mem_data_read(mem, data_idx, off);
            StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
            snprintf(elem_str, sizeof(elem_str), "\"%.*s\"",
                     (int)s.len, mem_str_ptr(mem, s));
        } else if (elem_tid == mem->vec_type_id && elem_tid != 0) {
            delegate_format_vec_at(mem, tab, (uint16_t)(data_idx + off), (uint8_t)(depth + 1), elem_str, sizeof(elem_str));
        } else if (elem_tid > 0 && elem_tid < tab->type_count &&
                   !tab->types[elem_tid].is_consuming) {
            delegate_format_struct_at(mem, tab, elem_tid, (uint16_t)(data_idx + off), (uint8_t)(depth + 1), elem_str, sizeof(elem_str));
        } else {
            snprintf(elem_str, sizeof(elem_str), "%.6g", mem_data_read(mem, data_idx, off));
        }
        delegate_buf_append(buf, cap, &used, "%s%s", (i > 0 ? ", " : ""), elem_str);
    }
    delegate_buf_append(buf, cap, &used, "]");
}

static void delegate_format_struct_at(const MemArena *mem, const SymTable *tab,
                                      uint16_t type_id, uint16_t pool_idx,
                                      uint8_t depth, char *buf, size_t cap) {
    if (depth > 16 || cap == 0) {
        if (cap > 0) snprintf(buf, cap, "<...>");
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
        buf[0] = '\0';
        delegate_format_mat_slice(mem, data_idx, 0, dims, rank, 0, buf, cap, &used);
        return;
    }
    if (type_id == mem->fn_type_id && type_id != 0) {
        unsigned pcnt = (unsigned)mem_read_pool(mem, pool_idx + 1) & 0x3Fu;
        snprintf(buf, cap, "<fn/%u>", pcnt);
        return;
    }
    if (type_id == mem->map_type_id && type_id != 0) {
        uint16_t len      = (uint16_t)mem_read_pool(mem, pool_idx + 1);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, pool_idx + 3);
        size_t used = 0;
        buf[0] = '\0';
        delegate_buf_append(buf, cap, &used, "{");
        for (uint16_t i = 0; i < len && used + 1 < cap; ++i) {
            StrSlice kslice = mem_map_entry_key(mem, data_idx, i);
            Value    vval   = mem_map_entry_val(mem, data_idx, i);
            char vstr[96];
            uint16_t vtid = val_get_type(mem, vval);
            if (vtid == mem->str_type_id && vtid != 0) {
                StrSlice vs = val_get_str(mem, vval);
                snprintf(vstr, sizeof(vstr), "\"%.*s\"", (int)vs.len, mem_str_ptr(mem, vs));
            } else if (vtid > 0 && vtid < tab->type_count && !tab->types[vtid].is_consuming) {
                delegate_format_struct_at(mem, tab, vtid, val_get_pool_idx(vval), (uint8_t)(depth + 1), vstr, sizeof(vstr));
            } else {
                snprintf(vstr, sizeof(vstr), "%.6g", val_get_f64(mem, vval));
            }
            delegate_buf_append(buf, cap, &used, "%s\"%.*s\": %s",
                                (i > 0 ? ", " : ""),
                                (int)kslice.len, mem_str_ptr(mem, kslice), vstr);
        }
        delegate_buf_append(buf, cap, &used, "}");
        return;
    }

    const TypeDef *td = &tab->types[type_id];
    char tname[32];
    sym_type_name(tab, mem, type_id, tname, sizeof(tname));

    if (td->parser_intr == INTR_COMPLEX_TYPE && td->flat_size == 2) {
        double re = mem_read_pool(mem, pool_idx);
        double im = mem_read_pool(mem, pool_idx + 1);
        if (im >= 0.0) {
            snprintf(buf, cap, "%.6g + %.6gi", re, im);
        } else {
            snprintf(buf, cap, "%.6g - %.6gi", re, -im);
        }
        return;
    }

    size_t used = 0;
    buf[0] = '\0';
    delegate_buf_append(buf, cap, &used, "%s(", tname);
    for (uint8_t i = 0; i < td->field_count && used + 1 < cap; ++i) {
        const FieldDef *fd = &tab->fields[td->start_idx + i];
        char f_str[64];
        if (!tab->types[fd->type_id].is_consuming) {
            delegate_format_struct_at(mem, tab, fd->type_id, (uint16_t)(pool_idx + fd->flat_offset),
                                      (uint8_t)(depth + 1), f_str, sizeof(f_str));
        } else {
            snprintf(f_str, sizeof(f_str), "%.6g", mem_read_pool(mem, (uint16_t)(pool_idx + fd->flat_offset)));
        }
        delegate_buf_append(buf, cap, &used, "%s%.*s=%s",
                            (i > 0 ? ", " : ""),
                            (int)fd->name.len, mem_str_ptr(mem, fd->name),
                            f_str);
    }
    delegate_buf_append(buf, cap, &used, ")");
}

static Value delegate_eval_tostr(MemArena *mem, const SymTable *tab, Value val) {
    uint16_t tid = val_get_type(mem, val);
    if (tid == mem->str_type_id && tid != 0) {
        return val;
    }
    char buf[1024];
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

    if (tlen >= 2 && trimmed[0] == '{' && trimmed[tlen - 1] == '}') {
        uint16_t inner_p = 1;
        while (inner_p + 1 < tlen && (trimmed[inner_p] == ' ' || trimmed[inner_p] == '\t')) inner_p++;
        if (inner_p + 1 >= tlen) {
            uint16_t mhdr = mem_map_alloc_hdr(mem, 0, 0);
            return mem->has_error ? val_void() : val_make_comp(mem, mem->map_type_id, mhdr, MEM_MAP_HDR_SLOTS);
        }
        char dict_buf[512];
        size_t dw = (size_t)snprintf(dict_buf, sizeof(dict_buf), "dict(");
        bool in_q = false;
        int  d_nest = 0;
        for (uint16_t k = 1; k + 1 < tlen && dw + 2 < sizeof(dict_buf); ++k) {
            char ch = trimmed[k];
            if (ch == '"' && (k == 1 || trimmed[k - 1] != '\\')) in_q = !in_q;
            if (!in_q) {
                if (ch == '(' || ch == '[' || ch == '{') d_nest++;
                else if (ch == ')' || ch == ']' || ch == '}') d_nest--;
                else if (ch == ':' && d_nest == 0) {
                    dict_buf[dw++] = ',';
                    continue;
                }
            }
            dict_buf[dw++] = ch;
        }
        if (dw + 2 < sizeof(dict_buf)) {
            dict_buf[dw++] = ')';
            dict_buf[dw]   = '\0';
            uint16_t saved_sp2 = mem->sp;
            Parser MP;
            parser_init(&MP, mem, tab, dict_buf, (uint16_t)dw, 0, false, false);
            lex_next(&MP.lex);
            uint16_t mroot = parser_parse_expr(&MP, 0);
            if (!mem->has_error && mroot != MEM_NO_IDX && MP.lex.cur.kind == TOK_EOF) {
                Value mres = eval_ast(mem, tab, mroot, saved_sp2, 0);
                if (!mem->has_error && val_get_type(mem, mres) == mem->map_type_id) {
                    return mres;
                }
            }
            mem->sp = saved_sp2;
            mem_clear_err(mem);
        }
    }

    if (tlen >= 2 && trimmed[0] == '[' && trimmed[tlen - 1] == ']') {
        bool has_semi_or_sub = false;
        int  bdepth = 0;
        for (uint16_t i = 1; i + 1 < tlen; ++i) {
            char ch = trimmed[i];
            if (ch == '(' || ch == '{') bdepth++;
            else if (ch == ')' || ch == '}') bdepth--;
            else if (ch == '[' && bdepth == 0) { has_semi_or_sub = true; break; }
            else if (ch == ';' && bdepth == 0) { has_semi_or_sub = true; break; }
            else if (bdepth == 0 && lex_is_ident_start((unsigned char)ch) &&
                     (i == 1 || !lex_is_ident_cont((unsigned char)trimmed[i - 1]))) {
                uint16_t wend = i;
                while (wend + 1 < tlen && lex_is_ident_cont((unsigned char)trimmed[wend])) wend++;
                if (hardcode_is_for_kw(mem, tab, trimmed + i, (uint16_t)(wend - i))) {
                    has_semi_or_sub = true;
                    break;
                }
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
        P.lex.cur.kind != TOK_EOF || P.vis_count != 0) {
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
 * 5a. STRING FORMATTING, MANIPULATION & BACKTRACKING REGEX ENGINE DELEGATES
 * ============================================================================ */

static bool rx_match_here(const char *pat, uint16_t plen, const char *txt, uint16_t tlen,
                          uint16_t *out_end);

static bool rx_char_in_class(char c, const char *cls, uint16_t clen) {
    bool neg = (clen > 0 && cls[0] == '^');
    uint16_t i = neg ? 1 : 0;
    bool hit = false;
    while (i < clen) {
        if (cls[i] == '\\' && i + 1 < clen) {
            char esc = cls[i + 1];
            bool m = false;
            if (esc == 'd') m = (c >= '0' && c <= '9');
            else if (esc == 'w') m = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
            else if (esc == 's') m = (c == ' ' || c == '\t' || c == '\r' || c == '\n');
            else m = (c == esc);
            if (m) hit = true;
            i = (uint16_t)(i + 2);
            continue;
        }
        if (i + 2 < clen && cls[i + 1] == '-') {
            if ((unsigned char)c >= (unsigned char)cls[i] &&
                (unsigned char)c <= (unsigned char)cls[i + 2]) {
                hit = true;
            }
            i = (uint16_t)(i + 3);
            continue;
        }
        if (c == cls[i]) hit = true;
        i++;
    }
    return neg ? !hit : hit;
}

static bool rx_atom_matches(const char *pat, uint16_t atom_len, char c) {
    if (atom_len == 1 && pat[0] == '.') return (c != '\n');
    if (atom_len == 2 && pat[0] == '\\') {
        char esc = pat[1];
        if (esc == 'd') return (c >= '0' && c <= '9');
        if (esc == 'D') return !(c >= '0' && c <= '9');
        if (esc == 'w') return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
        if (esc == 'W') return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
        if (esc == 's') return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
        if (esc == 'S') return !(c == ' ' || c == '\t' || c == '\r' || c == '\n');
        if (esc == 'n') return (c == '\n');
        if (esc == 't') return (c == '\t');
        return (c == esc);
    }
    if (atom_len >= 2 && pat[0] == '[' && pat[atom_len - 1] == ']') {
        return rx_char_in_class(c, pat + 1, (uint16_t)(atom_len - 2));
    }
    return (atom_len == 1 && pat[0] == c);
}

static uint16_t rx_atom_span(const char *pat, uint16_t plen) {
    if (plen == 0) return 0;
    if (pat[0] == '\\' && plen >= 2) return 2;
    if (pat[0] == '[') {
        uint16_t i = 1;
        while (i < plen && pat[i] != ']') {
            if (pat[i] == '\\' && i + 1 < plen) i++;
            i++;
        }
        return (i < plen) ? (uint16_t)(i + 1) : plen;
    }
    if (pat[0] == '(') {
        int depth = 1;
        uint16_t i = 1;
        while (i < plen && depth > 0) {
            if (pat[i] == '\\' && i + 1 < plen) { i = (uint16_t)(i + 2); continue; }
            if (pat[i] == '(') depth++;
            else if (pat[i] == ')') depth--;
            i++;
        }
        return i;
    }
    return 1;
}

static bool rx_match_here(const char *pat, uint16_t plen, const char *txt, uint16_t tlen,
                          uint16_t *out_end) {
    /* Top-level alternation `branch1|branch2` */
    int depth = 0;
    bool in_cls = false;
    for (uint16_t i = 0; i < plen; ++i) {
        if (pat[i] == '\\' && i + 1 < plen) { i++; continue; }
        if (pat[i] == '[' && !in_cls) { in_cls = true; continue; }
        if (pat[i] == ']' && in_cls)  { in_cls = false; continue; }
        if (!in_cls) {
            if (pat[i] == '(') depth++;
            else if (pat[i] == ')') depth--;
            else if (pat[i] == '|' && depth == 0) {
                uint16_t e1 = 0;
                if (rx_match_here(pat, i, txt, tlen, &e1)) {
                    *out_end = e1;
                    return true;
                }
                return rx_match_here(pat + i + 1, (uint16_t)(plen - i - 1), txt, tlen, out_end);
            }
        }
    }

    if (plen == 0) {
        *out_end = 0;
        return true;
    }
    if (plen == 1 && pat[0] == '$') {
        if (tlen == 0) { *out_end = 0; return true; }
        return false;
    }

    uint16_t alen = rx_atom_span(pat, plen);
    char quant = (alen < plen && (pat[alen] == '*' || pat[alen] == '+' || pat[alen] == '?'))
                 ? pat[alen] : '\0';
    uint16_t rem_off = (quant != '\0') ? (uint16_t)(alen + 1) : alen;
    const char *rem_pat = pat + rem_off;
    uint16_t    rem_len = (uint16_t)(plen - rem_off);

    bool is_group = (alen >= 2 && pat[0] == '(' && pat[alen - 1] == ')');
    const char *g_pat = is_group ? (pat + 1) : pat;
    uint16_t    g_len = is_group ? (uint16_t)(alen - 2) : alen;

    if (quant == '\0') {
        if (is_group) {
            uint16_t g_end = 0;
            if (!rx_match_here(g_pat, g_len, txt, tlen, &g_end)) return false;
            uint16_t r_end = 0;
            if (!rx_match_here(rem_pat, rem_len, txt + g_end, (uint16_t)(tlen - g_end), &r_end)) return false;
            *out_end = (uint16_t)(g_end + r_end);
            return true;
        }
        if (tlen > 0 && rx_atom_matches(pat, alen, txt[0])) {
            uint16_t r_end = 0;
            if (rx_match_here(rem_pat, rem_len, txt + 1, (uint16_t)(tlen - 1), &r_end)) {
                *out_end = (uint16_t)(1 + r_end);
                return true;
            }
        }
        return false;
    }

    if (quant == '?') {
        if (is_group) {
            uint16_t g_end = 0;
            if (rx_match_here(g_pat, g_len, txt, tlen, &g_end)) {
                uint16_t r_end = 0;
                if (rx_match_here(rem_pat, rem_len, txt + g_end, (uint16_t)(tlen - g_end), &r_end)) {
                    *out_end = (uint16_t)(g_end + r_end);
                    return true;
                }
            }
        } else if (tlen > 0 && rx_atom_matches(pat, alen, txt[0])) {
            uint16_t r_end = 0;
            if (rx_match_here(rem_pat, rem_len, txt + 1, (uint16_t)(tlen - 1), &r_end)) {
                *out_end = (uint16_t)(1 + r_end);
                return true;
            }
        }
        return rx_match_here(rem_pat, rem_len, txt, tlen, out_end);
    }

    /* '*' or '+' quantifier (greedy with backtracking) */
    uint16_t starts[128];
    uint16_t scnt = 0;
    if (quant == '*') starts[scnt++] = 0;
    uint16_t cur = 0;
    while (cur < tlen && scnt < 128) {
        uint16_t step = 0;
        if (is_group) {
            if (!rx_match_here(g_pat, g_len, txt + cur, (uint16_t)(tlen - cur), &step) || step == 0) break;
        } else {
            if (!rx_atom_matches(pat, alen, txt[cur])) break;
            step = 1;
        }
        cur = (uint16_t)(cur + step);
        starts[scnt++] = cur;
    }
    for (int k = (int)scnt - 1; k >= 0; --k) {
        uint16_t r_end = 0;
        if (rx_match_here(rem_pat, rem_len, txt + starts[k], (uint16_t)(tlen - starts[k]), &r_end)) {
            *out_end = (uint16_t)(starts[k] + r_end);
            return true;
        }
    }
    return false;
}

static bool rx_search(const char *pat, uint16_t plen, const char *txt, uint16_t tlen,
                      uint16_t *out_start, uint16_t *out_len) {
    if (plen > 0 && pat[0] == '^') {
        uint16_t mend = 0;
        if (rx_match_here(pat + 1, (uint16_t)(plen - 1), txt, tlen, &mend)) {
            if (out_start) *out_start = 0;
            if (out_len)   *out_len   = mend;
            return true;
        }
        return false;
    }
    for (uint16_t i = 0; i <= tlen; ++i) {
        uint16_t mend = 0;
        if (rx_match_here(pat, plen, txt + i, (uint16_t)(tlen - i), &mend)) {
            if (out_start) *out_start = i;
            if (out_len)   *out_len   = mend;
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * 5. SCALAR ARITHMETIC, BOOLEAN LOGIC & TRANSCENDENTAL DELEGATES
 * ============================================================================ */

/*Below section is the C's native top-level dispatcher for operator/function delegates @add..@err, @tostr, @fromstr*/
/*Construct +, -, *, /, %, ^, √, sin..tanh, ==..~, &&..nand, type_of, unit_of, x_, _s, error*/
Value delegate_eval_intrinsic(MemArena *mem, SymTable *tab,
                              IntrinsicId intr, Value a, Value b, Value c, uint8_t ret_type) {
    if (delegate_is_vec_or_map_intrinsic(intr)) {
        return delegate_eval_vec_intrinsic(mem, tab, intr, a, b, c);
    }
    if (delegate_is_mat_intrinsic(intr)) {
        return delegate_eval_mat_intrinsic(mem, tab, intr, a, b, c);
    }

    /*Below section is the C's native for operator delegates @type_name, @unit_name, @tostr, @fromstr, @err*/
    /*Construct type_of(x), unit_of(x), x_ (postfix "_"), _s (prefix "_"), error(msg)*/
    if (intr == INTR_GC_COMPACT) {
        uint16_t reclaimed = mem_compact_scratch(mem, NULL, 0);
        return val_make_f64(mem, mem->fast_real_type, (double)reclaimed);
    }
    if (intr == INTR_POOL_USED) {
        uint16_t used = mem_scratch_pool_used(mem);
        return val_make_f64(mem, mem->fast_real_type, (double)used);
    }
    if (intr == INTR_STR_FMT) {
        char fmt_buf[512];
        if (!val_as_cstr(mem, a, fmt_buf, sizeof(fmt_buf))) {
            mem_err(mem, "fmt() first argument must be a format string");
            return val_void();
        }
        Value fmt_args[16];
        uint8_t n_fargs = 0;
        if (mem->vec_type_id != 0 && val_get_type(mem, b) == mem->vec_type_id &&
            val_get_type(mem, c) == MEM_TYPE_VOID) {
            uint16_t vhdr = val_get_pool_idx(b);
            uint16_t raw0 = (uint16_t)mem_read_pool(mem, vhdr + 0);
            uint16_t etid = (uint16_t)(raw0 & 0xFFu);
            uint8_t  euid = (uint8_t)((raw0 >> 8u) & 0xFFu);
            uint16_t vlen = (uint16_t)mem_read_pool(mem, vhdr + 1);
            uint16_t vdat = (uint16_t)mem_read_pool(mem, vhdr + 3);
            uint16_t strd = (etid > 0 && etid < tab->type_count) ? tab->types[etid].flat_size : 1;
            for (uint16_t k = 0; k < vlen && n_fargs < 16; ++k) {
                fmt_args[n_fargs++] = delegate_vec_read_elem(mem, tab, etid, euid, vdat, (uint32_t)k * (uint32_t)strd);
            }
        } else {
            if (val_get_type(mem, b) != MEM_TYPE_VOID) fmt_args[n_fargs++] = b;
            if (val_get_type(mem, c) != MEM_TYPE_VOID) fmt_args[n_fargs++] = c;
        }
        char out[1024];
        size_t pos = 0;
        uint8_t auto_idx = 0;
        for (size_t i = 0; fmt_buf[i] != '\0' && pos + 1 < sizeof(out); ) {
            if (fmt_buf[i] == '{' && fmt_buf[i + 1] == '{') {
                out[pos++] = '{'; i += 2; continue;
            }
            if (fmt_buf[i] == '}' && fmt_buf[i + 1] == '}') {
                out[pos++] = '}'; i += 2; continue;
            }
            if (fmt_buf[i] == '{') {
                size_t close = i + 1;
                while (fmt_buf[close] != '\0' && fmt_buf[close] != '}') close++;
                if (fmt_buf[close] == '}') {
                    char spec[64];
                    size_t slen = close - (i + 1);
                    if (slen >= sizeof(spec)) slen = sizeof(spec) - 1;
                    memcpy(spec, fmt_buf + i + 1, slen);
                    spec[slen] = '\0';
                    uint8_t use_idx = auto_idx;
                    const char *colon = strchr(spec, ':');
                    if (colon != NULL) {
                        if (colon > spec) use_idx = (uint8_t)atoi(spec);
                        else auto_idx++;
                    } else if (slen > 0 && spec[0] >= '0' && spec[0] <= '9') {
                        use_idx = (uint8_t)atoi(spec);
                    } else {
                        auto_idx++;
                    }
                    if (use_idx < n_fargs) {
                        Value fv = fmt_args[use_idx];
                        char vbuf[256];
                        if (colon != NULL && colon[1] != '\0') {
                            const char *sp = colon + 1;
                            bool zero_pad = (*sp == '0');
                            int width = atoi(sp);
                            int prec = -1;
                            const char *dot = strchr(sp, '.');
                            if (dot) prec = atoi(dot + 1);
                            char conv = sp[strlen(sp) - 1];
                            if (conv == 'd' || conv == 'i') {
                                if (zero_pad && width > 0) snprintf(vbuf, sizeof(vbuf), "%0*lld", width, (long long)llround(val_get_f64(mem, fv)));
                                else if (width > 0) snprintf(vbuf, sizeof(vbuf), "%*lld", width, (long long)llround(val_get_f64(mem, fv)));
                                else snprintf(vbuf, sizeof(vbuf), "%lld", (long long)llround(val_get_f64(mem, fv)));
                            } else if (conv == 'x' || conv == 'X') {
                                unsigned long long uv = (unsigned long long)llround(val_get_f64(mem, fv));
                                if (conv == 'x') snprintf(vbuf, sizeof(vbuf), "%llx", uv);
                                else snprintf(vbuf, sizeof(vbuf), "%llX", uv);
                            } else if (conv == 'b') {
                                unsigned long long uv = (unsigned long long)llround(val_get_f64(mem, fv));
                                if (uv == 0) { snprintf(vbuf, sizeof(vbuf), "0"); }
                                else {
                                    char btmp[72]; int bi = 0;
                                    while (uv > 0 && bi < 64) { btmp[bi++] = (char)('0' + (uv & 1ULL)); uv >>= 1ULL; }
                                    int oi = 0; while (bi > 0) vbuf[oi++] = btmp[--bi];
                                    vbuf[oi] = '\0';
                                }
                            } else if (conv == 'e') {
                                snprintf(vbuf, sizeof(vbuf), "%.*e", (prec >= 0) ? prec : 6, val_get_f64(mem, fv));
                            } else if (conv == 'g') {
                                snprintf(vbuf, sizeof(vbuf), "%.*g", (prec > 0) ? prec : 6, val_get_f64(mem, fv));
                            } else {
                                snprintf(vbuf, sizeof(vbuf), "%.*f", (prec >= 0) ? prec : 6, val_get_f64(mem, fv));
                            }
                        } else {
                            Value sv = delegate_eval_tostr(mem, tab, fv);
                            val_as_cstr(mem, sv, vbuf, sizeof(vbuf));
                        }
                        size_t vl = strlen(vbuf);
                        if (pos + vl >= sizeof(out)) vl = sizeof(out) - 1 - pos;
                        memcpy(out + pos, vbuf, vl);
                        pos += vl;
                    } else {
                        /* Preserve unconsumed placeholder for chained multi-arg @str_fmt calls */
                        size_t raw_l = (close - i) + 1;
                        if (pos + raw_l < sizeof(out)) {
                            memcpy(out + pos, fmt_buf + i, raw_l);
                            pos += raw_l;
                        }
                    }
                    i = close + 1;
                    continue;
                }
            }
            if (fmt_buf[i] == '%' && fmt_buf[i + 1] != '\0') {
                if (fmt_buf[i + 1] == '%') {
                    out[pos++] = '%'; i += 2; continue;
                }
                size_t j = i + 1;
                while (fmt_buf[j] != '\0' &&
                       (fmt_buf[j] == '-' || fmt_buf[j] == '+' || fmt_buf[j] == '0' ||
                        fmt_buf[j] == '.' || (fmt_buf[j] >= '0' && fmt_buf[j] <= '9'))) {
                    j++;
                }
                char conv = fmt_buf[j];
                if (conv == 'f' || conv == 'g' || conv == 'e' || conv == 'd' || conv == 'i' ||
                    conv == 'x' || conv == 'X' || conv == 's') {
                    char cfmt[32];
                    size_t flen = (j - i);
                    if (flen >= sizeof(cfmt)) flen = sizeof(cfmt) - 1;
                    memcpy(cfmt, fmt_buf + i + 1, flen - 1);
                    cfmt[flen - 1] = '\0';
                    bool zero_pad = (cfmt[0] == '0');
                    int width = atoi(cfmt);
                    int prec = -1;
                    const char *dot = strchr(cfmt, '.');
                    if (dot) prec = atoi(dot + 1);
                    if (auto_idx < n_fargs) {
                        Value fv = fmt_args[auto_idx++];
                        char vbuf[256];
                        if (conv == 's') {
                            Value sv = delegate_eval_tostr(mem, tab, fv);
                            char sraw[200];
                            val_as_cstr(mem, sv, sraw, sizeof(sraw));
                            if (width != 0) snprintf(vbuf, sizeof(vbuf), "%*s", width, sraw);
                            else snprintf(vbuf, sizeof(vbuf), "%s", sraw);
                        } else if (conv == 'd' || conv == 'i') {
                            long long iv = (long long)llround(val_get_f64(mem, fv));
                            if (zero_pad && width > 0) snprintf(vbuf, sizeof(vbuf), "%0*lld", width, iv);
                            else if (width != 0) snprintf(vbuf, sizeof(vbuf), "%*lld", width, iv);
                            else snprintf(vbuf, sizeof(vbuf), "%lld", iv);
                        } else if (conv == 'x' || conv == 'X') {
                            unsigned long long uv = (unsigned long long)llround(val_get_f64(mem, fv));
                            if (conv == 'x') snprintf(vbuf, sizeof(vbuf), "%llx", uv);
                            else snprintf(vbuf, sizeof(vbuf), "%llX", uv);
                        } else if (conv == 'e') {
                            snprintf(vbuf, sizeof(vbuf), "%.*e", (prec >= 0) ? prec : 6, val_get_f64(mem, fv));
                        } else if (conv == 'g') {
                            snprintf(vbuf, sizeof(vbuf), "%.*g", (prec > 0) ? prec : 6, val_get_f64(mem, fv));
                        } else {
                            if (width > 0) snprintf(vbuf, sizeof(vbuf), "%*.*f", width, (prec >= 0) ? prec : 6, val_get_f64(mem, fv));
                            else snprintf(vbuf, sizeof(vbuf), "%.*f", (prec >= 0) ? prec : 6, val_get_f64(mem, fv));
                        }
                        size_t vl = strlen(vbuf);
                        if (pos + vl >= sizeof(out)) vl = sizeof(out) - 1 - pos;
                        memcpy(out + pos, vbuf, vl);
                        pos += vl;
                    } else {
                        size_t raw_l = (j - i) + 1;
                        if (pos + raw_l < sizeof(out)) {
                            memcpy(out + pos, fmt_buf + i, raw_l);
                            pos += raw_l;
                        }
                    }
                    i = j + 1;
                    continue;
                }
            }
            out[pos++] = fmt_buf[i++];
        }
        out[pos] = '\0';
        return val_from_cstr(mem, out);
    }
    if (intr == INTR_REGEX_MATCH || intr == INTR_REGEX_FIND ||
        intr == INTR_REGEX_FINDALL || intr == INTR_REGEX_REPLACE || intr == INTR_REGEX_SPLIT) {
        char txt[512], pat[256];
        if (!val_as_cstr(mem, a, txt, sizeof(txt)) || !val_as_cstr(mem, b, pat, sizeof(pat))) {
            mem_err(mem, "Regex functions require string arguments (text, pattern)");
            return val_void();
        }
        uint16_t tlen = (uint16_t)strlen(txt);
        uint16_t plen = (uint16_t)strlen(pat);
        if (intr == INTR_REGEX_MATCH) {
            uint16_t mstart = 0, mlen = 0;
            bool ok = rx_search(pat, plen, txt, tlen, &mstart, &mlen);
            return val_make_f64(mem, mem->fast_real_type, ok ? 1.0 : 0.0);
        }
        if (intr == INTR_REGEX_FIND) {
            uint16_t mstart = 0, mlen = 0;
            if (rx_search(pat, plen, txt, tlen, &mstart, &mlen)) {
                StrSlice ms = mem_intern_str(mem, txt + mstart, mlen);
                return val_make_str(mem, mem->str_type_id, ms);
            }
            return val_from_cstr(mem, "");
        }
        if (intr == INTR_REGEX_FINDALL) {
            uint16_t hdr = mem_vec_alloc_hdr(mem, mem->str_type_id, 4);
            if (mem->has_error) return val_void();
            uint16_t pos = 0;
            while (pos <= tlen) {
                uint16_t mstart = 0, mlen = 0;
                if (!rx_search(pat, plen, txt + pos, (uint16_t)(tlen - pos), &mstart, &mlen)) break;
                StrSlice ms = mem_intern_str(mem, txt + pos + mstart, mlen);
                Value mv = val_make_str(mem, mem->str_type_id, ms);
                uint16_t cur_len = (uint16_t)mem_read_pool(mem, hdr + 1);
                if (!mem_vec_reserve(mem, hdr, 1, (uint16_t)(cur_len + 1))) return val_void();
                uint16_t dat = (uint16_t)mem_read_pool(mem, hdr + 3);
                delegate_vec_write_elem(mem, tab, mem->str_type_id, dat, cur_len, mv);
                mem_write_pool(mem, hdr + 1, (double)(cur_len + 1));
                pos = (uint16_t)(pos + mstart + (mlen > 0 ? mlen : 1));
            }
            return val_make_comp(mem, mem->vec_type_id, hdr, MEM_VEC_HDR_SLOTS);
        }
        if (intr == INTR_REGEX_SPLIT) {
            uint16_t hdr = mem_vec_alloc_hdr(mem, mem->str_type_id, 4);
            if (mem->has_error) return val_void();
            uint16_t pos = 0;
            while (pos <= tlen) {
                uint16_t mstart = 0, mlen = 0;
                if (!rx_search(pat, plen, txt + pos, (uint16_t)(tlen - pos), &mstart, &mlen) || mlen == 0) {
                    StrSlice ms = mem_intern_str(mem, txt + pos, (uint16_t)(tlen - pos));
                    Value mv = val_make_str(mem, mem->str_type_id, ms);
                    uint16_t cur_len = (uint16_t)mem_read_pool(mem, hdr + 1);
                    if (!mem_vec_reserve(mem, hdr, 1, (uint16_t)(cur_len + 1))) return val_void();
                    uint16_t dat = (uint16_t)mem_read_pool(mem, hdr + 3);
                    delegate_vec_write_elem(mem, tab, mem->str_type_id, dat, cur_len, mv);
                    mem_write_pool(mem, hdr + 1, (double)(cur_len + 1));
                    break;
                }
                StrSlice ms = mem_intern_str(mem, txt + pos, mstart);
                Value mv = val_make_str(mem, mem->str_type_id, ms);
                uint16_t cur_len = (uint16_t)mem_read_pool(mem, hdr + 1);
                if (!mem_vec_reserve(mem, hdr, 1, (uint16_t)(cur_len + 1))) return val_void();
                uint16_t dat = (uint16_t)mem_read_pool(mem, hdr + 3);
                delegate_vec_write_elem(mem, tab, mem->str_type_id, dat, cur_len, mv);
                mem_write_pool(mem, hdr + 1, (double)(cur_len + 1));
                pos = (uint16_t)(pos + mstart + mlen);
            }
            return val_make_comp(mem, mem->vec_type_id, hdr, MEM_VEC_HDR_SLOTS);
        }
        if (intr == INTR_REGEX_REPLACE) {
            char rep[256];
            if (!val_as_cstr(mem, c, rep, sizeof(rep))) rep[0] = '\0';
            size_t rlen = strlen(rep);
            char out[1024];
            size_t out_p = 0;
            uint16_t pos = 0;
            while (pos <= tlen && out_p + 1 < sizeof(out)) {
                uint16_t mstart = 0, mlen = 0;
                if (!rx_search(pat, plen, txt + pos, (uint16_t)(tlen - pos), &mstart, &mlen) || mlen == 0) {
                    size_t rem = (size_t)(tlen - pos);
                    if (out_p + rem >= sizeof(out)) rem = sizeof(out) - 1 - out_p;
                    memcpy(out + out_p, txt + pos, rem);
                    out_p += rem;
                    break;
                }
                size_t pre = mstart;
                if (out_p + pre >= sizeof(out)) pre = sizeof(out) - 1 - out_p;
                memcpy(out + out_p, txt + pos, pre);
                out_p += pre;
                size_t nw = rlen;
                if (out_p + nw >= sizeof(out)) nw = sizeof(out) - 1 - out_p;
                memcpy(out + out_p, rep, nw);
                out_p += nw;
                pos = (uint16_t)(pos + mstart + mlen);
            }
            out[out_p] = '\0';
            return val_from_cstr(mem, out);
        }
    }

    if (intr == INTR_TYPE_NAME) {
        char tbuf[32];
        sym_type_name(tab, mem, val_get_type(mem, a), tbuf, sizeof(tbuf));
        return val_from_cstr(mem, tbuf);
    }
    if (intr == INTR_UNIT_NAME) {
        uint8_t uid = val_get_unit(mem, a);
        if (val_get_type(mem, b) != MEM_TYPE_VOID) {
            int ax = 0;
            if (!delegate_val_to_exact_int(mem, tab, b, "SI dimension axis", &ax)) {
                return val_void();
            }
            if (ax < 0) ax += 7;
            if (ax < 0 || ax >= 7) {
                mem_err(mem, "SI dimension axis %d out of bounds (expected 0..6 for m,kg,s,A,K,mol,cd)", ax);
                return val_void();
            }
            int8_t e = (uid < SYM_MAX_UNITS && tab->unit_dims[uid].has_dim)
                       ? tab->unit_dims[uid].exp[ax] : 0;
            return val_make_f64(mem, mem->fast_real_type, (double)e);
        }
        char ubuf[32];
        sym_unit_name(tab, mem, uid, ubuf, sizeof(ubuf));
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
            if (val_get_type(mem, a) == mem->str_type_id && val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0) {
                StrSlice sa = val_get_str(mem, a);
                StrSlice sb = val_get_str(mem, b);
                uint32_t tot_len = (uint32_t)sa.len + (uint32_t)sb.len;
                if (tot_len >= (uint32_t)MEM_SRC_BUF_CAP) {
                    mem_err(mem, "Concatenated string length (%u) exceeds string arena capacity (%u)",
                            (unsigned)tot_len, (unsigned)MEM_SRC_BUF_CAP);
                    return val_void();
                }
                char stack_sbuf[1024];
                char *sbuf = stack_sbuf;
                if (tot_len >= sizeof(stack_sbuf)) {
                    sbuf = (char *)malloc(tot_len + 1u);
                    if (!sbuf) {
                        mem_err(mem, "Out of memory concatenating strings");
                        return val_void();
                    }
                }
                if (sa.len > 0) memcpy(sbuf, mem_str_ptr(mem, sa), sa.len);
                if (sb.len > 0) memcpy(sbuf + sa.len, mem_str_ptr(mem, sb), sb.len);
                StrSlice sc = mem_intern_str(mem, sbuf, (uint16_t)tot_len);
                if (sbuf != stack_sbuf) free(sbuf);
                if (mem->has_error) return val_void();
                return val_make_str(mem, mem->str_type_id, sc);
            }
            if (!delegate_check_additive_unit_compat(mem, tab, "+", 1, a, b, &ruid)) return val_void();
            res = da + db;
            break;
        case INTR_SUB:
            if (!delegate_check_additive_unit_compat(mem, tab, "-", 1, a, b, &ruid)) return val_void();
            res = da - db;
            break;
        case INTR_MUL:
            ruid = delegate_resolve_mul_unit(mem, tab, ua, ub);
            res = da * db;
            break;
        case INTR_DIV:
            if (db == 0.0) { mem_err(mem, "Division by zero"); return val_void(); }
            ruid = delegate_resolve_div_unit(mem, tab, ua, ub);
            res = da / db;
            break;
        case INTR_MOD:
            if (db == 0.0) { mem_err(mem, "Division by zero in modulo"); return val_void(); }
            if (ua != ub && ub != MEM_UNIT_NONE) {
                if (!delegate_check_additive_unit_compat(mem, tab, "%", 1, a, b, &ruid)) return val_void();
            } else {
                ruid = ua;
            }
            res = fmod(da, db);
            break;
        case INTR_NEG:   ruid = ua; res = -da; break;
        case INTR_ABS:   ruid = ua; res = fabs(da); break;
        case INTR_FLOOR: ruid = ua; res = floor(da); break;
        case INTR_CEIL:  ruid = ua; res = ceil(da); break;
        case INTR_POW:   ruid = delegate_resolve_pow_unit(mem, tab, ua, ub, db); res = pow(da, db); break;
        case INTR_SQRT:  ruid = delegate_resolve_root_unit(mem, tab, ua, 2); res = sqrt(da); break;
        case INTR_CBRT:  ruid = delegate_resolve_root_unit(mem, tab, ua, 3); res = cbrt(da); break;
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
        case INTR_SINH:   res = sinh(da); break;
        case INTR_COSH:   res = cosh(da); break;
        case INTR_TANH:   res = tanh(da); break;
        case INTR_ERF:    res = erf(da); break;
        case INTR_ERFC:   res = erfc(da); break;
        case INTR_GAMMA:  res = tgamma(da); break;
        case INTR_LGAMMA: res = lgamma(da); break;

        /*Below section is the C's native for operator delegates @eq, @ne, @lt, @le, @gt, @ge, @approx, @land, @lor, @cnot, @lnot, @lxor, @lnor, @lnand, @raw*/
        /*Construct ==, !=, <, <=, >, >=, ~, && / and, || / or, !, not, xor / ^^, nor, nand, i (iota)*/
        case INTR_EQ:
            if (val_get_type(mem, a) == mem->str_type_id && val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0) {
                res = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, b)) ? 1.0 : 0.0;
                break;
            }
            if (!delegate_check_additive_unit_compat(mem, tab, "==", 2, a, b, NULL)) return val_void();
            if (isnan(da) || isnan(db)) {
                res = 0.0;
            } else if (isinf(da) || isinf(db)) {
                res = (da == db) ? 1.0 : 0.0;
            } else if (da == 0.0 || db == 0.0) {
                res = (da == db) ? 1.0 : 0.0;
            } else {
                double scale = fmax(fabs(da), fabs(db));
                res = (fabs(da - db) <= 1e-14 * scale) ? 1.0 : 0.0;
            }
            break;
        case INTR_NE:
            if (val_get_type(mem, a) == mem->str_type_id && val_get_type(mem, b) == mem->str_type_id && mem->str_type_id != 0) {
                res = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, b)) ? 0.0 : 1.0;
                break;
            }
            if (!delegate_check_additive_unit_compat(mem, tab, "!=", 2, a, b, NULL)) return val_void();
            if (isnan(da) || isnan(db)) {
                res = 1.0;
            } else if (isinf(da) || isinf(db)) {
                res = (da != db) ? 1.0 : 0.0;
            } else if (da == 0.0 || db == 0.0) {
                res = (da != db) ? 1.0 : 0.0;
            } else {
                double scale = fmax(fabs(da), fabs(db));
                res = (fabs(da - db) > 1e-14 * scale) ? 1.0 : 0.0;
            }
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
        case INTR_BAND:
        case INTR_BOR:
        case INTR_BXOR:
        case INTR_BNOT:
        case INTR_BSHL:
        case INTR_BSHR: {
            if (ua != MEM_UNIT_NONE || (intr != INTR_BNOT && ub != MEM_UNIT_NONE)) {
                mem_err(mem, "Bitwise operation requires unitless integer operands");
                return val_void();
            }
            double ra = floor(da + 0.5);
            if (!isfinite(da) || da < -9007199254740991.0 || da > 9007199254740991.0 || fabs(da - ra) > 1e-9) {
                mem_err(mem, "Bitwise operation requires an exact integer operand (got %.6g)", da);
                return val_void();
            }
            int64_t ia = (int64_t)ra;
            if (intr == INTR_BNOT) {
                res = (double)(~ia);
                break;
            }
            double rb = floor(db + 0.5);
            if (!isfinite(db) || db < -9007199254740991.0 || db > 9007199254740991.0 || fabs(db - rb) > 1e-9) {
                mem_err(mem, "Bitwise operation requires an exact integer operand (got %.6g)", db);
                return val_void();
            }
            int64_t ib = (int64_t)rb;
            if (intr == INTR_BSHL || intr == INTR_BSHR) {
                if (ib < 0 || ib > 52) {
                    mem_err(mem, "Bit shift count %lld out of range (0..52)", (long long)ib);
                    return val_void();
                }
                res = (intr == INTR_BSHL)
                      ? (double)((int64_t)((uint64_t)ia << (unsigned)ib))
                      : (double)(ia >> (unsigned)ib);
            } else if (intr == INTR_BAND) {
                res = (double)(ia & ib);
            } else if (intr == INTR_BOR) {
                res = (double)(ia | ib);
            } else {
                res = (double)(ia ^ ib);
            }
            break;
        }
        case INTR_RAW:   ruid = (val_get_type(mem, b) != MEM_TYPE_VOID) ? ub : ua; res = da; break;
        default:         mem_err(mem, "Unhandled intrinsic %d", (int)intr); return val_void();
    }
    return val_make_f64_unit(mem, rtid, ruid, res);
}

/* ============================================================================
 * 6. CALCULUS, SOLVER & EXTREMA DELEGATES (`@calc_diff` .. `@calc_fmax`)
 * ============================================================================ */

/*Below section is the C's native for operator/function delegates @calc_diff, @calc_diff2, @calc_diffn, @calc_integ, @calc_lim, @calc_lim_l, @calc_lim_r, @calc_sigma, @calc_prod, @calc_solve, @calc_fmin, @calc_fmax*/
/*Construct diff, diff2, diff_n, integrate, ntg, lim, lim_left, lim_right, sigma, prod, solve, fmin, fmax*/
static double snap_calc_noise(double res, double tol) {
    double nearest = floor(res + 0.5);
    if (fabs(res - nearest) < tol) return nearest;
    if (fabs(res) < tol * 0.1) return 0.0;
    return res;
}

/* ============================================================================
 * 6a. SYMBOLIC DIFFERENTIATION, ALGEBRAIC SIMPLIFICATION & TAYLOR EXPANSION
 * ============================================================================ */

static uint16_t sym_make_num(MemArena *mem, double v) {
    uint16_t pidx = mem_alloc_pool(mem, 1);
    if (mem->has_error) return MEM_NO_IDX;
    mem_write_pool(mem, pidx, v);
    return mem_alloc_ast(mem, AST_LIT, mem->fast_real_type, pidx, 0, 0);
}

static bool sym_is_num(const MemArena *mem, uint16_t idx, double *out_v) {
    if (idx == MEM_NO_IDX) return false;
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_LIT && n->sub == mem->fast_real_type) {
        if (out_v) *out_v = mem_read_pool(mem, n->ref);
        return true;
    }
    return false;
}

static bool sym_ast_equal(const MemArena *mem, const SymTable *tab, uint16_t a, uint16_t b) {
    if (a == b) return true;
    if (a == MEM_NO_IDX || b == MEM_NO_IDX) return false;
    const AstNode *na = mem_get_ast(mem, a);
    const AstNode *nb = mem_get_ast(mem, b);
    if (na->kind != nb->kind) return false;
    if (na->kind == AST_LIT) {
        if (na->sub != nb->sub) return false;
        if (na->sub == mem->fast_real_type) {
            return fabs(mem_read_pool(mem, na->ref) - mem_read_pool(mem, nb->ref)) <= 1e-12;
        }
        return (na->left == nb->left && na->right == nb->right);
    }
    if (na->kind == AST_SLOT_LOAD) return (na->sub == nb->sub);
    if (na->kind == AST_CONST_SYM) return (na->ref == nb->ref);
    if (na->kind == AST_UNARY) {
        return (na->sub == nb->sub && na->ref == nb->ref && sym_ast_equal(mem, tab, na->left, nb->left));
    }
    if (na->kind == AST_BINARY) {
        return (na->sub == nb->sub && na->ref == nb->ref &&
                sym_ast_equal(mem, tab, na->left, nb->left) &&
                sym_ast_equal(mem, tab, na->right, nb->right));
    }
    if (na->kind == AST_CALL) {
        if (na->ref != nb->ref || na->sub != nb->sub) return false;
        uint16_t ca = na->left, cb = nb->left;
        while (ca != MEM_NO_IDX && cb != MEM_NO_IDX) {
            const AstNode *la = mem_get_ast(mem, ca);
            const AstNode *lb = mem_get_ast(mem, cb);
            if (!sym_ast_equal(mem, tab, la->left, lb->left)) return false;
            ca = la->right;
            cb = lb->right;
        }
        return (ca == cb);
    }
    return false;
}

static uint16_t sym_find_op(MemArena *mem, SymTable *tab, const char *s) {
    int id = sym_find(tab, mem, s, (uint16_t)strlen(s));
    if (id >= 0) return (uint16_t)id;
    StrSlice sl = mem_intern_str(mem, s, (uint16_t)strlen(s));
    return sym_intern(tab, mem, sl);
}

static uint16_t sym_make_bin(MemArena *mem, SymTable *tab, const char *op, uint16_t l, uint16_t r) {
    uint16_t sid = sym_find_op(mem, tab, op);
    uint8_t fix = (strcmp(op, "^") == 0) ? (uint8_t)FIX_INFIX_RIGHT : (uint8_t)FIX_INFIX_LEFT;
    return mem_alloc_ast(mem, AST_BINARY, fix, sid, l, r);
}

static uint16_t sym_make_neg(MemArena *mem, SymTable *tab, uint16_t x) {
    uint16_t sid = sym_find_op(mem, tab, "-");
    return mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_PREFIX, sid, x, MEM_NO_IDX);
}

static uint16_t sym_make_call1(MemArena *mem, SymTable *tab, const char *fn, uint16_t arg) {
    uint16_t sid  = sym_find_op(mem, tab, fn);
    uint16_t link = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, arg, MEM_NO_IDX);
    return mem_alloc_ast(mem, AST_CALL, 1, sid, link, MEM_NO_IDX);
}

static uint16_t sym_simplify_ast(MemArena *mem, SymTable *tab, uint16_t idx, uint8_t depth) {
    if (idx == MEM_NO_IDX || mem->has_error || depth > 64) return idx;
    const AstNode *n = mem_get_ast(mem, idx);

    if (n->kind == AST_UNARY && n->sub == (uint8_t)FIX_PREFIX) {
        StrSlice s = tab->syms[n->ref];
        uint16_t op_l = sym_simplify_ast(mem, tab, n->left, (uint8_t)(depth + 1));
        double lv = 0.0;
        if (mem_slice_eq_cstr(mem, s, "-")) {
            if (sym_is_num(mem, op_l, &lv)) return sym_make_num(mem, -lv);
            const AstNode *ln = mem_get_ast(mem, op_l);
            if (ln->kind == AST_UNARY && ln->sub == (uint8_t)FIX_PREFIX &&
                mem_slice_eq_cstr(mem, tab->syms[ln->ref], "-")) {
                return ln->left;
            }
            if (ln->kind == AST_BINARY && mem_slice_eq_cstr(mem, tab->syms[ln->ref], "-")) {
                return sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "-", ln->right, ln->left), (uint8_t)(depth + 1));
            }
            return sym_make_neg(mem, tab, op_l);
        }
        if (mem_slice_eq_cstr(mem, s, "+")) return op_l;
        if (mem_slice_eq_cstr(mem, s, "√")) {
            if (sym_is_num(mem, op_l, &lv) && lv >= 0.0) {
                double r = sqrt(lv);
                if (fabs(r - floor(r + 0.5)) <= 1e-12) return sym_make_num(mem, floor(r + 0.5));
            }
            return mem_alloc_ast(mem, AST_UNARY, (uint8_t)FIX_PREFIX, n->ref, op_l, MEM_NO_IDX);
        }
        return mem_alloc_ast(mem, AST_UNARY, n->sub, n->ref, op_l, MEM_NO_IDX);
    }

    if (n->kind == AST_BINARY) {
        StrSlice s = tab->syms[n->ref];
        uint16_t l = sym_simplify_ast(mem, tab, n->left,  (uint8_t)(depth + 1));
        uint16_t r = sym_simplify_ast(mem, tab, n->right, (uint8_t)(depth + 1));
        double lv = 0.0, rv = 0.0;
        bool l_num = sym_is_num(mem, l, &lv);
        bool r_num = sym_is_num(mem, r, &rv);

        if (mem_slice_eq_cstr(mem, s, "+")) {
            if (l_num && r_num) return sym_make_num(mem, lv + rv);
            if (l_num && fabs(lv) <= 1e-12) return r;
            if (r_num && fabs(rv) <= 1e-12) return l;
            if (sym_ast_equal(mem, tab, l, r)) {
                return sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0), l), (uint8_t)(depth + 1));
            }
            /* Combine c1 * term + c2 * term (including implicit c=1 on bare term) */
            const AstNode *ln = mem_get_ast(mem, l);
            const AstNode *rn = mem_get_ast(mem, r);
            double c1 = 1.0, c2 = 1.0;
            uint16_t b1 = l, b2 = r;
            if (ln->kind == AST_BINARY && mem_slice_eq_cstr(mem, tab->syms[ln->ref], "*") &&
                sym_is_num(mem, ln->left, &c1)) {
                b1 = ln->right;
            }
            if (rn->kind == AST_BINARY && mem_slice_eq_cstr(mem, tab->syms[rn->ref], "*") &&
                sym_is_num(mem, rn->left, &c2)) {
                b2 = rn->right;
            }
            if (!l_num && !r_num && sym_ast_equal(mem, tab, b1, b2)) {
                return sym_simplify_ast(mem, tab,
                    sym_make_bin(mem, tab, "*", sym_make_num(mem, c1 + c2), b1),
                    (uint8_t)(depth + 1));
            }
            if (rn->kind == AST_UNARY && rn->sub == (uint8_t)FIX_PREFIX &&
                mem_slice_eq_cstr(mem, tab->syms[rn->ref], "-")) {
                return sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "-", l, rn->left), (uint8_t)(depth + 1));
            }
            return sym_make_bin(mem, tab, "+", l, r);
        }
        if (mem_slice_eq_cstr(mem, s, "-")) {
            if (l_num && r_num) return sym_make_num(mem, lv - rv);
            if (r_num && fabs(rv) <= 1e-12) return l;
            if (l_num && fabs(lv) <= 1e-12) return sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, r), (uint8_t)(depth + 1));
            if (sym_ast_equal(mem, tab, l, r)) return sym_make_num(mem, 0.0);
            return sym_make_bin(mem, tab, "-", l, r);
        }
        if (mem_slice_eq_cstr(mem, s, "*")) {
            if (l_num && r_num) return sym_make_num(mem, lv * rv);
            if ((l_num && fabs(lv) <= 1e-12) || (r_num && fabs(rv) <= 1e-12)) return sym_make_num(mem, 0.0);
            if (l_num && fabs(lv - 1.0) <= 1e-12) return r;
            if (r_num && fabs(rv - 1.0) <= 1e-12) return l;
            if (l_num && fabs(lv + 1.0) <= 1e-12) return sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, r), (uint8_t)(depth + 1));
            if (r_num && fabs(rv + 1.0) <= 1e-12) return sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, l), (uint8_t)(depth + 1));
            /* Canonicalize constant factor to the left: x * c -> c * x */
            if (!l_num && r_num) {
                uint16_t tmp = l; l = r; r = tmp;
                lv = rv; l_num = true; r_num = false;
            }
            /* Fold c1 * (c2 * x) -> (c1 * c2) * x */
            const AstNode *rn = mem_get_ast(mem, r);
            if (l_num && rn->kind == AST_BINARY && mem_slice_eq_cstr(mem, tab->syms[rn->ref], "*")) {
                double c2 = 0.0;
                if (sym_is_num(mem, rn->left, &c2)) {
                    return sym_simplify_ast(mem, tab,
                        sym_make_bin(mem, tab, "*", sym_make_num(mem, lv * c2), rn->right),
                        (uint8_t)(depth + 1));
                }
            }
            if (l_num && rn->kind == AST_BINARY && mem_slice_eq_cstr(mem, tab->syms[rn->ref], "/")) {
                double d2 = 0.0;
                if (sym_is_num(mem, rn->right, &d2) && fabs(d2) > 1e-12) {
                    double q = lv / d2;
                    if (fabs(q - floor(q + 0.5)) <= 1e-12) {
                        return sym_simplify_ast(mem, tab,
                            sym_make_bin(mem, tab, "*", sym_make_num(mem, floor(q + 0.5)), rn->left),
                            (uint8_t)(depth + 1));
                    }
                }
            }
            if (sym_ast_equal(mem, tab, l, r)) {
                return sym_make_bin(mem, tab, "^", l, sym_make_num(mem, 2.0));
            }
            return sym_make_bin(mem, tab, "*", l, r);
        }
        if (mem_slice_eq_cstr(mem, s, "/")) {
            if (l_num && fabs(lv) <= 1e-12) return sym_make_num(mem, 0.0);
            if (r_num && fabs(rv - 1.0) <= 1e-12) return l;
            if (l_num && r_num && fabs(rv) > 1e-12) {
                double q = lv / rv;
                if (fabs(q - floor(q + 0.5)) <= 1e-12) return sym_make_num(mem, floor(q + 0.5));
            }
            if (sym_ast_equal(mem, tab, l, r)) return sym_make_num(mem, 1.0);
            return sym_make_bin(mem, tab, "/", l, r);
        }
        if (mem_slice_eq_cstr(mem, s, "^")) {
            if (r_num && fabs(rv) <= 1e-12) return sym_make_num(mem, 1.0);
            if (r_num && fabs(rv - 1.0) <= 1e-12) return l;
            if (l_num && fabs(lv) <= 1e-12) return sym_make_num(mem, 0.0);
            if (l_num && fabs(lv - 1.0) <= 1e-12) return sym_make_num(mem, 1.0);
            if (l_num && r_num) return sym_make_num(mem, pow(lv, rv));
            const AstNode *ln = mem_get_ast(mem, l);
            if (r_num && ln->kind == AST_BINARY && mem_slice_eq_cstr(mem, tab->syms[ln->ref], "^")) {
                double p1 = 0.0;
                if (sym_is_num(mem, ln->right, &p1)) {
                    return sym_simplify_ast(mem, tab,
                        sym_make_bin(mem, tab, "^", ln->left, sym_make_num(mem, p1 * rv)),
                        (uint8_t)(depth + 1));
                }
            }
            return sym_make_bin(mem, tab, "^", l, r);
        }
        return mem_alloc_ast(mem, AST_BINARY, n->sub, n->ref, l, r);
    }

    if (n->kind == AST_CALL && n->sub == 1 && n->left != MEM_NO_IDX) {
        StrSlice fn_s = tab->syms[n->ref];
        const AstNode *link = mem_get_ast(mem, n->left);
        uint16_t arg = sym_simplify_ast(mem, tab, link->left, (uint8_t)(depth + 1));
        double av = 0.0;
        if (sym_is_num(mem, arg, &av)) {
            if (mem_slice_eq_cstr(mem, fn_s, "sin") && fabs(av) <= 1e-12) return sym_make_num(mem, 0.0);
            if (mem_slice_eq_cstr(mem, fn_s, "cos") && fabs(av) <= 1e-12) return sym_make_num(mem, 1.0);
            if (mem_slice_eq_cstr(mem, fn_s, "tan") && fabs(av) <= 1e-12) return sym_make_num(mem, 0.0);
            if (mem_slice_eq_cstr(mem, fn_s, "exp") && fabs(av) <= 1e-12) return sym_make_num(mem, 1.0);
            if (mem_slice_eq_cstr(mem, fn_s, "ln")  && fabs(av - 1.0) <= 1e-12) return sym_make_num(mem, 0.0);
            if (mem_slice_eq_cstr(mem, fn_s, "sqrt") && av >= 0.0) {
                double r = sqrt(av);
                if (fabs(r - floor(r + 0.5)) <= 1e-12) return sym_make_num(mem, floor(r + 0.5));
            }
        }
        if (mem_slice_eq_cstr(mem, fn_s, "ln")) {
            const AstNode *an = mem_get_ast(mem, arg);
            if (an->kind == AST_CONST_SYM && mem_slice_eq_cstr(mem, tab->syms[an->ref], "e")) {
                return sym_make_num(mem, 1.0);
            }
        }
        uint16_t nlink = mem_alloc_ast(mem, AST_ARG_LINK, 0, 0, arg, MEM_NO_IDX);
        return mem_alloc_ast(mem, AST_CALL, 1, n->ref, nlink, MEM_NO_IDX);
    }

    return idx;
}

static bool sym_depends_on(const MemArena *mem, uint16_t idx, uint8_t var_slot) {
    if (idx == MEM_NO_IDX) return false;
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_SLOT_LOAD) return (n->sub == var_slot);
    if (n->kind == AST_UNARY) return sym_depends_on(mem, n->left, var_slot);
    if (n->kind == AST_BINARY) {
        return sym_depends_on(mem, n->left, var_slot) || sym_depends_on(mem, n->right, var_slot);
    }
    if (n->kind == AST_CALL) {
        uint16_t cur = n->left;
        while (cur != MEM_NO_IDX) {
            const AstNode *l = mem_get_ast(mem, cur);
            if (sym_depends_on(mem, l->left, var_slot)) return true;
            cur = l->right;
        }
    }
    return false;
}

static uint16_t sym_diff_ast(MemArena *mem, SymTable *tab, uint16_t idx, uint8_t var_slot) {
    if (idx == MEM_NO_IDX || mem->has_error) return MEM_NO_IDX;
    if (!sym_depends_on(mem, idx, var_slot)) {
        return sym_make_num(mem, 0.0);
    }
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_SLOT_LOAD) {
        return sym_make_num(mem, (n->sub == var_slot) ? 1.0 : 0.0);
    }
    if (n->kind == AST_UNARY && n->sub == (uint8_t)FIX_PREFIX) {
        StrSlice s = tab->syms[n->ref];
        uint16_t u  = n->left;
        uint16_t du = sym_diff_ast(mem, tab, u, var_slot);
        if (mem_slice_eq_cstr(mem, s, "-")) return sym_make_neg(mem, tab, du);
        if (mem_slice_eq_cstr(mem, s, "+")) return du;
        if (mem_slice_eq_cstr(mem, s, "√")) {
            uint16_t denom = sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0),
                                          sym_make_call1(mem, tab, "sqrt", u));
            return sym_make_bin(mem, tab, "/", du, denom);
        }
    }
    if (n->kind == AST_BINARY) {
        StrSlice s = tab->syms[n->ref];
        uint16_t u  = n->left,  v  = n->right;
        uint16_t du = sym_diff_ast(mem, tab, u, var_slot);
        uint16_t dv = sym_diff_ast(mem, tab, v, var_slot);
        if (mem_slice_eq_cstr(mem, s, "+")) return sym_make_bin(mem, tab, "+", du, dv);
        if (mem_slice_eq_cstr(mem, s, "-")) return sym_make_bin(mem, tab, "-", du, dv);
        if (mem_slice_eq_cstr(mem, s, "*")) {
            uint16_t t1 = sym_make_bin(mem, tab, "*", du, v);
            uint16_t t2 = sym_make_bin(mem, tab, "*", u, dv);
            return sym_make_bin(mem, tab, "+", t1, t2);
        }
        if (mem_slice_eq_cstr(mem, s, "/")) {
            uint16_t num = sym_make_bin(mem, tab, "-",
                                        sym_make_bin(mem, tab, "*", du, v),
                                        sym_make_bin(mem, tab, "*", u, dv));
            uint16_t den = sym_make_bin(mem, tab, "^", v, sym_make_num(mem, 2.0));
            return sym_make_bin(mem, tab, "/", num, den);
        }
        if (mem_slice_eq_cstr(mem, s, "^")) {
            if (!sym_depends_on(mem, v, var_slot)) {
                uint16_t vm1  = sym_make_bin(mem, tab, "-", v, sym_make_num(mem, 1.0));
                uint16_t upow = sym_make_bin(mem, tab, "^", u, vm1);
                uint16_t base = sym_make_bin(mem, tab, "*", v, upow);
                return sym_make_bin(mem, tab, "*", base, du);
            }
            if (!sym_depends_on(mem, u, var_slot)) {
                uint16_t lnu  = sym_make_call1(mem, tab, "ln", u);
                uint16_t base = sym_make_bin(mem, tab, "*", idx, lnu);
                return sym_make_bin(mem, tab, "*", base, dv);
            }
            /* General u(x)^v(x) = u^v * (dv * ln(u) + v * du / u) */
            uint16_t lnu = sym_make_call1(mem, tab, "ln", u);
            uint16_t p1  = sym_make_bin(mem, tab, "*", dv, lnu);
            uint16_t p2  = sym_make_bin(mem, tab, "/", sym_make_bin(mem, tab, "*", v, du), u);
            return sym_make_bin(mem, tab, "*", idx, sym_make_bin(mem, tab, "+", p1, p2));
        }
    }
    if (n->kind == AST_CALL && n->sub == 1 && n->left != MEM_NO_IDX) {
        StrSlice s = tab->syms[n->ref];
        uint16_t u  = mem_get_ast(mem, n->left)->left;
        uint16_t du = sym_diff_ast(mem, tab, u, var_slot);
        uint16_t outer = MEM_NO_IDX;
        if (mem_slice_eq_cstr(mem, s, "sin")) {
            outer = sym_make_call1(mem, tab, "cos", u);
        } else if (mem_slice_eq_cstr(mem, s, "cos")) {
            outer = sym_make_neg(mem, tab, sym_make_call1(mem, tab, "sin", u));
        } else if (mem_slice_eq_cstr(mem, s, "tan")) {
            uint16_t cos_u = sym_make_call1(mem, tab, "cos", u);
            outer = sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0),
                                 sym_make_bin(mem, tab, "^", cos_u, sym_make_num(mem, 2.0)));
        } else if (mem_slice_eq_cstr(mem, s, "exp")) {
            outer = sym_make_call1(mem, tab, "exp", u);
        } else if (mem_slice_eq_cstr(mem, s, "ln")) {
            outer = sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0), u);
        } else if (mem_slice_eq_cstr(mem, s, "sqrt")) {
            uint16_t den = sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0),
                                        sym_make_call1(mem, tab, "sqrt", u));
            outer = sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0), den);
        } else if (mem_slice_eq_cstr(mem, s, "sinh")) {
            outer = sym_make_call1(mem, tab, "cosh", u);
        } else if (mem_slice_eq_cstr(mem, s, "cosh")) {
            outer = sym_make_call1(mem, tab, "sinh", u);
        } else if (mem_slice_eq_cstr(mem, s, "tanh")) {
            uint16_t ch = sym_make_call1(mem, tab, "cosh", u);
            outer = sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0),
                                 sym_make_bin(mem, tab, "^", ch, sym_make_num(mem, 2.0)));
        } else if (mem_slice_eq_cstr(mem, s, "asin")) {
            uint16_t rad = sym_make_bin(mem, tab, "-", sym_make_num(mem, 1.0),
                                        sym_make_bin(mem, tab, "^", u, sym_make_num(mem, 2.0)));
            outer = sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0),
                                 sym_make_call1(mem, tab, "sqrt", rad));
        } else if (mem_slice_eq_cstr(mem, s, "acos")) {
            uint16_t rad = sym_make_bin(mem, tab, "-", sym_make_num(mem, 1.0),
                                        sym_make_bin(mem, tab, "^", u, sym_make_num(mem, 2.0)));
            outer = sym_make_neg(mem, tab, sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0),
                                                        sym_make_call1(mem, tab, "sqrt", rad)));
        } else if (mem_slice_eq_cstr(mem, s, "atan")) {
            uint16_t den = sym_make_bin(mem, tab, "+", sym_make_num(mem, 1.0),
                                        sym_make_bin(mem, tab, "^", u, sym_make_num(mem, 2.0)));
            outer = sym_make_bin(mem, tab, "/", sym_make_num(mem, 1.0), den);
        }
        if (outer != MEM_NO_IDX) {
            return sym_make_bin(mem, tab, "*", outer, du);
        }
    }
    return sym_make_num(mem, 0.0);
}


static bool sym_extract_linear(const MemArena *mem, const SymTable *tab,
                               uint16_t idx, uint8_t var_slot,
                               double *out_a, double *out_b) {
    if (idx == MEM_NO_IDX) return false;
    double nv = 0.0;
    if (sym_is_num(mem, idx, &nv)) {
        *out_a = 0.0;
        *out_b = nv;
        return true;
    }
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_SLOT_LOAD && n->sub == var_slot) {
        *out_a = 1.0;
        *out_b = 0.0;
        return true;
    }
    if (!sym_depends_on(mem, idx, var_slot)) return false;
    if (n->kind == AST_UNARY && n->sub == (uint8_t)FIX_PREFIX) {
        StrSlice s = tab->syms[n->ref];
        double a = 0.0, b = 0.0;
        if (mem_slice_eq_cstr(mem, s, "-") && sym_extract_linear(mem, tab, n->left, var_slot, &a, &b)) {
            *out_a = -a;
            *out_b = -b;
            return true;
        }
        if (mem_slice_eq_cstr(mem, s, "+") && sym_extract_linear(mem, tab, n->left, var_slot, &a, &b)) {
            *out_a = a;
            *out_b = b;
            return true;
        }
        return false;
    }
    if (n->kind == AST_BINARY) {
        StrSlice s = tab->syms[n->ref];
        double al = 0.0, bl = 0.0, ar = 0.0, br = 0.0;
        if (mem_slice_eq_cstr(mem, s, "+")) {
            if (sym_extract_linear(mem, tab, n->left, var_slot, &al, &bl) &&
                sym_extract_linear(mem, tab, n->right, var_slot, &ar, &br)) {
                *out_a = al + ar;
                *out_b = bl + br;
                return true;
            }
        } else if (mem_slice_eq_cstr(mem, s, "-")) {
            if (sym_extract_linear(mem, tab, n->left, var_slot, &al, &bl) &&
                sym_extract_linear(mem, tab, n->right, var_slot, &ar, &br)) {
                *out_a = al - ar;
                *out_b = bl - br;
                return true;
            }
        } else if (mem_slice_eq_cstr(mem, s, "*")) {
            if (sym_is_num(mem, n->left, &al) &&
                sym_extract_linear(mem, tab, n->right, var_slot, &ar, &br)) {
                *out_a = al * ar;
                *out_b = al * br;
                return true;
            }
            if (sym_is_num(mem, n->right, &ar) &&
                sym_extract_linear(mem, tab, n->left, var_slot, &al, &bl)) {
                *out_a = al * ar;
                *out_b = bl * ar;
                return true;
            }
        } else if (mem_slice_eq_cstr(mem, s, "/")) {
            if (sym_is_num(mem, n->right, &ar) && fabs(ar) > 1e-12 &&
                sym_extract_linear(mem, tab, n->left, var_slot, &al, &bl)) {
                *out_a = al / ar;
                *out_b = bl / ar;
                return true;
            }
        }
    }
    return false;
}

static uint16_t sym_integ_ast(MemArena *mem, SymTable *tab, uint16_t idx, uint8_t var_slot, uint8_t depth) {
    if (idx == MEM_NO_IDX || mem->has_error || depth > 48) return MEM_NO_IDX;
    uint16_t var_ref = mem_alloc_ast(mem, AST_SLOT_LOAD, var_slot, 0, 0, 0);

    if (!sym_depends_on(mem, idx, var_slot)) {
        return sym_make_bin(mem, tab, "*", idx, var_ref);
    }

    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_SLOT_LOAD && n->sub == var_slot) {
        return sym_make_bin(mem, tab, "/",
                            sym_make_bin(mem, tab, "^", var_ref, sym_make_num(mem, 2.0)),
                            sym_make_num(mem, 2.0));
    }

    if (n->kind == AST_UNARY && n->sub == (uint8_t)FIX_PREFIX) {
        StrSlice s = tab->syms[n->ref];
        if (mem_slice_eq_cstr(mem, s, "-")) {
            uint16_t sub_i = sym_integ_ast(mem, tab, n->left, var_slot, (uint8_t)(depth + 1));
            return (sub_i != MEM_NO_IDX) ? sym_make_neg(mem, tab, sub_i) : MEM_NO_IDX;
        }
        if (mem_slice_eq_cstr(mem, s, "+")) {
            return sym_integ_ast(mem, tab, n->left, var_slot, (uint8_t)(depth + 1));
        }
        if (mem_slice_eq_cstr(mem, s, "√")) {
            double a = 0.0, b = 0.0;
            if (sym_extract_linear(mem, tab, n->left, var_slot, &a, &b) && fabs(a) > 1e-12) {
                uint16_t num = sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0),
                                            sym_make_bin(mem, tab, "^", n->left, sym_make_num(mem, 1.5)));
                return sym_make_bin(mem, tab, "/", num, sym_make_num(mem, 3.0 * a));
            }
        }
    }

    if (n->kind == AST_BINARY) {
        StrSlice s = tab->syms[n->ref];
        uint16_t u = n->left, v = n->right;
        if (mem_slice_eq_cstr(mem, s, "+")) {
            uint16_t iu = sym_integ_ast(mem, tab, u, var_slot, (uint8_t)(depth + 1));
            uint16_t iv = sym_integ_ast(mem, tab, v, var_slot, (uint8_t)(depth + 1));
            if (iu != MEM_NO_IDX && iv != MEM_NO_IDX) return sym_make_bin(mem, tab, "+", iu, iv);
            return MEM_NO_IDX;
        }
        if (mem_slice_eq_cstr(mem, s, "-")) {
            uint16_t iu = sym_integ_ast(mem, tab, u, var_slot, (uint8_t)(depth + 1));
            uint16_t iv = sym_integ_ast(mem, tab, v, var_slot, (uint8_t)(depth + 1));
            if (iu != MEM_NO_IDX && iv != MEM_NO_IDX) return sym_make_bin(mem, tab, "-", iu, iv);
            return MEM_NO_IDX;
        }
        if (mem_slice_eq_cstr(mem, s, "*")) {
            if (!sym_depends_on(mem, u, var_slot)) {
                uint16_t iv = sym_integ_ast(mem, tab, v, var_slot, (uint8_t)(depth + 1));
                return (iv != MEM_NO_IDX) ? sym_make_bin(mem, tab, "*", u, iv) : MEM_NO_IDX;
            }
            if (!sym_depends_on(mem, v, var_slot)) {
                uint16_t iu = sym_integ_ast(mem, tab, u, var_slot, (uint8_t)(depth + 1));
                return (iu != MEM_NO_IDX) ? sym_make_bin(mem, tab, "*", v, iu) : MEM_NO_IDX;
            }
            /* Integration by parts: \int u dv = u*v_int - \int (du * v_int) dx when du is constant wrt var_slot */
            uint16_t du = sym_simplify_ast(mem, tab, sym_diff_ast(mem, tab, u, var_slot), 0);
            if (!mem->has_error && !sym_depends_on(mem, du, var_slot)) {
                uint16_t iv = sym_integ_ast(mem, tab, v, var_slot, (uint8_t)(depth + 1));
                if (iv != MEM_NO_IDX && !mem->has_error) {
                    uint16_t du_iv = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", du, iv), 0);
                    uint16_t rem   = sym_integ_ast(mem, tab, du_iv, var_slot, (uint8_t)(depth + 1));
                    if (rem != MEM_NO_IDX && !mem->has_error) {
                        return sym_make_bin(mem, tab, "-", sym_make_bin(mem, tab, "*", u, iv), rem);
                    }
                }
                mem->has_error = false;
                mem->err_msg[0] = 0;
            }
            uint16_t dv = sym_simplify_ast(mem, tab, sym_diff_ast(mem, tab, v, var_slot), 0);
            if (!mem->has_error && !sym_depends_on(mem, dv, var_slot)) {
                uint16_t iu = sym_integ_ast(mem, tab, u, var_slot, (uint8_t)(depth + 1));
                if (iu != MEM_NO_IDX && !mem->has_error) {
                    uint16_t dv_iu = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", dv, iu), 0);
                    uint16_t rem   = sym_integ_ast(mem, tab, dv_iu, var_slot, (uint8_t)(depth + 1));
                    if (rem != MEM_NO_IDX && !mem->has_error) {
                        return sym_make_bin(mem, tab, "-", sym_make_bin(mem, tab, "*", v, iu), rem);
                    }
                }
                mem->has_error = false;
                mem->err_msg[0] = 0;
            }
        }
        if (mem_slice_eq_cstr(mem, s, "/")) {
            if (!sym_depends_on(mem, v, var_slot)) {
                uint16_t iu = sym_integ_ast(mem, tab, u, var_slot, (uint8_t)(depth + 1));
                return (iu != MEM_NO_IDX) ? sym_make_bin(mem, tab, "/", iu, v) : MEM_NO_IDX;
            }
            if (!sym_depends_on(mem, u, var_slot)) {
                double a = 0.0, b = 0.0;
                if (sym_extract_linear(mem, tab, v, var_slot, &a, &b) && fabs(a) > 1e-12) {
                    uint16_t lnv = sym_make_call1(mem, tab, "ln", v);
                    uint16_t num = sym_make_bin(mem, tab, "*", u, lnv);
                    return sym_make_bin(mem, tab, "/", num, sym_make_num(mem, a));
                }
            }
        }
        if (mem_slice_eq_cstr(mem, s, "^")) {
            double p = 0.0;
            if (sym_is_num(mem, v, &p)) {
                double a = 0.0, b = 0.0;
                if (sym_extract_linear(mem, tab, u, var_slot, &a, &b) && fabs(a) > 1e-12) {
                    if (fabs(p + 1.0) <= 1e-12) {
                        return sym_make_bin(mem, tab, "/", sym_make_call1(mem, tab, "ln", u), sym_make_num(mem, a));
                    }
                    uint16_t np1  = sym_make_num(mem, p + 1.0);
                    uint16_t upow = sym_make_bin(mem, tab, "^", u, np1);
                    return sym_make_bin(mem, tab, "/", upow, sym_make_num(mem, a * (p + 1.0)));
                }
            }
            const AstNode *un = mem_get_ast(mem, u);
            if (un->kind == AST_CONST_SYM && mem_slice_eq_cstr(mem, tab->syms[un->ref], "e")) {
                double a = 0.0, b = 0.0;
                if (sym_extract_linear(mem, tab, v, var_slot, &a, &b) && fabs(a) > 1e-12) {
                    return sym_make_bin(mem, tab, "/", idx, sym_make_num(mem, a));
                }
            }
        }
    }

    if (n->kind == AST_CALL && n->sub == 1 && n->left != MEM_NO_IDX) {
        StrSlice s = tab->syms[n->ref];
        uint16_t u = mem_get_ast(mem, n->left)->left;
        double a = 0.0, b = 0.0;
        if (sym_extract_linear(mem, tab, u, var_slot, &a, &b) && fabs(a) > 1e-12) {
            uint16_t ant = MEM_NO_IDX;
            if (mem_slice_eq_cstr(mem, s, "sin")) {
                ant = sym_make_neg(mem, tab, sym_make_call1(mem, tab, "cos", u));
            } else if (mem_slice_eq_cstr(mem, s, "cos")) {
                ant = sym_make_call1(mem, tab, "sin", u);
            } else if (mem_slice_eq_cstr(mem, s, "exp")) {
                ant = sym_make_call1(mem, tab, "exp", u);
            } else if (mem_slice_eq_cstr(mem, s, "sinh")) {
                ant = sym_make_call1(mem, tab, "cosh", u);
            } else if (mem_slice_eq_cstr(mem, s, "cosh")) {
                ant = sym_make_call1(mem, tab, "sinh", u);
            } else if (mem_slice_eq_cstr(mem, s, "sqrt")) {
                ant = sym_make_bin(mem, tab, "/",
                                   sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0),
                                                sym_make_bin(mem, tab, "^", u, sym_make_num(mem, 1.5))),
                                   sym_make_num(mem, 3.0));
            } else if (mem_slice_eq_cstr(mem, s, "ln")) {
                ant = sym_make_bin(mem, tab, "-",
                                   sym_make_bin(mem, tab, "*", u, sym_make_call1(mem, tab, "ln", u)),
                                   u);
            }
            if (ant != MEM_NO_IDX) {
                return sym_make_bin(mem, tab, "/", ant, sym_make_num(mem, a));
            }
        }
    }

    mem_err(mem, "Symbolic antiderivative does not have a elementary table rule for this expression");
    return MEM_NO_IDX;
}

static bool sym_is_zero_ast(const MemArena *mem, uint16_t idx) {
    double v = 0.0;
    return sym_is_num(mem, idx, &v) && fabs(v) <= 1e-12;
}

/* Decompose expression wrt `var_slot` into quadratic A*x^2 + B*x + C = 0 where A, B, C do not depend on `var_slot` */
static bool sym_decompose_poly(MemArena *mem, SymTable *tab, uint16_t idx, uint8_t var_slot,
                               uint16_t *out_a, uint16_t *out_b, uint16_t *out_c, uint8_t depth) {
    if (idx == MEM_NO_IDX || mem->has_error || depth > 48) return false;
    if (!sym_depends_on(mem, idx, var_slot)) {
        *out_a = sym_make_num(mem, 0.0);
        *out_b = sym_make_num(mem, 0.0);
        *out_c = idx;
        return true;
    }
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_SLOT_LOAD && n->sub == var_slot) {
        *out_a = sym_make_num(mem, 0.0);
        *out_b = sym_make_num(mem, 1.0);
        *out_c = sym_make_num(mem, 0.0);
        return true;
    }
    if (n->kind == AST_UNARY && n->sub == (uint8_t)FIX_PREFIX) {
        StrSlice s = tab->syms[n->ref];
        uint16_t a1 = 0, b1 = 0, c1 = 0;
        if (mem_slice_eq_cstr(mem, s, "-") &&
            sym_decompose_poly(mem, tab, n->left, var_slot, &a1, &b1, &c1, (uint8_t)(depth + 1))) {
            *out_a = sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, a1), 0);
            *out_b = sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, b1), 0);
            *out_c = sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, c1), 0);
            return true;
        }
        if (mem_slice_eq_cstr(mem, s, "+")) {
            return sym_decompose_poly(mem, tab, n->left, var_slot, out_a, out_b, out_c, (uint8_t)(depth + 1));
        }
        return false;
    }
    if (n->kind == AST_BINARY) {
        StrSlice s = tab->syms[n->ref];
        uint16_t al = 0, bl = 0, cl = 0, ar = 0, br = 0, cr = 0;
        if (mem_slice_eq_cstr(mem, s, "+") || mem_slice_eq_cstr(mem, s, "-")) {
            const char *op = mem_slice_eq_cstr(mem, s, "+") ? "+" : "-";
            if (!sym_decompose_poly(mem, tab, n->left,  var_slot, &al, &bl, &cl, (uint8_t)(depth + 1)) ||
                !sym_decompose_poly(mem, tab, n->right, var_slot, &ar, &br, &cr, (uint8_t)(depth + 1))) {
                return false;
            }
            *out_a = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, op, al, ar), 0);
            *out_b = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, op, bl, br), 0);
            *out_c = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, op, cl, cr), 0);
            return true;
        }
        if (mem_slice_eq_cstr(mem, s, "*")) {
            if (!sym_decompose_poly(mem, tab, n->left,  var_slot, &al, &bl, &cl, (uint8_t)(depth + 1)) ||
                !sym_decompose_poly(mem, tab, n->right, var_slot, &ar, &br, &cr, (uint8_t)(depth + 1))) {
                return false;
            }
            if (sym_is_zero_ast(mem, al) && sym_is_zero_ast(mem, bl)) {
                *out_a = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", cl, ar), 0);
                *out_b = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", cl, br), 0);
                *out_c = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", cl, cr), 0);
                return true;
            }
            if (sym_is_zero_ast(mem, ar) && sym_is_zero_ast(mem, br)) {
                *out_a = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", al, cr), 0);
                *out_b = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", bl, cr), 0);
                *out_c = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", cl, cr), 0);
                return true;
            }
            if (sym_is_zero_ast(mem, al) && sym_is_zero_ast(mem, ar)) {
                *out_a = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", bl, br), 0);
                uint16_t t1 = sym_make_bin(mem, tab, "*", bl, cr);
                uint16_t t2 = sym_make_bin(mem, tab, "*", cl, br);
                *out_b = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "+", t1, t2), 0);
                *out_c = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "*", cl, cr), 0);
                return true;
            }
            return false;
        }
        if (mem_slice_eq_cstr(mem, s, "/")) {
            if (!sym_depends_on(mem, n->right, var_slot) &&
                sym_decompose_poly(mem, tab, n->left, var_slot, &al, &bl, &cl, (uint8_t)(depth + 1))) {
                *out_a = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "/", al, n->right), 0);
                *out_b = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "/", bl, n->right), 0);
                *out_c = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "/", cl, n->right), 0);
                return true;
            }
            return false;
        }
        if (mem_slice_eq_cstr(mem, s, "^")) {
            double p = 0.0;
            if (sym_is_num(mem, n->right, &p) && fabs(p - 2.0) <= 1e-12 &&
                sym_decompose_poly(mem, tab, n->left, var_slot, &al, &bl, &cl, (uint8_t)(depth + 1)) &&
                sym_is_zero_ast(mem, al)) {
                *out_a = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "^", bl, sym_make_num(mem, 2.0)), 0);
                *out_b = sym_simplify_ast(mem, tab,
                             sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0),
                                          sym_make_bin(mem, tab, "*", bl, cl)), 0);
                *out_c = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "^", cl, sym_make_num(mem, 2.0)), 0);
                return true;
            }
        }
    }
    return false;
}

static int sym_op_prec(const MemArena *mem, const SymTable *tab, uint16_t sym_id) {
    StrSlice s = tab->syms[sym_id];
    if (mem_slice_eq_cstr(mem, s, "+") || mem_slice_eq_cstr(mem, s, "-")) return 50;
    if (mem_slice_eq_cstr(mem, s, "*") || mem_slice_eq_cstr(mem, s, "/")) return 60;
    if (mem_slice_eq_cstr(mem, s, "^")) return 85;
    return 40;
}

static void sym_buf_append(MemArena *mem, char *buf, size_t cap, size_t *used, const char *fmt, ...) {
    if (!buf || cap == 0 || mem->has_error) return;
    if (*used + 1 >= cap) {
        mem_err(mem, "Symbolic expression output exceeds buffer capacity (%zu bytes)", cap);
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + *used, cap - *used, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= cap - *used) {
        *used = cap - 1;
        buf[cap - 1] = '\0';
        mem_err(mem, "Symbolic expression output exceeds buffer capacity (%zu bytes)", cap);
        return;
    }
    *used += (size_t)n;
}

static void sym_ast_to_str(MemArena *mem, const SymTable *tab, uint16_t idx,
                           int parent_prec, char *buf, size_t cap, size_t *used) {
    if (idx == MEM_NO_IDX || mem->has_error) return;
    if (*used + 1 >= cap) {
        mem_err(mem, "Symbolic expression output exceeds buffer capacity (%zu bytes)", cap);
        return;
    }
    const AstNode *n = mem_get_ast(mem, idx);
    if (n->kind == AST_LIT) {
        double v = mem_read_pool(mem, n->ref);
        if (fabs(v) < 1e-15) v = 0.0;
        sym_buf_append(mem, buf, cap, used, "%.10g", v);
        return;
    }
    if (n->kind == AST_SLOT_LOAD) {
        uint8_t slot = n->sub;
        if (slot < 64 && mem->local_slot_names[slot].len > 0) {
            StrSlice nm = mem->local_slot_names[slot];
            sym_buf_append(mem, buf, cap, used, "%.*s",
                           (int)nm.len, mem_str_ptr(mem, nm));
        } else {
            sym_buf_append(mem, buf, cap, used, "x");
        }
        return;
    }
    if (n->kind == AST_CONST_SYM) {
        StrSlice cs = tab->syms[n->ref];
        sym_buf_append(mem, buf, cap, used, "%.*s",
                       (int)cs.len, mem_str_ptr(mem, cs));
        return;
    }
    if (n->kind == AST_UNARY && n->sub == (uint8_t)FIX_PREFIX) {
        StrSlice os = tab->syms[n->ref];
        bool wrap = (parent_prec > 80);
        if (wrap) sym_buf_append(mem, buf, cap, used, "(");
        sym_buf_append(mem, buf, cap, used, "%.*s",
                       (int)os.len, mem_str_ptr(mem, os));
        sym_ast_to_str(mem, tab, n->left, 80, buf, cap, used);
        if (wrap) sym_buf_append(mem, buf, cap, used, ")");
        return;
    }
    if (n->kind == AST_BINARY) {
        StrSlice os = tab->syms[n->ref];
        int prec = sym_op_prec(mem, tab, n->ref);
        bool wrap = (prec < parent_prec);
        if (wrap) sym_buf_append(mem, buf, cap, used, "(");
        sym_ast_to_str(mem, tab, n->left, prec, buf, cap, used);
        sym_buf_append(mem, buf, cap, used, " %.*s ",
                       (int)os.len, mem_str_ptr(mem, os));
        int r_prec = (mem_slice_eq_cstr(mem, os, "-") || mem_slice_eq_cstr(mem, os, "/"))
                     ? (prec + 1) : prec;
        sym_ast_to_str(mem, tab, n->right, r_prec, buf, cap, used);
        if (wrap) sym_buf_append(mem, buf, cap, used, ")");
        return;
    }
    if (n->kind == AST_CALL) {
        StrSlice fs = tab->syms[n->ref];
        sym_buf_append(mem, buf, cap, used, "%.*s(",
                       (int)fs.len, mem_str_ptr(mem, fs));
        uint16_t cur = n->left;
        bool first = true;
        while (cur != MEM_NO_IDX && !mem->has_error) {
            const AstNode *l = mem_get_ast(mem, cur);
            if (!first) sym_buf_append(mem, buf, cap, used, ", ");
            sym_ast_to_str(mem, tab, l->left, 0, buf, cap, used);
            first = false;
            cur = l->right;
        }
        sym_buf_append(mem, buf, cap, used, ")");
    }
}

static bool sym_parse_str_arg(MemArena *mem, SymTable *tab, uint16_t arg_ast,
                              uint16_t frame_base, uint8_t cur_ret_type,
                              const char *preferred_var, uint8_t *io_var_slot,
                              uint16_t *out_ast) {
    const AstNode *an = mem_get_ast(mem, arg_ast);
    if (an->kind == AST_LIT && an->sub == mem->str_type_id) {
        StrSlice s = { an->left, an->right };
        char sbuf[4096];
        if (s.len >= sizeof(sbuf)) {
            mem_err(mem, "Symbolic expression string exceeds maximum length (%zu bytes)", sizeof(sbuf) - 1);
            return false;
        }
        uint16_t slen = s.len;
        memcpy(sbuf, mem_str_ptr(mem, s), slen);
        sbuf[slen] = '\0';
        Parser SP;
        parser_init(&SP, mem, tab, sbuf, slen, 0, false, false);
        SP.in_symbolic_expr = 1;
        if (preferred_var && preferred_var[0] != '\0') {
            int sslot = parser_add_local(&SP, preferred_var, (uint16_t)strlen(preferred_var), mem->fast_real_type);
            if (io_var_slot) *io_var_slot = (uint8_t)sslot;
        }
        lex_next(&SP.lex);
        uint16_t r = parser_parse_expr(&SP, 0);
        if (mem->has_error || r == MEM_NO_IDX) return false;
        *out_ast = r;
        return true;
    }
    (void)frame_base;
    (void)cur_ret_type;
    *out_ast = arg_ast;
    return true;
}

Value delegate_eval_sym_meta(MemArena *mem, SymTable *tab,
                             uint8_t mtag, const AstNode *n,
                             uint16_t frame_base, uint8_t cur_ret_type) {
    uint16_t arg_asts[16];
    uint8_t  argc = 0;
    uint16_t cur  = n->left;
    while (cur != MEM_NO_IDX && argc < 16) {
        const AstNode *link = mem_get_ast(mem, cur);
        arg_asts[argc++] = link->left;
        cur = link->right;
    }

    if (mtag == INTR_SYM_SIMPLIFY) {
        if (argc < 1) {
            mem_err(mem, "sym_simplify(expr) requires an expression");
            return val_void();
        }
        uint16_t expr_ast = MEM_NO_IDX;
        if (!sym_parse_str_arg(mem, tab, arg_asts[0], frame_base, cur_ret_type, NULL, NULL, &expr_ast)) {
            return val_void();
        }
        uint16_t simp = sym_simplify_ast(mem, tab, expr_ast, 0);
        simp = sym_simplify_ast(mem, tab, simp, 0);
        char out[4096];
        size_t used = 0;
        out[0] = '\0';
        sym_ast_to_str(mem, tab, simp, 0, out, sizeof(out), &used);
        if (mem->has_error) return val_void();
        return val_from_cstr(mem, out);
    }

    if (mtag == INTR_SYM_DIFF) {
        if (argc < 2) {
            mem_err(mem, "sym_diff(x, expr[, order]) requires variable and expression");
            return val_void();
        }
        char vname[32] = "x";
        uint8_t var_slot = 0;
        const AstNode *vn = mem_get_ast(mem, arg_asts[0]);
        if (vn->kind == AST_SLOT_LOAD) {
            var_slot = vn->sub;
            if (var_slot < 64 && mem->local_slot_names[var_slot].len > 0) {
                StrSlice nm = mem->local_slot_names[var_slot];
                uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
                memcpy(vname, mem_str_ptr(mem, nm), cl);
                vname[cl] = '\0';
            }
        } else if (vn->kind == AST_LIT && vn->sub == mem->str_type_id) {
            StrSlice nm = { vn->left, vn->right };
            uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
            memcpy(vname, mem_str_ptr(mem, nm), cl);
            vname[cl] = '\0';
        }
        uint16_t expr_ast = MEM_NO_IDX;
        if (!sym_parse_str_arg(mem, tab, arg_asts[1], frame_base, cur_ret_type, vname, &var_slot, &expr_ast)) {
            return val_void();
        }
        int order = 1;
        if (argc >= 3) {
            Value vord = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
            if (mem->has_error) return val_void();
            order = (int)val_get_f64(mem, vord);
            if (order < 0) order = 0;
            if (order > 8) order = 8;
        }
        uint16_t cur_ast = sym_simplify_ast(mem, tab, expr_ast, 0);
        for (int k = 0; k < order && !mem->has_error; ++k) {
            cur_ast = sym_diff_ast(mem, tab, cur_ast, var_slot);
            cur_ast = sym_simplify_ast(mem, tab, cur_ast, 0);
            cur_ast = sym_simplify_ast(mem, tab, cur_ast, 0);
        }
        char out[4096];
        size_t used = 0;
        out[0] = '\0';
        sym_ast_to_str(mem, tab, cur_ast, 0, out, sizeof(out), &used);
        if (mem->has_error) return val_void();
        return val_from_cstr(mem, out);
    }

    if (mtag == INTR_SYM_INTEG) {
        if (argc < 2) {
            mem_err(mem, "sym_integ(x, expr) requires variable and expression");
            return val_void();
        }
        char vname[32] = "x";
        uint8_t var_slot = 0;
        const AstNode *vn = mem_get_ast(mem, arg_asts[0]);
        if (vn->kind == AST_SLOT_LOAD) {
            var_slot = vn->sub;
            if (var_slot < 64 && mem->local_slot_names[var_slot].len > 0) {
                StrSlice nm = mem->local_slot_names[var_slot];
                uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
                memcpy(vname, mem_str_ptr(mem, nm), cl);
                vname[cl] = '\0';
            }
        } else if (vn->kind == AST_LIT && vn->sub == mem->str_type_id) {
            StrSlice nm = { vn->left, vn->right };
            uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
            memcpy(vname, mem_str_ptr(mem, nm), cl);
            vname[cl] = '\0';
        }
        uint16_t expr_ast = MEM_NO_IDX;
        if (!sym_parse_str_arg(mem, tab, arg_asts[1], frame_base, cur_ret_type, vname, &var_slot, &expr_ast)) {
            return val_void();
        }
        uint16_t cur_ast = sym_simplify_ast(mem, tab, expr_ast, 0);
        cur_ast = sym_integ_ast(mem, tab, cur_ast, var_slot, 0);
        if (mem->has_error || cur_ast == MEM_NO_IDX) return val_void();
        cur_ast = sym_simplify_ast(mem, tab, cur_ast, 0);
        cur_ast = sym_simplify_ast(mem, tab, cur_ast, 0);
        char out[4096];
        size_t used = 0;
        out[0] = '\0';
        sym_ast_to_str(mem, tab, cur_ast, 0, out, sizeof(out), &used);
        if (mem->has_error) return val_void();
        return val_from_cstr(mem, out);
    }

    if (mtag == INTR_SYM_SOLVE) {
        if (argc < 2) {
            mem_err(mem, "sym_solve(x, eq) requires variable and equation");
            return val_void();
        }
        char vname[32] = "x";
        uint8_t var_slot = 0;
        const AstNode *vn = mem_get_ast(mem, arg_asts[0]);
        if (vn->kind == AST_SLOT_LOAD) {
            var_slot = vn->sub;
            if (var_slot < 64 && mem->local_slot_names[var_slot].len > 0) {
                StrSlice nm = mem->local_slot_names[var_slot];
                uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
                memcpy(vname, mem_str_ptr(mem, nm), cl);
                vname[cl] = '\0';
            }
        } else if (vn->kind == AST_LIT && vn->sub == mem->str_type_id) {
            StrSlice nm = { vn->left, vn->right };
            uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
            memcpy(vname, mem_str_ptr(mem, nm), cl);
            vname[cl] = '\0';
        }
        uint16_t eq_ast = MEM_NO_IDX;
        if (!sym_parse_str_arg(mem, tab, arg_asts[1], frame_base, cur_ret_type, vname, &var_slot, &eq_ast)) {
            return val_void();
        }
        const AstNode *en = mem_get_ast(mem, eq_ast);
        if (en->kind == AST_BINARY) {
            StrSlice os = tab->syms[en->ref];
            if (mem_slice_eq_cstr(mem, os, "==") || mem_slice_eq_cstr(mem, os, "=") ||
                mem_slice_eq_cstr(mem, os, ":=") || mem_slice_eq_cstr(mem, os, "~")) {
                eq_ast = sym_make_bin(mem, tab, "-", en->left, en->right);
            }
        }
        eq_ast = sym_simplify_ast(mem, tab, eq_ast, 0);
        uint16_t a_ast = 0, b_ast = 0, c_ast = 0;
        if (!sym_decompose_poly(mem, tab, eq_ast, var_slot, &a_ast, &b_ast, &c_ast, 0)) {
            mem_err(mem, "sym_solve(x, eq) supports linear and quadratic symbolic/numeric equations in '%s'", vname);
            return val_void();
        }
        a_ast = sym_simplify_ast(mem, tab, a_ast, 0);
        b_ast = sym_simplify_ast(mem, tab, b_ast, 0);
        c_ast = sym_simplify_ast(mem, tab, c_ast, 0);

        char out[4096];
        size_t used = 0;
        out[0] = '\0';

        if (sym_is_zero_ast(mem, a_ast)) {
            if (sym_is_zero_ast(mem, b_ast)) {
                mem_err(mem, "Equation in sym_solve does not depend on '%s' (or is degenerate)", vname);
                return val_void();
            }
            uint16_t neg_c = sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, c_ast), 0);
            uint16_t sol   = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "/", neg_c, b_ast), 0);
            sol = sym_simplify_ast(mem, tab, sol, 0);
            sym_ast_to_str(mem, tab, sol, 0, out, sizeof(out), &used);
            if (mem->has_error) return val_void();
            return val_from_cstr(mem, out);
        }

        double av = 0.0, bv = 0.0, cv = 0.0;
        if (sym_is_num(mem, a_ast, &av) && sym_is_num(mem, b_ast, &bv) && sym_is_num(mem, c_ast, &cv)) {
            double disc = bv * bv - 4.0 * av * cv;
            if (disc < -1e-12) {
                mem_err(mem, "Quadratic equation in sym_solve has no real roots (disc = %.6g)", disc);
                return val_void();
            }
            if (fabs(disc) <= 1e-12) {
                double r0 = -bv / (2.0 * av);
                if (fabs(r0) < 1e-15) r0 = 0.0;
                snprintf(out, sizeof(out), "%.10g", r0);
                return val_from_cstr(mem, out);
            }
            double sd = sqrt(disc);
            double r1 = (-bv - sd) / (2.0 * av);
            double r2 = (-bv + sd) / (2.0 * av);
            if (r1 > r2) { double t = r1; r1 = r2; r2 = t; }
            if (fabs(r1) < 1e-15) r1 = 0.0;
            if (fabs(r2) < 1e-15) r2 = 0.0;
            snprintf(out, sizeof(out), "[%.10g, %.10g]", r1, r2);
            return val_from_cstr(mem, out);
        }

        /* Symbolic quadratic: if B == 0 -> sqrt(-C / A), else (-B + sqrt(B^2 - 4*A*C)) / (2*A) */
        if (sym_is_zero_ast(mem, b_ast)) {
            uint16_t neg_c = sym_simplify_ast(mem, tab, sym_make_neg(mem, tab, c_ast), 0);
            uint16_t quot  = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "/", neg_c, a_ast), 0);
            uint16_t sol   = sym_simplify_ast(mem, tab, sym_make_call1(mem, tab, "sqrt", quot), 0);
            sym_ast_to_str(mem, tab, sol, 0, out, sizeof(out), &used);
            if (mem->has_error) return val_void();
            return val_from_cstr(mem, out);
        }
        uint16_t b2   = sym_make_bin(mem, tab, "^", b_ast, sym_make_num(mem, 2.0));
        uint16_t ac4  = sym_make_bin(mem, tab, "*", sym_make_num(mem, 4.0),
                                     sym_make_bin(mem, tab, "*", a_ast, c_ast));
        uint16_t disc = sym_make_call1(mem, tab, "sqrt", sym_make_bin(mem, tab, "-", b2, ac4));
        uint16_t num  = sym_make_bin(mem, tab, "+", sym_make_neg(mem, tab, b_ast), disc);
        uint16_t den  = sym_make_bin(mem, tab, "*", sym_make_num(mem, 2.0), a_ast);
        uint16_t sol  = sym_simplify_ast(mem, tab, sym_make_bin(mem, tab, "/", num, den), 0);
        sym_ast_to_str(mem, tab, sol, 0, out, sizeof(out), &used);
        if (mem->has_error) return val_void();
        return val_from_cstr(mem, out);
    }

    if (mtag == INTR_SYM_TAYLOR) {
        if (argc < 4) {
            mem_err(mem, "sym_taylor(x, expr, x0, order) requires 4 arguments");
            return val_void();
        }
        char vname[32] = "x";
        uint8_t var_slot = 0;
        const AstNode *vn = mem_get_ast(mem, arg_asts[0]);
        if (vn->kind == AST_SLOT_LOAD) {
            var_slot = vn->sub;
            if (var_slot < 64 && mem->local_slot_names[var_slot].len > 0) {
                StrSlice nm = mem->local_slot_names[var_slot];
                uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
                memcpy(vname, mem_str_ptr(mem, nm), cl);
                vname[cl] = '\0';
            }
        }
        uint16_t expr_ast = MEM_NO_IDX;
        if (!sym_parse_str_arg(mem, tab, arg_asts[1], frame_base, cur_ret_type, vname, &var_slot, &expr_ast)) {
            return val_void();
        }
        Value vx0  = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        Value vord = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
        if (mem->has_error) return val_void();
        double x0 = val_get_f64(mem, vx0);
        int order = (int)val_get_f64(mem, vord);
        if (order < 0) order = 0;
        if (order > 8) order = 8;

        Value saved_slot = mem_get_slot(mem, frame_base, var_slot);
        uint16_t d_ast = sym_simplify_ast(mem, tab, expr_ast, 0);
        uint16_t poly_ast = sym_make_num(mem, 0.0);
        double fact = 1.0;
        for (int k = 0; k <= order && !mem->has_error; ++k) {
            if (k > 0) fact *= (double)k;
            mem_set_slot(mem, frame_base, var_slot, val_make_f64(mem, mem->fast_real_type, x0));
            Value fk_val = eval_ast(mem, tab, d_ast, frame_base, cur_ret_type);
            double ck = snap_calc_noise(val_get_f64(mem, fk_val) / fact, 1e-12);
            if (fabs(ck) > 1e-12) {
                uint16_t var_ref = mem_alloc_ast(mem, AST_SLOT_LOAD, var_slot, 0, 0, 0);
                uint16_t base = (fabs(x0) <= 1e-12)
                                ? var_ref
                                : sym_make_bin(mem, tab, "-", var_ref, sym_make_num(mem, x0));
                uint16_t term = MEM_NO_IDX;
                if (k == 0) {
                    term = sym_make_num(mem, ck);
                } else if (k == 1) {
                    term = sym_make_bin(mem, tab, "*", sym_make_num(mem, ck), base);
                } else {
                    term = sym_make_bin(mem, tab, "*", sym_make_num(mem, ck),
                                        sym_make_bin(mem, tab, "^", base, sym_make_num(mem, (double)k)));
                }
                poly_ast = sym_make_bin(mem, tab, "+", poly_ast, term);
            }
            if (k < order) {
                d_ast = sym_simplify_ast(mem, tab, sym_diff_ast(mem, tab, d_ast, var_slot), 0);
            }
        }
        mem_set_slot(mem, frame_base, var_slot, saved_slot);
        poly_ast = sym_simplify_ast(mem, tab, poly_ast, 0);
        char out[4096];
        size_t used = 0;
        out[0] = '\0';
        sym_ast_to_str(mem, tab, poly_ast, 0, out, sizeof(out), &used);
        if (mem->has_error) return val_void();
        return val_from_cstr(mem, out);
    }

    if (mtag == INTR_SYM_EVAL) {
        if (argc < 3) {
            mem_err(mem, "sym_eval(expr_str, x, val) requires 3 arguments");
            return val_void();
        }
        Value s_val = eval_ast(mem, tab, arg_asts[0], frame_base, cur_ret_type);
        const AstNode *vn = mem_get_ast(mem, arg_asts[1]);
        Value x_val = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        if (mem->has_error) return val_void();
        if (vn->kind != AST_SLOT_LOAD) {
            mem_err(mem, "Second argument to sym_eval must be a variable name");
            return val_void();
        }
        uint8_t var_slot = vn->sub;
        char vname[32] = "x";
        if (var_slot < 64 && mem->local_slot_names[var_slot].len > 0) {
            StrSlice nm = mem->local_slot_names[var_slot];
            uint16_t cl = (nm.len < sizeof(vname) - 1) ? nm.len : (uint16_t)(sizeof(vname) - 1);
            memcpy(vname, mem_str_ptr(mem, nm), cl);
            vname[cl] = '\0';
        }
        char expr_buf[4096];
        if (!val_as_cstr(mem, s_val, expr_buf, sizeof(expr_buf))) {
            mem_err(mem, "First argument to sym_eval must be a string expression");
            return val_void();
        }
        Parser SP;
        parser_init(&SP, mem, tab, expr_buf, (uint16_t)strlen(expr_buf), 0, false, false);
        int sslot = parser_add_local(&SP, vname, (uint16_t)strlen(vname), mem->fast_real_type);
        lex_next(&SP.lex);
        uint16_t r = parser_parse_expr(&SP, 0);
        if (mem->has_error || r == MEM_NO_IDX) return val_void();
        uint16_t sub_frame = mem_push_frame(mem, SP.local_count > 0 ? SP.local_count : 1);
        if (mem->has_error) return val_void();
        mem_set_slot(mem, sub_frame, (uint8_t)sslot, x_val);
        Value res = eval_ast(mem, tab, r, sub_frame, cur_ret_type);
        mem_pop_frame(mem, sub_frame);
        return res;
    }

    return val_void();
}
