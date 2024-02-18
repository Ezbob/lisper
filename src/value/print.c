#include "print.h"
#include "lvalue.h"
#include "lfunction.h"
#include "lfile.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpc.h"

/**
 * Prints lvalue expressions (such as sexprs) given the prefix,
 * suffix and delimiter
 */
void lvalue_expr_print(struct lvalue *val, char prefix, char suffix, char delimiter) {
  putchar(prefix);

  size_t len = val->val.l.count;

  if (len > 0)
    lvalue_print(val->val.l.cells[0]);

  for (size_t i = 1; i < len; i++) {
    putchar(delimiter);
    lvalue_print(val->val.l.cells[i]);
  }

  putchar(suffix);
}

/**
 * Escapes the string value of the input lvalue
 * and prints the value to the stdout
 */
void lvalue_print_str(struct lvalue *v) {
  char *escaped = malloc(strlen(v->val.strval) + 1);
  strcpy(escaped, v->val.strval);

  escaped = mpcf_escape(escaped);
  printf("\"%s\"", escaped);

  free(escaped);
}

/**
 * Prints the lvalue contents to stdout
 */
void lvalue_print(struct lvalue *val) {
  switch (val->type) {
  case LVAL_FLOAT:
    printf("%lf", val->val.floatval);
    break;
  case LVAL_INT:
    printf("%lli", val->val.intval);
    break;
  case LVAL_BOOL:
    if (val->val.intval == 0) {
      printf("false");
    } else {
      printf("true");
    }
    break;
  case LVAL_ERR:
    printf("Error: %s", val->val.strval);
    break;
  case LVAL_SYM:
    printf("%s", val->val.strval);
    break;
  case LVAL_STR:
    lvalue_print_str(val);
    break;
  case LVAL_SEXPR:
    lvalue_expr_print(val, '(', ')', ' ');
    break;
  case LVAL_QEXPR:
    lvalue_expr_print(val, '{', '}', ' ');
    break;
  case LVAL_FUNCTION:
    printf("(\\ ");
    lvalue_print(val->val.fun->formals);
    putchar(' ');
    lvalue_print(val->val.fun->body);
    putchar(')');
    break;
  case LVAL_BUILTIN:
    printf("<builtin>");
    break;
  case LVAL_FILE:
    printf("<file ");
    lvalue_print(val->val.file->path);
    printf(" @ mode ");
    lvalue_print(val->val.file->mode);
    printf(">");
    break;
  }
}

void lvalue_println(struct lvalue *val) {
  lvalue_print(val);
  putchar('\n');
}

/**
 * Given a lvalue type, return it's string representation
 */
char *ltype_name(int t) {
  switch (t) {
  case LVAL_SYM:
    return "symbol";
  case LVAL_ERR:
    return "error";
  case LVAL_STR:
    return "string";
  case LVAL_BUILTIN:
    return "builtin";
  case LVAL_FUNCTION:
    return "function";
  case LVAL_FLOAT:
    return "float";
  case LVAL_BOOL:
    return "boolean";
  case LVAL_INT:
    return "integer";
  case LVAL_QEXPR:
    return "q-expression";
  case LVAL_SEXPR:
    return "s-expression";
  default:
    break;
  }
  return "Unknown";
}

/**
 * Helper function for printing lvalue
 */
void lvalue_depth_print(struct lvalue *v, size_t depth) {
  for (size_t i = 0; i < depth; ++i) {
    putchar(' ');
  }

  printf("`-t: %s = ", ltype_name(v->type));

  lvalue_println(v);

  /* only q-expressions and s-expression has children */
  if (v->type == LVAL_QEXPR || v->type == LVAL_SEXPR) {
    for (size_t i = 0; i < v->val.l.count; ++i) {
      lvalue_depth_print(v->val.l.cells[i], depth + 1);
    }
  }
}

/**
 *  Pretty print a lvalue revealing it's structure.
 *  Good for debugging your thoughts
 */
void lvalue_pretty_print(struct lvalue *v) { lvalue_depth_print(v, 0); }
