/*
 * user_loader.h — Platform-Agnostic User Extension Compiler & Loader
 *
 * Purpose:
 *   Provides a strictly in-memory interface for user-defined `.math` extensions.
 *   All file paths, folders, and platform storage are handled by the frontend;
 *   the math engine does not touch files or directories.
 *
 *   Only two operations exist:
 *     1. `user_compile_ext`: Takes raw `.math` text, compiles & hooks it into
 *        `Engine`, and hands back the compiled `.mathc` binary image in memory
 *        (`*out_mathc_data`, `*out_mathc_len`) for the frontend to save.
 *     2. `user_load_mathc`: Takes a compiled `.mathc` binary image in memory
 *        (`mathc_data`, `mathc_len`) and loads it into `Engine` in O(1) time.
 */

#ifndef USER_LOADER_H
#define USER_LOADER_H

#include "math_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Take raw `.math` source text (`raw_math_text`), compile & hook its declarations
 * into `eng`, and hand back the compiled `.mathc` binary image via
 * `*out_mathc_data` and `*out_mathc_len`.
 *
 * Note: `*out_mathc_data` points to an internal static buffer managed by the
 * loader (no `free()` required by the caller). `out_mathc_data` and
 * `out_mathc_len` may be NULL if the caller does not need the compiled bytes.
 */
bool user_compile_ext(Engine *eng, const char *raw_math_text,
                      const uint8_t **out_mathc_data, size_t *out_mathc_len);

/*
 * Take in a compiled `.mathc` binary image (`mathc_data`, `mathc_len`) from the
 * frontend and load it directly into `eng` in O(1) `memcpy` time.
 */
bool user_load_mathc(Engine *eng, const uint8_t *mathc_data, size_t mathc_len);

#ifdef __cplusplus
}
#endif

#endif /* USER_LOADER_H */