/* bbs/debug.h */
#ifndef DEBUG_SENTRY
#define DEBUG_SENTRY

#include "protocol.h"
#include <stdio.h>

void debug_log_p_role(p_role role);
void debug_log_p_type(p_type type);
void debug_log_p_message(p_message *msg);
void debug_cat_file(const char *filename);
void debug_print_buf(const char *buf, size_t len);

#endif
