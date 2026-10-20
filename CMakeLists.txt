cmake_minimum_required(VERSION 3.16)
project(MyProject C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)


# Hardening flags that work without needing virtual-address-space tricks --
# useful on sandboxed environments (proot, etc.) where ASan/Valgrind can't run.
# _FORTIFY_SOURCE needs at least some optimization to do anything at -O0, it's
# effectively a no-op there -- hence -Og instead of -O0 in Debug below.
add_compile_options(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2)
add_compile_options(-fstack-protector-strong)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug: debug symbols, light optimization (needed for fortify to take effect),
# sanitizers if the toolchain supports them.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -Og)

    include(CheckCSourceRuns)

    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
    check_c_source_runs("int main(void) { return 0; }" ASAN_SUPPORTED)
    unset(CMAKE_REQUIRED_FLAGS)

    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    check_c_source_runs("int main(void) { return 0; }" UBSAN_SUPPORTED)
    unset(CMAKE_REQUIRED_FLAGS)

    set(ENABLED_SANITIZERS "")
    if(ASAN_SUPPORTED)
        list(APPEND ENABLED_SANITIZERS "address")
    else()
        message(WARNING "ASan not available on this toolchain -- skipping")
    endif()
    if(UBSAN_SUPPORTED)
        list(APPEND ENABLED_SANITIZERS "undefined")
    else()
        message(WARNING "UBSan not available on this toolchain -- skipping")
    endif()

    if(ENABLED_SANITIZERS)
        list(JOIN ENABLED_SANITIZERS "," SANITIZER_FLAG_LIST)
        message(STATUS "Enabling sanitizers: ${SANITIZER_FLAG_LIST}")
        add_compile_options(-fsanitize=${SANITIZER_FLAG_LIST})
        add_link_options(-fsanitize=${SANITIZER_FLAG_LIST})
    else()
        message(WARNING "No sanitizers available on this toolchain -- building plain debug")
    endif()
endif()

# Release: optimized, no sanitizers
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
endif()

# Gather engine implementation sources:
#   - Core/Code/*.c          : Core engine pipeline (arena, sym_table, lexer, parser, ext_loader, evaluator)
#   - Core/lib/delegate/*.c  : All `@` intrinsic C delegates (@add, @sin, @vnew, @mzero, @calc_*, etc.)
#   - Core/lib/hardcode/*.c  : All hardcoded grammar & ergonomic rules (reserved kw, implicit *, UFCS, etc.)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Core/Code/*.c"
    "${CMAKE_SOURCE_DIR}/Core/lib/delegate/*.c"
    "${CMAKE_SOURCE_DIR}/Core/lib/hardcode/*.c"
)
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Test/*.c")

# Reusable static library target (ready for Android NDK / host embedding)
add_library(mathengine STATIC ${CORE_SOURCES})
target_include_directories(mathengine PUBLIC
    "${CMAKE_SOURCE_DIR}/Core/Headers"
    "${CMAKE_SOURCE_DIR}/Core/lib/delegate"
    "${CMAKE_SOURCE_DIR}/Core/lib/hardcode"
)
target_link_libraries(mathengine PUBLIC m)

# Test & demo executable linked against `mathengine`
add_executable(myapp ${TEST_SOURCES})
target_link_libraries(myapp PRIVATE mathengine)

# Optional standalone AOT extension text -> `.mathc` & header compiler (`make stdlib`)
if(EXISTS "${CMAKE_SOURCE_DIR}/Library/mathc.c")
    add_executable(mathc EXCLUDE_FROM_ALL
        "${CMAKE_SOURCE_DIR}/Library/mathc.c"
    )
    target_link_libraries(mathc PRIVATE mathengine)

    add_custom_target(stdlib
        COMMAND mathc
            "${CMAKE_SOURCE_DIR}/Library/stdlib.mathc"
            "${CMAKE_SOURCE_DIR}/Core/Headers/stdlib_precompiled.h"
            k_stdlib_mathc
            "${CMAKE_SOURCE_DIR}/Library/00_bootstrap.math"
            "${CMAKE_SOURCE_DIR}/Library/10_arith.math"
            "${CMAKE_SOURCE_DIR}/Library/20_trig.math"
            "${CMAKE_SOURCE_DIR}/Library/30_complex.math"
            "${CMAKE_SOURCE_DIR}/Library/40_geom.math"
            "${CMAKE_SOURCE_DIR}/Library/50_algebra.math"
            "${CMAKE_SOURCE_DIR}/Library/55_calculus.math"
            "${CMAKE_SOURCE_DIR}/Library/60_units.math"
            "${CMAKE_SOURCE_DIR}/Library/65_conversions.math"
            "${CMAKE_SOURCE_DIR}/Library/70_physics.math"
            "${CMAKE_SOURCE_DIR}/Library/80_chemistry.math"
            "${CMAKE_SOURCE_DIR}/Library/90_biology.math"
        DEPENDS mathc
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Recompiling Library/* -> Library/stdlib.mathc & Core/Headers/stdlib_precompiled.h"
    )
endif()
