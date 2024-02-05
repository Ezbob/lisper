cmake_minimum_required(VERSION 3.20)

project(linenoise C)

add_library(linenoise_shared SHARED)
add_library(linenoise_static STATIC)

set(sources 
    linenoise.c
)

set(public_header_dirs
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(linenoise_shared PRIVATE ${sources})
target_include_directories(linenoise_shared PUBLIC ${public_header_dirs})

target_sources(linenoise_static PRIVATE ${sources})
target_include_directories(linenoise_static PUBLIC ${public_header_dirs})

