/*
 * delegate.h — Internal C `@` Intrinsic Delegate Registry & Execution Engine
 *
 * Location: Core/lib/delegate/delegate.h
 *
 * Purpose:
 *   Isolates all C-backed `@<intrinsic>` operations that extension modules
 *   delegate to via `@` syntax (`@add`, `@sub`, `@sin`, `@vnew`, `@vpush`,
 *   `@vrange`, `@vin`, `@mzero`, `@mmul`, `@mdet`, `@minv`, `@calc_diff`,
 *   `@calc_integ`, `@calc_lim`, `@calc_sigma`, `@calc_prod`, `@calc_solve`,
 *   `@calc_fmin`, `@calc_fmax`, `@f64`, `@lparen`, `@seq`, `@store`, etc.).
 *
 *   Distinct from:
 *     - `Library/`           : External text/binary extension definitions
 *     - `Core/lib/hardcode/` : Hardcoded parser/evaluator ergonomic rules
 */

#ifndef CORE_DELEGATE_H
#define CORE_DELEGATE_H

#include "../../Headers/sym_table.h"
#include "../../Headers/evaluator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lookup a `@<name>` intrinsic opcode in the delegate registry */
IntrinsicId delegate_lookup_intrinsic(const char *s, uint16_t len);

/* Scalar & unit validation helpers shared by delegate operations */
bool  delegate_is_zero_unitless_scalar(const MemArena *mem, const SymTable *tab, Value v);
bool  delegate_check_additive_unit_compat(MemArena *mem, const SymTable *tab,
                                          const char *op_name, int op_len,
                                          Value a, Value b, uint8_t *out_unit);
bool  delegate_val_to_exact_int(MemArena *mem, const SymTable *tab, Value v,
                                const char *ctx, int *out_int);

/* Dynamic vector (`$vec`) element read/write & `@v*` intrinsic delegate */
void  delegate_vec_write_elem(MemArena *mem, const SymTable *tab,
                              uint16_t elem_tid, uint16_t dst_pidx, Value item);
Value delegate_vec_read_elem(MemArena *mem, const SymTable *tab,
                             uint16_t elem_tid, uint8_t elem_uid, uint16_t src_pidx);
Value delegate_eval_vec_intrinsic(MemArena *mem, const SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c);

/* Multidimensional matrix (`$mat`) inspection & `@m*` intrinsic delegate */
bool  delegate_mat_get_info(const MemArena *mem, Value v,
                            uint8_t *out_rank, uint16_t *out_total_len,
                            uint16_t *out_data_idx, uint16_t dims[4]);
Value delegate_eval_mat_intrinsic(MemArena *mem, const SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c);

/* Top-level `@<intrinsic>(a, b, c)` dispatcher (arithmetic, trig, logic, vec, mat) */
Value delegate_eval_intrinsic(MemArena *mem, const SymTable *tab,
                              IntrinsicId intr, Value a, Value b, Value c, uint8_t ret_type);

/* Lazy expression-evaluating `@calc_*` delegates (`diff`, `integrate`, `lim`, `sigma`, `prod`, `solve`, `fmin`, `fmax`) */
Value delegate_eval_calculus_meta(MemArena *mem, const SymTable *tab,
                                  uint8_t mtag, const AstNode *n,
                                  uint16_t frame_base, uint8_t cur_ret_type);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DELEGATE_H */
