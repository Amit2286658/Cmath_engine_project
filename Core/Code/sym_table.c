/*
 * sym_table.c — Dynamic Module, Type, Unit, Struct, Symbol, Overload & Extern Implementation
 */

#include "sym_table.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    IntrinsicId id;
} IntrEntry;

static const IntrEntry k_intrinsics[] = {
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
    { NULL,           INTR_NONE }
};

void sym_init(SymTable *tab) {
    memset(tab, 0, sizeof(*tab));
    tab->type_count = 1; /* Index 0 reserved for MEM_TYPE_VOID */
    tab->unit_count = 1; /* Index 0 reserved for MEM_UNIT_NONE */
}

uint8_t sym_find_or_add_mod(SymTable *tab, MemArena *mem, StrSlice name) {
    for (uint8_t i = 0; i < tab->mod_count; ++i) {
        if (mem_slice_eq(mem, tab->mod_names[i], name)) return i;
    }
    if (tab->mod_count >= SYM_MAX_MODULES) {
        mem_err(mem, "Maximum module count (%d) exceeded", SYM_MAX_MODULES);
        return 0;
    }
    uint8_t id = tab->mod_count++;
    tab->mod_names[id] = name;
    tab->active_modules |= (1ULL << id);
    return id;
}

bool sym_set_module(SymTable *tab, const MemArena *mem, const char *mod_name, bool enabled) {
    for (uint8_t i = 0; i < tab->mod_count; ++i) {
        if (mem_slice_eq_cstr(mem, tab->mod_names[i], mod_name)) {
            if (enabled) tab->active_modules |= (1ULL << i);
            else         tab->active_modules &= ~(1ULL << i);
            return true;
        }
    }
    return false;
}

bool sym_is_module_active(const SymTable *tab, const MemArena *mem, const char *mod_name) {
    for (uint8_t i = 0; i < tab->mod_count; ++i) {
        if (mem_slice_eq_cstr(mem, tab->mod_names[i], mod_name)) {
            return (tab->active_modules & (1ULL << i)) != 0;
        }
    }
    return false;
}

bool sym_mod_enabled(const SymTable *tab, uint8_t mod_id) {
    return (tab->active_modules & (1ULL << mod_id)) != 0;
}

uint16_t sym_intern(SymTable *tab, MemArena *mem, StrSlice s) {
    for (uint16_t i = 0; i < tab->sym_count; ++i) {
        if (mem_slice_eq(mem, tab->syms[i], s)) return i;
    }
    if (tab->sym_count >= SYM_MAX_SYMS) {
        mem_err(mem, "Symbol table overflow");
        return 0;
    }
    uint16_t id = tab->sym_count++;
    tab->syms[id] = s;
    return id;
}

int sym_find(const SymTable *tab, const MemArena *mem, const char *str, uint16_t len) {
    for (uint16_t i = 0; i < tab->sym_count; ++i) {
        StrSlice s = tab->syms[i];
        if (s.len == len && memcmp(mem_str_ptr(mem, s), str, len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

bool sym_has_active_op(const SymTable *tab, uint16_t sym_id) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->fixity != FIX_FUNC && ov->fixity != FIX_SYNTAX &&
            sym_mod_enabled(tab, ov->mod_id)) {
            return true;
        }
    }
    return false;
}

uint8_t sym_get_active_syntax(const SymTable *tab, uint16_t sym_id) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->fixity == FIX_SYNTAX &&
            sym_mod_enabled(tab, ov->mod_id)) {
            return ov->meta_tag;
        }
    }
    return 0;
}

int sym_get_active_prec(const SymTable *tab, uint16_t sym_id, OpFixity fixity) {
    int best = -1;
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->fixity == (uint8_t)fixity && sym_mod_enabled(tab, ov->mod_id)) {
            if ((int)ov->prec > best) best = (int)ov->prec;
        }
    }
    return best;
}

uint8_t sym_active_meta_tag(const SymTable *tab, uint16_t sym_id, OpFixity fix_filter) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && sym_mod_enabled(tab, ov->mod_id)) {
            if (fix_filter == FIX_FUNC && ov->fixity != FIX_FUNC) continue;
            if ((fix_filter == FIX_PREFIX || fix_filter == FIX_POSTFIX) && ov->fixity != (uint8_t)fix_filter) continue;
            if (fix_filter != FIX_FUNC && (ov->fixity == FIX_FUNC || ov->fixity == FIX_SYNTAX)) continue;
            if (ov->meta_tag >= INTR_SEQ && ov->meta_tag <= INTR_FIELD) {
                return ov->meta_tag;
            }
        }
    }
    return 0;
}

uint16_t sym_find_type(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len) {
    if ((len == 1 && name[0] == 'T') || (len == 3 && memcmp(name, "any", 3) == 0)) {
        return SYM_TYPE_ANY;
    }
    for (uint16_t i = 1; i < tab->type_count; ++i) {
        if (!sym_mod_enabled(tab, tab->types[i].mod_id)) continue;
        StrSlice s = tab->types[i].name;
        if (s.len == len && memcmp(mem_str_ptr(mem, s), name, len) == 0) {
            return i;
        }
    }
    return MEM_TYPE_VOID;
}

uint8_t sym_find_unit(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len) {
    if ((len == 1 && name[0] == 'T') || (len == 3 && memcmp(name, "any", 3) == 0)) {
        return SYM_UNIT_GENERIC;
    }
    if (len == 4 && memcmp(name, "none", 4) == 0) {
        return SYM_UNIT_LOCK_NONE;
    }
    for (uint8_t i = 1; i < tab->unit_count; ++i) {
        StrSlice s = tab->units[i];
        if (s.len == len && memcmp(mem_str_ptr(mem, s), name, len) == 0) {
            return i;
        }
    }
    return MEM_UNIT_NONE;
}

uint8_t sym_find_or_add_unit(SymTable *tab, MemArena *mem, const char *name, uint16_t len) {
    uint8_t existing = sym_find_unit(tab, mem, name, len);
    if (existing != MEM_UNIT_NONE) return existing;
    if (tab->unit_count >= SYM_MAX_UNITS) {
        mem_err(mem, "Unit table overflow (max %d)", SYM_MAX_UNITS);
        return MEM_UNIT_NONE;
    }
    uint8_t id = tab->unit_count++;
    tab->units[id] = mem_intern_str(mem, name, len);
    return id;
}

bool sym_resolve_field(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                       const char *fname, uint16_t flen,
                       uint16_t *out_field_type, uint16_t *out_flat_off) {
    if (type_id == 0 || type_id >= tab->type_count) return false;
    const TypeDef *td = &tab->types[type_id];
    if (td->is_consuming || td->field_count == 0) return false;
    for (uint8_t i = 0; i < td->field_count; ++i) {
        const FieldDef *fd = &tab->fields[td->start_idx + i];
        if (fd->name.len == flen && memcmp(mem_str_ptr(mem, fd->name), fname, flen) == 0) {
            *out_field_type = fd->type_id;
            *out_flat_off   = fd->flat_offset;
            return true;
        }
    }
    return false;
}

const char *sym_type_name(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                          char *buf, size_t cap) {
    if (type_id == SYM_TYPE_ANY) {
        snprintf(buf, cap, "T");
        return buf;
    }
    if (type_id == 0 || type_id >= tab->type_count) {
        snprintf(buf, cap, "void");
        return buf;
    }
    StrSlice s = tab->types[type_id].name;
    snprintf(buf, cap, "%.*s", (int)s.len, mem_str_ptr(mem, s));
    return buf;
}

const char *sym_unit_name(const SymTable *tab, const MemArena *mem, uint8_t unit_id,
                          char *buf, size_t cap) {
    if (unit_id == SYM_UNIT_GENERIC || unit_id == SYM_UNIT_OMITTED) {
        snprintf(buf, cap, "T");
        return buf;
    }
    if (unit_id == 0 || unit_id == SYM_UNIT_LOCK_NONE || unit_id >= tab->unit_count) {
        snprintf(buf, cap, "none");
        return buf;
    }
    StrSlice s = tab->units[unit_id];
    snprintf(buf, cap, "%.*s", (int)s.len, mem_str_ptr(mem, s));
    return buf;
}

IntrinsicId sym_lookup_intrinsic(const char *s, uint16_t len) {
    for (int i = 0; k_intrinsics[i].name != NULL; ++i) {
        if (strlen(k_intrinsics[i].name) == len &&
            memcmp(k_intrinsics[i].name, s, len) == 0) {
            return k_intrinsics[i].id;
        }
    }
    return INTR_NONE;
}

bool sym_register_extern(SymTable *tab, MemArena *mem, const char *name,
                         EngExternFn fn, void *user_ctx) {
    if (!tab || !name || !fn) return false;
    size_t nlen = strlen(name);
    if (nlen == 0 || nlen >= sizeof(tab->externs[0].name)) {
        if (mem) mem_err(mem, "Extern name '%s' is empty or exceeds 31 chars", name);
        return false;
    }
    for (uint8_t i = 0; i < tab->extern_count; ++i) {
        if (strcmp(tab->externs[i].name, name) == 0) {
            tab->externs[i].fn       = fn;
            tab->externs[i].user_ctx = user_ctx;
            return true;
        }
    }
    if (tab->extern_count >= SYM_MAX_EXTERNS) {
        if (mem) mem_err(mem, "Host extern table overflow (max %d)", SYM_MAX_EXTERNS);
        return false;
    }
    ExternDef *ed = &tab->externs[tab->extern_count++];
    memcpy(ed->name, name, nlen + 1);
    ed->fn       = fn;
    ed->user_ctx = user_ctx;
    return true;
}

const ExternDef *sym_find_extern(const SymTable *tab, const MemArena *mem, uint16_t sym_id) {
    if (sym_id >= tab->sym_count) return NULL;
    StrSlice s = tab->syms[sym_id];
    for (uint8_t i = 0; i < tab->extern_count; ++i) {
        if (mem_slice_eq_cstr(mem, s, tab->externs[i].name)) {
            return &tab->externs[i];
        }
    }
    return NULL;
}
