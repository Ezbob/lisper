
#include "compat_string.h"

#include <string.h>
#include <stdlib.h>

#ifndef STRDUP_DEFINED
char *strdup(const char *s) {
    if (s == NULL) {
        return s;
    }
    size_t sl = strlen(s);
    if (s == 0) {
        return NULL;
    }
    char *b = malloc(sizeof(char) * (sl + 1));
    if (!b) {
        return NULL;
    }
    memcpy(b, s, sl);
    b[sl + 1] = '\0';
    return b;
}
#endif