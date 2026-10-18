/*
 * sym_table.h — Dynamic Module, Type, Struct, Symbol & Overload Registry Module
 *
 * Purpose:
 *   Stores and queries extension modules (with O(1) bitmask toggling),
 *   consuming lexical types, flat C-style structs, dynamic vectors (`$vec`),
 *   generic type wildcards (`T` / `any`), operator/function overloads,
 *   global constants, and private C intrinsic IDs.
 */

#ifndef SYM_TABLE_H
#define SYM_TABLE_H

#include "mem_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. REGISTRY CAPACITIES & POLYMORPHIC TYPE SENTINEL
 * ============================================================================ */

#define SYM_MAX_MODULES       32
#define SYM_MAX_TYPES         32
#define SYM_MAX_PAT_ELEMS    128
#define SYM_MAX_FIELDS        64
#define SYM_MAX_SYMS          96
#define SYM_MAX_OVERLOADS    160
#define SYM_MAX_CONSTS        32

#define SYM_TYPE_ANY       0xFEu   /* Generic `T` / `any` parameter or return   */

/* ============================================================================
 * 2. PRIVATE C INTRINSIC OPCODES & AST NODE KINDS
 * ============================================================================ */

typedef enum {
    INTR_NONE = 0,
    /* Lexer & Container Type Intrinsics (`$type ~ pattern => @parser`) */
    INTR_PARSE_F64,
    INTR_PARSE_F64_TRIM1,
    INTR_PARSE_I64,
    INTR_PARSE_STR_SLICE,
    INTR_VEC_TYPE,  /* @vec_type (dynamic growable/shrinkable vector of any T) */

    /* Bootstrap Expression Syntax Delimiters (`"sym" => @lparen`) */
    INTR_LPAREN,    /* @lparen */
    INTR_RPAREN,    /* @rparen */
    INTR_COMMA,     /* @comma  */

    /* Bootstrap Meta / Lazy Control Intrinsics (`00_bootstrap.math`) */
    INTR_SEQ,       /* @seq(a, b)      */
    INTR_STORE,     /* @store(dst, v)  */
    INTR_WHILE,     /* @while(c, body) */
    INTR_IF,        /* @if(c, t, e)    */
    INTR_FIELD,     /* @field(s, f)    */

    /* Dynamic Vector Intrinsics (`$vec` of any `T` type) */
    INTR_VNEW,      /* @vnew(cap)          */
    INTR_VFILL,     /* @vfill(cnt, init)   */
    INTR_VPUSH,     /* @vpush(v, item)     */
    INTR_VPOP,      /* @vpop(v)            */
    INTR_VGET,      /* @vget(v, idx)       */
    INTR_VSET,      /* @vset(v, idx, item) */
    INTR_VLEN,      /* @vlen(v)            */
    INTR_VCAP,      /* @vcap(v)            */
    INTR_VRESIZE,   /* @vresize(v, n)      */
    INTR_VSHRINK,   /* @vshrink(v)         */
    INTR_VCLEAR,    /* @vclear(v)          */

    /* Core Arithmetic Intrinsics */
    INTR_ADD,
    INTR_SUB,
    INTR_MUL,
    INTR_DIV,
    INTR_MOD,
    INTR_NEG,
    INTR_ABS,
    INTR_FLOOR,
    INTR_CEIL,

    /* Powers, Roots & Exponential Intrinsics */
    INTR_POW,
    INTR_SQRT,
    INTR_CBRT,
    INTR_EXP,
    INTR_LN,
    INTR_LOG10,

    /* Trigonometric & Hyperbolic Intrinsics */
    INTR_SIN,
    INTR_COS,
    INTR_TAN,
    INTR_ASIN,
    INTR_ACOS,
    INTR_ATAN,
    INTR_ATAN2,
    INTR_SINH,
    INTR_COSH,
    INTR_TANH,

    /* Comparison Intrinsics */
    INTR_EQ,
    INTR_NE,
    INTR_LT,
    INTR_LE,
    INTR_GT,
    INTR_GE,

    /* Type Conversion Intrinsics */
    INTR_RAW,
    INTR_COUNT
} IntrinsicId;

typedef enum {
    AST_LIT = 0,
    AST_CONST_SYM,
    AST_SLOT_LOAD,
    AST_FIELD_LOAD,
    AST_UNARY,
    AST_BINARY,
    AST_CALL,
    AST_ARG_LINK,
    AST_INTRINSIC,
    AST_CONSTRUCT
} AstKind;

/* ============================================================================
 * 3. TYPE, STRUCT, PATTERN & OPERATOR DESCRIPTORS
 * ============================================================================ */

typedef enum {
    PAT_DIGIT = 1,
    PAT_ALPHA,
    PAT_LIT,
    PAT_NOT_CHARS,
    PAT_GROUP_START,
    PAT_GROUP_END
} PatKind;

typedef enum {
    QUANT_ONE = 0,
    QUANT_PLUS,
    QUANT_STAR,
    QUANT_OPT
} PatQuant;

typedef struct {
    uint8_t  kind;
    uint8_t  quant;
    StrSlice chars;
} PatElem;

typedef struct {
    StrSlice name;
    uint16_t type_id;
    uint16_t flat_offset;
} FieldDef;

typedef struct {
    StrSlice name;
    uint8_t  mod_id;
    uint8_t  is_consuming;
    uint8_t  parser_intr;
    uint8_t  field_count;
    uint16_t start_idx;
    uint16_t flat_size;
} TypeDef;

typedef enum {
    FIX_PREFIX      = 0,
    FIX_POSTFIX     = 1,
    FIX_INFIX_LEFT  = 2,
    FIX_INFIX_RIGHT = 3,
    FIX_FUNC        = 4,
    FIX_SYNTAX      = 5
} OpFixity;

typedef struct {
    uint8_t  sym_id;
    uint8_t  mod_id;
    uint8_t  fixity;
    uint8_t  prec;
    uint8_t  param_count;
    uint8_t  local_count;
    uint8_t  ret_type;
    uint8_t  meta_tag;       /* 1 = inlined alias, or INTR_LPAREN..INTR_FIELD */
    uint8_t  param_types[4];
    uint16_t body_ast;
} OpOverload;

typedef struct {
    uint8_t  sym_id;
    uint8_t  mod_id;
    uint8_t  type_id;
    uint8_t  val_idx;
    uint16_t pool_idx;       /* Index into MemArena.f64_pool for ABI-neutral .mathc */
} ConstDef;

/* ============================================================================
 * 4. SYMBOL & TYPE TABLE CONTEXT
 * ============================================================================ */

typedef struct {
    StrSlice    mod_names[SYM_MAX_MODULES];
    uint8_t     mod_count;
    uint64_t    active_modules;

    TypeDef     types[SYM_MAX_TYPES];
    uint8_t     type_count;

    PatElem     pat_elems[SYM_MAX_PAT_ELEMS];
    uint16_t    pat_elem_count;

    FieldDef    fields[SYM_MAX_FIELDS];
    uint16_t    field_count;

    StrSlice    syms[SYM_MAX_SYMS];
    uint8_t     sym_count;

    OpOverload  overloads[SYM_MAX_OVERLOADS];
    uint16_t    overload_count;

    ConstDef    consts[SYM_MAX_CONSTS];
    Value       const_vals[SYM_MAX_CONSTS];
    uint8_t     const_count;
} SymTable;

/* ============================================================================
 * 5. SYMBOL TABLE API
 * ============================================================================ */

void        sym_init(SymTable *tab);

/* Module management & O(1) bitmask toggling */
uint8_t     sym_find_or_add_mod(SymTable *tab, MemArena *mem, StrSlice name);
bool        sym_set_module(SymTable *tab, const MemArena *mem, const char *mod_name, bool enabled);
bool        sym_is_module_active(const SymTable *tab, const MemArena *mem, const char *mod_name);
bool        sym_mod_enabled(const SymTable *tab, uint8_t mod_id);

/* Symbol Interning & Lookup */
uint8_t     sym_intern(SymTable *tab, MemArena *mem, StrSlice s);
int         sym_find(const SymTable *tab, const MemArena *mem, const char *str, uint16_t len);
bool        sym_has_active_op(const SymTable *tab, uint8_t sym_id);
uint8_t     sym_get_active_syntax(const SymTable *tab, uint8_t sym_id);
int         sym_get_active_prec(const SymTable *tab, uint8_t sym_id, OpFixity fixity);
uint8_t     sym_active_meta_tag(const SymTable *tab, uint8_t sym_id, OpFixity fix_filter);

/* Type & Flat Struct Queries */
uint16_t    sym_find_type(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len);
bool        sym_resolve_field(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                              const char *fname, uint16_t flen,
                              uint16_t *out_field_type, uint16_t *out_flat_off);
const char *sym_type_name(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                          char *buf, size_t cap);

/* Intrinsic Name Lookup */
IntrinsicId sym_lookup_intrinsic(const char *s, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* SYM_TABLE_H */