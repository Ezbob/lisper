
#ifndef _HEADER_FILE_transformers_20240217150348_
#define _HEADER_FILE_transformers_20240217150348_

struct lvalue;

struct lvalue *lvalue_add(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_offer(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_join(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_join_str(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_pop(struct lvalue *, int);
struct lvalue *lvalue_take(struct lvalue *, int); /* same as pop except frees input lvalue */
struct lvalue *lvalue_copy(struct lvalue *);
int lvalue_eq(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_call(struct lenvironment *, struct lvalue *, struct lvalue *);
struct lvalue *lvalue_eval(struct lenvironment *, struct lvalue *);

#endif