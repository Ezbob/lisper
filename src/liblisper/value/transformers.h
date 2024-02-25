
#ifndef _HEADER_FILE_transformers_20240217150348_
#define _HEADER_FILE_transformers_20240217150348_

struct lvalue;
struct lenvironment;
struct mempool;
struct linterpreter;

struct lvalue *lvalue_add(struct mempool *, struct lvalue *, struct lvalue *);
struct lvalue *lvalue_offer(struct mempool *, struct lvalue *, struct lvalue *);
struct lvalue *lvalue_join(struct mempool *, struct lvalue *, struct lvalue *);
struct lvalue *lvalue_join_str(struct mempool *, struct lvalue *, struct lvalue *);
struct lvalue *lvalue_pop(struct mempool *, struct lvalue *, int);
struct lvalue *lvalue_take(struct mempool *, struct lvalue *,
                           int); /* same as pop except frees input lvalue */
struct lvalue *lvalue_copy(struct mempool *,struct lvalue *);
int lvalue_eq(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_eval(struct linterpreter *, struct lvalue *);

#endif