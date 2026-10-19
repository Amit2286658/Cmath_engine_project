/*
 * ext_loader.h — In-Memory `.math` Extension Compiler & `.mathc` Binary Serializer
 *
 * Purpose:
 *   1. Compiles raw `.math` source text strings into `MemArena` + `SymTable`
 *      permanent tables (`ext_load_str`) with transactional rollback on error.
 *   2. Serializes and deserializes precompiled `.mathc` binary images strictly
 *      in memory (`ext_save_mathc_mem`, `ext_load_mathc_mem`) so the engine has
 *      zero filesystem, directory, or OS path dependencies.
 */

#ifndef EXT_LOADER_H
#define EXT_LOADER_H

#include "mem_arena.h"
#include "sym_table.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. PRECOMPILED `.mathc` BINARY IMAGE HEADER (40 BYTES, ABI- & OS-NEUTRAL)
 * ============================================================================ */

#define MATHC_MAGIC            0x4348544Du  /* "MTHC" in little-endian        */
#define MATHC_VERSION          1u
#define MATHC_MAX_IMAGE_BYTES  262144u      /* Max serialized .mathc size     */

typedef struct {
    uint32_t magic;              /* MATHC_MAGIC ("MTHC")                      */
    uint16_t version;            /* MATHC_VERSION (1)                         */
    uint8_t  fast_real_type;     /* MemArena.fast_real_type                   */
    uint8_t  str_type_id;        /* MemArena.str_type_id                      */
    uint8_t  vec_type_id;        /* MemArena.vec_type_id                      */
    uint8_t  mod_count;          /* Active modules in SymTable                */
    uint8_t  type_count;         /* Active types in SymTable                  */
    uint8_t  const_count;        /* Active constants in SymTable              */
    uint16_t sym_count;          /* Active interned symbols in SymTable       */
    uint8_t  unit_count;         /* Active unit dimensions in SymTable        */
    uint8_t  mat_type_id;        /* MemArena.mat_type_id                      */
    uint16_t perm_src_len;       /* Active bytes in MemArena.src_buf          */
    uint16_t perm_ast_count;     /* Active AstNodes in MemArena.ast           */
    uint16_t perm_pool_top;      /* Active doubles in MemArena.f64_pool       */
    uint16_t pat_elem_count;     /* Active pattern atoms in SymTable          */
    uint16_t field_count;        /* Active struct fields in SymTable          */
    uint16_t overload_count;     /* Active operator/func overloads            */
    uint32_t payload_bytes;      /* Total byte size following this header     */
    uint64_t active_modules;     /* 64-bit active module bitmask              */
} MathcHeader;

_Static_assert(sizeof(MathcHeader) == 40, "MathcHeader must be strictly 40 bytes");

/* ============================================================================
 * 2. IN-MEMORY `.math` TEXT COMPILER & `.mathc` BINARY SERIALIZER API
 * ============================================================================ */

/* Compile raw `.math` extension source text into permanent Arena + SymTable state */
bool     ext_load_str(MemArena *mem, SymTable *tab, const char *ext_src);

/* Compute the exact byte size required to serialize the current permanent state */
uint32_t ext_calc_mathc_size(const MemArena *mem, const SymTable *tab);

/* Serialize current permanent state into `out_buf` (returns bytes written, or 0 on error) */
size_t   ext_save_mathc_mem(MemArena *mem, const SymTable *tab,
                            uint8_t *out_buf, size_t buf_cap);

/* Load a precompiled `.mathc` binary image from memory in O(1) `memcpy` time */
bool     ext_load_mathc_mem(MemArena *mem, SymTable *tab,
                            const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* EXT_LOADER_H */
