/*
 * ext_loader.h — External `.math` Extension Compiler & Directory Bootstrapper Module
 *
 * Purpose:
 *   Reads `.math` extension files and directories (`00_bootstrap.math`, etc.),
 *   registers modules, `$type` patterns, flat C-style structs, constants, and
 *   operators/functions in `SymTable`, and compiles their bodies via `Parser`.
 */

#ifndef EXT_LOADER_H
#define EXT_LOADER_H

#include "mem_arena.h"
#include "sym_table.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ext_load_str(MemArena *mem, SymTable *tab, const char *ext_src);
bool ext_load_file(MemArena *mem, SymTable *tab, const char *filepath);
bool ext_boot_dir(MemArena *mem, SymTable *tab, const char *lib_dir_path);

#ifdef __cplusplus
}
#endif

#endif /* EXT_LOADER_H */