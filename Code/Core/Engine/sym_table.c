/*
 * sym_table.c — Dynamic Module, Type, Struct, Symbol & Overload Registry Implementation
 */

#include "../Headers/sym_table.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    IntrinsicId id;
} IntrEntry;

static const IntrEntry k_intrinsics[] = {
    { "f64",       INTR_PARSE_F64 },
    { "f64_trim1", INTR_PARSE_F64_TRIM1 },
    { "i64",       INTR_PARSE_I64 },
    { "str_slice", INTR_PARSE_STR_SLICE },
    { "lparen",    INTR_LPAREN },
    { "rparen",    INTR_RPAREN },
    { "comma",     INTR_COMMA },
    { "seq",       INTR_SEQ },
    { "store",     INTR_STORE },
    { "while",     INTR_WHILE },
    { "if",        INTR_IF },
    { "field",     INTR_FIELD },
    { "add",       INTR_ADD },
    { "sub",       INTR_SUB },
    { "mul",       INTR_MUL },
    { "div",       INTR_DIV },
    { "mod",       INTR_MOD },
    { "neg",       INTR_NEG },
    { "abs",       INTR_ABS },
    { "floor",     INTR_FLOOR },
    { "ceil",      INTR_CEIL },
    { "pow",       INTR_POW },
    { "sqrt",      INTR_SQRT },
    { "cbrt",      INTR_CBRT },
    { "exp",       INTR_EXP },
    { "ln",        INTR_LN },
    { "log10",     INTR_LOG10 },
    { "sin",       INTR_SIN },
    { "cos",       INTR_COS },
    { "tan",       INTR_TAN },
    { "asin",      INTR_ASIN },
    { "acos",      INTR_ACOS },
    { "atan",      INTR_ATAN },
    { "atan2",     INTR_ATAN2 },
    { "sinh",      INTR_SINH },
    { "cosh",      INTR_COSH },
    { "tanh",      INTR_TANH },
    { "eq",        INTR_EQ },
    { "ne",        INTR_NE },
    { "lt",        INTR_LT },
    { "le",        INTR_LE },
    { "gt",        INTR_GT },
    { "ge",        INTR_GE },
    { "raw",       INTR_RAW },
    { NULL,        INTR_NONE }
};

void sym_init(SymTable *tab) {
    memset(tab, 0, sizeof(*tab));
    tab->type_count = 1; /* Index 0 reserved for MEM_TYPE_VOID */
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

uint8_t sym_intern(SymTable *tab, MemArena *mem, StrSlice s) {
    for (uint8_t i = 0; i < tab->sym_count; ++i) {
        if (mem_slice_eq(mem, tab->syms[i], s)) return i;
    }
    if (tab->sym_count >= SYM_MAX_SYMS) {
        mem_err(mem, "Symbol table overflow");
        return 0;
    }
    uint8_t id = tab->sym_count++;
    tab->syms[id] = s;
    return id;
}

int sym_find(const SymTable *tab, const MemArena *mem, const char *str, uint16_t len) {
    for (uint8_t i = 0; i < tab->sym_count; ++i) {
        StrSlice s = tab->syms[i];
        if (s.len == len && memcmp(mem_str_ptr(mem, s), str, len) == 0) {
            return (int)i;
        }
    }
    return -1;
}

bool sym_has_active_op(const SymTable *tab, uint8_t sym_id) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->fixity != FIX_FUNC && ov->fixity != FIX_SYNTAX &&
            sym_mod_enabled(tab, ov->mod_id)) {
            return true;
        }
    }
    return false;
}

uint8_t sym_get_active_syntax(const SymTable *tab, uint8_t sym_id) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->fixity == FIX_SYNTAX &&
            sym_mod_enabled(tab, ov->mod_id)) {
            return ov->meta_tag;
        }
    }
    return 0;
}

int sym_get_active_prec(const SymTable *tab, uint8_t sym_id, OpFixity fixity) {
    int best = -1;
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && ov->fixity == (uint8_t)fixity && sym_mod_enabled(tab, ov->mod_id)) {
            if ((int)ov->prec > best) best = (int)ov->prec;
        }
    }
    return best;
}

uint8_t sym_active_meta_tag(const SymTable *tab, uint8_t sym_id, OpFixity fix_filter) {
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id == sym_id && sym_mod_enabled(tab, ov->mod_id)) {
            if (fix_filter == FIX_FUNC && ov->fixity != FIX_FUNC) continue;
            if (fix_filter != FIX_FUNC && (ov->fixity == FIX_FUNC || ov->fixity == FIX_SYNTAX)) continue;
            if (ov->meta_tag >= INTR_SEQ && ov->meta_tag <= INTR_FIELD) {
                return ov->meta_tag;
            }
        }
    }
    return 0;
}

uint16_t sym_find_type(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len) {
    for (uint16_t i = 1; i < tab->type_count; ++i) {
        if (!sym_mod_enabled(tab, tab->types[i].mod_id)) continue;
        StrSlice s = tab->types[i].name;
        if (s.len == len && memcmp(mem_str_ptr(mem, s), name, len) == 0) {
            return i;
        }
    }
    return MEM_TYPE_VOID;
}

bool sym_resolve_field(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                       const char *fname, uint16_t flen,
                       uint16_t *out_field_type, uint16_t *out_flat_off) {
    if (type_id == 0 || type_id >= tab->type_count) return false;
    const TypeDef *td = &tab->types[type_id];
    if (td->is_consuming) return false;
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
    if (type_id == 0 || type_id >= tab->type_count) {
        snprintf(buf, cap, "void");
        return buf;
    }
    StrSlice s = tab->types[type_id].name;
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