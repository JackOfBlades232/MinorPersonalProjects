/* bbs/protocol.c */
#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>

#define DELIM ':'
#define ENDC ','

enum { 
    BYTE_POT = 256,
    WORD_BYTES = 2,
    MAX_WORD_LEN = 65535, // 256**WORD_BYTES - 1
    WORD_CNT_BYTES = 1,
    MAX_WORD_CNT = 255, // 256**WORD_CNT_BYTES - 1

    MESSAGE_BASE_CAP = 64,
    WORD_BASE_CAP = 16,

    HEADER_LEN = 6,
    BYTE_FIELDS_LEN = 2,
    MAX_DIGITS = 32, // overkill

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
    msg->w_total_len = 0;
    msg->cnt = 0;
    msg->cap = MESSAGE_BASE_CAP;
    msg->words = malloc(msg->cap * sizeof(char *));
    return msg;
}

void p_free_message(p_message *msg)
{
    free(msg->words);
    free(msg);
}

int p_add_word_to_message(p_message *msg, const char *word)
{
    if (msg->cnt >= MAX_WORD_CNT)
        return 0;

    if (msg->cnt >= msg->cap) {
        while (msg->cnt >= msg->cap)
            msg->cap += MESSAGE_BASE_CAP;
        if (msg->cap > MAX_WORD_CNT)
            msg->cap = MAX_WORD_CNT;

        msg->words = realloc(msg->words, msg->cap * sizeof(char *));
    }

    size_t wlen = strlen(word);
    msg->words[msg->cnt] = malloc((wlen+1) * sizeof(char));
    memcpy(msg->words[msg->cnt], word, wlen);
    msg->words[msg->cnt][wlen] = '\0';

    msg->cnt++;
    return 1;
}

char *p_construct_sendable_message(p_message *msg)
{
    char *full_msg, *write_p;
    size_t full_len;

    // header + delim + role&type bytes + delim + word cnt + ... + end char
    full_len  = HEADER_LEN + 1 + BYTE_FIELDS_LEN + 1 + WORD_CNT_BYTES + 1;

    for (size_t i = 0;  < msg->cnt; i++) {
        size_t wlen = strlen(msg->words[i]);
        if (wlen > MAX_WORD_LEN)
            return NULL;

        full_len += 1 + WORD_BYTES + wlen; // delim + 2 len bytes + content
    }

    full_msg = malloc((full_len+1) * sizeof(char));

    // header
    memcpy(full_msg, header, HEADER_LEN);
    write_p = full_msg + HEADER_LEN;

    // Role & type
    *write_p = DELIM;
    write_p[1] = msg->role;
    write_p[2] = msg->type;
    write_p += 1 + BYTE_FIELDS_LEN;

    // word cnt
    *write_p = DELIM;
    write_p[1] = msg->cnt;
    write_p += 1 + WORD_CNT_BYTES;

    // Content
    for (size_t i = 0; i < msg->cnt; i++) {
        char *word = msg->words[i];
        size_t wlen = strlen(word);

        *write_p = DELIM; // delim before word
        write_p++;

        for (size_t j = WORD_BYTES-1; j >= 0; j--) { // length in fixed bytes
            write_p[j] = wlen % BYTE_POT;
            wlen /= BYTE_POT;
        }

        write_p += WORD_BYTES;

        memcpy(write_p, word, wlen); // contents
        write_p += wlen;
    }
        
    // End char and string delimiter
    *write_p = ENDC;
    write_p[1] = '\0';

    return full_msg;
}

void p_init_reader(p_message_reader *reader)
{
    reader->state = rs_header;
    reader->header_match_idx = 0;
    reader->wlen_bytes_read = 0;
    reader->msg = p_create_message(r_unknown, t_unknown);
    reader->cur_word = NULL;
    reader->wlen = 0;
    reader->wcap = 0;
}

void p_clear_reader(p_message_reader *reader)
{
    reader->state = rs_empty;
    reader->header_match_idx = 0;
    reader->wlen_bytes_read = 0;
    reader->wlen = 0;
    reader->wcap = 0;
    if (reader->msg) {
        p_free_message(reader->msg);
        reader->msg = NULL;
    }
    if (reader->cur_word) {
        free(reader->cur_word);
        reader->cur_word = NULL;
    }
}

int p_reader_is_live(p_message_reader *reader)
{
    return reader->state != rs_empty && reader->msg != NULL;
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

#define MAKE_TRY_GET_BYTE_FIELD(TYPE, NUM_ITEMS) \
    int try_get_ ## TYPE ## _from_byte(char r, p_ ## TYPE *out) \
    { \
        for (size_t i = 0; i < NUM_ITEMS; i++) { \
            if (r == valid_ ## TYPE ##s[i]) { \
                *out = valid_ ## TYPE ##s[i]; \
                return 1; \
            } \
        } \
        return 0; \
    }

#define MAKE_TRY_PARSE_BYTE_FIELD(TYPE, NEXT_STATE) \
    size_t parse_ ## TYPE(p_message_reader *reader, const char *str, size_t len) \
    { \
        if (len >= 1) { \
            reader->state = try_get_ ## TYPE ## _from_byte(*str, &reader->msg->TYPE) ? \
                            NEXT_STATE : rs_error; \
            return 1; \
        } \
        return 0; \
    }

MAKE_TRY_GET_BYTE_FIELD(role, NUM_ROLES)
MAKE_TRY_PARSE_BYTE_FIELD(role, rs_type)

MAKE_TRY_GET_BYTE_FIELD(type, NUM_TYPES)
MAKE_TRY_PARSE_BYTE_FIELD(type, rs_content)

size_t parse_word_cnt(p_message_reader *reader, const char *str, size_t len)
{
    if (*str
}

void store_reader_cur_word(p_message_reader *reader)
{
    reader->current_content_len += reader->wlen + 1;
    reader->cur_word[reader->wlen] = '\0';
    add_word_to_words(reader->msg, reader->cur_word);
    free(reader->cur_word);
}

void store_and_recreate_reader_cur_word(p_message_reader *reader)
{
    if (reader->cur_word)
        store_reader_cur_word(reader);

    reader->wcap = WORD_BASE_CAP;
    reader->wlen = 0;
    reader->cur_word = malloc(reader->wcap * sizeof(char));
    *reader->cur_word = '\0';
}

size_t parse_content(p_message_reader *reader, const char *str, size_t len)
{
    size_t chars_read = 0;
    if (len == 0)
        return 0;

    for (; chars_read < len; chars_read++) {
        char c = str[chars_read];
        if (reader->wlen_bytes_read == -1) {
            if (c == ENDC) {
                reader->state = rs_finished;
                break;
            } else if (c != DELIM) {
                reader->state = rs_error;
                break;
            } else
                reader->wlen_bytes_read = 0;
        } else if (reader->wlen_bytes_read < WORD_BYTES

        if (tot_len > reader->msg->w_total_len) {
            reader->state = rs_error;
            break;
        } else if (tot_len == reader->msg->w_total_len) {
            if (c != ENDC) {
                reader->state = rs_error;
                break;
            }

            store_reader_cur_word(reader);
            reader->cur_word = NULL;

            reader->state = rs_finished;
            break;
        } else if (c == DELIM)
            store_and_recreate_reader_cur_word(reader);
        else {
            reader->cur_word[reader->wlen] = c;
            reader->wlen++;

            try_resize_reader_cur_word(reader);
        }
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
            case rs_cnt:
                chars_read = parse_word_cnt(reader, str, len);
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
