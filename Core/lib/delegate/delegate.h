/*
 * delegate.h — Internal C `@` Intrinsic Delegate Registry & Execution Engine
 *
 * Location: Core/lib/delegate/delegate.h
 *
 * Purpose:
 *   Isolates all C-backed `@<intrinsic>` operations that extension modules
 *   delegate to via `@` syntax (`@add`, `@sub`, `@sin`, `@vnew`, `@vpush`,
 *   `@vshift`, `@vpeek`, `@vrange`, `@vin`, `@fcall`, `@vmap`, `@vfilter`,
 *   `@vreduce`, `@vany`, `@vall`, `@vfind`, `@vcount`, `@mzero`, `@mmul`,
 *   `@mdet`, `@minv`, `@calc_diff`, `@calc_integ`, `@calc_lim`, `@calc_sigma`,
 *   `@calc_prod`, `@calc_solve`, `@calc_fmin`, `@calc_fmax`, `@tostr`,
 *   `@fromstr`, `@f64`, `@lparen`, `@seq`, `@store`, etc.).
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

/*Below section is the C's native lookup resolver for operator/function delegates @<intrinsic>*/
/*Construct @f64, @lparen, @seq, @store, @add, @sin, @vnew, @mmul, @calc_diff, @tostr, @fromstr*/
IntrinsicId delegate_lookup_intrinsic(const char *s, uint16_t len);

/*Below section is the C's native scalar and unit validation helpers for operator delegates @add, @sub, @eq..@approx*/
/*Construct unit-checked arithmetic/comparison and integer index/dimension checks*/
bool  delegate_is_zero_unitless_scalar(const MemArena *mem, const SymTable *tab, Value v);
bool  delegate_check_additive_unit_compat(MemArena *mem, const SymTable *tab,
                                          const char *op_name, int op_len,
                                          Value a, Value b, uint8_t *out_unit);
bool  delegate_val_to_exact_int(MemArena *mem, const SymTable *tab, Value v,
                                const char *ctx, int *out_int);

/*Below section is the C's native closure invocation engine for operator/function delegates @lambda and @fcall*/
/*Construct (x) => expr, f(x), call(f, x)*/
Value delegate_call_fn(MemArena *mem, SymTable *tab,
                       Value fn_val, uint8_t argc, const Value *args);

/*Below section is the C's native for operator/function delegates @vnew, @vfill, @vpush, @vpop, @vshift, @vpeek, @vget, @vset, @vlen, @vcap, @vresize, @vshrink, @vclear, @vrange, @vin, @vmap..@vcount*/
/*Construct vec<T>(), list<T>(), stack<T>(), queue<T>(), push, pop, enqueue, dequeue, peek, front, .., ..., ..<, <.., in, into, map, filter, reduce, fold, any, all, find, count_if*/
void  delegate_vec_write_elem(MemArena *mem, const SymTable *tab,
                              uint16_t elem_tid, uint16_t dst_pidx, Value item);
Value delegate_vec_read_elem(MemArena *mem, const SymTable *tab,
                             uint16_t elem_tid, uint8_t elem_uid, uint16_t src_pidx);
Value delegate_eval_vec_intrinsic(MemArena *mem, SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c);

/*Below section is the C's native for operator/function delegates @mzero, @mones, @meye, @mfill, @mrows, @mcols, @mrank, @mlen, @mshape, @madd, @msub, @mscal, @mmul, @mtrans, @mtrace, @mdet, @minv*/
/*Construct [a, b ; c, d], A + B, A - B, A * B, s * A, det(A), trace(A), transpose(A), inv(A), mat_zeros, mat_ones, mat_eye, mat_fill*/
bool  delegate_mat_get_info(const MemArena *mem, Value v,
                            uint8_t *out_rank, uint16_t *out_total_len,
                            uint16_t *out_data_idx, uint16_t dims[4]);
Value delegate_eval_mat_intrinsic(MemArena *mem, const SymTable *tab,
                                  IntrinsicId intr, Value a, Value b, Value c);

/*Below section is the C's native top-level dispatcher for operator/function delegates @add..@err, @tostr, @fromstr*/
/*Construct +, -, *, /, %, ^, √, sin..tanh, ==..~, &&..nand, type_of, unit_of, x_, _s, error*/
Value delegate_eval_intrinsic(MemArena *mem, SymTable *tab,
                              IntrinsicId intr, Value a, Value b, Value c, uint8_t ret_type);

/*Below section is the C's native for calculus and solver delegates @calc_diff, @calc_diff2, @calc_diffn, @calc_integ, @calc_lim, @calc_lim_l, @calc_lim_r, @calc_sigma, @calc_prod, @calc_solve, @calc_fmin, @calc_fmax*/
/*Construct diff, diff2, diff_n, integrate, ntg, lim, lim_left, lim_right, sigma, prod, solve, fmin, fmax*/
Value delegate_eval_calculus_meta(MemArena *mem, SymTable *tab,
                                  uint8_t mtag, const AstNode *n,
                                  uint16_t frame_base, uint8_t cur_ret_type);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DELEGATE_H */
