/* bbs/protocol.c */
#include "protocol.h"
#include <stdlib.h>
#include <string.h>

#define DELIM ':'

enum { MESSAGE_BASE_CAP = 64 };

const char msg_header[] = "BBS232:";

p_message *p_create_message(p_role role, p_type type)
{
    p_message *msg = malloc(sizeof(p_message));
    msg->role = role;
    msg->type = type;
    msg->cnt = 0;
    msg->cap = MESSAGE_BASE_CAP;
    msg->words = malloc(msg->cap * sizeof(char *));
    return msg;
}

void p_free_message(p_message *msg)
{
    // @TODO: Null check?
    free(msg->words);
    free(msg);
}

void p_add_word_to_message(p_message *msg, const char *word)
{
    // @TODO: Null check/max words?
    if (msg->cnt >= msg->cap) {
        while (msg->cnt >= msg->cap)
            msg->cap += MESSAGE_BASE_CAP;
        msg->words = realloc(msg->words, msg->cap * sizeof(char *));
    }

    size_t wlen = strlen(word);
    msg->words[msg->cnt] = malloc((wlen+1) * sizeof(char));
    strncpy(msg->words[msg->cnt], word, wlen);
    msg->words[msg->cnt][wlen] = '\0';

    msg->cnt++;
}

char *p_construct_sendable_message(p_message *msg)
{
    char *full_msg, *write_p;
    size_t full_len;
    size_t h_len = strlen(msg_header);

    full_len = h_len + 4; // 2 delim & bytes for role/type 
    for (size_t i = 0; i < msg->cnt; i++)
        full_len += strlen(msg->words[i]) + 1;

    full_msg = malloc((full_len+1) * sizeof(char));
    strncpy(full_msg, msg_header, h_len);

    write_p = full_msg + h_len;

    // Role & type
    *write_p = DELIM;
    write_p++;
    *write_p = msg->role;
    write_p++;
    *write_p = DELIM;
    write_p++;
    *write_p = msg->type;
    write_p++;

    for (size_t i = 0; i < msg->cnt; i++) {
        char *word = msg->words[i];
        size_t wlen = strlen(word);

        *write_p = DELIM;
        write_p++;

        strncpy(write_p, word, wlen);
    }
        
    full_msg[full_len] = '\n';
    return full_msg;
}
