
#ifndef _HEADER_FILE_function_20240217143111_
#define _HEADER_FILE_function_20240217143111_

struct lvalue;
struct lenvironment;

struct lfunction {
  char *name;
  struct lenvironment *env;
  struct lvalue *formals;
  struct lvalue *body;
};

struct lfunction *lfunc_new(struct lenvironment *env, struct lvalue *formals,
                            struct lvalue *body);

#endif