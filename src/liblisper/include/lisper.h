
#ifndef _HEADER_FILE_interpreter_20240215215413_
#define _HEADER_FILE_interpreter_20240215215413_

#ifdef __cplusplus
extern "C" {
#endif

#define LISPER_VERSION "0.3.0"

#if defined(_WIN32)
#define LLISPERAPI __declspec(dllexport)
#endif

struct lvalue;
struct linterpreter;

LLISPERAPI typedef long long linteger;
LLISPERAPI typedef struct {
  int count;
  struct lvalue **cells;
} llist;

LLISPERAPI int lisper_init(struct linterpreter **intp, int argc, char **argv);
LLISPERAPI void lisper_destroy(struct linterpreter *intp);

LLISPERAPI struct lvalue *lisper_exec(struct linterpreter *intp, const char *exec);

LLISPERAPI int lvalue_is_int(struct lvalue *);
LLISPERAPI linteger lvalue_get_int(struct lvalue *);

LLISPERAPI int lvalue_is_float(struct lvalue *);
LLISPERAPI double lvalue_get_float(struct lvalue *);

LLISPERAPI int lvalue_is_string(struct lvalue *);
LLISPERAPI const char *lvalue_get_string(struct lvalue *);

LLISPERAPI int lvalue_is_symbol(struct lvalue *);
LLISPERAPI const char *lvalue_get_symbol(struct lvalue *);

LLISPERAPI int lvalue_is_list(struct lvalue *);
LLISPERAPI llist *lvalue_get_list(struct lvalue *);

LLISPERAPI int lvalue_is_error(struct lvalue *);
LLISPERAPI const char *lvalue_get_error(struct lvalue *);

LLISPERAPI void lvalue_delete(struct linterpreter *, struct lvalue *);


#ifdef __cplusplus
}
#endif

#endif