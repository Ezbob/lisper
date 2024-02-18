#ifndef LISPER_EVAL
#define LISPER_EVAL

typedef struct mpc_ast_t mpc_ast_t;
struct lvalue;
struct mempool;

struct lvalue *lvalue_read(struct mempool *mp, mpc_ast_t *);

void lvalue_print(struct lvalue *);
void lvalue_println(struct lvalue *);
void lvalue_del(struct mempool *mp, struct lvalue *);

char *ltype_name(int);
void lvalue_pretty_print(struct lvalue *);

#endif
