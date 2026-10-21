/*
 * linker.c — Topological `<[dep]` Dependency Linker Implementation
 *
 * Implements Phase 2 (Linking) of extension building:
 *   - Verifies that every `<[dep]` dependency is either already loaded in
 *     `SymTable` or provided by a staged unit in `ExtBuildCtx`.
 *   - Runs Kahn's topological sort algorithm to order modules and detect
 *     circular `<[dep]` dependencies.
 *   - Invokes `ext_compile_slice` in resolved topological order with full
 *     transactional rollback on error.
 *   - Zero global or static mutable state across engines or threads.
 */

#include "linker.h"
#include <stdlib.h>
#include <string.h>

bool ext_link_units(MemArena *mem, SymTable *tab, const ExtBuildCtx *ctx) {
    if (!mem || !tab || !ctx) return false;
    mem_clear_err(mem);
    if (ctx->has_error) {
        mem_err(mem, "%s", ctx->err_msg[0] ? ctx->err_msg : "Compilation failed prior to linking");
        return false;
    }
    if (ctx->unit_count == 0) {
        mem_snapshot_perm(mem);
        return true;
    }

    uint8_t n = ctx->unit_count;
    bool    adj[EXT_MAX_BUILD_UNITS][EXT_MAX_BUILD_UNITS];
    uint8_t in_deg[EXT_MAX_BUILD_UNITS];
    memset(adj, 0, sizeof(adj));
    memset(in_deg, 0, sizeof(in_deg));

    /* 1. Verify all `<[dep]` dependencies and construct the dependency DAG */
    for (uint8_t u = 0; u < n; ++u) {
        const ExtCompiledUnit *unit = &ctx->units[u];

        /* Ensure named module has not been loaded into `tab` since compilation */
        if (!unit->is_default_mod) {
            for (uint8_t m = 0; m < tab->mod_count; ++m) {
                if (mem_slice_eq_cstr(mem, tab->mod_names[m], unit->mod_name)) {
                    mem_err(mem, "Module '[%s]' is already loaded", unit->mod_name);
                    return false;
                }
            }
        }

        for (uint8_t d = 0; d < unit->dep_count; ++d) {
            const char *dep_name = unit->deps[d];

            if (!unit->is_default_mod && strcmp(dep_name, unit->mod_name) == 0) {
                mem_err(mem, "Circular module dependency involving '[%s]'", unit->mod_name);
                return false;
            }

            /* Check if dependency is already present in `tab` */
            bool in_tab = false;
            for (uint8_t m = 0; m < tab->mod_count; ++m) {
                if (mem_slice_eq_cstr(mem, tab->mod_names[m], dep_name)) {
                    in_tab = true;
                    break;
                }
            }

            /* Check if dependency is provided by another unit in `ctx` */
            int provider = -1;
            for (uint8_t v = 0; v < n; ++v) {
                if (!ctx->units[v].is_default_mod &&
                    strcmp(ctx->units[v].mod_name, dep_name) == 0) {
                    provider = (int)v;
                    break;
                }
            }

            if (!in_tab && provider < 0) {
                mem_err(mem, "Unresolved module dependency '<[%s]' required by '[%s]'",
                        dep_name, unit->mod_name);
                return false;
            }

            if (provider >= 0 && !adj[provider][u]) {
                adj[provider][u] = true;
                in_deg[u]++;
            }
        }
    }

    /* Schedule implicit `[default]` units after named units in the same link batch */
    for (uint8_t u = 0; u < n; ++u) {
        if (!ctx->units[u].is_default_mod) continue;
        for (uint8_t v = 0; v < n; ++v) {
            if (ctx->units[v].is_default_mod) continue;
            if (!adj[v][u] && !adj[u][v]) {
                adj[v][u] = true;
                in_deg[u]++;
            }
        }
    }

    /* 2. Kahn's Topological Sort with Cycle Detection */
    bool    scheduled[EXT_MAX_BUILD_UNITS];
    uint8_t order[EXT_MAX_BUILD_UNITS];
    uint8_t order_cnt = 0;
    memset(scheduled, 0, sizeof(scheduled));

    while (order_cnt < n) {
        int pick = -1;
        for (uint8_t u = 0; u < n; ++u) {
            if (!scheduled[u] && in_deg[u] == 0) {
                pick = (int)u;
                break;
            }
        }
        if (pick < 0) {
            for (uint8_t u = 0; u < n; ++u) {
                if (!scheduled[u]) {
                    mem_err(mem, "Circular module dependency involving '[%s]'",
                            ctx->units[u].mod_name);
                    break;
                }
            }
            return false;
        }
        scheduled[pick]    = true;
        order[order_cnt++] = (uint8_t)pick;
        for (uint8_t w = 0; w < n; ++w) {
            if (adj[pick][w] && in_deg[w] > 0) {
                in_deg[w]--;
            }
        }
    }

    /* 3. Save watermarks for transactional rollback across the entire link pass */
    uint16_t saved_src_len        = mem->src_len;
    uint16_t saved_ast_count      = mem->ast_count;
    uint16_t saved_pool_top       = mem->pool_top;
    uint8_t  saved_fast_real      = mem->fast_real_type;
    uint8_t  saved_str_type       = mem->str_type_id;
    uint8_t  saved_vec_type       = mem->vec_type_id;
    uint8_t  saved_mat_type       = mem->mat_type_id;
    uint8_t  saved_fn_type        = mem->fn_type_id;

    uint8_t  saved_mod_count      = tab->mod_count;
    uint64_t saved_active_modules = tab->active_modules;
    uint8_t  saved_type_count     = tab->type_count;
    uint8_t  saved_unit_count     = tab->unit_count;
    uint16_t saved_pat_elem_count = tab->pat_elem_count;
    uint16_t saved_field_count    = tab->field_count;
    uint16_t saved_sym_count      = tab->sym_count;
    uint16_t saved_overload_count = tab->overload_count;
    uint8_t  saved_const_count    = tab->const_count;

    /* 4. Compile each module slice in resolved topological order */
    for (uint8_t k = 0; k < order_cnt; ++k) {
        const ExtCompiledUnit *u = &ctx->units[order[k]];
        if (!ext_compile_slice(mem, tab, ctx->src_pool + u->src_off, u->src_len)) {
            break;
        }
    }

    if (mem->has_error) {
        mem->src_len        = saved_src_len;
        mem->ast_count      = saved_ast_count;
        mem->pool_top       = saved_pool_top;
        mem->fast_real_type = saved_fast_real;
        mem->str_type_id    = saved_str_type;
        mem->vec_type_id    = saved_vec_type;
        mem->mat_type_id    = saved_mat_type;
        mem->fn_type_id     = saved_fn_type;

        tab->mod_count      = saved_mod_count;
        tab->active_modules = saved_active_modules;
        tab->type_count     = saved_type_count;
        tab->unit_count     = saved_unit_count;
        tab->pat_elem_count = saved_pat_elem_count;
        tab->field_count    = saved_field_count;
        tab->sym_count      = saved_sym_count;
        tab->overload_count = saved_overload_count;
        tab->const_count    = saved_const_count;
        sym_rebuild_index(tab);
        return false;
    }

    mem_snapshot_perm(mem);
    return true;
}

bool ext_compile_and_link_ctx(MemArena *mem, SymTable *tab, ExtBuildCtx *ctx,
                              const char *const *raw_texts, size_t unit_count) {
    if (!mem || !tab || !ctx) return false;
    mem_clear_err(mem);
    if (!raw_texts && unit_count > 0) {
        mem_err(mem, "Null extension source array");
        return false;
    }
    ext_build_init(ctx);
    for (size_t i = 0; i < unit_count; ++i) {
        if (!ext_compile_unit(mem, tab, ctx, raw_texts[i])) {
            return false;
        }
    }
    return ext_link_units(mem, tab, ctx);
}

bool ext_compile_and_link(MemArena *mem, SymTable *tab,
                          const char *const *raw_texts, size_t unit_count) {
    if (!mem || !tab) return false;
    mem_clear_err(mem);
    if (!raw_texts && unit_count > 0) {
        mem_err(mem, "Null extension source array");
        return false;
    }
    ExtBuildCtx *ctx = ext_build_create();
    if (!ctx) {
        mem_err(mem, "Out of memory allocating temporary ExtBuildCtx");
        return false;
    }
    bool ok = ext_compile_and_link_ctx(mem, tab, ctx, raw_texts, unit_count);
    ext_build_destroy(ctx);
    return ok;
}

bool ext_load_str(MemArena *mem, SymTable *tab, const char *ext_src) {
    if (!mem || !tab) return false;
    mem_clear_err(mem);
    if (!ext_src) {
        mem_err(mem, "Null extension source string");
        return false;
    }
    return ext_compile_and_link(mem, tab, &ext_src, 1);
}
