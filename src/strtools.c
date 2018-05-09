#include "strtools.h"
#include <string.h>

size_t lstr_splitws(char *str, char***resed) {
    char **resized;
    char **res;
    char cur, n;
    int start = 0;
    int chcount = 0;
    size_t i = 0;
    size_t words = 0;
    size_t bufsize = 10;

    if ( str == NULL ) {
        *resed = NULL;
        return 0;
    }

    res = malloc(bufsize * sizeof(char *));

    for ( i = 0; i < strlen(str); ++i ) {
        cur = str[i];

        if ( cur == ' ' || cur == '\t' ) {
            chcount = 0; 
        } else {
            if ( !chcount ) {
                start = i;
            }
            chcount++;

            n = str[i + 1];

            if ( n == ' ' || n == '\t' || n == '\0' ) {
                /* copy from  str + start to str + start + (count - 1) */
                res[words] = calloc(chcount + 1, sizeof(char));
                strncpy(res[words], str + start, chcount);
                words++;
                /* resize if words reaches initial limit */
                if ( words == bufsize ) {
                    bufsize <<= 1; /* double the buf size */
                    resized = realloc(res, bufsize * sizeof(char *));
                    if ( resized == NULL ) {
                        goto resize_err;
                    }
                    res = resized;
                }
            }
        }
    }

    if ( words % bufsize != 0 && words > 0 ) {
        resized = realloc(res, words * sizeof(char *));
        if ( resized == NULL ) {
            goto resize_err;
        }
        res = resized;
    } else if ( words == 0 ) {
        free(res);
        res = NULL;
    }

    *resed = res;
    return words;

resize_err:
    for ( i = 0; i < words; ++i ) {
        free(res[i]);
    }
    free(res);
    *resed = NULL;
    return 0;
}


