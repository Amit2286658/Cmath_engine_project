/*
 * mem_arena.c — Isolated Memory Management & Value Representation Implementation
 */

#include "../Headers/mem_arena.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ============================================================================
 * 1. ARENA LIFECYCLE & ERROR HANDLING
 * ============================================================================ */

void mem_init(MemArena *mem) {
    memset(mem, 0, sizeof(*mem));
}

void mem_err(MemArena *mem, const char *fmt, ...) {
    if (mem->has_error) return;
    mem->has_error = true;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(mem->err_msg, sizeof(mem->err_msg), fmt, ap);
    va_end(ap);
}

void mem_clear_err(MemArena *mem) {
    mem->has_error  = false;
    mem->err_msg[0] = '\0';
}

void mem_snapshot_perm(MemArena *mem) {
    mem->perm_src_len   = mem->src_len;
    mem->perm_ast_count = mem->ast_count;
    mem->perm_pool_top  = mem->pool_top;
}

void mem_rewind_scratch(MemArena *mem) {
    mem->src_len   = mem->perm_src_len;
    mem->ast_count = mem->perm_ast_count;
    mem->pool_top  = mem->perm_pool_top;
    mem->sp        = 0;
}

/* ============================================================================
 * 2. UTF-8 STRING INTERNING & SLICES
 * ============================================================================ */

StrSlice mem_intern_str(MemArena *mem, const char *str, uint16_t len) {
    for (uint16_t i = 0; i + len <= mem->src_len; ++i) {
        if (memcmp(mem->src_buf + i, str, len) == 0) {
            StrSlice s = { i, len };
            return s;
        }
    }
    if (mem->src_len + len >= MEM_SRC_BUF_CAP) {
        mem_err(mem, "Source arena buffer overflow (%u bytes)", (unsigned)MEM_SRC_BUF_CAP);
        StrSlice empty = { 0, 0 };
        return empty;
    }
    StrSlice s = { mem->src_len, len };
    memcpy(mem->src_buf + mem->src_len, str, len);
    mem->src_len += len;
    return s;
}

const char *mem_str_ptr(const MemArena *mem, StrSlice s) {
    return mem->src_buf + s.off;
}

bool mem_slice_eq(const MemArena *mem, StrSlice a, StrSlice b) {
    if (a.len != b.len) return false;
    return memcmp(mem->src_buf + a.off, mem->src_buf + b.off, a.len) == 0;
}

bool mem_slice_eq_cstr(const MemArena *mem, StrSlice s, const char *cstr) {
    size_t len = strlen(cstr);
    if (s.len != len) return false;
    return memcmp(mem->src_buf + s.off, cstr, len) == 0;
}

/* ============================================================================
 * 3. FLOAT POOL, AST POOL & CALL STACK ALLOCATION
 * ============================================================================ */

uint16_t mem_alloc_pool(MemArena *mem, uint16_t count) {
    if (mem->pool_top + count > MEM_MAX_F64_POOL) {
        mem_err(mem, "Float value pool exhausted");
        return 0;
    }
    uint16_t idx = mem->pool_top;
    mem->pool_top += count;
    return idx;
}

double mem_read_pool(const MemArena *mem, uint16_t idx) {
    return (idx < MEM_MAX_F64_POOL) ? mem->f64_pool[idx] : 0.0;
}

void mem_write_pool(MemArena *mem, uint16_t idx, double val) {
    if (idx < MEM_MAX_F64_POOL) {
        mem->f64_pool[idx] = val;
    }
}

uint16_t mem_alloc_ast(MemArena *mem, uint8_t kind, uint8_t sub,
                       uint16_t ref, uint16_t left, uint16_t right) {
    if (mem->ast_count >= MEM_MAX_AST_NODES) {
        mem_err(mem, "AST node pool exhausted");
        return MEM_NO_IDX;
    }
    uint16_t idx = mem->ast_count++;
    AstNode *n = &mem->ast[idx];
    n->kind  = kind;
    n->sub   = sub;
    n->ref   = ref;
    n->left  = left;
    n->right = right;
    return idx;
}

const AstNode *mem_get_ast(const MemArena *mem, uint16_t idx) {
    return &mem->ast[idx];
}

AstNode *mem_get_ast_mut(MemArena *mem, uint16_t idx) {
    return &mem->ast[idx];
}

uint16_t mem_push_frame(MemArena *mem, uint8_t slot_count) {
    uint16_t base = mem->sp;
    if (base + slot_count > MEM_MAX_CALL_STACK) {
        mem_err(mem, "Call stack overflow (max %d slots)", MEM_MAX_CALL_STACK);
        return base;
    }
    mem->sp += slot_count;
    return base;
}

void mem_pop_frame(MemArena *mem, uint16_t frame_base) {
    mem->sp = frame_base;
}

Value mem_get_slot(const MemArena *mem, uint16_t frame_base, uint8_t slot) {
    return mem->stack[frame_base + slot];
}

void mem_set_slot(MemArena *mem, uint16_t frame_base, uint8_t slot, Value v) {
    mem->stack[frame_base + slot] = v;
}

/* ============================================================================
 * 4. VALUE ENCODING (8-BYTE NAN-BOXING vs. 12-BYTE TAGGED UNION)
 * ============================================================================ */

#if USE_TAGGED_12B

Value val_void(void) {
    Value v;
    memset(&v, 0, sizeof(v));
    return v;
}

Value val_make_f64(MemArena *mem, uint16_t type_id, double d) {
    (void)mem;
    Value v;
    v.type_id = type_id;
    v.flags   = 0;
    v.as.f64  = d;
    return v;
}

double val_get_f64(const MemArena *mem, Value v) {
    (void)mem;
    return v.as.f64;
}

Value val_make_str(MemArena *mem, uint16_t type_id, StrSlice s) {
    (void)mem;
    Value v;
    v.type_id    = type_id;
    v.flags      = 0;
    v.as.str.off = s.off;
    v.as.str.len = s.len;
    return v;
}

StrSlice val_get_str(const MemArena *mem, Value v) {
    (void)mem;
    StrSlice s = { (uint16_t)v.as.str.off, (uint16_t)v.as.str.len };
    return s;
}

Value val_make_comp(MemArena *mem, uint16_t type_id, uint16_t pool_idx, uint16_t flat_size) {
    (void)mem;
    Value v;
    v.type_id     = type_id;
    v.flags       = 0;
    v.as.comp.idx = pool_idx;
    v.as.comp.cnt = flat_size;
    return v;
}

uint16_t val_get_pool_idx(Value v) {
    return (uint16_t)v.as.comp.idx;
}

uint16_t val_get_type(const MemArena *mem, Value v) {
    (void)mem;
    return v.type_id;
}

#else /* 8-BYTE IEEE-754 NAN-BOXING */

#define NANBOX_PREFIX     0xFFF8000000000000ULL
#define NANBOX_MASK       0xFFF8000000000000ULL
#define NANBOX_TAG_SHIFT  48
#define NANBOX_TYPE_SHIFT 32

#define NB_TAG_VOID       0xFFF8ULL
#define NB_TAG_POOL       0xFFF9ULL
#define NB_TAG_STR        0xFFFAULL

static inline uint64_t f64_to_bits(double d) {
    uint64_t u;
    memcpy(&u, &d, sizeof(u));
    if ((u & NANBOX_MASK) == NANBOX_PREFIX) {
        u = 0x7FF8000000000000ULL;
    }
    return u;
}

static inline double bits_to_f64(uint64_t u) {
    double d;
    memcpy(&d, &u, sizeof(d));
    return d;
}

Value val_void(void) {
    return (NB_TAG_VOID << NANBOX_TAG_SHIFT);
}

Value val_make_f64(MemArena *mem, uint16_t type_id, double d) {
    if (type_id == mem->fast_real_type && type_id != 0) {
        return f64_to_bits(d);
    }
    uint16_t idx = mem_alloc_pool(mem, 1);
    mem_write_pool(mem, idx, d);
    return (NB_TAG_POOL << NANBOX_TAG_SHIFT) | ((uint64_t)type_id << NANBOX_TYPE_SHIFT) | idx;
}

double val_get_f64(const MemArena *mem, Value v) {
    uint16_t top = (uint16_t)(v >> NANBOX_TAG_SHIFT);
    if (top < NB_TAG_VOID) {
        return bits_to_f64(v);
    }
    uint16_t idx = (uint16_t)(v & 0xFFFFu);
    return mem_read_pool(mem, idx);
}

Value val_make_str(MemArena *mem, uint16_t type_id, StrSlice s) {
    (void)mem;
    uint32_t payload = ((uint32_t)s.off << 16) | (uint32_t)s.len;
    return (NB_TAG_STR << NANBOX_TAG_SHIFT) | ((uint64_t)type_id << NANBOX_TYPE_SHIFT) | payload;
}

StrSlice val_get_str(const MemArena *mem, Value v) {
    (void)mem;
    uint32_t payload = (uint32_t)(v & 0xFFFFFFFFu);
    StrSlice s = { (uint16_t)(payload >> 16), (uint16_t)(payload & 0xFFFFu) };
    return s;
}

Value val_make_comp(MemArena *mem, uint16_t type_id, uint16_t pool_idx, uint16_t flat_size) {
    (void)mem;
    uint32_t payload = ((uint32_t)flat_size << 16) | (uint32_t)pool_idx;
    return (NB_TAG_POOL << NANBOX_TAG_SHIFT) | ((uint64_t)type_id << NANBOX_TYPE_SHIFT) | payload;
}

uint16_t val_get_pool_idx(Value v) {
    return (uint16_t)(v & 0xFFFFu);
}

uint16_t val_get_type(const MemArena *mem, Value v) {
    uint16_t top = (uint16_t)(v >> NANBOX_TAG_SHIFT);
    if (top < NB_TAG_VOID) {
        return mem->fast_real_type;
    }
    if (top == NB_TAG_VOID) return MEM_TYPE_VOID;
    return (uint16_t)((v >> NANBOX_TYPE_SHIFT) & 0xFFFFu);
}

#endif /* USE_TAGGED_12B */

Value val_clone_comp(MemArena *mem, Value v, uint16_t flat_size) {
    uint16_t tid = val_get_type(mem, v);
    uint16_t old_idx = val_get_pool_idx(v);
    uint16_t new_idx = mem_alloc_pool(mem, flat_size);
    for (uint16_t i = 0; i < flat_size; ++i) {
        mem_write_pool(mem, new_idx + i, mem_read_pool(mem, old_idx + i));
    }
    return val_make_comp(mem, tid, new_idx, flat_size);
}