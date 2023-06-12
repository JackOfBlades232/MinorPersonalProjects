/* bbs/debug.h */
#ifndef DEBUG_SENTRY
#define DEBUG_SENTRY

#include "protocol.h"
#include <stdio.h>

void debug_log_p_role(FILE* f, p_role role);
void debug_log_p_type(FILE* f, p_type type);
void debug_log_p_message(FILE* f, p_message *msg);
void debug_cat_file(FILE* f, const char *filename);

#endif
