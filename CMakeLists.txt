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

# Gather engine implementation sources separated by folder/responsibility:
#   - Core/Code/*.c          : Core engine pipeline (arena, sym_table, lexer, parser, evaluator, math_engine)
#   - Core/compile/*.c       : Extension compiler (`compiler.c`), `<[dep]` linker (`linker.c`),
#                              `.mathc` binary serializer (`mathc_bin.c`) & user loader (`user_loader.c`)
#   - Core/session/*.c       : Parallel in-memory sessions (`session.c`) & order-tagged snapshots (`snapshot.c`)
#   - Core/lib/delegate/*.c  : All `@` intrinsic C delegates (@add, @sin, @vnew, @mzero, @calc_*, etc.)
#   - Core/lib/hardcode/*.c  : All hardcoded grammar & ergonomic rules (reserved kw, implicit *, UFCS, etc.)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Core/Code/*.c"
    "${CMAKE_SOURCE_DIR}/Core/compile/*.c"
    "${CMAKE_SOURCE_DIR}/Core/session/*.c"
    "${CMAKE_SOURCE_DIR}/Core/lib/delegate/*.c"
    "${CMAKE_SOURCE_DIR}/Core/lib/hardcode/*.c"
)
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Test/*.c")

# Reusable static library target (ready for Android NDK / host embedding)
add_library(mathengine STATIC ${CORE_SOURCES})
target_include_directories(mathengine PUBLIC
    "${CMAKE_SOURCE_DIR}/Core/Headers"
    "${CMAKE_SOURCE_DIR}/Core/compile"
    "${CMAKE_SOURCE_DIR}/Core/session"
    "${CMAKE_SOURCE_DIR}/Core/lib/delegate"
    "${CMAKE_SOURCE_DIR}/Core/lib/hardcode"
)
target_link_libraries(mathengine PUBLIC m)

# Test & demo executable linked against `mathengine`
add_executable(myapp ${TEST_SOURCES})
target_link_libraries(myapp PRIVATE mathengine)

# Optional standalone AOT extension compiler & linker (`make stdlib`)
# Note: Library/*.math files have no numerical prefixes and are passed in
# alphabetical order (`algebra.math` first, `bootstrap.math` fourth, etc.);
# `mathc` compiles each unit and topologically links them by `<[dep]`.
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
            "${CMAKE_SOURCE_DIR}/Library/algebra.math"
            "${CMAKE_SOURCE_DIR}/Library/arith.math"
            "${CMAKE_SOURCE_DIR}/Library/biology.math"
            "${CMAKE_SOURCE_DIR}/Library/bootstrap.math"
            "${CMAKE_SOURCE_DIR}/Library/calculus.math"
            "${CMAKE_SOURCE_DIR}/Library/chemistry.math"
            "${CMAKE_SOURCE_DIR}/Library/complex.math"
            "${CMAKE_SOURCE_DIR}/Library/conversions.math"
            "${CMAKE_SOURCE_DIR}/Library/geom.math"
            "${CMAKE_SOURCE_DIR}/Library/physics.math"
            "${CMAKE_SOURCE_DIR}/Library/trig.math"
            "${CMAKE_SOURCE_DIR}/Library/units.math"
        DEPENDS mathc
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Compiling & linking Library/*.math -> Library/stdlib.mathc & Core/Headers/stdlib_precompiled.h"
    )
endif()
