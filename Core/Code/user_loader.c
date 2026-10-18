/*
 * user_loader.c — Platform-Agnostic User Extension Compiler & Loader
 *
 * Operates strictly on raw `.math` text strings and in-memory `.mathc` binary
 * buffers without any filesystem, directory, or path logic.
 */

#include "user_loader.h"
#include "ext_loader.h"

/* Internal static buffer holding the most recently compiled `.mathc` image */
static uint8_t s_compiled_mathc_buf[MATHC_MAX_IMAGE_BYTES];

bool user_compile_ext(Engine *eng, const char *raw_math_text,
                      const uint8_t **out_mathc_data, size_t *out_mathc_len) {
    if (out_mathc_data) *out_mathc_data = NULL;
    if (out_mathc_len)  *out_mathc_len  = 0;
    if (!eng || !raw_math_text) return false;

    if (!ext_load_str(&eng->mem, &eng->sym, raw_math_text)) {
        return false;
    }

    if (out_mathc_data || out_mathc_len) {
        size_t written = ext_save_mathc_mem(&eng->mem, &eng->sym,
                                            s_compiled_mathc_buf,
                                            sizeof(s_compiled_mathc_buf));
        if (written == 0) return false;
        if (out_mathc_data) *out_mathc_data = s_compiled_mathc_buf;
        if (out_mathc_len)  *out_mathc_len  = written;
    }
    return true;
}

bool user_load_mathc(Engine *eng, const uint8_t *mathc_data, size_t mathc_len) {
    if (!eng || !mathc_data || mathc_len == 0) return false;
    return ext_load_mathc_mem(&eng->mem, &eng->sym, mathc_data, mathc_len);
}