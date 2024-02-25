cmake_minimum_required(VERSION 3.25)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/cmake")

project(lisper C)

add_subdirectory(src/liblisper)

add_executable(linterper "")

target_sources(linterper
  PRIVATE
    src/main.c
)

target_link_libraries(linterper
  PRIVATE
    lisper
)

if (CMAKE_HOST_WIN32)
add_custom_command(
  TARGET linterper
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:linterper> $<TARGET_RUNTIME_DLLS:linterper>
  COMMAND_EXPAND_LISTS
)
endif()

set_property(TARGET linterper PROPERTY C_STANDARD 17)
set_property(TARGET linterper PROPERTY C_STANDARD_REQUIRED ON)

if (NOT CMAKE_HOST_WIN32)
  # do not add linenoise on windows
  add_subdirectory(lib/linenoise)
  target_link_libraries(lisper PRIVATE linenoise_static)
endif()
