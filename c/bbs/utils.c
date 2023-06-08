/* bbs/utils.c */
#include "utils.h"
#include <stdlib.h>
#include <string.h>

int add_string_to_string_array(char ***arr, const char *str, 
                               size_t *cnt, size_t *cap,
                               size_t cap_step, size_t max_cnt)
{
    if (*cnt >= max_cnt)
        return 0;

    if (*cnt >= *cap) {
        while (*cnt >= *cap)
            *cap += cap_step;
        if (*cap > max_cnt)
            *cap = max_cnt;

        *arr = realloc(*arr, *cap * sizeof(char *));
    }

    size_t slen = strlen(str);
    (*arr)[*cnt] = malloc((slen+1) * sizeof(char));
    memcpy((*arr)[*cnt], str, slen);
    (*arr)[*cnt][slen] = '\0';

    (*cnt)++;
    return 1;
}
