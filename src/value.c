#include "value.h"
#include "environment.h"
#include "mempool.h"
#include "mpc.h"
#include "value/constructors.h"
#include "value/lfile.h"
#include "value/lfunction.h"
#include "value/lvalue.h"
#include "value/transformers.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct mempool *lvalue_mp;



void lvalue_del(struct lvalue *val) {
  struct lfile *file;
  struct lfunction *func;
  switch (val->type) {
  case LVAL_FLOAT:
  case LVAL_INT:
  case LVAL_BUILTIN:
  case LVAL_BOOL:
    break;
  case LVAL_FUNCTION:
    func = val->val.fun;
    lenvironment_del(func->env);
    lvalue_del(func->formals);
    lvalue_del(func->body);
    free(func);
    break;
  case LVAL_FILE:
    file = val->val.file;
    lvalue_del(file->path);
    lvalue_del(file->mode);
    free(file);
    break;
  case LVAL_ERR:
  case LVAL_SYM:
  case LVAL_STR:
    free(val->val.strval);
    break;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (size_t i = 0; i < val->val.l.count; ++i) {
      lvalue_del(val->val.l.cells[i]);
    }
    free(val->val.l.cells);
    break;
  }
  mempool_recycle(lvalue_mp, val);
}

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

enum { LREAD_FLOAT = 0, LREAD_INT = 1 };

struct lvalue *lvalue_read_num(mpc_ast_t *t, int choice) {
  int code = 0;
  long long int int_read = 0;
  double float_read = 0.0;

  switch (choice) {
  case LREAD_FLOAT:
    code = sscanf(t->contents, "%lf", &float_read);

    if (code == 1) {
      return lvalue_float(float_read);
    }
    break;
  case LREAD_INT:
    code = sscanf(t->contents, "%lli", &int_read);

    if (code == 1) {
      return lvalue_int(int_read);
    }
    break;
  }

  return lvalue_err("Cloud not parse '%s' as a number.", t->contents);
}

struct lvalue *lvalue_read_str(mpc_ast_t *t) {
  t->contents[strlen(t->contents) - 1] = '\0';
  /* clip off the newline */

  char *unescaped = malloc(strlen(t->contents + 1) + 1);
  strcpy(unescaped, t->contents + 1);
  /*  but make room for the newline in the unescaped output */

  unescaped = mpcf_unescape(unescaped);
  /* unescape probably inserts a newline into the string */
  struct lvalue *str = lvalue_str(unescaped);

  free(unescaped);
  return str;
}

struct lvalue *lvalue_read(mpc_ast_t *t) {
  struct lvalue *val = NULL;

  if (strstr(t->tag, "boolean")) {
    long long res = (strcmp(t->contents, "true") == 0) ? 1 : 0;
    return lvalue_bool(res);
  }

  if (strstr(t->tag, "string")) {
    return lvalue_read_str(t);
  }

  if (strstr(t->tag, "float")) {
    return lvalue_read_num(t, LREAD_FLOAT);
  }

  if (strstr(t->tag, "integer")) {
    return lvalue_read_num(t, LREAD_INT);
  }

  if (strstr(t->tag, "symbol")) {
    return lvalue_sym(t->contents);
  }

  if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
    /* ">" is the root of the AST */
    val = lvalue_sexpr();
  }

  if (strstr(t->tag, "qexpr")) {
    val = lvalue_qexpr();
  }

  for (int i = 0; i < t->children_num; i++) {
    struct mpc_ast_t *child = t->children[i];
    if (strcmp(child->contents, "(") == 0 || strcmp(child->contents, ")") == 0 ||
        strcmp(child->contents, "{") == 0 || strcmp(child->contents, "}") == 0 ||
        strcmp(child->tag, "regex") == 0 || strstr(child->tag, "comment")) {
      continue;
    }
    val = lvalue_add(val, lvalue_read(child));
  }

  return val;
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
