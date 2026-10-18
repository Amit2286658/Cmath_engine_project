# Chapter 1 — Getting Started, Building & Host C API

This chapter covers building the project, initializing `Engine`, evaluating expressions, managing user `.math` extensions in memory, and toggling modules in O(1) time.

## 1. Building & Running the Project

Using the root `makefile` and `CMakeLists.txt`:

```bash
# Build and run Debug (8-byte IEEE-754 NaN-boxing by default)
make run

# Build and run Release with 12-byte Tagged Union Value representation
make run release USE_TAGGED_12B=1

# Override compiler (e.g. gcc instead of clang)
make run CC=gcc

# Recompile Library/*.math into Library/stdlib.mathc and Core/Headers/stdlib_precompiled.h
make stdlib
```

## 2. Initializing & Evaluating Expressions (`math_engine.h`)

The entire engine state lives inside a single static-friendly `Engine` struct (`sizeof(Engine) ≈ 80 KB`, zero `malloc` calls):

```c
#include "math_engine.h"
#include <stdio.h>

static Engine g_eng;

int main(void) {
    if (!eng_init(&g_eng)) {
        fprintf(stderr, "Init error: %s\n", eng_last_error(&g_eng));
        return 1;
    }

    Value out;
    if (eng_eval(&g_eng, "ke(2 kg, 36 kph)", &out)) {
        char val_str[128], type_str[32];
        eng_format_value(&g_eng, out, val_str, sizeof(val_str));
        eng_type_name(&g_eng, eng_val_type(&g_eng, out), type_str, sizeof(type_str));
        printf("Result: %s (type: %s)\n", val_str, type_str);
    }
    return 0;
}
```

## 3. In-Memory User Extension Workflow (`user_loader.h`)

To keep `Core/` 100% independent of OS filesystems, the frontend host application owns all file paths and change-detection logic. `user_loader.h` exposes only two in-memory functions:

1. **`user_compile_ext(eng, raw_math_text, &out_mathc_data, &out_mathc_len)`**:
   Takes raw `.math` source text, compiles & hooks its declarations into `eng`, and hands back a pointer and byte length to the serialized `.mathc` binary image in memory.
2. **`user_load_mathc(eng, mathc_data, mathc_len)`**:
   Loads a cached `.mathc` binary image directly into `eng` in O(1) `memcpy` time.

## 4. O(1) Dynamic Module Toggling

Every `[module_name]` block in a `.math` file is assigned a bit position (`0..31`) in `SymTable.active_modules`. You can enable or disable any module at runtime in O(1) time via `eng_set_module(&g_eng, "units", false)`.
