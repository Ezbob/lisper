
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
  LVAL_STR
};

typedef struct mpc_ast_t mpc_ast_t;
struct lenvironment;
struct lfunction;
struct lfile;

// lvalue is the standard return type from any lisper computation
struct lvalue {
  enum ltype type;
  union val {
    double floatval;
    long long intval;
    char *strval;
    struct lcells l;
    struct lvalue *(*builtin)(struct lenvironment *, struct lvalue *);
    struct lfunction *fun;
    struct lfile *file;
  } val;
};



#endif