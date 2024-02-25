
#ifndef _HEADER_FILE_declaration_20240217153155_
#define _HEADER_FILE_declaration_20240217153155_

#include "lcells.h"

enum ltype {
  LVAL_INT,
  LVAL_FLOAT,
  LVAL_ERR,
  LVAL_SYM,
  LVAL_SEXPR,
  LVAL_QEXPR,
  LVAL_BUILTIN,
  LVAL_FUNCTION,
  LVAL_FILE,
  LVAL_BOOL,
  LVAL_STR,
  LVAL_USER_EXIT
};

typedef struct mpc_ast_t mpc_ast_t;
struct lfunction;
struct lfile;
struct linterpreter;

// lvalue is the standard return type from any lisper computation
struct lvalue {
  enum ltype type;
  union {
    double floatval;
    long long intval;
    char *strval;
    int small_intval;
    struct lcells list;
    struct lvalue *(*builtin)(struct linterpreter *, struct lvalue *);
    struct lfunction *fun;
    struct lfile *file;
  } val;
};

#endif