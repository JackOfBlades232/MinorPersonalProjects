/* bbs/utils.c */
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

int resize_dynamic_arr(void **arr, size_t size, size_t *cnt, size_t *cap,
                       size_t cap_step, size_t max_cnt, int null_terminated)
{
    if (*cnt >= max_cnt)
        return 0;
    if (*cnt >= *cap - null_terminated) {
        while (*cnt >= *cap - null_terminated)
            *cap += cap_step;
        if (*cap - null_terminated > max_cnt)
            *cap = max_cnt;
        *arr = realloc(*arr, *cap * size);
    }

    return 1;
}

int add_string_to_string_array(char ***arr, const char *str, 
                               size_t *cnt, size_t *cap,
                               size_t cap_step, size_t max_cnt)
{
    if (!resize_dynamic_arr((void **) arr, sizeof(**arr), cnt, cap, cap_step, max_cnt, 0))
        return 0;

    size_t slen = strlen(str);
    (*arr)[*cnt] = malloc((slen+1) * sizeof(*((*arr)[*cnt])));
    memcpy((*arr)[*cnt], str, slen);
    (*arr)[*cnt][slen] = '\0';

    (*cnt)++;
    return 1;
}

int add_string_to_bytestr_array(byte_arr **arr, const char *str, size_t len,
                                size_t *cnt, size_t *cap,
                                size_t cap_step, size_t max_cnt)
{
    if (!resize_dynamic_arr((void **) arr, sizeof(**arr), cnt, cap, cap_step, max_cnt, 0))
        return 0;

    ((*arr)[*cnt]).str = malloc(len * sizeof(*(((*arr)[*cnt]).str)));
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

int is_sep(int c)
{
    return c == '\n' || c == '\r' || c == ' ' || c == '\t';
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

char *extract_word_from_buf(const char *buf, size_t bufsize, size_t *chars_read)
{
    char *word;

    *chars_read = 0;
    while (bufsize > 0 && is_sep(*buf)) {
        buf++;
        bufsize--;
        (*chars_read)++;
    }

    if (bufsize <= 0)
        return NULL;
    
    const char *bufp = buf;
    size_t bufsize_cpy = bufsize;
    while (bufsize_cpy > 0 && !is_sep(*bufp)) {
        bufp++;
        bufsize_cpy--;
    }

    size_t wlen = bufsize-bufsize_cpy;
    word = strndup(buf, wlen);
    *chars_read += wlen;
    return word;
}

const char *stripped_filename(const char *filename)
{
    size_t i;
    for (i = strlen(filename)-1; i > 0; i--) {
        if (filename[i] == '/')
            break;
    }

    if (filename[i] == '/')
        return filename + i + 1;
    else
        return filename;
}

int filename_is_stripped(const char *filename)
{
    for (; *filename; filename++) {
        if (*filename == '/')
            return 0;
    }

    return 1;
}
