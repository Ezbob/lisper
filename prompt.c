#include <stdio.h>
#include <stdlib.h>
#include "meta.h"
#include "prompt.h"
#include "grammar.h"
#include "eval.h"

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

void do_repl(void) {
    char *input;
    printf("lisper version %s\n", LISPER_VERSION);
    printf("Anders Busch 2018\n");
    puts("Press Ctrl+c to Exit\n");

    grammar_elems elems;
    mpc_result_t r;
    lval_t eval_res;

    grammar_elems_init(&elems);
    grammar_make_lang(&elems);

    while ( 1 ) {
        input = readline("lisper>>> ");

        add_history(input);

        if ( mpc_parse("<stdin>", input, elems.Lisper, &r) ) {
            eval_res = eval(r.output);
#ifdef _DEBUG
            mpc_ast_print(r.output);
            putchar('\n');
#endif
            lval_println(&eval_res);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input); 
    }

    grammar_elems_destroy(&elems);
}


