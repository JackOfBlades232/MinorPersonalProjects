/* bbs/utils.h */
#ifndef UTILS_SENTRY
#define UTILS_SENTRY

#include <stddef.h> 
#include <stdarg.h> 

#define return_defer(code) do { result = code; goto defer; } while (0)

typedef struct byte_arr_tag {
    char *str;
    size_t len;
} byte_arr;

int resize_dynamic_arr(void **arr, size_t size, size_t *cnt, size_t *cap,
                       size_t cap_step, size_t max_cnt, int null_terminated);

int add_string_to_string_array(char ***arr, const char *str, 
                               size_t *cnt, size_t *cap,
                               size_t cap_step, size_t max_cnt);
int add_string_to_bytestr_array(byte_arr **arr, const char *str, size_t len,
                                size_t *cnt, size_t *cap,
                                size_t cap_step, size_t max_cnt);


int strings_are_equal(const char *str1, const char *str2);
char *concat_strings(const char *str, ...);

int is_nl(int c);
int strip_nl(char *str);
int check_spc(const char *str);

#endif
