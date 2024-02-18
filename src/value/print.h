#ifndef LISPER_EVAL
#define LISPER_EVAL

struct lvalue;

void lvalue_print(struct lvalue *);
void lvalue_println(struct lvalue *);

char *ltype_name(int);
void lvalue_pretty_print(struct lvalue *);

#endif
