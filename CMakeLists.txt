cmake_minimum_required(VERSION 3.25)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

if (CMAKE_HOST_LINUX)
  find_package(LibEdit REQUIRED)
endif()

project(lisper C)

add_library(linenoise STATIC)
target_sources(linenoise PRIVATE linenoise/linenoise.c)
target_include_directories(linenoise PUBLIC linenoise)

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
)

set_property(TARGET lisper PROPERTY C_STANDARD 17)
set_property(TARGET lisper PROPERTY C_STANDARD_REQUIRED ON)

target_compile_options(lisper PRIVATE $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:CLANG>>:-Wall -Wextra -Wpedantic>)

find_library(MATH_LIBRARY m)
target_link_libraries(lisper PRIVATE ${MATH_LIBRARY} linenoise)

