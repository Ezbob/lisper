
#ifndef _HEADER_FILE_constructors_20240217150145_
#define _HEADER_FILE_constructors_20240217150145_

struct lvalue;

struct lvalue *lvalue_err(char *, ...);
struct lvalue *lvalue_float(double);
struct lvalue *lvalue_bool(long long);
struct lvalue *lvalue_int(long long);
struct lvalue *lvalue_sym(char *);
struct lvalue *lvalue_str(char *);
struct lvalue *lvalue_builtin(struct lvalue *(*)(struct lenvironment *, struct lvalue *));
struct lvalue *lvalue_sexpr(void);
struct lvalue *lvalue_qexpr(void);
struct lvalue *lvalue_lambda(struct lvalue *, struct lvalue *, size_t);
struct lvalue *lvalue_file(struct lvalue *, struct lvalue *, void *);

#endif