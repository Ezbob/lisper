cmake_minimum_required(VERSION 3.25)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

if (${CMAKE_HOST_LINUX} OR ${CMAKE_HOST_APPLE})
  find_package(LibEdit REQUIRED)
endif()

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
)

set_property(TARGET lisper PROPERTY C_STANDARD 17)
set_property(TARGET lisper PROPERTY C_STANDARD_REQUIRED ON)

target_compile_options(lisper PRIVATE $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:CLANG>>:-Wall -Wextra -Wpedantic>)

if (${CMAKE_HOST_LINUX})
  find_library(MATH_LIBRARY m)
  target_link_libraries(lisper PRIVATE ${libedit_LIBRARIES} ${MATH_LIBRARY})
elseif (${CMAKE_HOST_APPLE})
  target_link_libraries(lisper PRIVATE ${libedit_LIBRARIES})
endif()
