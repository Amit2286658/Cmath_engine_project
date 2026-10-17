cmake_minimum_required(VERSION 3.16)
project(MyProject C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Hardening flags that work without needing virtual-address-space tricks --
# useful on sandboxed environments (proot, etc.) where ASan/Valgrind can't run.
# _FORTIFY_SOURCE needs at least some optimization to do anything at -O0, it's
# effectively a no-op there -- hence -Og instead of -O0 in Debug below.
add_compile_definitions(_FORTIFY_SOURCE=2)
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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/Code/*.c")

add_executable(myapp ${SOURCES})

target_include_directories(myapp PRIVATE ${CMAKE_SOURCE_DIR}/Code)