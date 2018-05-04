#include <stdio.h>
#include <stdlib.h>
#include "prompt.h"
#include "grammar.h"
#include "lval.h"
#include "lenv.h"
#include "builtin.h"

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

void do_repl(lenv_t *env, grammar_elems elems) {
    char *input;
    printf("lisper version %s\n", "0.1.0");
    printf("Anders Busch 2018\n");
    puts("Press Ctrl+c to Exit\n");

    mpc_result_t r;
    lval_t *val;

#ifdef _DEBUG
    lenv_pretty_print(env);
#endif
    while ( 1 ) {
        input = readline("lisper>>> ");

        add_history(input);

        if ( mpc_parse("<stdin>", input, elems.Lisper, &r) ) {
            lval_t *read = lval_read(r.output);
#ifdef _DEBUG
            lval_pretty_print(read);
#endif
            val = lval_eval(env, read);
#ifdef _DEBUG
            mpc_ast_print(r.output);
            putchar('\n');
            lenv_pretty_print(env);
#endif
            lval_println(val);
            lval_del(val);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input); 
    }
}

