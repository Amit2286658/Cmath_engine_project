/*
 * sym_table.h — Dynamic Module, Type, Unit, Struct, Symbol, Overload & Extern Registry
 *
 * Purpose:
 *   Stores and queries extension modules (with O(1) bitmask toggling),
 *   consuming lexical types, physical/dimensional unit types (`distance`,
 *   `angle`, `mass`, `time`, `speed`, ...), flat C-style structs, dynamic
 *   vectors (`$vec`), generic entity & unit wildcards (`T` / `any`),
 *   operator/function overloads (with O(1) per-symbol chain indexing),
 *   global constants, private C intrinsic IDs, and host-registered extern
 *   callbacks (`@<extern_name>`).
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
#define SYM_MAX_TYPES          64
#define SYM_MAX_UNITS          64
#define SYM_MAX_PAT_ELEMS     256
#define SYM_MAX_FIELDS        256
#define SYM_MAX_SYMS         2048
#define SYM_MAX_OVERLOADS    2048
#define SYM_MAX_CONSTS        128
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
    INTR_MAT_TYPE,  /* @mat_type (multidimensional matrix/tensor `[...]`)      */

    /* Bootstrap Expression Syntax Delimiters (`"sym" => @lparen`) */
    INTR_LPAREN,    /* @lparen */
    INTR_RPAREN,    /* @rparen */
    INTR_LBRACE,    /* @lbrace */
    INTR_RBRACE,    /* @rbrace */
    INTR_LBRACK,    /* @lbrack */
    INTR_RBRACK,    /* @rbrack */
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
    INTR_LOOP_KW,      /* @loop_kw(iter, b)   */
    INTR_CAND,         /* @cand(a, b) (`&` distributed/short-circuit AND) */
    INTR_COR,          /* @cor(a, b)  (`|` distributed/short-circuit OR)  */
    INTR_CNOT,         /* @cnot(a)    (`!` strict boolean condition NOT)  */
    INTR_CALC_DIFF,    /* @calc_diff(x, expr, x0)       */
    INTR_CALC_DIFF2,   /* @calc_diff2(x, expr, x0)      */
    INTR_CALC_DIFFN,   /* @calc_diffn(x, expr, x0, n)   */
    INTR_CALC_INTEG,   /* @calc_integ(x, expr, a, b)    */
    INTR_CALC_LIM,     /* @calc_lim(x, expr, x0)        */
    INTR_CALC_LIM_L,   /* @calc_lim_l(x, expr, x0)      */
    INTR_CALC_LIM_R,   /* @calc_lim_r(x, expr, x0)      */
    INTR_CALC_SIGMA,   /* @calc_sigma(k, expr, lo, hi)  */
    INTR_CALC_PROD,    /* @calc_prod(k, expr, lo, hi)   */
    INTR_CALC_SOLVE,   /* @calc_solve(x, eq, guess)     */
    INTR_CALC_FMIN,    /* @calc_fmin(x, expr, a, b)     */
    INTR_CALC_FMAX,    /* @calc_fmax(x, expr, a, b)     */
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
    INTR_VRANGE,    /* @vrange(a, b) (`a..b` / `a...b` inclusive range) */
    INTR_VIN,       /* @vin(item, v) (`x in v` vector membership check) */

    /* Multidimensional Matrix Intrinsics (`$mat`) */
    INTR_MZERO,     /* @mzero(r, c)        */
    INTR_MONES,     /* @mones(r, c)        */
    INTR_MEYE,      /* @meye(n)            */
    INTR_MFILL,     /* @mfill(r, c, val)   */
    INTR_MROWS,     /* @mrows(m)           */
    INTR_MCOLS,     /* @mcols(m)           */
    INTR_MRANK,     /* @mrank(m)           */
    INTR_MLEN,      /* @mlen(m)            */
    INTR_MSHAPE,    /* @mshape(m) -> $vec  */
    INTR_MADD,      /* @madd(a, b)         */
    INTR_MSUB,      /* @msub(a, b)         */
    INTR_MSCAL,     /* @mscal(m, s)        */
    INTR_MMUL,      /* @mmul(a, b)         */
    INTR_MTRANS,    /* @mtrans(m)          */
    INTR_MTRACE,    /* @mtrace(m)          */
    INTR_MDET,      /* @mdet(m)            */
    INTR_MINV,      /* @minv(m)            */

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
    INTR_APPROX,    /* @approx(a, b) (`~` approximate equality <= 1e-3) */
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
    INTR_ERR,       /* @err(msg:str) -> raises custom error */
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
    AST_BREAK,         /* Break out of loop/scope `break` / `break(Tag)`        */
    AST_MAT_LIT,       /* Multidimensional matrix literal `[...]`               */
    AST_INDEX,         /* C-style subscript `base[i]` / `base[r, c]`            */
    AST_FOR_IN,        /* Range/collection loop `(i in a..b) { ... }` / `loop`  */
    AST_UNPACK_ELEM    /* Multi-variable destructuring / scalar broadcast       */
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
    uint8_t  ret_unit;       /* SYM_UNIT_OMITTED, SYM_UNIT_GENERIC, SYM_UNIT_LOCK_NONE, or 1..63 */
    uint8_t  meta_tag;       /* 1 = inlined alias, or INTR_LPAREN..INTR_FIELD                    */
    uint8_t  param_types[8];
    uint8_t  param_units[8]; /* SYM_UNIT_OMITTED, SYM_UNIT_GENERIC, SYM_UNIT_LOCK_NONE, or 1..63 */
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

    /* Per-symbol overload chains for O(1) symbol dispatch (rebuilt in-memory) */
    uint16_t    sym_first_ov[SYM_MAX_SYMS];
    uint16_t    sym_last_ov[SYM_MAX_SYMS];
    uint16_t    ov_next[SYM_MAX_OVERLOADS];

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
void        sym_rebuild_index(SymTable *tab);
void        sym_link_overload(SymTable *tab, uint16_t ov_idx);

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
bool        sym_is_reserved_kw(const SymTable *tab, const MemArena *mem, const char *name, uint16_t len);

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
