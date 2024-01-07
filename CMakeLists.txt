cmake_minimum_required(VERSION 3.25)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

if (${CMAKE_HOST_LINUX})
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

if (${CMAKE_HOST_LINUX})
  find_library(MATH_LIBRARY m)
  target_link_libraries(lisper PRIVATE ${libedit_LIBRARIES} ${MATH_LIBRARY})
endif()
