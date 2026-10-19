/*
 * evaluator.c — Standalone AST Evaluator & Private C Intrinsics Implementation
 */

#include "evaluator.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

/* ============================================================================
 * 1. UNIT COMPATIBILITY HELPERS
 * ============================================================================ */

static bool is_zero_unitless_scalar(const MemArena *mem, const SymTable *tab, Value v) {
    if (val_get_unit(mem, v) != MEM_UNIT_NONE) return false;
    uint16_t tid = val_get_type(mem, v);
    if (tid == 0 || tid >= tab->type_count || !tab->types[tid].is_consuming) return false;
    if (tid == mem->str_type_id) return false;
    return fabs(val_get_f64(mem, v)) <= 1e-15;
}

static bool check_additive_unit_compat(MemArena *mem, const SymTable *tab,
                                       const char *op_name, int op_len,
                                       Value a, Value b, uint8_t *out_unit) {
    uint8_t u1 = val_get_unit(mem, a);
    uint8_t u2 = val_get_unit(mem, b);
    if (u1 == u2) {
        if (out_unit) *out_unit = u1;
        return true;
    }
    if (is_zero_unitless_scalar(mem, tab, a)) {
        if (out_unit) *out_unit = u2;
        return true;
    }
    if (is_zero_unitless_scalar(mem, tab, b)) {
        if (out_unit) *out_unit = u1;
        return true;
    }
    char u1_buf[32], u2_buf[32];
    sym_unit_name(tab, mem, u1, u1_buf, sizeof(u1_buf));
    sym_unit_name(tab, mem, u2, u2_buf, sizeof(u2_buf));
    mem_err(mem, "Unit mismatch in '%.*s': cannot combine '%s' and '%s'",
            op_len, op_name, u1_buf, u2_buf);
    return false;
}

/* ============================================================================
 * 2. DYNAMIC VECTOR (`$vec` OF ANY `T` TYPE) ELEMENT READ/WRITE & INTRINSICS
 * ============================================================================ */

static void vec_write_elem(MemArena *mem, const SymTable *tab,
                           uint16_t elem_tid, uint16_t dst_pidx, Value item) {
    if (elem_tid == mem->str_type_id && elem_tid != 0) {
        StrSlice s = val_get_str(mem, item);
        uint32_t packed = ((uint32_t)s.off << 16) | (uint32_t)s.len;
        mem_write_pool(mem, dst_pidx, (double)packed);
    } else if (elem_tid < tab->type_count && !tab->types[elem_tid].is_consuming) {
        uint16_t stride   = tab->types[elem_tid].flat_size;
        uint16_t src_pidx = val_get_pool_idx(item);
        for (uint16_t k = 0; k < stride; ++k) {
            mem_write_pool(mem, dst_pidx + k, mem_read_pool(mem, src_pidx + k));
        }
    } else {
        mem_write_pool(mem, dst_pidx, val_get_f64(mem, item));
    }
}

static Value vec_read_elem(MemArena *mem, const SymTable *tab,
                           uint16_t elem_tid, uint8_t elem_uid, uint16_t src_pidx) {
    if (elem_tid == mem->str_type_id && elem_tid != 0) {
        uint32_t packed = (uint32_t)mem_read_pool(mem, src_pidx);
        StrSlice s = { (uint16_t)(packed >> 16), (uint16_t)(packed & 0xFFFFu) };
        return val_make_str(mem, elem_tid, s);
    } else if (elem_tid < tab->type_count && !tab->types[elem_tid].is_consuming) {
        uint16_t stride = tab->types[elem_tid].flat_size;
        return val_with_unit(mem, val_make_comp(mem, elem_tid, src_pidx, stride), elem_uid);
    } else {
        return val_make_f64_unit(mem, elem_tid ? elem_tid : mem->fast_real_type,
                                 elem_uid, mem_read_pool(mem, src_pidx));
    }
}

static Value eval_vec_intrinsic(MemArena *mem, const SymTable *tab,
                                IntrinsicId intr, Value a, Value b, Value c) {
    if (mem->vec_type_id == 0) {
        mem_err(mem, "Dynamic vector type ($vec) is not loaded");
        return val_void();
    }

    if (intr == INTR_VNEW) {
        int cap_in = (val_get_type(mem, a) != MEM_TYPE_VOID) ? (int)val_get_f64(mem, a) : 0;
        if (cap_in < 0) {
            mem_err(mem, "Vector capacity cannot be negative (%d)", cap_in);
            return val_void();
        }
        uint16_t hdr_idx = mem_vec_alloc_hdr(mem, 0, (uint16_t)cap_in);
        if (mem->has_error) return val_void();
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    if (intr == INTR_VFILL) {
        int count = (int)val_get_f64(mem, a);
        if (count < 0) {
            mem_err(mem, "Vector fill count cannot be negative (%d)", count);
            return val_void();
        }
        uint16_t elem_tid = val_get_type(mem, b);
        uint8_t  elem_uid = val_get_unit(mem, b);
        uint16_t packed_t = (uint16_t)(((uint16_t)elem_uid << 8) | (elem_tid & 0xFFu));
        uint16_t stride   = (elem_tid < tab->type_count) ? tab->types[elem_tid].flat_size : 1;
        uint16_t hdr_idx  = mem_vec_alloc_hdr(mem, packed_t, (uint16_t)count);
        if (mem->has_error) return val_void();
        if (count > 0) {
            if (!mem_vec_reserve(mem, hdr_idx, stride, (uint16_t)count)) return val_void();
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            for (uint16_t i = 0; i < (uint16_t)count; ++i) {
                vec_write_elem(mem, tab, elem_tid, (uint16_t)(data_idx + i * stride), b);
            }
            mem_write_pool(mem, hdr_idx + 1, (double)count);
        }
        return val_make_comp(mem, mem->vec_type_id, hdr_idx, MEM_VEC_HDR_SLOTS);
    }

    if (val_get_type(mem, a) != mem->vec_type_id) {
        mem_err(mem, "Expected dynamic vector ($vec) argument");
        return val_void();
    }
    uint16_t hdr_idx  = val_get_pool_idx(a);
    uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
    uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
    uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
    uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
    uint16_t cap      = (uint16_t)mem_read_pool(mem, hdr_idx + 2);
    uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                        ? tab->types[elem_tid].flat_size : 1;

    switch (intr) {
        case INTR_VLEN:
            return val_make_f64(mem, mem->fast_real_type, (double)len);

        case INTR_VCAP:
            return val_make_f64(mem, mem->fast_real_type, (double)cap);

        case INTR_VCLEAR:
            mem_write_pool(mem, hdr_idx + 1, 0.0);
            return a;

        case INTR_VSHRINK:
            mem_vec_shrink_to_fit(mem, hdr_idx, stride);
            return a;

        case INTR_VPUSH: {
            uint16_t item_tid = val_get_type(mem, b);
            uint8_t  item_uid = val_get_unit(mem, b);
            if (elem_tid == 0) {
                elem_tid = item_tid;
                elem_uid = item_uid;
                stride   = (elem_tid < tab->type_count) ? tab->types[elem_tid].flat_size : 1;
                uint16_t packed_t = (uint16_t)(((uint16_t)elem_uid << 8) | (elem_tid & 0xFFu));
                mem_write_pool(mem, hdr_idx + 0, (double)packed_t);
            } else if (elem_tid != item_tid || elem_uid != item_uid) {
                char exp_t[32], got_t[32];
                sym_type_name(tab, mem, elem_tid, exp_t, sizeof(exp_t));
                sym_type_name(tab, mem, item_tid, got_t, sizeof(got_t));
                mem_err(mem, "Vector type mismatch: cannot push '%s' into vec<%s>", got_t, exp_t);
                return val_void();
            }
            if (!mem_vec_reserve(mem, hdr_idx, stride, (uint16_t)(len + 1))) {
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            vec_write_elem(mem, tab, elem_tid, (uint16_t)(data_idx + len * stride), b);
            mem_write_pool(mem, hdr_idx + 1, (double)(len + 1));
            return a;
        }

        case INTR_VPOP: {
            if (len == 0) {
                mem_err(mem, "Cannot pop from an empty vector");
                return val_void();
            }
            uint16_t new_len  = (uint16_t)(len - 1);
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return vec_read_elem(mem, tab, elem_tid, elem_uid, (uint16_t)(data_idx + new_len * stride));
        }

        case INTR_VGET: {
            int idx = (int)val_get_f64(mem, b);
            if (idx < 0) idx += (int)len;
            if (idx < 0 || idx >= (int)len) {
                mem_err(mem, "Vector index %d out of bounds (len=%u)",
                        (int)val_get_f64(mem, b), (unsigned)len);
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            return vec_read_elem(mem, tab, elem_tid, elem_uid, (uint16_t)(data_idx + (uint16_t)idx * stride));
        }

        case INTR_VSET: {
            int idx = (int)val_get_f64(mem, b);
            if (idx < 0) idx += (int)len;
            if (idx < 0 || idx >= (int)len) {
                mem_err(mem, "Vector index %d out of bounds (len=%u)",
                        (int)val_get_f64(mem, b), (unsigned)len);
                return val_void();
            }
            uint16_t item_tid = val_get_type(mem, c);
            uint8_t  item_uid = val_get_unit(mem, c);
            if (elem_tid != 0 && (elem_tid != item_tid || elem_uid != item_uid)) {
                char exp_t[32], got_t[32];
                sym_type_name(tab, mem, elem_tid, exp_t, sizeof(exp_t));
                sym_type_name(tab, mem, item_tid, got_t, sizeof(got_t));
                mem_err(mem, "Vector type mismatch: cannot set '%s' in vec<%s>", got_t, exp_t);
                return val_void();
            }
            uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
            vec_write_elem(mem, tab, elem_tid, (uint16_t)(data_idx + (uint16_t)idx * stride), c);
            return a;
        }

        case INTR_VRESIZE: {
            int new_len_in = (int)val_get_f64(mem, b);
            if (new_len_in < 0) {
                mem_err(mem, "Vector resize length cannot be negative (%d)", new_len_in);
                return val_void();
            }
            uint16_t new_len = (uint16_t)new_len_in;
            if (elem_tid == 0) {
                elem_tid = mem->fast_real_type;
                stride   = 1;
                mem_write_pool(mem, hdr_idx + 0, (double)elem_tid);
            }
            if (new_len > len) {
                if (!mem_vec_reserve(mem, hdr_idx, stride, new_len)) return val_void();
                uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
                memset(&mem->f64_pool[data_idx + len * stride], 0,
                       (size_t)((new_len - len) * stride) * sizeof(double));
            }
            mem_write_pool(mem, hdr_idx + 1, (double)new_len);
            return a;
        }

        default:
            return val_void();
    }
}

/* ============================================================================
 * 3. SCALAR ARITHMETIC, BOOLEAN LOGIC & TRANSCENDENTAL INTRINSICS
 * ============================================================================ */


/* ============================================================================
 * 2b. MULTIDIMENSIONAL MATRIX (`$mat`) INTRINSICS & LINEAR ALGEBRA
 * ============================================================================ */

static bool mat_get_info(const MemArena *mem, Value v,
                         uint8_t *out_rank, uint16_t *out_total_len,
                         uint16_t *out_data_idx, uint16_t dims[4]) {
    if (mem->mat_type_id == 0 || val_get_type(mem, v) != mem->mat_type_id) {
        return false;
    }
    uint16_t hdr_idx = val_get_pool_idx(v);
    uint8_t  rank    = (uint8_t)mem_read_pool(mem, hdr_idx + 0);
    uint16_t tot_len = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
    uint16_t dat_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 2);
    for (uint8_t i = 0; i < 4; ++i) {
        dims[i] = (i < rank) ? (uint16_t)mem_read_pool(mem, hdr_idx + 4 + i) : 0;
    }
    if (out_rank)      *out_rank      = rank;
    if (out_total_len) *out_total_len = tot_len;
    if (out_data_idx)  *out_data_idx  = dat_idx;
    return true;
}

static Value eval_mat_intrinsic(MemArena *mem, const SymTable *tab,
                                IntrinsicId intr, Value a, Value b, Value c) {
    if (mem->mat_type_id == 0) {
        mem_err(mem, "Multidimensional matrix type ($mat) is not loaded");
        return val_void();
    }

    if (intr == INTR_MZERO || intr == INTR_MONES || intr == INTR_MFILL) {
        int r_in = (int)val_get_f64(mem, a);
        int c_in = (int)val_get_f64(mem, b);
        if (r_in <= 0 || c_in <= 0) {
            mem_err(mem, "Matrix dimensions must be positive (%d x %d)", r_in, c_in);
            return val_void();
        }
        uint16_t dims[4] = { (uint16_t)r_in, (uint16_t)c_in, 0, 0 };
        uint16_t tot_len = 0, data_idx = 0;
        uint16_t hdr_idx = mem_mat_alloc_hdr(mem, 2, dims, &tot_len, &data_idx);
        if (mem->has_error) return val_void();

        double fill_v = (intr == INTR_MONES) ? 1.0 :
                        (intr == INTR_MFILL) ? val_get_f64(mem, c) : 0.0;
        uint8_t uid   = (intr == INTR_MFILL) ? val_get_unit(mem, c) : MEM_UNIT_NONE;
        if (fill_v != 0.0) {
            for (uint16_t i = 0; i < tot_len; ++i) {
                mem_write_pool(mem, data_idx + i, fill_v);
            }
        }
        return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS), uid);
    }

    if (intr == INTR_MEYE) {
        int n_in = (int)val_get_f64(mem, a);
        if (n_in <= 0) {
            mem_err(mem, "Identity matrix size must be positive (%d)", n_in);
            return val_void();
        }
        uint16_t n = (uint16_t)n_in;
        uint16_t dims[4] = { n, n, 0, 0 };
        uint16_t tot_len = 0, data_idx = 0;
        uint16_t hdr_idx = mem_mat_alloc_hdr(mem, 2, dims, &tot_len, &data_idx);
        if (mem->has_error) return val_void();
        for (uint16_t i = 0; i < n; ++i) {
            mem_write_pool(mem, (uint16_t)(data_idx + i * n + i), 1.0);
        }
        return val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS);
    }

    uint8_t  ra = 0;
    uint16_t tot_a = 0, dat_a = 0, da[4] = { 0 };
    if (!mat_get_info(mem, a, &ra, &tot_a, &dat_a, da)) {
        mem_err(mem, "Expected matrix ($mat) argument");
        return val_void();
    }
    uint8_t ua = val_get_unit(mem, a);

    switch (intr) {
        case INTR_MROWS:
            return val_make_f64(mem, mem->fast_real_type, (double)da[0]);
        case INTR_MCOLS:
            return val_make_f64(mem, mem->fast_real_type, (double)((ra >= 2) ? da[1] : da[0]));
        case INTR_MRANK:
            return val_make_f64(mem, mem->fast_real_type, (double)ra);
        case INTR_MLEN:
            return val_make_f64(mem, mem->fast_real_type, (double)tot_a);
        case INTR_MSHAPE: {
            double shape_d[4];
            for (uint8_t i = 0; i < ra; ++i) shape_d[i] = (double)da[i];
            return val_from_f64_vec(mem, shape_d, ra);
        }
        case INTR_MSCAL: {
            double  s  = val_get_f64(mem, b);
            uint8_t ub = val_get_unit(mem, b);
            uint8_t ru = (ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE);
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, ra, da, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t i = 0; i < tot_a; ++i) {
                mem_write_pool(mem, dat_out + i, mem_read_pool(mem, dat_a + i) * s);
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
        }
        case INTR_MADD:
        case INTR_MSUB: {
            uint8_t  rb = 0;
            uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
            if (!mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims)) {
                mem_err(mem, "Expected second matrix ($mat) argument");
                return val_void();
            }
            if (ra != rb) {
                mem_err(mem, "Matrix rank mismatch (%uD vs %uD)", (unsigned)ra, (unsigned)rb);
                return val_void();
            }
            for (uint8_t i = 0; i < ra; ++i) {
                if (da[i] != db_dims[i]) {
                    mem_err(mem, "Matrix shape mismatch on axis %u (%u vs %u)",
                            (unsigned)i, (unsigned)da[i], (unsigned)db_dims[i]);
                    return val_void();
                }
            }
            uint8_t ru = MEM_UNIT_NONE;
            if (!check_additive_unit_compat(mem, tab, (intr == INTR_MADD) ? "+" : "-", 1, a, b, &ru)) {
                return val_void();
            }
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, ra, da, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t i = 0; i < tot_a; ++i) {
                double va = mem_read_pool(mem, dat_a + i);
                double vb = mem_read_pool(mem, dat_b + i);
                mem_write_pool(mem, dat_out + i, (intr == INTR_MADD) ? (va + vb) : (va - vb));
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
        }
        case INTR_MMUL: {
            uint8_t  rb = 0;
            uint16_t tot_b = 0, dat_b = 0, db_dims[4] = { 0 };
            if (!mat_get_info(mem, b, &rb, &tot_b, &dat_b, db_dims)) {
                mem_err(mem, "Expected second matrix ($mat) argument for '*'");
                return val_void();
            }
            uint8_t ub = val_get_unit(mem, b);
            uint8_t ru = (ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE);

            /* 2D (M x K) * 2D (K x N) -> 2D (M x N) */
            if (ra == 2 && rb == 2) {
                uint16_t M = da[0], K = da[1], Kb = db_dims[0], N = db_dims[1];
                if (K != Kb) {
                    mem_err(mem, "Matrix dimension mismatch in '*': (%ux%u) vs (%ux%u)",
                            (unsigned)M, (unsigned)K, (unsigned)Kb, (unsigned)N);
                    return val_void();
                }
                uint16_t out_dims[4] = { M, N, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t r = 0; r < M; ++r) {
                    for (uint16_t col = 0; col < N; ++col) {
                        double sum = 0.0;
                        for (uint16_t k = 0; k < K; ++k) {
                            sum += mem_read_pool(mem, (uint16_t)(dat_a + r * K + k)) *
                                   mem_read_pool(mem, (uint16_t)(dat_b + k * N + col));
                        }
                        mem_write_pool(mem, (uint16_t)(dat_out + r * N + col), sum);
                    }
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            /* 2D (M x K) * 1D (K) -> 1D (M) */
            if (ra == 2 && rb == 1) {
                uint16_t M = da[0], K = da[1], Kb = db_dims[0];
                if (K != Kb) {
                    mem_err(mem, "Matrix-vector dimension mismatch in '*': (%ux%u) vs (%u)",
                            (unsigned)M, (unsigned)K, (unsigned)Kb);
                    return val_void();
                }
                uint16_t out_dims[4] = { M, 0, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 1, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t r = 0; r < M; ++r) {
                    double sum = 0.0;
                    for (uint16_t k = 0; k < K; ++k) {
                        sum += mem_read_pool(mem, (uint16_t)(dat_a + r * K + k)) *
                               mem_read_pool(mem, (uint16_t)(dat_b + k));
                    }
                    mem_write_pool(mem, (uint16_t)(dat_out + r), sum);
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            /* 1D (K) * 1D (K) -> element-wise or dot product? Return 1D element-wise product */
            if (ra == 1 && rb == 1 && da[0] == db_dims[0]) {
                uint16_t out_dims[4] = { da[0], 0, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 1, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < da[0]; ++i) {
                    mem_write_pool(mem, dat_out + i,
                                   mem_read_pool(mem, dat_a + i) * mem_read_pool(mem, dat_b + i));
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ru);
            }
            mem_err(mem, "Matrix multiplication '*' requires 2D*2D or 2D*1D compatible dimensions");
            return val_void();
        }
        case INTR_MTRANS: {
            if (ra == 1) {
                uint16_t out_dims[4] = { da[0], 1, 0, 0 };
                uint16_t tot_out = 0, dat_out = 0;
                uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
                if (mem->has_error) return val_void();
                for (uint16_t i = 0; i < da[0]; ++i) {
                    mem_write_pool(mem, dat_out + i, mem_read_pool(mem, dat_a + i));
                }
                return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ua);
            }
            if (ra != 2) {
                mem_err(mem, "transpose() requires a 2D matrix (got %uD)", (unsigned)ra);
                return val_void();
            }
            uint16_t R = da[0], C = da[1];
            uint16_t out_dims[4] = { C, R, 0, 0 };
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t r = 0; r < R; ++r) {
                for (uint16_t col = 0; col < C; ++col) {
                    mem_write_pool(mem, (uint16_t)(dat_out + col * R + r),
                                   mem_read_pool(mem, (uint16_t)(dat_a + r * C + col)));
                }
            }
            return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS), ua);
        }
        case INTR_MTRACE: {
            if (ra != 2 || da[0] != da[1]) {
                mem_err(mem, "trace() requires a square 2D matrix");
                return val_void();
            }
            uint16_t N = da[0];
            double tr = 0.0;
            for (uint16_t i = 0; i < N; ++i) {
                tr += mem_read_pool(mem, (uint16_t)(dat_a + i * N + i));
            }
            return val_make_f64_unit(mem, mem->fast_real_type, ua, tr);
        }
        case INTR_MDET: {
            if (ra != 2 || da[0] != da[1] || da[0] == 0 || da[0] > 16) {
                mem_err(mem, "det() requires a square 2D matrix (1x1 up to 16x16)");
                return val_void();
            }
            uint16_t N = da[0];
            double tmp[256];
            for (uint16_t i = 0; i < (uint16_t)(N * N); ++i) {
                tmp[i] = mem_read_pool(mem, dat_a + i);
            }
            double det_val = 1.0;
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t piv = i;
                double max_v = fabs(tmp[i * N + i]);
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double v = fabs(tmp[r * N + i]);
                    if (v > max_v) { max_v = v; piv = r; }
                }
                if (max_v <= 1e-14) return val_make_f64(mem, mem->fast_real_type, 0.0);
                if (piv != i) {
                    for (uint16_t col = i; col < N; ++col) {
                        double t = tmp[i * N + col];
                        tmp[i * N + col] = tmp[piv * N + col];
                        tmp[piv * N + col] = t;
                    }
                    det_val = -det_val;
                }
                double diag = tmp[i * N + i];
                det_val *= diag;
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double factor = tmp[r * N + i] / diag;
                    for (uint16_t col = (uint16_t)(i + 1); col < N; ++col) {
                        tmp[r * N + col] -= factor * tmp[i * N + col];
                    }
                }
            }
            return val_make_f64(mem, mem->fast_real_type, det_val);
        }
        case INTR_MINV: {
            if (ra != 2 || da[0] != da[1] || da[0] == 0 || da[0] > 16) {
                mem_err(mem, "inv() requires a square 2D matrix (1x1 up to 16x16)");
                return val_void();
            }
            uint16_t N = da[0];
            double aug[512]; /* N x 2N augmented matrix */
            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < N; ++col) {
                    aug[r * (2 * N) + col]     = mem_read_pool(mem, (uint16_t)(dat_a + r * N + col));
                    aug[r * (2 * N) + N + col] = (r == col) ? 1.0 : 0.0;
                }
            }
            for (uint16_t i = 0; i < N; ++i) {
                uint16_t piv = i;
                double max_v = fabs(aug[i * (2 * N) + i]);
                for (uint16_t r = (uint16_t)(i + 1); r < N; ++r) {
                    double v = fabs(aug[r * (2 * N) + i]);
                    if (v > max_v) { max_v = v; piv = r; }
                }
                if (max_v <= 1e-14) {
                    mem_err(mem, "Singular matrix cannot be inverted (det == 0)");
                    return val_void();
                }
                if (piv != i) {
                    for (uint16_t col = 0; col < (uint16_t)(2 * N); ++col) {
                        double t = aug[i * (2 * N) + col];
                        aug[i * (2 * N) + col] = aug[piv * (2 * N) + col];
                        aug[piv * (2 * N) + col] = t;
                    }
                }
                double diag = aug[i * (2 * N) + i];
                for (uint16_t col = 0; col < (uint16_t)(2 * N); ++col) {
                    aug[i * (2 * N) + col] /= diag;
                }
                for (uint16_t r = 0; r < N; ++r) {
                    if (r == i) continue;
                    double factor = aug[r * (2 * N) + i];
                    if (fabs(factor) > 0.0) {
                        for (uint16_t col = 0; col < (uint16_t)(2 * N); ++col) {
                            aug[r * (2 * N) + col] -= factor * aug[i * (2 * N) + col];
                        }
                    }
                }
            }
            uint16_t out_dims[4] = { N, N, 0, 0 };
            uint16_t tot_out = 0, dat_out = 0;
            uint16_t hdr_out = mem_mat_alloc_hdr(mem, 2, out_dims, &tot_out, &dat_out);
            if (mem->has_error) return val_void();
            for (uint16_t r = 0; r < N; ++r) {
                for (uint16_t col = 0; col < N; ++col) {
                    double v = aug[r * (2 * N) + N + col];
                    if (fabs(v) < 1e-15) v = 0.0;
                    mem_write_pool(mem, (uint16_t)(dat_out + r * N + col), v);
                }
            }
            return val_make_comp(mem, mem->mat_type_id, hdr_out, MEM_MAT_HDR_SLOTS);
        }
        default:
            return val_void();
    }
}

static Value eval_intrinsic(MemArena *mem, const SymTable *tab,
                            IntrinsicId intr, Value a, Value b, Value c, uint8_t ret_type) {
    if (intr >= INTR_VNEW && intr <= INTR_VCLEAR) {
        return eval_vec_intrinsic(mem, tab, intr, a, b, c);
    }
    if (intr >= INTR_MZERO && intr <= INTR_MINV) {
        return eval_mat_intrinsic(mem, tab, intr, a, b, c);
    }

    if (intr == INTR_TYPE_NAME) {
        char tbuf[32];
        sym_type_name(tab, mem, val_get_type(mem, a), tbuf, sizeof(tbuf));
        return val_from_cstr(mem, tbuf);
    }
    if (intr == INTR_UNIT_NAME) {
        char ubuf[32];
        sym_unit_name(tab, mem, val_get_unit(mem, a), ubuf, sizeof(ubuf));
        return val_from_cstr(mem, ubuf);
    }

    uint16_t rtid = (ret_type != 0 && ret_type < tab->type_count && tab->types[ret_type].is_consuming)
                    ? ret_type
                    : mem->fast_real_type;
    uint8_t  ua   = val_get_unit(mem, a);
    uint8_t  ub   = (val_get_type(mem, b) != MEM_TYPE_VOID) ? val_get_unit(mem, b) : MEM_UNIT_NONE;
    uint8_t  ruid = MEM_UNIT_NONE;

    double da  = val_get_f64(mem, a);
    double db  = (val_get_type(mem, b) != MEM_TYPE_VOID) ? val_get_f64(mem, b) : 0.0;
    double res = 0.0;

    switch (intr) {
        case INTR_ADD:
            if (!check_additive_unit_compat(mem, tab, "+", 1, a, b, &ruid)) return val_void();
            res = da + db;
            break;
        case INTR_SUB:
            if (!check_additive_unit_compat(mem, tab, "-", 1, a, b, &ruid)) return val_void();
            res = da - db;
            break;
        case INTR_MUL:
            ruid = (ua == ub) ? ua : ((ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE));
            res = da * db;
            break;
        case INTR_DIV:
            if (db == 0.0) { mem_err(mem, "Division by zero"); return val_void(); }
            ruid = (ua == ub) ? ua : ((ua != 0 && ub == 0) ? ua : ((ua == 0 && ub != 0) ? ub : MEM_UNIT_NONE));
            res = da / db;
            break;
        case INTR_MOD:
            ruid = (ua == ub) ? ua : ((ua != 0 && ub == 0) ? ua : MEM_UNIT_NONE);
            res = fmod(da, db);
            break;
        case INTR_NEG:   ruid = ua; res = -da; break;
        case INTR_ABS:   ruid = ua; res = fabs(da); break;
        case INTR_FLOOR: ruid = ua; res = floor(da); break;
        case INTR_CEIL:  ruid = ua; res = ceil(da); break;
        case INTR_POW:   ruid = (ub == 0) ? ua : MEM_UNIT_NONE; res = pow(da, db); break;
        case INTR_SQRT:  ruid = ua; res = sqrt(da); break;
        case INTR_CBRT:  ruid = ua; res = cbrt(da); break;
        case INTR_EXP:   res = exp(da); break;
        case INTR_LN:    res = log(da); break;
        case INTR_LOG10: res = log10(da); break;
        case INTR_SIN:   res = sin(da); break;
        case INTR_COS:   res = cos(da); break;
        case INTR_TAN:   res = tan(da); break;
        case INTR_ASIN:  res = asin(da); break;
        case INTR_ACOS:  res = acos(da); break;
        case INTR_ATAN:  res = atan(da); break;
        case INTR_ATAN2: res = atan2(da, db); break;
        case INTR_SINH:  res = sinh(da); break;
        case INTR_COSH:  res = cosh(da); break;
        case INTR_TANH:  res = tanh(da); break;
        case INTR_EQ:
            if (!check_additive_unit_compat(mem, tab, "==", 2, a, b, NULL)) return val_void();
            res = (fabs(da - db) <= 1e-12) ? 1.0 : 0.0;
            break;
        case INTR_NE:
            if (!check_additive_unit_compat(mem, tab, "!=", 2, a, b, NULL)) return val_void();
            res = (fabs(da - db) >  1e-12) ? 1.0 : 0.0;
            break;
        case INTR_LT:
            if (!check_additive_unit_compat(mem, tab, "<", 1, a, b, NULL)) return val_void();
            res = (da <  db) ? 1.0 : 0.0;
            break;
        case INTR_LE:
            if (!check_additive_unit_compat(mem, tab, "<=", 2, a, b, NULL)) return val_void();
            res = (da <= db) ? 1.0 : 0.0;
            break;
        case INTR_GT:
            if (!check_additive_unit_compat(mem, tab, ">", 1, a, b, NULL)) return val_void();
            res = (da >  db) ? 1.0 : 0.0;
            break;
        case INTR_GE:
            if (!check_additive_unit_compat(mem, tab, ">=", 2, a, b, NULL)) return val_void();
            res = (da >= db) ? 1.0 : 0.0;
            break;
        case INTR_APPROX:
            if (!check_additive_unit_compat(mem, tab, "~", 1, a, b, NULL)) return val_void();
            res = (fabs(da - db) <= 0.001 + 1e-12) ? 1.0 : 0.0;
            break;
        case INTR_LAND:  res = (fabs(da) > 1e-15 && fabs(db) > 1e-15) ? 1.0 : 0.0; break;
        case INTR_LOR:   res = (fabs(da) > 1e-15 || fabs(db) > 1e-15) ? 1.0 : 0.0; break;
        case INTR_CNOT:
        case INTR_LNOT:  res = (fabs(da) <= 1e-15) ? 1.0 : 0.0; break;
        case INTR_LXOR:  res = ((fabs(da) > 1e-15) != (fabs(db) > 1e-15)) ? 1.0 : 0.0; break;
        case INTR_LNOR:  res = (fabs(da) <= 1e-15 && fabs(db) <= 1e-15) ? 1.0 : 0.0; break;
        case INTR_LNAND: res = !(fabs(da) > 1e-15 && fabs(db) > 1e-15) ? 1.0 : 0.0; break;
        case INTR_RAW:   ruid = ua; res = da; break;
        default:         mem_err(mem, "Unhandled intrinsic %d", (int)intr); return val_void();
    }
    return val_make_f64_unit(mem, rtid, ruid, res);
}

/* ============================================================================
 * 4. TAGGED SCOPE (`Tag : { ... }`) & SEQUENCE JUMP/BREAK RESOLUTION
 * ============================================================================ */

static void scan_all_labels_in_tree(MemArena *mem, uint16_t node_idx) {
    if (node_idx == MEM_NO_IDX) return;
    const AstNode *n = mem_get_ast(mem, node_idx);
    if (n->kind == AST_TAGGED_SCOPE && n->sub < MEM_MAX_LABELS) {
        mem->label_ast[n->sub] = n->left;
    }
    if (n->kind != AST_LIT && n->kind != AST_CONST_SYM &&
        n->kind != AST_SLOT_LOAD && n->kind != AST_JUMP &&
        n->kind != AST_RUN_TAG && n->kind != AST_BREAK) {
        scan_all_labels_in_tree(mem, n->left);
        if (n->kind != AST_FIELD_LOAD) {
            scan_all_labels_in_tree(mem, n->right);
        }
    }
}

static int stmt_direct_label_id(const MemArena *mem, const SymTable *tab, uint16_t stmt_idx) {
    if (stmt_idx == MEM_NO_IDX) return -1;
    const AstNode *n = mem_get_ast(mem, stmt_idx);
    if (n->kind == AST_TAGGED_SCOPE) {
        return (int)n->sub;
    }
    if (n->kind == AST_BINARY &&
        sym_active_meta_tag(tab, n->ref, (OpFixity)n->sub) == INTR_STORE &&
        n->right != MEM_NO_IDX) {
        const AstNode *rhs = mem_get_ast(mem, n->right);
        if (rhs->kind == AST_TAGGED_SCOPE) {
            return (int)rhs->sub;
        }
    }
    return -1;
}

static void collect_seq_stmts(const MemArena *mem, const SymTable *tab,
                              uint16_t node_idx, uint16_t *stmts, uint16_t *count, uint16_t cap) {
    if (node_idx == MEM_NO_IDX || *count >= cap) return;
    const AstNode *n = mem_get_ast(mem, node_idx);
    if (n->kind == AST_BINARY &&
        sym_active_meta_tag(tab, n->ref, (OpFixity)n->sub) == INTR_SEQ) {
        collect_seq_stmts(mem, tab, n->left, stmts, count, cap);
        collect_seq_stmts(mem, tab, n->right, stmts, count, cap);
        return;
    }
    stmts[(*count)++] = node_idx;
}

static Value eval_seq_chain(MemArena *mem, const SymTable *tab,
                            uint16_t seq_root, uint16_t frame_base, uint8_t cur_ret_type) {
    uint16_t stmts[64];
    uint16_t count = 0;
    collect_seq_stmts(mem, tab, seq_root, stmts, &count, 64);

    uint16_t stmt_for_label[MEM_MAX_LABELS];
    for (uint8_t i = 0; i < MEM_MAX_LABELS; ++i) {
        stmt_for_label[i] = MEM_NO_IDX;
    }

    for (uint16_t i = 0; i < count; ++i) {
        scan_all_labels_in_tree(mem, stmts[i]);
        int lid = stmt_direct_label_id(mem, tab, stmts[i]);
        if (lid >= 0 && lid < MEM_MAX_LABELS) {
            stmt_for_label[lid] = i;
        }
    }

    Value last = val_void();
    uint16_t pc = 0;
    uint32_t guard = 0;

    while (pc < count && !mem->has_error) {
        last = eval_ast(mem, tab, stmts[pc], frame_base, cur_ret_type);
        if (mem->has_error) return val_void();

        if (mem->jump_active == JUMP_GOTO) {
            uint8_t target = mem->jump_target_id;
            if (target < MEM_MAX_LABELS && stmt_for_label[target] != MEM_NO_IDX) {
                mem->jump_active = JUMP_NONE;
                pc = stmt_for_label[target];
                if (++guard > 1000000u) {
                    mem_err(mem, "Jump loop iteration limit exceeded");
                    return val_void();
                }
                continue;
            }
            return val_void();
        }
        if (mem->jump_active != JUMP_NONE) {
            return last;
        }
        pc++;
    }
    return last;
}

/* ============================================================================
 * 5. MULTIPLE DISPATCH (ENTITY TYPES + UNIT TYPES + GENERIC `T`)
 * ============================================================================ */

static bool is_additive_or_rel_sym(const MemArena *mem, const SymTable *tab,
                                   uint16_t sym_id, OpFixity fixity, bool *out_is_rel) {
    if (out_is_rel) *out_is_rel = false;
    if (fixity != FIX_INFIX_LEFT && fixity != FIX_INFIX_RIGHT) return false;
    StrSlice s = tab->syms[sym_id];
    const char *p = mem_str_ptr(mem, s);
    if ((s.len == 1 && (p[0] == '+' || p[0] == '-')) ||
        (s.len == 4 && memcmp(p, "plus", 4) == 0) ||
        (s.len == 5 && memcmp(p, "minus", 5) == 0)) {
        return true;
    }
    if ((s.len == 1 && (p[0] == '<' || p[0] == '>' || p[0] == '~')) ||
        (s.len == 2 && (memcmp(p, "==", 2) == 0 || memcmp(p, "!=", 2) == 0 ||
                        memcmp(p, "<=", 2) == 0 || memcmp(p, ">=", 2) == 0))) {
        if (out_is_rel) *out_is_rel = true;
        return true;
    }
    return false;
}

static Value dispatch_overload(MemArena *mem, const SymTable *tab,
                               uint16_t sym_id, OpFixity fixity,
                               uint8_t argc, const Value *args) {
    uint8_t arg_types[8] = { 0 };
    uint8_t arg_units[8] = { 0 };
    for (uint8_t i = 0; i < argc && i < 8; ++i) {
        arg_types[i] = (uint8_t)val_get_type(mem, args[i]);
        arg_units[i] = val_get_unit(mem, args[i]);
    }

    const OpOverload *matched = NULL;
    int     best_score        = -1;
    uint8_t matched_bound_T   = MEM_UNIT_NONE;
    bool    matched_has_T     = false;
    bool    type_only_matched = false;

    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id != sym_id || ov->fixity != (uint8_t)fixity) continue;
        if (ov->param_count != argc) continue;
        if (!sym_mod_enabled(tab, ov->mod_id)) continue;

        bool type_ok = true;
        int  score   = 0;
        for (uint8_t p = 0; p < argc; ++p) {
            if (ov->param_types[p] == arg_types[p]) {
                score += 4;
            } else if (ov->param_types[p] == SYM_TYPE_ANY) {
                score += 1;
            } else {
                type_ok = false;
                break;
            }
        }
        if (!type_ok) continue;
        type_only_matched = true;

        bool    unit_ok     = true;
        bool    has_bound_T = false;
        uint8_t bound_T     = MEM_UNIT_NONE;

        for (uint8_t p = 0; p < argc; ++p) {
            uint8_t pu = ov->param_units[p];
            uint8_t au = arg_units[p];
            if (pu == SYM_UNIT_OMITTED || pu == 0) {
                score += 1;
            } else if (pu == SYM_UNIT_GENERIC) {
                score += 2;
                if (!has_bound_T) {
                    has_bound_T = true;
                    bound_T     = au;
                } else if (au != bound_T) {
                    if (is_zero_unitless_scalar(mem, tab, args[p])) {
                        /* universal 0 compatible with bound_T */
                    } else if (bound_T == MEM_UNIT_NONE && p > 0 &&
                               is_zero_unitless_scalar(mem, tab, args[0])) {
                        bound_T = au;
                    } else {
                        unit_ok = false;
                        break;
                    }
                }
            } else if (pu == SYM_UNIT_LOCK_NONE) {
                if (au != MEM_UNIT_NONE) { unit_ok = false; break; }
                score += 8;
            } else {
                if (au != pu) { unit_ok = false; break; }
                score += 8;
            }
        }
        if (!unit_ok) continue;

        if (score >= best_score) {
            best_score      = score;
            matched         = ov;
            matched_has_T   = has_bound_T;
            matched_bound_T = bound_T;
        }
    }

    if (!matched) {
        StrSlice s = tab->syms[sym_id];
        char t1[32] = "", t2[32] = "", u1[32] = "", u2[32] = "";
        if (argc >= 1) {
            sym_type_name(tab, mem, arg_types[0], t1, sizeof(t1));
            sym_unit_name(tab, mem, arg_units[0], u1, sizeof(u1));
        }
        if (argc >= 2) {
            sym_type_name(tab, mem, arg_types[1], t2, sizeof(t2));
            sym_unit_name(tab, mem, arg_units[1], u2, sizeof(u2));
        }
        if (type_only_matched) {
            if (argc == 2) {
                mem_err(mem, "Unit mismatch for '%.*s' with (%s [%s], %s [%s])",
                        (int)s.len, mem_str_ptr(mem, s), t1, u1, t2, u2);
            } else {
                mem_err(mem, "Unit mismatch for '%.*s' with (%s [%s])",
                        (int)s.len, mem_str_ptr(mem, s), t1, u1);
            }
        } else {
            if (argc == 2) {
                mem_err(mem, "No active overload for '%.*s' with types (%s, %s)",
                        (int)s.len, mem_str_ptr(mem, s), t1, t2);
            } else {
                mem_err(mem, "No active overload for '%.*s' with type (%s)",
                        (int)s.len, mem_str_ptr(mem, s), t1);
            }
        }
        return val_void();
    }

    /* Enforce additive/relational unit compatibility across all types (`real`, `iota`, `complex`, `vec2`) */
    bool is_rel_op      = false;
    bool has_common_u   = false;
    uint8_t common_unit = MEM_UNIT_NONE;
    if (argc == 2 &&
        (matched->param_units[0] == SYM_UNIT_OMITTED || matched->param_units[0] == SYM_UNIT_GENERIC || matched->param_units[0] == 0) &&
        (matched->param_units[1] == SYM_UNIT_OMITTED || matched->param_units[1] == SYM_UNIT_GENERIC || matched->param_units[1] == 0) &&
        is_additive_or_rel_sym(mem, tab, sym_id, fixity, &is_rel_op)) {
        StrSlice s = tab->syms[sym_id];
        if (!check_additive_unit_compat(mem, tab, mem_str_ptr(mem, s), (int)s.len,
                                        args[0], args[1], &common_unit)) {
            return val_void();
        }
        has_common_u = !is_rel_op;
    }

    uint16_t frame_base = mem_push_frame(mem, matched->local_count);
    if (mem->has_error) return val_void();

    for (uint8_t i = 0; i < matched->local_count; ++i) {
        if (i < argc) {
            uint16_t tid = val_get_type(mem, args[i]);
            if (tid != 0 && tid < tab->type_count &&
                !tab->types[tid].is_consuming && tid != mem->vec_type_id && tid != mem->mat_type_id) {
                mem_set_slot(mem, frame_base, i, val_clone_comp(mem, args[i], tab->types[tid].flat_size));
            } else {
                mem_set_slot(mem, frame_base, i, args[i]);
            }
            if (matched->param_types[i] != SYM_TYPE_ANY && matched->param_types[i] != 0) {
                mem->slot_locked_type[frame_base + i] = matched->param_types[i];
            }
            if (matched->param_units[i] != SYM_UNIT_OMITTED &&
                matched->param_units[i] != SYM_UNIT_GENERIC &&
                matched->param_units[i] != 0) {
                mem->slot_locked_unit[frame_base + i] = matched->param_units[i];
            }
        } else {
            mem_set_slot(mem, frame_base, i, val_make_f64(mem, mem->fast_real_type, 0.0));
        }
    }

    uint16_t saved_labels[MEM_MAX_LABELS];
    memcpy(saved_labels, mem->label_ast, sizeof(saved_labels));
    for (uint8_t i = 0; i < MEM_MAX_LABELS; ++i) {
        mem->label_ast[i] = MEM_NO_IDX;
    }
    scan_all_labels_in_tree(mem, matched->body_ast);

    Value result = eval_ast(mem, tab, matched->body_ast, frame_base, matched->ret_type);
    if (mem->jump_active != JUMP_NONE && !mem->has_error) {
        mem->jump_active = JUMP_NONE;
        mem_err(mem, "Unhandled jump or break outside active loop/scope");
    }

    memcpy(mem->label_ast, saved_labels, sizeof(saved_labels));
    mem_pop_frame(mem, frame_base);
    if (mem->has_error) return val_void();

    if (matched->ret_unit == SYM_UNIT_LOCK_NONE) {
        result = val_with_unit(mem, result, MEM_UNIT_NONE);
    } else if (matched->ret_unit == SYM_UNIT_GENERIC) {
        if (matched_has_T) {
            result = val_with_unit(mem, result, matched_bound_T);
        } else if (has_common_u) {
            result = val_with_unit(mem, result, common_unit);
        }
    } else if (matched->ret_unit != SYM_UNIT_OMITTED && matched->ret_unit != 0) {
        result = val_with_unit(mem, result, matched->ret_unit);
    } else if (has_common_u && val_get_unit(mem, result) == MEM_UNIT_NONE) {
        result = val_with_unit(mem, result, common_unit);
    }

    return result;
}

/* ============================================================================
 * 6. RECURSIVE AST EVALUATOR
 * ============================================================================ */


static Value eval_index_op(MemArena *mem, const SymTable *tab,
                           const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type,
                           bool is_write, Value rhs) {
    Value base = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();

    uint8_t argc = n->sub;
    int idx_k[4] = { 0, 0, 0, 0 };
    uint16_t cur = n->right;
    for (uint8_t i = 0; i < argc && cur != MEM_NO_IDX; ++i) {
        const AstNode *link = mem_get_ast(mem, cur);
        Value iv = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
        idx_k[i] = (int)val_get_f64(mem, iv);
        cur = link->right;
    }

    uint16_t base_tid  = val_get_type(mem, base);
    uint8_t  base_unit = val_get_unit(mem, base);

    /* 1. Multidimensional Matrix (`$mat`) Indexing & Slicing (`M[r, c]`, `M[r][c]`, `T[i, j, k]`) */
    if (base_tid == mem->mat_type_id && mem->mat_type_id != 0) {
        uint8_t  rank = 0;
        uint16_t tot_len = 0, data_idx = 0, dims[4] = { 0 };
        mat_get_info(mem, base, &rank, &tot_len, &data_idx, dims);

        if (argc == 0 || argc > rank) {
            mem_err(mem, "Matrix subscript has %u indices, but matrix rank is %u",
                    (unsigned)argc, (unsigned)rank);
            return val_void();
        }

        uint16_t prefix_off = 0;
        for (uint8_t a = 0; a < argc; ++a) {
            int dim_a = (int)dims[a];
            int ka    = idx_k[a];
            if (ka < 0) ka += dim_a;
            if (ka < 0 || ka >= dim_a) {
                mem_err(mem, "Matrix index %d out of bounds on axis %u (size=%d)",
                        idx_k[a], (unsigned)a, dim_a);
                return val_void();
            }
            prefix_off = (uint16_t)(prefix_off * (uint16_t)dim_a + (uint16_t)ka);
        }

        uint16_t sub_len = 1;
        for (uint8_t a = argc; a < rank; ++a) {
            sub_len = (uint16_t)(sub_len * dims[a]);
        }
        uint16_t target_data_idx = (uint16_t)(data_idx + prefix_off * sub_len);

        if (argc == rank) {
            if (!is_write) {
                return val_make_f64_unit(mem, mem->fast_real_type, base_unit,
                                         mem_read_pool(mem, target_data_idx));
            } else {
                mem_write_pool(mem, target_data_idx, val_get_f64(mem, rhs));
                return rhs;
            }
        } else {
            uint8_t sub_rank = (uint8_t)(rank - argc);
            if (!is_write) {
                uint16_t view_hdr = mem_alloc_pool(mem, MEM_MAT_HDR_SLOTS);
                if (mem->has_error) return val_void();
                mem_write_pool(mem, view_hdr + 0, (double)sub_rank);
                mem_write_pool(mem, view_hdr + 1, (double)sub_len);
                mem_write_pool(mem, view_hdr + 2, (double)target_data_idx);
                mem_write_pool(mem, view_hdr + 3, (double)mem->fast_real_type);
                for (uint8_t i = 0; i < 4; ++i) {
                    mem_write_pool(mem, view_hdr + 4 + i,
                                   (i < sub_rank) ? (double)dims[argc + i] : 0.0);
                }
                return val_with_unit(mem,
                                     val_make_comp(mem, mem->mat_type_id, view_hdr, MEM_MAT_HDR_SLOTS),
                                     base_unit);
            } else {
                uint8_t  rhs_rank = 0;
                uint16_t rhs_tot = 0, rhs_dat = 0, rhs_dims[4] = { 0 };
                if (!mat_get_info(mem, rhs, &rhs_rank, &rhs_tot, &rhs_dat, rhs_dims) ||
                    rhs_rank != sub_rank || rhs_tot != sub_len) {
                    mem_err(mem, "Sub-matrix assignment shape mismatch");
                    return val_void();
                }
                for (uint16_t i = 0; i < sub_len; ++i) {
                    mem_write_pool(mem, target_data_idx + i, mem_read_pool(mem, rhs_dat + i));
                }
                return rhs;
            }
        }
    }

    /* 2. Dynamic Vector (`$vec`) Indexing (`v[i]`, `v[-1]`, `pts[i, j]`) */
    if (base_tid == mem->vec_type_id && mem->vec_type_id != 0) {
        uint16_t hdr_idx  = val_get_pool_idx(base);
        uint16_t raw_hdr0 = (uint16_t)mem_read_pool(mem, hdr_idx + 0);
        uint16_t elem_tid = (uint16_t)(raw_hdr0 & 0xFFu);
        uint8_t  elem_uid = (uint8_t)((raw_hdr0 >> 8) & 0xFFu);
        uint16_t len      = (uint16_t)mem_read_pool(mem, hdr_idx + 1);
        uint16_t data_idx = (uint16_t)mem_read_pool(mem, hdr_idx + 3);
        uint16_t stride   = (elem_tid > 0 && elem_tid < tab->type_count)
                            ? tab->types[elem_tid].flat_size : 1;

        int k = idx_k[0];
        if (k < 0) k += (int)len;
        if (k < 0 || k >= (int)len) {
            mem_err(mem, "Vector index %d out of bounds (len=%u)", idx_k[0], (unsigned)len);
            return val_void();
        }
        uint16_t elem_pidx = (uint16_t)(data_idx + (uint16_t)k * stride);

        if (argc == 1) {
            if (!is_write) {
                return vec_read_elem(mem, tab, elem_tid, elem_uid, elem_pidx);
            } else {
                uint16_t item_tid = val_get_type(mem, rhs);
                uint8_t  item_uid = val_get_unit(mem, rhs);
                if (elem_tid != 0 && (elem_tid != item_tid || elem_uid != item_uid)) {
                    char exp_t[32], got_t[32];
                    sym_type_name(tab, mem, elem_tid, exp_t, sizeof(exp_t));
                    sym_type_name(tab, mem, item_tid, got_t, sizeof(got_t));
                    mem_err(mem, "Vector type mismatch: cannot set '%s' in vec<%s>", got_t, exp_t);
                    return val_void();
                }
                vec_write_elem(mem, tab, elem_tid, elem_pidx, rhs);
                return rhs;
            }
        }
        /* Multi-index `pts[i, j]` where `pts` is `vec<struct>` */
        base_tid  = elem_tid;
        base_unit = elem_uid;
        uint16_t cur_pidx = elem_pidx;
        uint16_t cur_tid  = base_tid;
        for (uint8_t a = 1; a < argc; ++a) {
            if (cur_tid == 0 || cur_tid >= tab->type_count ||
                tab->types[cur_tid].is_consuming || tab->types[cur_tid].field_count == 0) {
                mem_err(mem, "Cannot subscript scalar vector element");
                return val_void();
            }
            const TypeDef *td = &tab->types[cur_tid];
            int fk = idx_k[a];
            if (fk < 0) fk += (int)td->field_count;
            if (fk < 0 || fk >= (int)td->field_count) {
                mem_err(mem, "Struct/vector field index %d out of bounds (fields=%u)",
                        idx_k[a], (unsigned)td->field_count);
                return val_void();
            }
            const FieldDef *fd = &tab->fields[td->start_idx + (uint16_t)fk];
            cur_pidx = (uint16_t)(cur_pidx + fd->flat_offset);
            cur_tid  = fd->type_id;
        }
        if (!is_write) {
            if (!tab->types[cur_tid].is_consuming) {
                return val_with_unit(mem, val_make_comp(mem, cur_tid, cur_pidx, tab->types[cur_tid].flat_size),
                                     base_unit);
            }
            return val_make_f64_unit(mem, cur_tid ? cur_tid : mem->fast_real_type, base_unit,
                                     mem_read_pool(mem, cur_pidx));
        } else {
            mem_write_pool(mem, cur_pidx, val_get_f64(mem, rhs));
            return rhs;
        }
    }

    /* 3. Fixed-Size Vector or Flat C-Style Struct (`$vec2`, `$vec3`, `$complex`, `$circle`, `$mat2`) */
    if (base_tid > 0 && base_tid < tab->type_count && !tab->types[base_tid].is_consuming) {
        uint16_t cur_pidx = val_get_pool_idx(base);
        uint16_t cur_tid  = base_tid;
        for (uint8_t a = 0; a < argc; ++a) {
            if (cur_tid == 0 || cur_tid >= tab->type_count ||
                tab->types[cur_tid].is_consuming || tab->types[cur_tid].field_count == 0) {
                mem_err(mem, "Cannot subscript scalar field with '[...]'");
                return val_void();
            }
            const TypeDef *td = &tab->types[cur_tid];
            int fk = idx_k[a];
            if (fk < 0) fk += (int)td->field_count;
            if (fk < 0 || fk >= (int)td->field_count) {
                mem_err(mem, "Vector/struct index %d out of bounds (fields=%u)",
                        idx_k[a], (unsigned)td->field_count);
                return val_void();
            }
            const FieldDef *fd = &tab->fields[td->start_idx + (uint16_t)fk];
            cur_pidx = (uint16_t)(cur_pidx + fd->flat_offset);
            cur_tid  = fd->type_id;
        }

        if (!is_write) {
            if (cur_tid != 0 && cur_tid < tab->type_count && !tab->types[cur_tid].is_consuming) {
                return val_with_unit(mem,
                                     val_make_comp(mem, cur_tid, cur_pidx, tab->types[cur_tid].flat_size),
                                     base_unit);
            }
            return val_make_f64_unit(mem, cur_tid ? cur_tid : mem->fast_real_type, base_unit,
                                     mem_read_pool(mem, cur_pidx));
        } else {
            if (cur_tid != 0 && cur_tid < tab->type_count && !tab->types[cur_tid].is_consuming) {
                uint16_t fsize   = tab->types[cur_tid].flat_size;
                uint16_t src_idx = val_get_pool_idx(rhs);
                for (uint16_t i = 0; i < fsize; ++i) {
                    mem_write_pool(mem, cur_pidx + i, mem_read_pool(mem, src_idx + i));
                }
            } else {
                mem_write_pool(mem, cur_pidx, val_get_f64(mem, rhs));
            }
            return rhs;
        }
    }

    mem_err(mem, "Cannot subscript non-indexable type with '[...]'");
    return val_void();
}

static Value eval_mat_lit_ast(MemArena *mem, const SymTable *tab,
                              const AstNode *n, uint16_t frame_base, uint8_t cur_ret_type) {
    if (mem->mat_type_id == 0) {
        mem_err(mem, "Multidimensional matrix type ($mat) is not loaded");
        return val_void();
    }
    uint8_t count = n->sub;
    if (count == 0) {
        uint16_t dims[4] = { 0, 0, 0, 0 };
        uint16_t hdr_idx = mem_alloc_pool(mem, MEM_MAT_HDR_SLOTS);
        if (mem->has_error) return val_void();
        for (uint8_t i = 0; i < MEM_MAT_HDR_SLOTS; ++i) mem_write_pool(mem, hdr_idx + i, 0.0);
        mem_write_pool(mem, hdr_idx + 0, 1.0);
        (void)dims;
        return val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS);
    }

    Value items[64];
    memset(items, 0, sizeof(items));
    uint8_t actual = 0;
    uint16_t cur = n->left;
    while (cur != MEM_NO_IDX && actual < 64) {
        const AstNode *link = mem_get_ast(mem, cur);
        items[actual] = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
        actual++;
        cur = link->right;
    }
    if (actual == 0) return val_void();

    uint16_t first_tid = val_get_type(mem, items[0]);
    uint8_t  common_u  = val_get_unit(mem, items[0]);

    /* Case 1: Children are `$mat` sub-tensors -> build (sub_rank + 1)D matrix */
    if (first_tid == mem->mat_type_id) {
        uint8_t  sub_rank = 0;
        uint16_t sub_tot = 0, sub_dat0 = 0, sub_dims[4] = { 0 };
        mat_get_info(mem, items[0], &sub_rank, &sub_tot, &sub_dat0, sub_dims);
        if (sub_rank >= 4) {
            mem_err(mem, "Maximum matrix rank (4D) exceeded");
            return val_void();
        }
        for (uint8_t i = 1; i < actual; ++i) {
            uint8_t  r_i = 0;
            uint16_t t_i = 0, d_i = 0, dims_i[4] = { 0 };
            if (!mat_get_info(mem, items[i], &r_i, &t_i, &d_i, dims_i) || r_i != sub_rank) {
                mem_err(mem, "Matrix rows/sub-tensors must have matching dimensions");
                return val_void();
            }
            for (uint8_t k = 0; k < sub_rank; ++k) {
                if (dims_i[k] != sub_dims[k]) {
                    mem_err(mem, "Matrix rows/sub-tensors must have matching dimensions");
                    return val_void();
                }
            }
        }
        uint16_t dims[4] = { actual, 0, 0, 0 };
        for (uint8_t k = 0; k < sub_rank && k < 3; ++k) {
            dims[k + 1] = sub_dims[k];
        }
        uint16_t tot_len = 0, data_idx = 0;
        uint16_t hdr_idx = mem_mat_alloc_hdr(mem, (uint8_t)(sub_rank + 1), dims, &tot_len, &data_idx);
        if (mem->has_error) return val_void();

        for (uint8_t i = 0; i < actual; ++i) {
            uint16_t d_i = 0;
            uint16_t dummy_dims[4];
            mat_get_info(mem, items[i], NULL, NULL, &d_i, dummy_dims);
            for (uint16_t k = 0; k < sub_tot; ++k) {
                mem_write_pool(mem, (uint16_t)(data_idx + i * sub_tot + k),
                               mem_read_pool(mem, (uint16_t)(d_i + k)));
            }
        }
        return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS), common_u);
    }

    /* Case 2: Children are scalar numbers -> build 1D `$mat` of shape `[actual]` */
    for (uint8_t i = 0; i < actual; ++i) {
        uint16_t tid = val_get_type(mem, items[i]);
        if (tid == 0 || tid >= tab->type_count || !tab->types[tid].is_consuming || tid == mem->str_type_id) {
            mem_err(mem, "Matrix elements must be numeric scalars or sub-matrices");
            return val_void();
        }
    }
    uint16_t dims[4] = { actual, 0, 0, 0 };
    uint16_t tot_len = 0, data_idx = 0;
    uint16_t hdr_idx = mem_mat_alloc_hdr(mem, 1, dims, &tot_len, &data_idx);
    if (mem->has_error) return val_void();
    for (uint8_t i = 0; i < actual; ++i) {
        mem_write_pool(mem, data_idx + i, val_get_f64(mem, items[i]));
    }
    return val_with_unit(mem, val_make_comp(mem, mem->mat_type_id, hdr_idx, MEM_MAT_HDR_SLOTS), common_u);
}


static bool is_boolean_condition_ast(const MemArena *mem, const SymTable *tab, uint16_t idx) {
    if (idx == MEM_NO_IDX) return false;
    const AstNode *node = mem_get_ast(mem, idx);

    if (node->kind == AST_SWITCH_EQ) return true;

    if (node->kind == AST_UNARY && node->sub == (uint8_t)FIX_PREFIX) {
        if (sym_active_meta_tag(tab, node->ref, FIX_PREFIX) == INTR_CNOT) {
            return true;
        }
        for (uint16_t i = 0; i < tab->overload_count; ++i) {
            const OpOverload *ov = &tab->overloads[i];
            if (ov->sym_id == node->ref && ov->fixity == (uint8_t)FIX_PREFIX &&
                ov->param_count == 1 && sym_mod_enabled(tab, ov->mod_id) &&
                ov->body_ast != MEM_NO_IDX) {
                const AstNode *broot = mem_get_ast(mem, ov->body_ast);
                if (broot->kind == AST_INTRINSIC &&
                    (broot->sub == INTR_LNOT || broot->sub == INTR_CNOT)) {
                    return true;
                }
            }
        }
        return false;
    }

    if (node->kind == AST_BINARY) {
        uint8_t m = sym_active_meta_tag(tab, node->ref, (OpFixity)node->sub);
        if (m == INTR_CAND || m == INTR_COR) return true;
        for (uint16_t i = 0; i < tab->overload_count; ++i) {
            const OpOverload *ov = &tab->overloads[i];
            if (ov->sym_id == node->ref && ov->param_count == 2 &&
                sym_mod_enabled(tab, ov->mod_id) && ov->body_ast != MEM_NO_IDX) {
                const AstNode *broot = mem_get_ast(mem, ov->body_ast);
                if (broot->kind == AST_INTRINSIC &&
                    broot->sub >= INTR_EQ && broot->sub <= INTR_LNAND) {
                    return true;
                }
                if (broot->kind == AST_BINARY &&
                    is_boolean_condition_ast(mem, tab, ov->body_ast)) {
                    return true;
                }
            }
        }
        return false;
    }

    if (node->kind == AST_CALL && node->ref < tab->sym_count) {
        StrSlice s = tab->syms[node->ref];
        const char *sp = mem_str_ptr(mem, s);
        if (s.len >= 3 && memcmp(sp, "is_", 3) == 0) {
            return true;
        }
    }

    return false;
}

static double eval_calc_point(MemArena *mem, const SymTable *tab,
                              uint8_t var_slot, uint16_t expr_ast,
                              uint16_t frame_base, uint8_t cur_ret_type, double x_val) {
    mem_set_slot(mem, frame_base, var_slot, val_make_f64(mem, mem->fast_real_type, x_val));
    Value v = eval_ast(mem, tab, expr_ast, frame_base, cur_ret_type);
    return val_get_f64(mem, v);
}

static double snap_calc_noise(double res, double tol) {
    double nearest = floor(res + 0.5);
    if (fabs(res - nearest) < tol) return nearest;
    if (fabs(res) < tol * 0.1) return 0.0;
    return res;
}

static Value eval_calculus_meta(MemArena *mem, const SymTable *tab,
                                uint8_t mtag, const AstNode *n,
                                uint16_t frame_base, uint8_t cur_ret_type) {
    uint16_t arg_asts[6];
    uint8_t  argc = 0;
    uint16_t cur  = n->left;
    while (cur != MEM_NO_IDX && argc < 6) {
        const AstNode *link = mem_get_ast(mem, cur);
        arg_asts[argc++] = link->left;
        cur = link->right;
    }
    if (argc < 3) {
        mem_err(mem, "Calculus construct requires at least 3 arguments");
        return val_void();
    }

    const AstNode *var_node = mem_get_ast(mem, arg_asts[0]);
    if (var_node->kind != AST_SLOT_LOAD) {
        mem_err(mem, "First argument to calculus construct must be a variable name");
        return val_void();
    }
    uint8_t  var_slot = var_node->sub;
    uint16_t expr_ast = arg_asts[1];
    Value    saved_v  = mem_get_slot(mem, frame_base, var_slot);
    double   res      = 0.0;

    if (mtag == INTR_CALC_DIFF || mtag == INTR_CALC_DIFF2 || mtag == INTR_CALC_DIFFN) {
        Value v_x0 = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double x0 = val_get_f64(mem, v_x0);
        int order = 1;
        if (mtag == INTR_CALC_DIFF2) {
            order = 2;
        } else if (mtag == INTR_CALC_DIFFN && argc >= 4) {
            Value v_ord = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                return val_void();
            }
            order = (int)val_get_f64(mem, v_ord);
        }

        double scale = (fabs(x0) > 1.0) ? fabs(x0) : 1.0;
        if (order <= 0) {
            res = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0);
        } else if (order == 1) {
            double h   = 1e-5 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (-fp2 + 8.0 * fp1 - 8.0 * fm1 + fm2) / (12.0 * h);
            res = snap_calc_noise(res, 1e-9);
        } else if (order == 2) {
            double h   = 1e-4 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double f0  = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (-fp2 + 16.0 * fp1 - 30.0 * f0 + 16.0 * fm1 - fm2) / (12.0 * h * h);
            res = snap_calc_noise(res, 1e-7);
        } else if (order == 3) {
            double h   = 1e-3 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (fp2 - 2.0 * fp1 + 2.0 * fm1 - fm2) / (2.0 * h * h * h);
            res = snap_calc_noise(res, 1e-5);
        } else {
            double h   = 2e-3 * scale;
            double fp2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + 2.0 * h);
            double fp1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h);
            double f0  = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0);
            double fm1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h);
            double fm2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - 2.0 * h);
            res = (fp2 - 4.0 * fp1 + 6.0 * f0 - 4.0 * fm1 + fm2) / (h * h * h * h);
            res = snap_calc_noise(res, 1e-4);
        }
    } else if (mtag == INTR_CALC_INTEG) {
        if (argc < 4) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            mem_err(mem, "integrate(x, expr, a, b) requires lower and upper bounds");
            return val_void();
        }
        Value va = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        Value vb = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double a = val_get_f64(mem, va);
        double b = val_get_f64(mem, vb);
        int steps = 200;
        if (argc >= 5) {
            Value vn = eval_ast(mem, tab, arg_asts[4], frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                return val_void();
            }
            steps = (int)val_get_f64(mem, vn);
            if (steps < 2) steps = 2;
            if (steps % 2 != 0) steps++;
            if (steps > 10000) steps = 10000;
        }
        double h   = (b - a) / (double)steps;
        double sum = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, a)
                   + eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, b);
        for (int k = 1; k < steps && !mem->has_error; ++k) {
            double xk = a + (double)k * h;
            double fk = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, xk);
            sum += ((k % 2 == 1) ? 4.0 : 2.0) * fk;
        }
        res = (h / 3.0) * sum;
        res = snap_calc_noise(res, 1e-9);
    } else if (mtag == INTR_CALC_LIM || mtag == INTR_CALC_LIM_L || mtag == INTR_CALC_LIM_R) {
        Value v_x0 = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        double x0    = val_get_f64(mem, v_x0);
        double scale = (fabs(x0) > 1.0) ? fabs(x0) : 1.0;
        double h1    = 1e-6 * scale;
        double h2    = 1e-7 * scale;

        if (mtag == INTR_CALC_LIM_R) {
            double r1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h1);
            double r2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h2);
            res = snap_calc_noise((10.0 * r2 - r1) / 9.0, 1e-8);
        } else if (mtag == INTR_CALC_LIM_L) {
            double l1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h1);
            double l2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h2);
            res = snap_calc_noise((10.0 * l2 - l1) / 9.0, 1e-8);
        } else {
            double r1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h1);
            double r2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 + h2);
            double l1 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h1);
            double l2 = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, x0 - h2);
            double lim_r = (10.0 * r2 - r1) / 9.0;
            double lim_l = (10.0 * l2 - l1) / 9.0;
            double tol   = 1e-4 * (1.0 + fabs(lim_r) + fabs(lim_l));
            if (fabs(lim_r - lim_l) > tol) {
                mem_set_slot(mem, frame_base, var_slot, saved_v);
                mem_err(mem, "Two-sided limit does not exist (left %.6g != right %.6g)", lim_l, lim_r);
                return val_void();
            }
            res = snap_calc_noise(0.5 * (lim_l + lim_r), 1e-8);
        }
    } else if (mtag == INTR_CALC_SIGMA || mtag == INTR_CALC_PROD) {
        if (argc < 4) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            mem_err(mem, "sigma/prod(k, expr, lo, hi) requires lower and upper bounds");
            return val_void();
        }
        Value vlo = eval_ast(mem, tab, arg_asts[2], frame_base, cur_ret_type);
        Value vhi = eval_ast(mem, tab, arg_asts[3], frame_base, cur_ret_type);
        if (mem->has_error || mem->jump_active != JUMP_NONE) {
            mem_set_slot(mem, frame_base, var_slot, saved_v);
            return val_void();
        }
        long long lo = (long long)floor(val_get_f64(mem, vlo) + 0.5);
        long long hi = (long long)floor(val_get_f64(mem, vhi) + 0.5);
        res = (mtag == INTR_CALC_PROD) ? 1.0 : 0.0;
        for (long long k = lo; k <= hi && !mem->has_error; ++k) {
            double vk = eval_calc_point(mem, tab, var_slot, expr_ast, frame_base, cur_ret_type, (double)k);
            if (mtag == INTR_CALC_PROD) res *= vk;
            else                        res += vk;
        }
    }

    mem_set_slot(mem, frame_base, var_slot, saved_v);
    if (mem->has_error) return val_void();
    return val_make_f64(mem, mem->fast_real_type, res);
}

Value eval_ast(MemArena *mem, const SymTable *tab,
               uint16_t node_idx, uint16_t frame_base, uint8_t cur_ret_type) {
    if (mem->has_error || mem->jump_active != JUMP_NONE || node_idx == MEM_NO_IDX) return val_void();
    const AstNode *n = mem_get_ast(mem, node_idx);

    switch ((AstKind)n->kind) {
        case AST_LIT: {
            uint16_t tid = n->sub;
            if (tid == MEM_TYPE_VOID) return val_void();
            if (tid == mem->str_type_id && tid != 0) {
                StrSlice s = { n->left, n->right };
                return val_make_str(mem, tid, s);
            }
            return val_make_f64(mem, tid, mem_read_pool(mem, n->ref));
        }

        case AST_CONST_SYM: {
            uint16_t sym_id = n->ref;
            for (uint8_t i = 0; i < tab->const_count; ++i) {
                if (tab->consts[i].sym_id == sym_id && sym_mod_enabled(tab, tab->consts[i].mod_id)) {
                    return tab->const_vals[tab->consts[i].val_idx];
                }
            }
            StrSlice s = tab->syms[sym_id];
            mem_err(mem, "Constant '%.*s' is disabled or undefined", (int)s.len, mem_str_ptr(mem, s));
            return val_void();
        }

        case AST_SLOT_LOAD:
            return mem_get_slot(mem, frame_base, n->sub);

        case AST_FIELD_LOAD: {
            Value base = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            uint16_t base_pidx = val_get_pool_idx(base);
            uint8_t  base_unit = val_get_unit(mem, base);
            uint16_t ftid      = n->sub;
            uint16_t foff      = n->ref;
            if (ftid != 0 && ftid < tab->type_count && !tab->types[ftid].is_consuming) {
                return val_with_unit(mem, val_make_comp(mem, ftid, base_pidx + foff, tab->types[ftid].flat_size),
                                     base_unit);
            }
            return val_make_f64_unit(mem, ftid ? ftid : mem->fast_real_type, base_unit,
                                     mem_read_pool(mem, base_pidx + foff));
        }

        case AST_UNARY: {
            if (sym_active_meta_tag(tab, n->ref, (OpFixity)n->sub) == INTR_CNOT) {
                if (!is_boolean_condition_ast(mem, tab, n->left)) {
                    mem_err(mem, "Operator '!' requires a boolean condition (e.g. !(x == 5)); not defined for non-boolean values");
                    return val_void();
                }
                Value arg = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                return val_make_f64(mem, mem->fast_real_type,
                                    (fabs(val_get_f64(mem, arg)) <= 1e-15) ? 1.0 : 0.0);
            }
            Value arg = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            return dispatch_overload(mem, tab, n->ref, (OpFixity)n->sub, 1, &arg);
        }

        case AST_BINARY: {
            uint8_t mtag = sym_active_meta_tag(tab, n->ref, (OpFixity)n->sub);

            if (mtag == INTR_SEQ) {
                return eval_seq_chain(mem, tab, node_idx, frame_base, cur_ret_type);
            }

            if (mtag == INTR_STORE) {
                Value rhs = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                const AstNode *lhs = mem_get_ast(mem, n->left);
                if (lhs->kind == AST_SLOT_LOAD) {
                    uint16_t abs_slot = frame_base + lhs->sub;
                    if (lhs->left != 0 && lhs->left != MEM_NO_IDX) {
                        mem->slot_locked_type[abs_slot] = (uint8_t)lhs->left;
                    }
                    if (lhs->right != 0 && lhs->right != MEM_NO_IDX) {
                        mem->slot_locked_unit[abs_slot] = (uint8_t)lhs->right;
                    }

                    uint8_t req_type = mem->slot_locked_type[abs_slot];
                    if (req_type != 0 && req_type != SYM_TYPE_ANY) {
                        uint16_t rhs_type = val_get_type(mem, rhs);
                        if (rhs_type != req_type) {
                            char exp_t[32], got_t[32];
                            sym_type_name(tab, mem, req_type, exp_t, sizeof(exp_t));
                            sym_type_name(tab, mem, rhs_type, got_t, sizeof(got_t));
                            mem_err(mem, "Type mismatch: variable holds concrete type '%s', cannot assign '%s'",
                                    exp_t, got_t);
                            return val_void();
                        }
                    }

                    uint8_t req_unit = mem->slot_locked_unit[abs_slot];
                    if (req_unit != 0 && req_unit != SYM_UNIT_GENERIC && req_unit != SYM_UNIT_OMITTED) {
                        uint8_t expected_u = (req_unit == SYM_UNIT_LOCK_NONE) ? MEM_UNIT_NONE : req_unit;
                        uint8_t rhs_unit   = val_get_unit(mem, rhs);
                        if (rhs_unit != expected_u) {
                            char exp_u[32], got_u[32];
                            sym_unit_name(tab, mem, expected_u, exp_u, sizeof(exp_u));
                            sym_unit_name(tab, mem, rhs_unit,   got_u, sizeof(got_u));
                            mem_err(mem, "Unit mismatch: variable holds concrete unit '%s', cannot assign '%s'",
                                    exp_u, got_u);
                            return val_void();
                        }
                    }

                    mem_set_slot(mem, frame_base, lhs->sub, rhs);
                    return rhs;
                } else if (lhs->kind == AST_INDEX) {
                    return eval_index_op(mem, tab, lhs, frame_base, cur_ret_type, true, rhs);
                } else if (lhs->kind == AST_FIELD_LOAD) {
                    Value target       = (lhs->right != MEM_NO_IDX)
                                         ? mem_get_slot(mem, frame_base, (uint8_t)lhs->right)
                                         : eval_ast(mem, tab, lhs->left, frame_base, cur_ret_type);
                    if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                    uint16_t base_pidx = val_get_pool_idx(target);
                    uint16_t foff      = lhs->ref;
                    uint16_t ftid      = lhs->sub;
                    if (ftid != 0 && ftid < tab->type_count && !tab->types[ftid].is_consuming) {
                        uint16_t fsize   = tab->types[ftid].flat_size;
                        uint16_t src_idx = val_get_pool_idx(rhs);
                        for (uint16_t i = 0; i < fsize; ++i) {
                            mem_write_pool(mem, base_pidx + foff + i, mem_read_pool(mem, src_idx + i));
                        }
                    } else {
                        mem_write_pool(mem, base_pidx + foff, val_get_f64(mem, rhs));
                    }
                    return rhs;
                }
                mem_err(mem, "Left side of assignment operator must be a local variable or struct field");
                return val_void();
            }

            if (mtag == INTR_WHILE) {
                Value last = val_make_f64(mem, mem->fast_real_type, 0.0);
                uint32_t guard = 0;
                while (!mem->has_error) {
                    Value cond = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
                    if (mem->has_error || mem->jump_active != JUMP_NONE ||
                        fabs(val_get_f64(mem, cond)) <= 1e-15) {
                        break;
                    }
                    last = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
                    if (mem->jump_active == JUMP_BREAK_ANON) {
                        mem->jump_active = JUMP_NONE;
                        break;
                    }
                    if (mem->jump_active != JUMP_NONE) {
                        break;
                    }
                    if (++guard > 1000000u) {
                        mem_err(mem, "Loop iteration limit exceeded");
                        break;
                    }
                }
                return last;
            }

            if (mtag == INTR_CAND) {
                Value lv = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, lv)) <= 1e-15) {
                    return val_make_f64(mem, mem->fast_real_type, 0.0);
                }
                Value rv = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                return val_make_f64(mem, mem->fast_real_type,
                                    (fabs(val_get_f64(mem, rv)) > 1e-15) ? 1.0 : 0.0);
            }

            if (mtag == INTR_COR) {
                Value lv = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, lv)) > 1e-15) {
                    return val_make_f64(mem, mem->fast_real_type, 1.0);
                }
                Value rv = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                return val_make_f64(mem, mem->fast_real_type,
                                    (fabs(val_get_f64(mem, rv)) > 1e-15) ? 1.0 : 0.0);
            }

            Value args[2];
            args[0] = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            args[1] = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            return dispatch_overload(mem, tab, n->ref, (OpFixity)n->sub, 2, args);
        }

        case AST_TERNARY: {
            Value c = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            const AstNode *pair = mem_get_ast(mem, n->right);
            if (fabs(val_get_f64(mem, c)) > 1e-15) {
                return eval_ast(mem, tab, pair->left, frame_base, cur_ret_type);
            } else if (pair->right != MEM_NO_IDX) {
                return eval_ast(mem, tab, pair->right, frame_base, cur_ret_type);
            }
            return val_void();
        }

        case AST_SWITCH_EQ: {
            Value a = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            Value b = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            uint16_t ta = val_get_type(mem, a);
            uint16_t tb = val_get_type(mem, b);
            bool eq = false;
            if (ta == mem->str_type_id && tb == mem->str_type_id && mem->str_type_id != 0) {
                eq = mem_slice_eq(mem, val_get_str(mem, a), val_get_str(mem, b));
            } else if (ta == tb && val_get_unit(mem, a) == val_get_unit(mem, b)) {
                eq = (fabs(val_get_f64(mem, a) - val_get_f64(mem, b)) <= 1e-12);
            }
            return val_make_f64(mem, mem->fast_real_type, eq ? 1.0 : 0.0);
        }

        case AST_TAGGED_SCOPE: {
            uint8_t label_id = n->sub;
            if (label_id < MEM_MAX_LABELS) {
                mem->label_ast[label_id] = n->left;
            }
            Value res = val_void();
            uint32_t guard = 0;
            while (!mem->has_error) {
                res = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
                if (mem->jump_active == JUMP_GOTO && mem->jump_target_id == label_id) {
                    mem->jump_active = JUMP_NONE;
                    if (++guard > 1000000u) {
                        mem_err(mem, "Loop iteration limit exceeded in tagged scope");
                        return val_void();
                    }
                    continue;
                }
                if (mem->jump_active == JUMP_BREAK_ANON ||
                    (mem->jump_active == JUMP_BREAK_TAG && mem->jump_target_id == label_id)) {
                    mem->jump_active = JUMP_NONE;
                    break;
                }
                break;
            }
            return res;
        }

        case AST_JUMP: {
            mem->jump_active    = JUMP_GOTO;
            mem->jump_target_id = n->sub;
            return val_void();
        }

        case AST_BREAK: {
            if (n->sub == 0xFFu) {
                mem->jump_active = JUMP_BREAK_ANON;
            } else {
                mem->jump_active    = JUMP_BREAK_TAG;
                mem->jump_target_id = n->sub;
            }
            return val_void();
        }

        case AST_RUN_TAG: {
            uint8_t label_id = n->sub;
            if (label_id >= MEM_MAX_LABELS || mem->label_ast[label_id] == MEM_NO_IDX) {
                mem_err(mem, "Cannot reuse uninitialized or unknown scope tag");
                return val_void();
            }
            uint16_t target_ast = mem->label_ast[label_id];
            Value res = val_void();
            uint32_t guard = 0;
            while (!mem->has_error) {
                res = eval_ast(mem, tab, target_ast, frame_base, cur_ret_type);
                if (mem->jump_active == JUMP_GOTO && mem->jump_target_id == label_id) {
                    mem->jump_active = JUMP_NONE;
                    if (++guard > 1000000u) {
                        mem_err(mem, "Loop iteration limit exceeded in reused tagged scope");
                        return val_void();
                    }
                    continue;
                }
                if (mem->jump_active == JUMP_BREAK_ANON ||
                    (mem->jump_active == JUMP_BREAK_TAG && mem->jump_target_id == label_id)) {
                    mem->jump_active = JUMP_NONE;
                    break;
                }
                break;
            }
            return res;
        }

        case AST_CALL: {
            uint8_t mtag = sym_active_meta_tag(tab, n->ref, FIX_FUNC);
            if (mtag >= INTR_CALC_DIFF && mtag <= INTR_CALC_PROD) {
                return eval_calculus_meta(mem, tab, mtag, n, frame_base, cur_ret_type);
            }
            if (mtag == INTR_IF && n->sub == 3) {
                const AstNode *l1 = mem_get_ast(mem, n->left);
                const AstNode *l2 = mem_get_ast(mem, l1->right);
                const AstNode *l3 = mem_get_ast(mem, l2->right);
                Value c = eval_ast(mem, tab, l1->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                if (fabs(val_get_f64(mem, c)) > 1e-15) {
                    return eval_ast(mem, tab, l2->left, frame_base, cur_ret_type);
                } else {
                    return eval_ast(mem, tab, l3->left, frame_base, cur_ret_type);
                }
            }

            Value args[8];
            uint8_t argc = 0;
            uint16_t cur = n->left;
            while (cur != MEM_NO_IDX && argc < 8) {
                const AstNode *link = mem_get_ast(mem, cur);
                args[argc++] = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                cur = link->right;
            }
            return dispatch_overload(mem, tab, n->ref, FIX_FUNC, argc, args);
        }

        case AST_INTRINSIC: {
            Value a = (n->left  != MEM_NO_IDX) ? eval_ast(mem, tab, n->left,  frame_base, cur_ret_type) : val_void();
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            Value b = (n->right != MEM_NO_IDX) ? eval_ast(mem, tab, n->right, frame_base, cur_ret_type) : val_void();
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            Value c = (n->ref   != MEM_NO_IDX && n->ref != 0)
                      ? eval_ast(mem, tab, n->ref, frame_base, cur_ret_type) : val_void();
            if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
            return eval_intrinsic(mem, tab, (IntrinsicId)n->sub, a, b, c, cur_ret_type);
        }

        case AST_EXTERN: {
            Value args[8];
            uint8_t argc = 0;
            uint16_t cur = n->left;
            while (cur != MEM_NO_IDX && argc < 8) {
                const AstNode *link = mem_get_ast(mem, cur);
                args[argc++] = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();
                cur = link->right;
            }
            const ExternDef *ext = sym_find_extern(tab, mem, n->ref);
            if (!ext || !ext->fn) {
                StrSlice s = tab->syms[n->ref];
                mem_err(mem, "Unregistered host extern '@%.*s'",
                        (int)s.len, mem_str_ptr(mem, s));
                return val_void();
            }
            Value out = val_void();
            if (!ext->fn(mem, tab, args, argc, ext->user_ctx, &out)) {
                if (!mem->has_error) {
                    StrSlice s = tab->syms[n->ref];
                    mem_err(mem, "Host extern '@%.*s' failed",
                            (int)s.len, mem_str_ptr(mem, s));
                }
                return val_void();
            }
            return out;
        }

        case AST_CONSTRUCT: {
            uint16_t st_tid = n->ref;
            const TypeDef *td = &tab->types[st_tid];
            uint16_t pidx = mem_alloc_pool(mem, td->flat_size);
            if (mem->has_error) return val_void();

            uint16_t cur_link = n->left;
            uint16_t slot_cur = 0;
            uint8_t  comp_unit = MEM_UNIT_NONE;
            while (cur_link != MEM_NO_IDX && slot_cur < td->flat_size) {
                const AstNode *link = mem_get_ast(mem, cur_link);
                Value fval = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
                if (mem->has_error || mem->jump_active != JUMP_NONE) return val_void();

                uint8_t fu = val_get_unit(mem, fval);
                if (fu != MEM_UNIT_NONE && comp_unit == MEM_UNIT_NONE) {
                    comp_unit = fu;
                }

                uint16_t fval_tid = val_get_type(mem, fval);
                if (fval_tid != 0 && fval_tid < tab->type_count && !tab->types[fval_tid].is_consuming) {
                    uint16_t sub_sz  = tab->types[fval_tid].flat_size;
                    uint16_t sub_idx = val_get_pool_idx(fval);
                    for (uint16_t k = 0; k < sub_sz && slot_cur < td->flat_size; ++k) {
                        mem_write_pool(mem, pidx + slot_cur++, mem_read_pool(mem, sub_idx + k));
                    }
                } else {
                    mem_write_pool(mem, pidx + slot_cur++, val_get_f64(mem, fval));
                }
                cur_link = link->right;
            }
            return val_with_unit(mem, val_make_comp(mem, st_tid, pidx, td->flat_size), comp_unit);
        }

        case AST_MAT_LIT:
            return eval_mat_lit_ast(mem, tab, n, frame_base, cur_ret_type);

        case AST_INDEX:
            return eval_index_op(mem, tab, n, frame_base, cur_ret_type, false, val_void());

        default:
            mem_err(mem, "Corrupt AST node kind %u", (unsigned)n->kind);
            return val_void();
    }
}
