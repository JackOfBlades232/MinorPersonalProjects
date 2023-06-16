/* bbs/utils.c */
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

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

int add_string_to_bytestr_array(byte_arr **arr, const char *str, size_t len,
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
        *arr = realloc(*arr, *cap * sizeof(byte_arr));
    }

    ((*arr)[*cnt]).str = malloc(len * sizeof(char));
    memcpy(((*arr)[*cnt]).str, str, len);
    ((*arr)[*cnt]).len = len;

    (*cnt)++;
    return 1;
}

int strings_are_equal(const char *str1, const char *str2)
{
    return strcmp(str1, str2) == 0;
}

char *concat_strings(const char *str, ...)
{
    va_list vl;
    size_t full_len;
    char *full_str, *write_p;

    full_len = 0;
    va_start(vl, str);
    for (const char *p = str; p; p = va_arg(vl, const char *))
        full_len += strlen(p);
    va_end(vl);

    full_str = malloc((full_len+1) * sizeof(*full_str));

    write_p = full_str;
    va_start(vl, str);
    for (const char *p = str; p; p = va_arg(vl, const char *)) {
        size_t len = strlen(p);
        memcpy(write_p, p, len);
        write_p += len;
    }
    va_end(vl);

    *write_p = '\0';
    return full_str;
}

int is_nl(int c)
{
    return c == '\n' || c == '\r';
}

int strip_nl(char *str)
{
    int result;
    for (; *str && !is_nl(*str); str++) {}
    result = *str != '\0';
    if (result) *str = '\0';
    return result;
}

int check_spc(const char *str)
{
    for (; *str && *str != ' ' && *str != '\t'; str++) {}
    return *str;
}
