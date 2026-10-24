/*
 * mathc_bin.h — In-Memory `.mathc` Binary Image Format, Serializer & O(1) Loader
 *
 * Purpose:
 *   Defines the 48-byte `MathcHeader` binary format (including the 32-bit
 *   intrinsic table fingerprint `intr_hash` and full 64-bit `active_modules`
 *   mask) and provides in-memory `.mathc` serialization, FNV-1a checksumming,
 *   structural validation, and O(1) `memcpy` loading inside `Core/compile/`.
 *
 * Portability & Layout Guarantees:
 *   - The `.mathc` binary format is little-endian (matching x86-64, ARM/AArch64
 *     Android NDK, and RISC-V targets) and relies on exact fixed-width C11
 *     struct layouts verified at compile time via `_Static_assert(sizeof(...))`
 *     and `_Static_assert(offsetof(...))` checks below.
 */

#ifndef MATHC_BIN_H
#define MATHC_BIN_H

#include "mem_arena.h"
#include "sym_table.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. PRECOMPILED `.mathc` BINARY IMAGE HEADER (48 BYTES, LITTLE-ENDIAN)
 * ============================================================================ */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
               ".mathc binary serialization requires a little-endian target (x86_64/ARM64/ARMv7)");
#endif

#define MATHC_MAGIC            0x4348544Du  /* "MTHC" in little-endian        */
#define MATHC_VERSION          1u
#define MATHC_MAX_IMAGE_BYTES  262144u      /* Max serialized .mathc size     */

typedef struct {
    uint32_t magic;              /* MATHC_MAGIC ("MTHC")                      */
    uint16_t version;            /* MATHC_VERSION (1)                         */
    uint8_t  fast_real_type;     /* MemArena.fast_real_type                   */
    uint8_t  str_type_id;        /* MemArena.str_type_id                      */
    uint8_t  vec_type_id;        /* MemArena.vec_type_id                      */
    uint8_t  mod_count;          /* Active modules in SymTable (0..64)        */
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
    uint32_t intr_hash;          /* 32-bit FNV-1a fingerprint of IntrinsicId  */
    uint32_t checksum;           /* 32-bit FNV-1a checksum of header+payload  */
    uint64_t active_modules;     /* 64-bit active module bitmask (0..63)      */
} MathcHeader;

/* Compile-time size and field-offset verification for all serialized `.mathc` structs */
_Static_assert(sizeof(MathcHeader) == 48, "MathcHeader must be strictly 48 bytes");
_Static_assert(offsetof(MathcHeader, magic)          == 0,  "MathcHeader.magic offset");
_Static_assert(offsetof(MathcHeader, version)        == 4,  "MathcHeader.version offset");
_Static_assert(offsetof(MathcHeader, sym_count)      == 12, "MathcHeader.sym_count offset");
_Static_assert(offsetof(MathcHeader, perm_src_len)   == 16, "MathcHeader.perm_src_len offset");
_Static_assert(offsetof(MathcHeader, payload_bytes)  == 28, "MathcHeader.payload_bytes offset");
_Static_assert(offsetof(MathcHeader, intr_hash)      == 32, "MathcHeader.intr_hash offset");
_Static_assert(offsetof(MathcHeader, checksum)       == 36, "MathcHeader.checksum offset");
_Static_assert(offsetof(MathcHeader, active_modules) == 40, "MathcHeader.active_modules offset");

_Static_assert(sizeof(StrSlice) == 4, "StrSlice must be 4 bytes");
_Static_assert(sizeof(UnitDimDef) == 8, "UnitDimDef must be 8 bytes");
_Static_assert(offsetof(StrSlice, off) == 0 && offsetof(StrSlice, len) == 2, "StrSlice layout");

_Static_assert(sizeof(AstNode) == 8, "AstNode must be 8 bytes");
_Static_assert(offsetof(AstNode, kind)  == 0 && offsetof(AstNode, sub)   == 1 &&
               offsetof(AstNode, ref)   == 2 && offsetof(AstNode, left)  == 4 &&
               offsetof(AstNode, right) == 6, "AstNode layout");

_Static_assert(sizeof(PatElem) == 6, "PatElem must be 6 bytes");
_Static_assert(offsetof(PatElem, kind)  == 0 && offsetof(PatElem, quant) == 1 &&
               offsetof(PatElem, chars) == 2, "PatElem layout");

_Static_assert(sizeof(FieldDef) == 8, "FieldDef must be 8 bytes");
_Static_assert(offsetof(FieldDef, name)        == 0 &&
               offsetof(FieldDef, type_id)     == 4 &&
               offsetof(FieldDef, flat_offset) == 6, "FieldDef layout");

_Static_assert(sizeof(TypeDef) == 12, "TypeDef must be 12 bytes");
_Static_assert(offsetof(TypeDef, name)         == 0 &&
               offsetof(TypeDef, mod_id)       == 4 &&
               offsetof(TypeDef, is_consuming) == 5 &&
               offsetof(TypeDef, parser_intr)  == 6 &&
               offsetof(TypeDef, field_count)  == 7 &&
               offsetof(TypeDef, start_idx)    == 8 &&
               offsetof(TypeDef, flat_size)    == 10, "TypeDef layout");

_Static_assert(sizeof(OpOverload) == 44, "OpOverload must be 44 bytes");
_Static_assert(offsetof(OpOverload, sym_id)      == 0  &&
               offsetof(OpOverload, mod_id)      == 2  &&
               offsetof(OpOverload, fixity)      == 3  &&
               offsetof(OpOverload, prec)        == 4  &&
               offsetof(OpOverload, param_count) == 5  &&
               offsetof(OpOverload, local_count) == 6  &&
               offsetof(OpOverload, ret_type)    == 7  &&
               offsetof(OpOverload, ret_unit)    == 8  &&
               offsetof(OpOverload, meta_tag)    == 9  &&
               offsetof(OpOverload, param_types) == 10 &&
               offsetof(OpOverload, param_units) == 26 &&
               offsetof(OpOverload, body_ast)    == 42, "OpOverload layout");

_Static_assert(sizeof(ConstDef) == 8, "ConstDef must be 8 bytes");
_Static_assert(offsetof(ConstDef, sym_id)   == 0 &&
               offsetof(ConstDef, mod_id)   == 2 &&
               offsetof(ConstDef, type_id)  == 3 &&
               offsetof(ConstDef, unit_id)  == 4 &&
               offsetof(ConstDef, val_idx)  == 5 &&
               offsetof(ConstDef, pool_idx) == 6, "ConstDef layout");

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
