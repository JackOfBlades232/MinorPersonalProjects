/* bbs/utils.h */
#ifndef UTILS_SENTRY
#define UTILS_SENTRY

#include <stddef.h> 
#include <stdarg.h> 

#define return_defer(code) do { result = code; goto defer; } while (0)

int add_string_to_string_array(char ***arr, const char *str, 
                               size_t *cnt, size_t *cap,
                               size_t cap_step, size_t max_cnt);

int strings_are_equal(const char *str1, const char *str2);
char *concat_strings(const char *str, ...);

int is_nl(int c);
int strip_nl(char *str);
int check_spc(const char *str);

#endif
