#include <stdio.h>
#include <stdlib.h>
#include "exec.h"
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

void goodbye_exit(void) {
    printf("\nBye.");
    putchar('\n');
}

void exec_repl(lenv_t *env, grammar_elems elems) {
    char *input;
    printf("lisper version %s\n", "0.1.0");
    printf("Anders Busch 2018\n");
    puts("Press Ctrl+c to Exit\n");

    mpc_result_t r;
    lval_t *val;

    atexit(goodbye_exit);

#ifdef _DEBUG
    lenv_pretty_print(env);
#endif
    while ( 1 ) {
        input = readline("lisper>>> ");
        if (input == NULL) {
            putchar('\n');
            continue;
        }

        add_history(input);

        if ( mpc_parse("<stdin>", input, elems.Lisper, &r) ) {
            lval_t *read = lval_read(r.output);
#ifdef _DEBUG
            printf("Parsed input:\n");
            mpc_ast_print(r.output);
            printf("Lval object:\n");
            lval_pretty_print(read);
            printf("Current env:\n");
            lenv_pretty_print(env);
            printf("Eval result:\n");
#endif
            val = lval_eval(env, read);
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


void exec_filein(lenv_t *env, int argc, char** filenames) {
    for ( int i = 1; i < argc; ++i ) {
        lval_t *args = lval_add(lval_sexpr(), lval_str(filenames[i]));
        lval_t *x = builtin_load(env, args);
        if ( x->type == LVAL_ERR ) {
            lval_println(x);
        }
        lval_del(x);
    }
}

