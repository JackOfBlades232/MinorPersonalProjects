/* bbs/utils.h */
#ifndef UTILS_SENTRY
#define UTILS_SENTRY

#include <stddef.h> 

#define return_defer(code) do { result = code; goto defer; } while (0)

int add_string_to_string_array(char ***arr, const char *str, 
                               size_t *cnt, size_t *cap,
                               size_t cap_step, size_t max_cnt);

#endif
