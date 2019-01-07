# Lisper repo

Repo for a implementation of lisp, following the [buildyourownlisp.com](http://www.buildyourownlisp.com) tutorial.

## Features

- Single byte strings
- Simple file IO operations with strings
- Q-expression operations (expressions that does not automatically get evaluated) 
- Source file import
- User defined functions and lambdas
- User defined immutable values 
- Read Eval Print Loop (REPL) support
- Cross OS portablity (Can be compiled to run on Windows, Mac OS or Linux)

## Setup

The repo contains a makefile with the following phony targets:

- **all** (default target) compiles the interpreter `lisper`
- **clean** removes the object files and the interpreter
- **debug** compiles the interpreter in debug mode by adding in debug symbols in the object files and exposes the `_DEBUG` macro symbol to the C preprocessor

By default, the makefile compilation exposes the `_ARCHLINUX` macro symbol to the preprocessor to enable compilation of the interpreter on the Arch Linux distribution. This symbol can be turned off by setting the environmental variable `SYMBOLS` to the empty string, to enable Mac OS or other Linux support. The code can also be compiled with Visual Basic under Windows.

## Usage

Like the Python interpreter, the Lisper interpreter works in two ways; by running as a REPL or evaluating source files. 
To use the REPL simply invoke the intepreter:
```
./lisper
```
This will drop you into a interactive session that can be exited by invoking the `exit 0` expression or pressing Ctrl+C .
To evaluate source files, simply add the filenames as command line arguments to the interpreter invocation:
```
./lisper mysource.lspr
```
The `examples` directory in the repo contains some example source files that can be run in this manner.
```
./lisper --help
```
Lists the command line options and usage available.
