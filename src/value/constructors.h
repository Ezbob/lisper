
#ifndef _HEADER_FILE_constructors_20240217150145_
#define _HEADER_FILE_constructors_20240217150145_

struct lvalue;
struct mempool;
struct linterpreter;
typedef struct mpc_ast_t mpc_ast_t;

struct lvalue *lvalue_err(struct mempool *mp, char *, ...);
struct lvalue *lvalue_float(struct mempool *mp, double);
struct lvalue *lvalue_bool(struct mempool *mp, long long);
struct lvalue *lvalue_int(struct mempool *mp, long long);
struct lvalue *lvalue_exit(struct mempool *mp, int rc);
struct lvalue *lvalue_sym(struct mempool *mp, char *);
struct lvalue *lvalue_str(struct mempool *mp, char *);
struct lvalue *lvalue_builtin(struct mempool *mp,
                              struct lvalue *(*)(struct linterpreter *, struct lvalue *));
struct lvalue *lvalue_sexpr(struct mempool *mp);
struct lvalue *lvalue_qexpr(struct mempool *mp);
struct lvalue *lvalue_lambda(struct mempool *mp, struct lvalue *, struct lvalue *,
                             size_t);
struct lvalue *lvalue_file(struct mempool *mp, struct lvalue *, struct lvalue *, void *);
struct lvalue *lvalue_read(struct mempool *mp, mpc_ast_t *);
void lvalue_del(struct mempool *mp, struct lvalue *);

#endif