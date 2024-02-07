cmake_minimum_required(VERSION 3.25)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

project(lisper C)


add_executable(lisper "")

target_sources(lisper
  PRIVATE
    src/grammar.c
    src/builtin.c
    src/environment.c
    src/execute.c
    src/lisper.c
    src/mempool.c
    src/mpc.c
    src/value.c
    src/prgparams.c
    src/compat_string.c
)

set_property(TARGET lisper PROPERTY C_STANDARD 17)
set_property(TARGET lisper PROPERTY C_STANDARD_REQUIRED ON)

target_compile_options(lisper PRIVATE $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:CLANG>>:-Wall -Wextra -Wpedantic>)

include(CheckSymbolExists)
check_symbol_exists(strdup "string.h" STRDUP_DEFINED)

target_compile_definitions(lisper PRIVATE -DSTRDUP_DEFINED=${STRDUP_DEFINED})

if (CMAKE_HOST_LINUX)
  find_library(MATH_LIBRARY m)
  target_link_libraries(lisper PRIVATE ${MATH_LIBRARY})
endif()

if (NOT CMAKE_HOST_WIN32)
  # do not add linenoise on windows
  add_subdirectory(lib/linenoise)
  target_link_libraries(lisper PRIVATE linenoise_static)
endif()
