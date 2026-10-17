/*
 * evaluator.c — Standalone AST Evaluator & Private C Intrinsics Implementation
 */

#include "../Headers/evaluator.h"
#include <math.h>
#include <stddef.h>

static Value eval_intrinsic(MemArena *mem, const SymTable *tab,
                            IntrinsicId intr, Value a, Value b, uint8_t ret_type) {
    uint16_t rtid = (ret_type != 0 && tab->types[ret_type].is_consuming)
                    ? ret_type
                    : mem->fast_real_type;
    double da  = val_get_f64(mem, a);
    double db  = (val_get_type(mem, b) != MEM_TYPE_VOID) ? val_get_f64(mem, b) : 0.0;
    double res = 0.0;

    switch (intr) {
        case INTR_ADD:   res = da + db; break;
        case INTR_SUB:   res = da - db; break;
        case INTR_MUL:   res = da * db; break;
        case INTR_DIV:
            if (db == 0.0) { mem_err(mem, "Division by zero"); return val_void(); }
            res = da / db;
            break;
        case INTR_MOD:   res = fmod(da, db); break;
        case INTR_NEG:   res = -da; break;
        case INTR_ABS:   res = fabs(da); break;
        case INTR_FLOOR: res = floor(da); break;
        case INTR_CEIL:  res = ceil(da); break;
        case INTR_POW:   res = pow(da, db); break;
        case INTR_SQRT:  res = sqrt(da); break;
        case INTR_CBRT:  res = cbrt(da); break;
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
        case INTR_EQ:    res = (fabs(da - db) <= 1e-12) ? 1.0 : 0.0; break;
        case INTR_NE:    res = (fabs(da - db) >  1e-12) ? 1.0 : 0.0; break;
        case INTR_LT:    res = (da <  db) ? 1.0 : 0.0; break;
        case INTR_LE:    res = (da <= db) ? 1.0 : 0.0; break;
        case INTR_GT:    res = (da >  db) ? 1.0 : 0.0; break;
        case INTR_GE:    res = (da >= db) ? 1.0 : 0.0; break;
        case INTR_RAW:   res = da; break;
        default:         mem_err(mem, "Unhandled intrinsic %d", (int)intr); return val_void();
    }
    return val_make_f64(mem, rtid, res);
}

static Value dispatch_overload(MemArena *mem, const SymTable *tab,
                               uint8_t sym_id, OpFixity fixity,
                               uint8_t argc, const Value *args) {
    uint8_t arg_types[4] = { 0, 0, 0, 0 };
    for (uint8_t i = 0; i < argc && i < 4; ++i) {
        arg_types[i] = (uint8_t)val_get_type(mem, args[i]);
    }

    const OpOverload *matched = NULL;
    for (uint16_t i = 0; i < tab->overload_count; ++i) {
        const OpOverload *ov = &tab->overloads[i];
        if (ov->sym_id != sym_id || ov->fixity != (uint8_t)fixity) continue;
        if (ov->param_count != argc) continue;
        if (!sym_mod_enabled(tab, ov->mod_id)) continue;

        bool sig_ok = true;
        for (uint8_t p = 0; p < argc; ++p) {
            if (ov->param_types[p] != arg_types[p]) {
                sig_ok = false;
                break;
            }
        }
        if (sig_ok) {
            matched = ov;
            break;
        }
    }

    if (!matched) {
        StrSlice s = tab->syms[sym_id];
        char t1[32] = "", t2[32] = "";
        if (argc >= 1) sym_type_name(tab, mem, arg_types[0], t1, sizeof(t1));
        if (argc >= 2) sym_type_name(tab, mem, arg_types[1], t2, sizeof(t2));
        if (argc == 2) {
            mem_err(mem, "No active overload for '%.*s' with types (%s, %s)",
                    (int)s.len, mem_str_ptr(mem, s), t1, t2);
        } else {
            mem_err(mem, "No active overload for '%.*s' with type (%s)",
                    (int)s.len, mem_str_ptr(mem, s), t1);
        }
        return val_void();
    }

    uint16_t frame_base = mem_push_frame(mem, matched->local_count);
    if (mem->has_error) return val_void();

    for (uint8_t i = 0; i < matched->local_count; ++i) {
        if (i < argc) {
            uint16_t tid = val_get_type(mem, args[i]);
            if (tid != 0 && tid < tab->type_count && !tab->types[tid].is_consuming) {
                mem_set_slot(mem, frame_base, i, val_clone_comp(mem, args[i], tab->types[tid].flat_size));
            } else {
                mem_set_slot(mem, frame_base, i, args[i]);
            }
        } else {
            mem_set_slot(mem, frame_base, i, val_make_f64(mem, mem->fast_real_type, 0.0));
        }
    }

    Value result = eval_ast(mem, tab, matched->body_ast, frame_base, matched->ret_type);
    mem_pop_frame(mem, frame_base);
    return result;
}

Value eval_ast(MemArena *mem, const SymTable *tab,
               uint16_t node_idx, uint16_t frame_base, uint8_t cur_ret_type) {
    if (mem->has_error || node_idx == MEM_NO_IDX) return val_void();
    const AstNode *n = mem_get_ast(mem, node_idx);

    switch ((AstKind)n->kind) {
        case AST_LIT: {
            uint16_t tid = n->sub;
            if (tid == mem->str_type_id && tid != 0) {
                StrSlice s = { n->left, n->right };
                return val_make_str(mem, tid, s);
            }
            return val_make_f64(mem, tid, mem_read_pool(mem, n->ref));
        }

        case AST_CONST_SYM: {
            uint8_t sym_id = (uint8_t)n->ref;
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
            if (mem->has_error) return val_void();
            uint16_t base_pidx = val_get_pool_idx(base);
            uint16_t ftid      = n->sub;
            uint16_t foff      = n->ref;
            if (ftid != 0 && !tab->types[ftid].is_consuming) {
                return val_make_comp(mem, ftid, base_pidx + foff, tab->types[ftid].flat_size);
            }
            return val_make_f64(mem, ftid ? ftid : mem->fast_real_type, mem_read_pool(mem, base_pidx + foff));
        }

        case AST_UNARY: {
            Value arg = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error) return val_void();
            return dispatch_overload(mem, tab, (uint8_t)n->ref, (OpFixity)n->sub, 1, &arg);
        }

        case AST_BINARY: {
            uint8_t mtag = sym_active_meta_tag(tab, (uint8_t)n->ref, (OpFixity)n->sub);

            if (mtag == INTR_SEQ) {
                (void)eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
                if (mem->has_error) return val_void();
                return eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
            }

            if (mtag == INTR_STORE) {
                Value rhs = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
                if (mem->has_error) return val_void();
                const AstNode *lhs = mem_get_ast(mem, n->left);
                if (lhs->kind == AST_SLOT_LOAD) {
                    mem_set_slot(mem, frame_base, lhs->sub, rhs);
                    return rhs;
                } else if (lhs->kind == AST_FIELD_LOAD && lhs->right != MEM_NO_IDX) {
                    Value target       = mem_get_slot(mem, frame_base, (uint8_t)lhs->right);
                    uint16_t base_pidx = val_get_pool_idx(target);
                    uint16_t foff      = lhs->ref;
                    uint16_t ftid      = lhs->sub;
                    if (ftid != 0 && !tab->types[ftid].is_consuming) {
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
                    if (mem->has_error || fabs(val_get_f64(mem, cond)) <= 1e-15) break;
                    last = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
                    if (++guard > 1000000u) {
                        mem_err(mem, "Loop iteration limit exceeded");
                        break;
                    }
                }
                return last;
            }

            Value args[2];
            args[0] = eval_ast(mem, tab, n->left, frame_base, cur_ret_type);
            if (mem->has_error) return val_void();
            args[1] = eval_ast(mem, tab, n->right, frame_base, cur_ret_type);
            if (mem->has_error) return val_void();
            return dispatch_overload(mem, tab, (uint8_t)n->ref, (OpFixity)n->sub, 2, args);
        }

        case AST_CALL: {
            uint8_t mtag = sym_active_meta_tag(tab, (uint8_t)n->ref, FIX_FUNC);
            if (mtag == INTR_IF && n->sub == 3) {
                const AstNode *l1 = mem_get_ast(mem, n->left);
                const AstNode *l2 = mem_get_ast(mem, l1->right);
                const AstNode *l3 = mem_get_ast(mem, l2->right);
                Value c = eval_ast(mem, tab, l1->left, frame_base, cur_ret_type);
                if (mem->has_error) return val_void();
                if (fabs(val_get_f64(mem, c)) > 1e-15) {
                    return eval_ast(mem, tab, l2->left, frame_base, cur_ret_type);
                } else {
                    return eval_ast(mem, tab, l3->left, frame_base, cur_ret_type);
                }
            }

            Value args[4];
            uint8_t argc = 0;
            uint16_t cur = n->left;
            while (cur != MEM_NO_IDX && argc < 4) {
                const AstNode *link = mem_get_ast(mem, cur);
                args[argc++] = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
                if (mem->has_error) return val_void();
                cur = link->right;
            }
            return dispatch_overload(mem, tab, (uint8_t)n->ref, FIX_FUNC, argc, args);
        }

        case AST_INTRINSIC: {
            Value a = (n->left  != MEM_NO_IDX) ? eval_ast(mem, tab, n->left,  frame_base, cur_ret_type) : val_void();
            if (mem->has_error) return val_void();
            Value b = (n->right != MEM_NO_IDX) ? eval_ast(mem, tab, n->right, frame_base, cur_ret_type) : val_void();
            if (mem->has_error) return val_void();
            return eval_intrinsic(mem, tab, (IntrinsicId)n->sub, a, b, cur_ret_type);
        }

        case AST_CONSTRUCT: {
            uint16_t st_tid = n->ref;
            const TypeDef *td = &tab->types[st_tid];
            uint16_t pidx = mem_alloc_pool(mem, td->flat_size);
            if (mem->has_error) return val_void();

            uint16_t cur_link = n->left;
            uint16_t slot_cur = 0;
            while (cur_link != MEM_NO_IDX && slot_cur < td->flat_size) {
                const AstNode *link = mem_get_ast(mem, cur_link);
                Value fval = eval_ast(mem, tab, link->left, frame_base, cur_ret_type);
                if (mem->has_error) return val_void();

                uint16_t fval_tid = val_get_type(mem, fval);
                if (fval_tid != 0 && !tab->types[fval_tid].is_consuming) {
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
            return val_make_comp(mem, st_tid, pidx, td->flat_size);
        }

        default:
            mem_err(mem, "Corrupt AST node kind %u", (unsigned)n->kind);
            return val_void();
    }
}