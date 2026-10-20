/*
 * mathc_bin.h — In-Memory `.mathc` Binary Image Format, Serializer & O(1) Loader
 *
 * Purpose:
 *   Defines the 40-byte ABI- & OS-neutral `MathcHeader` binary format and
 *   provides in-memory `.mathc` serialization, FNV-1a checksumming, structural
 *   validation, and O(1) `memcpy` loading inside `Core/compile/`.
 */

#ifndef MATHC_BIN_H
#define MATHC_BIN_H

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
    uint32_t active_modules;     /* 32-bit active module bitmask (0..31)      */
    uint32_t checksum;           /* 32-bit FNV-1a checksum of header+payload  */
} MathcHeader;

_Static_assert(sizeof(MathcHeader) == 40, "MathcHeader must be strictly 40 bytes");

/* ============================================================================
 * 2. IN-MEMORY `.mathc` BINARY SERIALIZER & O(1) LOADER API
 * ============================================================================ */

/* Compute the exact byte size required to serialize the current permanent state */
uint32_t ext_calc_mathc_size(const MemArena *mem, const SymTable *tab);

/* Serialize current permanent state into `out_buf` (returns bytes written, or 0 on error) */
size_t   ext_save_mathc_mem(MemArena *mem, const SymTable *tab,
                            uint8_t *out_buf, size_t buf_cap);

/* Compute 32-bit FNV-1a checksum over a serialized `.mathc` image buffer */
uint32_t ext_calc_mathc_checksum(const MathcHeader *hdr,
                                 const uint8_t *payload, size_t payload_len);

/* Validate all header & payload indices in a `.mathc` buffer without mutating state */
bool     ext_validate_mathc_buf(const uint8_t *data, size_t len);

/* Load a precompiled `.mathc` binary image from memory (transactional & checksummed) */
bool     ext_load_mathc_mem(MemArena *mem, SymTable *tab,
                            const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* MATHC_BIN_H */
