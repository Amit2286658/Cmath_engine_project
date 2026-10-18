/*
 * sym_table.h — Dynamic Module, Type, Unit, Struct, Symbol, Overload & Extern Registry
 *
 * Purpose:
 *   Stores and queries extension modules (with O(1) bitmask toggling),
 *   consuming lexical types, physical/dimensional unit types (`distance`,
 *   `angle`, `mass`, `time`, `speed`, ...), flat C-style structs, dynamic
 *   vectors (`$vec`), generic entity & unit wildcards (`T` / `any`),
 *   operator/function overloads, global constants, private C intrinsic IDs,
 *   and host-registered extern callbacks (`@<extern_name>`).
 */

#ifndef SYM_TABLE_H
#define SYM_TABLE_H

#include "mem_arena.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. REGISTRY CAPACITIES & POLYMORPHIC TYPE/UNIT SENTINELS
 * ============================================================================ */

#define SYM_MAX_MODULES        32
#define SYM_MAX_TYPES          32
#define SYM_MAX_UNITS          32
#define SYM_MAX_PAT_ELEMS     128
#define SYM_MAX_FIELDS        128
#define SYM_MAX_SYMS          512
#define SYM_MAX_OVERLOADS     416
#define SYM_MAX_CONSTS         64
#define SYM_MAX_EXTERNS        32

#define SYM_TYPE_ANY        0xFEu   /* Generic `T` / `any` entity type           */
#define SYM_UNIT_OMITTED    0xFFu   /* Unannotated parameter/return unit         */
#define SYM_UNIT_GENERIC    0xFEu   /* Explicit generic unit `T` / `any`         */
#define SYM_UNIT_LOCK_NONE  0xFDu   /* Explicit concrete unitless `none`         */

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
    INTR_LBRACE,    /* @lbrace */
    INTR_RBRACE,    /* @rbrace */
    INTR_COMMA,     /* @comma  */

    /* Bootstrap Meta / Lazy Control Intrinsics (`00_bootstrap.math`) */
    INTR_SEQ,          /* @seq(a, b)          */
    INTR_STORE,        /* @store(dst, v)      */
    INTR_WHILE,        /* @while(c, body)     */
    INTR_IF,           /* @if(c, t, e)        */
    INTR_OR_BRANCH,    /* @or_branch(c, b) / @then_branch(c, b) */
    INTR_ELSE_BRANCH,  /* @else_branch(b)     */
    INTR_SWITCH_BLOCK, /* @switch_block(v, b) */
    INTR_CASE_BRANCH,  /* @case_branch(v, b)  */
    INTR_TERNARY,      /* @ternary(c, pair)   */
    INTR_COLON,        /* @colon(a, b)        */
    INTR_JUMP_TAG,     /* @jump_tag(tag)      */
    INTR_RUN_TAG,      /* @run_tag(tag)       */
    INTR_BREAK_LOOP,   /* @break_loop(tag)    */
    INTR_CAND,         /* @cand(a, b) (`&` distributed/short-circuit AND) */
    INTR_COR,          /* @cor(a, b)  (`|` distributed/short-circuit OR)  */
    INTR_FIELD,        /* @field(s, f)        */

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

    /* Comparison & Boolean Logic Intrinsics */
    INTR_EQ,
    INTR_NE,
    INTR_LT,
    INTR_LE,
    INTR_GT,
    INTR_GE,
    INTR_LAND,
    INTR_LOR,
    INTR_LNOT,
    INTR_LXOR,
    INTR_LNOR,
    INTR_LNAND,

    /* Type & Unit Introspection / Conversion Intrinsics */
    INTR_RAW,
    INTR_TYPE_NAME, /* @type_name(x) -> $str */
    INTR_UNIT_NAME, /* @unit_name(x) -> $str */
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
    AST_CONSTRUCT,
    AST_EXTERN,        /* Host-registered callback `@<extern_name>(args...)`    */
    AST_TERNARY,       /* Lazy branch `cond ? then_ast : else_ast` & `if/or`    */
    AST_SWITCH_EQ,     /* Value equality check for `switch` cases (num & str)   */
    AST_TAGGED_SCOPE,  /* Labeled scope/loop `Tag : { body }` (sub = label_id)  */
    AST_JUMP,          /* Jump to labeled scope `jump(Tag)` (sub = label_id)    */
    AST_RUN_TAG,       /* Inline reuse of labeled scope `run(Tag)`              */
    AST_BREAK          /* Break out of loop/scope `break` / `break(Tag)`        */
} AstKind;

/* ============================================================================
 * 3. TYPE, STRUCT, PATTERN, OPERATOR & HOST EXTERN DESCRIPTORS
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
    uint16_t sym_id;
    uint8_t  mod_id;
    uint8_t  fixity;
    uint8_t  prec;
    uint8_t  param_count;
    uint8_t  local_count;
    uint8_t  ret_type;
    uint8_t  ret_unit;       /* SYM_UNIT_OMITTED, SYM_UNIT_GENERIC, SYM_UNIT_LOCK_NONE, or 1..31 */
    uint8_t  meta_tag;       /* 1 = inlined alias, or INTR_LPAREN..INTR_FIELD                    */
    uint8_t  param_types[8];
    uint8_t  param_units[8]; /* SYM_UNIT_OMITTED, SYM_UNIT_GENERIC, SYM_UNIT_LOCK_NONE, or 1..31 */
    uint16_t body_ast;
} OpOverload;

typedef struct {
    uint16_t sym_id;
    uint8_t  mod_id;
    uint8_t  type_id;
    uint8_t  unit_id;
    uint8_t  val_idx;
    uint16_t pool_idx;       /* Index into MemArena.f64_pool for ABI-neutral .mathc */
} ConstDef;

struct SymTable;

typedef bool (*EngExternFn)(MemArena *mem, const struct SymTable *tab,
                            const Value *args, uint8_t argc,
                            void *user_ctx, Value *out_val);

typedef struct {
    char        name[32];    /* Self-contained name (survives .mathc reloads) */
    EngExternFn fn;
    void       *user_ctx;
} ExternDef;

/* ============================================================================
 * 4. SYMBOL, TYPE & UNIT TABLE CONTEXT
 * ============================================================================ */

typedef struct SymTable {
    StrSlice    mod_names[SYM_MAX_MODULES];
    uint8_t     mod_count;
    uint64_t    active_modules;

    TypeDef     types[SYM_MAX_TYPES];
    uint8_t     type_count;

    StrSlice    units[SYM_MAX_UNITS];
    uint8_t     unit_count;

    PatElem     pat_elems[SYM_MAX_PAT_ELEMS];
    uint16_t    pat_elem_count;

    FieldDef    fields[SYM_MAX_FIELDS];
    uint16_t    field_count;

    StrSlice    syms[SYM_MAX_SYMS];
    uint16_t    sym_count;

    OpOverload  overloads[SYM_MAX_OVERLOADS];
    uint16_t    overload_count;

    ConstDef    consts[SYM_MAX_CONSTS];
    Value       const_vals[SYM_MAX_CONSTS];
    uint8_t     const_count;

    /* Host-registered runtime callbacks (not serialized into `.mathc` files) */
    ExternDef   externs[SYM_MAX_EXTERNS];
    uint8_t     extern_count;
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
uint16_t    sym_intern(SymTable *tab, MemArena *mem, StrSlice s);
int         sym_find(const SymTable *tab, const MemArena *mem, const char *str, uint16_t len);
bool        sym_has_active_op(const SymTable *tab, uint16_t sym_id);
uint8_t     sym_get_active_syntax(const SymTable *tab, uint16_t sym_id);
int         sym_get_active_prec(const SymTable *tab, uint16_t sym_id, OpFixity fixity);
uint8_t     sym_active_meta_tag(const SymTable *tab, uint16_t sym_id, OpFixity fix_filter);

/* Entity Type, Unit Type & Flat Struct Queries */
uint16_t    sym_find_type(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len);
uint8_t     sym_find_or_add_unit(SymTable *tab, MemArena *mem, const char *name, uint16_t len);
uint8_t     sym_find_unit(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len);
bool        sym_resolve_field(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                              const char *fname, uint16_t flen,
                              uint16_t *out_field_type, uint16_t *out_flat_off);
const char *sym_type_name(const SymTable *tab, const MemArena *mem, uint16_t type_id,
                          char *buf, size_t cap);
const char *sym_unit_name(const SymTable *tab, const MemArena *mem, uint8_t unit_id,
                          char *buf, size_t cap);

/* Intrinsic Name Lookup & Host Extern Callback Registration */
IntrinsicId      sym_lookup_intrinsic(const char *s, uint16_t len);
bool             sym_register_extern(SymTable *tab, MemArena *mem, const char *name,
                                     EngExternFn fn, void *user_ctx);
const ExternDef *sym_find_extern(const SymTable *tab, const MemArena *mem, uint16_t sym_id);

#ifdef __cplusplus
}
#endif

#endif /* SYM_TABLE_H */
