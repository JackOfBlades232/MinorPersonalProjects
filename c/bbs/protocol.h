/* bbs/protocol.h */
#ifndef PROTOCOL_SENTRY
#define PROTOCOL_SENTRY

#include <stddef.h>

typedef enum p_role_tag { 
    r_server = 0, 
    r_client = 1
} p_role;

typedef enum p_type_tag { 
    s_init = 0,
    c_login = 1
} p_type;

typedef struct p_message_tag {
    p_role role;
    p_type type;
    char **words;
    size_t cnt, cap;
} p_message;

p_message *p_create_message(p_role role, p_type type);
void p_free_message(p_message *msg);
void p_add_word_to_message(p_message *msg, const char *word);
char *p_construct_sendable_message(p_message *msg);

#endif
