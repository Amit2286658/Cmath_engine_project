cmake_minimum_required(VERSION 3.16)
project(MyProject C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Value representation option: OFF = 8-byte IEEE-754 NaN-boxing (0), ON = 12-byte Tagged Union (1)
option(USE_TAGGED_12B "Use 12-byte tagged union Value representation instead of 8-byte NaN-boxing" OFF)
if(USE_TAGGED_12B)
    add_compile_definitions(USE_TAGGED_12B=1)
else()
    add_compile_definitions(USE_TAGGED_12B=0)
endif()

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
# Sanitizers are RUN-tested, not just compiled -- ASan in particular can link
# successfully but crash at startup on some sandboxed environments (e.g. proot
# on Android, which some Alpine-in-app setups use), since it needs to reserve
# large fixed virtual memory regions that such sandboxes don't support.
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

# Gather engine implementation sources (Core/Code/*.c) and host/test sources (Test/*.c)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Core/Code/*.c")
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Test/*.c")

add_executable(myapp ${CORE_SOURCES} ${TEST_SOURCES})

target_include_directories(myapp PRIVATE
    "${CMAKE_SOURCE_DIR}/Core/Headers"
)

# Link standard C math library (<math.h>: sin, cos, pow, sqrt, cbrt, fmod, etc.)
target_link_libraries(myapp PRIVATE m)

# Optional standalone AOT `.math` -> `.mathc` & header compiler (`make stdlib`)
if(EXISTS "${CMAKE_SOURCE_DIR}/Library/mathc.c")
    add_executable(mathc EXCLUDE_FROM_ALL
        "${CMAKE_SOURCE_DIR}/Library/mathc.c"
        "${CMAKE_SOURCE_DIR}/Core/Code/mem_arena.c"
        "${CMAKE_SOURCE_DIR}/Core/Code/sym_table.c"
        "${CMAKE_SOURCE_DIR}/Core/Code/lexer.c"
        "${CMAKE_SOURCE_DIR}/Core/Code/parser.c"
        "${CMAKE_SOURCE_DIR}/Core/Code/ext_loader.c"
        "${CMAKE_SOURCE_DIR}/Core/Code/evaluator.c"
    )
    target_include_directories(mathc PRIVATE "${CMAKE_SOURCE_DIR}/Core/Headers")
    target_link_libraries(mathc PRIVATE m)

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
        DEPENDS mathc
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Recompiling Library/*.math -> Library/stdlib.mathc & Core/Headers/stdlib_precompiled.h"
    )
endif()
