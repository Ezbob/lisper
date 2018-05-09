#ifndef LISPER_STRTOOLS
#define LISPER_STRTOOLS

#include <stdlib.h>

#define LSTR_FREE_STRARR(strs, n) for (size_t i = 0; i < n; ++i) { free(strs[i]); } free(strs)

/* splits a string to it's whitespace seperated components */
size_t lstr_splitws(char *, char ***);

#endif

