cmake_minimum_required(VERSION 3.20)

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
