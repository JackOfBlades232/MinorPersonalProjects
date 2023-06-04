/* bbs/protocol.c */
#include "protocol.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#define DELIM ':'

enum { 
    MESSAGE_BASE_CAP = 64,
    WORD_BASE_CAP = 16,

    HEADER_LEN = 6,
    TYPES_AND_CUTOFF_LEN = 4,

    NUM_ROLES = 2,
    NUM_TYPES = 4
};

const char header[HEADER_LEN+1] = "BBS232";

const p_role valid_roles[NUM_ROLES] = { r_server, r_client };
const p_type valid_types[NUM_TYPES] = {
    ts_init, tc_login, ts_login_success, ts_login_failed
};

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
    memcpy(msg->words[msg->cnt], word, wlen);
    msg->words[msg->cnt][wlen] = '\0';

    msg->cnt++;
}

char *p_construct_sendable_message(p_message *msg)
{
    char *full_msg, *write_p;
    size_t full_len;

    full_len = HEADER_LEN + TYPES_AND_CUTOFF_LEN; // 2 delim & bytes for role/type + '\n' cutoff
    for (size_t i = 0; i < msg->cnt; i++)
        full_len += strlen(msg->words[i]) + 1;

    full_msg = malloc((full_len+1) * sizeof(char));
    memcpy(full_msg, header, HEADER_LEN);

    write_p = full_msg + HEADER_LEN;

    // Role & type
    *write_p = DELIM;
    write_p++;
    *write_p = msg->role;
    write_p++;
    *write_p = msg->type;
    write_p++;

    for (size_t i = 0; i < msg->cnt; i++) {
        char *word = msg->words[i];
        size_t wlen = strlen(word);

        *write_p = DELIM;
        write_p++;

        memcpy(write_p, word, wlen);
        write_p += wlen;
    }
        
    full_msg[full_len-1] = '\n';
    full_msg[full_len] = '\0';

    return full_msg;
}

void p_init_reader(p_message_reader *reader)
{
    reader->state = rs_header;
    reader->header_match_idx = 0;
    reader->msg = p_create_message(r_unknown, t_unknown);
    reader->cur_word = NULL;
    reader->wlen = 0;
    reader->wcap = 0;
}

void p_clear_reader(p_message_reader *reader)
{
    reader->state = rs_empty;
    reader->header_match_idx = 0;
    if (reader->msg) {
        p_free_message(reader->msg);
        reader->msg = NULL;
    }
    if (reader->cur_word) {
        free(reader->cur_word);
        reader->cur_word = NULL;
        reader->wlen = 0;
        reader->wcap = 0;
    }
}

int p_reader_is_live(p_message_reader *reader)
{
    return reader->msg != NULL;
}

size_t match_header(p_message_reader *reader, const char *str, size_t len)
{
    size_t chars_read;
    for (chars_read = 0; chars_read < len; chars_read++) {
        char hc = header[reader->header_match_idx];
        if (hc == '\0') {
            reader->state = str[chars_read] == DELIM ? rs_role : rs_error;
            chars_read++;
            break;
        } else if (hc != str[chars_read]) {
            reader->state = rs_error;
            break;
        }
        
        reader->header_match_idx++;
    }

    return chars_read;
}

int try_get_role_from_byte(char r, p_role *out)
{
    for (size_t i = 0; i < NUM_ROLES; i++) {
        if (r == valid_roles[i]) {
            *out = valid_roles[i];
            return 1;
        }
    }

    return 0;
}

int try_get_type_from_byte(char r, p_type *out)
{
    for (size_t i = 0; i < NUM_TYPES; i++) {
        if (r == valid_types[i]) {
            *out = valid_types[i];
            return 1;
        }
    }

    return 0;
}

size_t parse_role(p_message_reader *reader, const char *str, size_t len)
{
    if (len >= 1) {
        reader->state = try_get_role_from_byte(*str, &reader->msg->role) ?
                        rs_type :
                        rs_error;

        return 1;
    }

    return 0;
}

size_t parse_type(p_message_reader *reader, const char *str, size_t len)
{
    if (len >= 1) {
        reader->state = try_get_type_from_byte(*str, &reader->msg->type) ?
                        rs_content :
                        rs_error;

        return 1;
    }

    return 0;
}

void store_and_recreate_reader_cur_word(p_message_reader *reader)
{
    if (reader->cur_word) {
        reader->cur_word[reader->wlen] = '\0';
        p_add_word_to_message(reader->msg, reader->cur_word);
        free(reader->cur_word);
    }

    reader->wcap = WORD_BASE_CAP;
    reader->wlen = 0;
    reader->cur_word = malloc(reader->wcap * sizeof(char));
    *reader->cur_word = '\0';
}

void try_resize_reader_cur_word(p_message_reader *reader)
{
    if (reader->wcap > reader->wlen)
        return;

    while (reader->wcap <= reader->wlen)
        reader->wcap += WORD_BASE_CAP;
    reader->cur_word = realloc(reader->cur_word, reader->wcap); 
}

size_t parse_content(p_message_reader *reader, const char *str, size_t len)
{
    size_t chars_read = 0;
    if (len == 0)
        return 0;

    if (reader->cur_word == NULL) {
        if (*str == '\n') {
            reader->state = rs_finished;
            return 1;
        } else if (*str != DELIM) {
            reader->state = rs_error;
            return 1;
        }
    }

    for (; chars_read < len; chars_read++) {
        char c = str[chars_read];
        if (c == '\n') {
            reader->cur_word[reader->wlen] = '\0';
            p_add_word_to_message(reader->msg, reader->cur_word);
            free(reader->cur_word);
            reader->cur_word = NULL;

            reader->state = rs_finished;
            return chars_read;
        } else if (c == DELIM) {
            store_and_recreate_reader_cur_word(reader);
            continue;
        }

        reader->cur_word[reader->wlen] = c;
        reader->wlen++;

        try_resize_reader_cur_word(reader);
    }

    return chars_read;
}

int p_reader_process_str(p_message_reader *reader, const char *str, size_t len)
{
    if (reader->state == rs_empty || reader->state == rs_error)
        return -1;

    int result = 0;

    while (len > 0) {
        size_t chars_read;
        switch (reader->state) {
            case rs_header:
                chars_read = match_header(reader, str, len);
                break;
            case rs_role:
                chars_read = parse_role(reader, str, len);
                break;
            case rs_type:
                chars_read = parse_type(reader, str, len);
                break;
            case rs_content:
                chars_read = parse_content(reader, str, len);
                break;
            case rs_finished:
                break;
            default:
                reader->state = rs_error;
                break;
        }

        if (reader->state == rs_finished) {
            result = 1;
            break;
        }

        if (reader->state == rs_error) {
            result = -1;
            break;
        }

        str += chars_read;
        len -= chars_read;
    }

    return result;
}
