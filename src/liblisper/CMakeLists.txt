
project(lisper C)

add_library(lisper SHARED)

target_sources(lisper
  PRIVATE
    grammar.c
    builtin.c
    environment.c
    lisper.c
    mempool.c
    mpc.c
    prgparams.c
    value/transformers.c
    value/constructors.c
    value/print.c
    compat/string.c
)

set_property(TARGET lisper PROPERTY C_STANDARD 17)
set_property(TARGET lisper PROPERTY C_STANDARD_REQUIRED ON)

target_compile_options(lisper PRIVATE
  $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:CLANG>>:-Wall -Wextra -Wpedantic>
  $<$<C_COMPILER_ID:MSVC>:/W4>
)

target_include_directories(lisper 
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

include(CheckSymbolExists)
check_symbol_exists(strdup "string.h" STRDUP_DEFINED)

target_compile_definitions(lisper PRIVATE
  -DSTRDUP_DEFINED=${STRDUP_DEFINED}
  $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE>
)

if (CMAKE_HOST_LINUX)
  find_library(MATH_LIBRARY m)
  target_link_libraries(lisper PRIVATE ${MATH_LIBRARY})
endif()
