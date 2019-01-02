#include <stdio.h>
#include <stdlib.h>
#include "exec.h"
#include "grammar.h"
#include "lval.h"
#include "lenv.h"
#include "builtin.h"
#include "lisper.h"

#ifdef _WIN32
#include <string.h>
/* windows support */
static char buf[2048];

char *readline(char *prompt) {
    fputs(prompt, stdout);
    fgets(buf, 2048, stdin);
    char *cpy = malloc(strlen(buf) + 1);
    strcpy(cpy, buf);

    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#elif _ARCHLINUX
/* arch linux support */
#include <histedit.h>
#include <editline/readline.h>

#else
/* other linux / Mac support */
#include <editline/history.h>
#include <editline/readline.h>

#endif

void goodbye_exit(void) {
    printf("\nBye.");
    putchar('\n');
}

void exec_repl(struct lenvironment *env, struct grammar_elems *elems) {
    char *input;
    printf("lisper version %s\n", LISPER_VERSION);
    printf("Anders Busch 2018\n");
    puts("Press Ctrl+c to Exit\n");

    mpc_result_t r;
    struct lvalue *val;

    atexit(goodbye_exit);

#ifdef _DEBUG
    lenvironment_pretty_print(env);
#endif
    while ( 1 ) {
        input = readline("lisper>>> ");
        if (input == NULL) {
            putchar('\n');
            continue;
        }

        add_history(input);

        if ( mpc_parse("<stdin>", input, elems->Lisper, &r) ) {
            struct lvalue *read = lvalue_read(r.output);
#ifdef _DEBUG
            printf("Parsed input:\n");
            mpc_ast_print(r.output);
            printf("Lval object:\n");
            lvalue_pretty_print(read);
            printf("Current env:\n");
            lenvironment_pretty_print(env);
            printf("Eval result:\n");
#endif
            val = lvalue_eval(env, read);
            lvalue_println(val);
            lvalue_del(val);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input); 
    }
}


void exec_filein(struct lenvironment *env, struct lisper_params *params) {
    struct lvalue *args = lvalue_add(lvalue_sexpr(), lvalue_str(params->filename));
    struct lvalue *x = builtin_load(env, args);
    if ( x->type == LVAL_ERR ) {
        lvalue_println(x);
    }
    lvalue_del(x);
}

