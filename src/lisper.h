
#ifndef _HEADER_FILE_interpreter_20240215215413_
#define _HEADER_FILE_interpreter_20240215215413_

#ifdef __cplusplus
extern "C" {
#endif

#define LISPER_VERSION "0.2.1"

struct lvalue;
struct linterpreter;

typedef long long linteger;
typedef struct {
  int count;
  struct lvalue **cells;
} llist;

int linterpreter_init(struct linterpreter **intp, int argc, char **argv);
void linterpreter_destroy(struct linterpreter *intp);

int linterpreter_exec(struct linterpreter *intp, const char *exec, struct lvalue **out);

int lvalue_is_int(struct lvalue *);
linteger lvalue_get_int(struct lvalue *);

int lvalue_is_float(struct lvalue *);
double lvalue_get_float(struct lvalue *);

int lvalue_is_string(struct lvalue *);
const char *lvalue_get_string(struct lvalue *);

int lvalue_is_symbol(struct lvalue *);
const char *lvalue_get_symbol(struct lvalue *);

int lvalue_is_list(struct lvalue *);
llist *lvalue_get_list(struct lvalue *);

int lvalue_is_list(struct lvalue *);
llist *lvalue_get_list(struct lvalue *);

void lvalue_delete(struct linterpreter *, struct lvalue *);


#ifdef __cplusplus
}
#endif

#endif