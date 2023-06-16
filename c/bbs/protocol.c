/* bbs/protocol.c */
#include "protocol.h"
#include "constants.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>

#define DELIM ':'
#define ENDC ','

// @TODO: I should cap max total message len, 4 megs sounds unreasonable

enum { 
    BYTE_POT = 256,
    WORD_LEN_BYTES = 2,
    MAX_MSG_WORD_LEN = MAX_WORD_LEN, // < 256**WORD_LEN_BYTES
    WORD_CNT_BYTES = 1,
    MAX_WORD_CNT = 255, // 256**WORD_CNT_BYTES - 1

    MESSAGE_BASE_CAP = 64,
    WORD_BASE_CAP = 16,

    HEADER_LEN = 6,
    BYTE_FIELDS_LEN = 2,
    MAX_DIGITS = 32, // overkill

    NUM_ROLES = 2,
    NUM_TYPES = 13
};

const char header[HEADER_LEN+1] = "BBS232";

const p_role valid_roles[NUM_ROLES] = { r_server, r_client };
const p_type valid_types[NUM_TYPES] = {
    ts_init, tc_login, ts_login_success, ts_login_failed,
    tc_list_files, ts_file_list_response, tc_file_query, ts_file_not_found,
    ts_file_restricted, ts_start_file_transfer, ts_file_packet,
    tc_leave_message, ts_message_done
};

static p_message *create_empty_message()
{
    p_message *msg = malloc(sizeof(p_message));
    msg->role = r_unknown;
    msg->type = t_unknown;
    msg->cnt = 0;
    msg->cap = 0;
    msg->words = NULL;
    return msg;
}

p_message *p_create_message(p_role role, p_type type)
{
    p_message *msg = create_empty_message();
    msg->role = role;
    msg->type = type;
    msg->cap = MESSAGE_BASE_CAP;
    msg->words = malloc(msg->cap * sizeof(char *));
    return msg;
}

void p_free_message(p_message *msg)
{
    for (size_t i = 0; i < msg->cnt; i++)
        free(msg->words[i]);
    free(msg->words);
    free(msg);
}

int p_add_word_to_message(p_message *msg, const char *word)
{
    return add_string_to_string_array(&msg->words, word, 
                                      &msg->cnt, &msg->cap, 
                                      MESSAGE_BASE_CAP, MAX_MSG_WORD_LEN);
}

p_sendable_message p_construct_sendable_message(p_message *msg)
{
    p_sendable_message smsg;
    char *write_p;

    smsg.str = NULL;
    smsg.len = 0;

    // header + delim + role&type bytes + delim + word cnt + ... + end char
    smsg.len = HEADER_LEN + 1 + BYTE_FIELDS_LEN + 1 + WORD_CNT_BYTES + 1;

    for (size_t i = 0; i < msg->cnt; i++) {
        size_t wlen = strlen(msg->words[i]);
        if (wlen > MAX_MSG_WORD_LEN)
            return smsg;

        smsg.len += 1 + WORD_LEN_BYTES + wlen; // delim + 2 len bytes + content
    }

    smsg.str = malloc((smsg.len+1) * sizeof(char));

    // header
    memcpy(smsg.str, header, HEADER_LEN);
    write_p = smsg.str + HEADER_LEN;

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

        size_t wlen_tmp = wlen;
        for (int j = WORD_LEN_BYTES-1; j >= 0; j--) { // little endian
            write_p[j] = wlen_tmp % BYTE_POT;
            wlen_tmp /= BYTE_POT;
        }

        write_p += WORD_LEN_BYTES;

        memcpy(write_p, word, wlen); // contents
        write_p += wlen;
    }

    // End char and string delimiter
    *write_p = ENDC;
    write_p++;

    return smsg;
}

void p_deinit_sendable_message(p_sendable_message *msg)
{
    if (msg->str) free(msg->str);
    msg->str = NULL;
    msg->len = 0;
}

void p_init_reader(p_message_reader *reader)
{
    reader->state = rs_header;
    reader->header_match_idx = 0;
    reader->int_bytes_read = -1;

    reader->msg = create_empty_message();

    reader->cur_word = NULL;
    reader->wlen = 0;
    reader->wcap = 0;
}

void p_deinit_reader(p_message_reader *reader)
{
    reader->state = rs_empty;
    reader->header_match_idx = 0;
    reader->int_bytes_read = -1;
    reader->wlen = 0;
    reader->wcap = 0;
    if (reader->msg) {
        if (reader->msg->words)
            p_free_message(reader->msg);
        else
            free(reader->msg);
        reader->msg = NULL;
    }
    if (reader->cur_word) {
        free(reader->cur_word);
        reader->cur_word = NULL;
    }
}

void p_reset_reader(p_message_reader *reader)
{
    // @TODO: optimize
    p_deinit_reader(reader);
    p_init_reader(reader);
}

int p_reader_is_live(p_message_reader *reader)
{
    return reader->state != rs_empty && reader->msg != NULL;
}

size_t match_header(p_message_reader *reader, const char *str, size_t len)
{
    size_t chars_read;
    for (chars_read = 0; chars_read < len; chars_read++) {
        char c = str[chars_read];
        char hc = header[reader->header_match_idx];
        if (hc == '\0') {
            reader->state = c == DELIM ? rs_role : rs_error;
            chars_read++;
            break;
        } else if (hc != c) {
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
MAKE_TRY_PARSE_BYTE_FIELD(type, rs_cnt)

// @HACK
int char_to_uint_byte_val(char c)
{
    int ic = (int) c;
    if (ic < 0) ic += BYTE_POT;
    return ic;
}

size_t parse_cnt(p_message_reader *reader, const char *str, size_t len)
{
    size_t chars_read;
    for (chars_read = 0; chars_read < len; chars_read++) {
        char c = str[chars_read];
        if (reader->int_bytes_read == -1) {
            if (c == DELIM)
                reader->int_bytes_read++;
            else {
                reader->state = rs_error;
                break;
            }
        } else if (reader->int_bytes_read < WORD_CNT_BYTES) {
            int ic = char_to_uint_byte_val(c);
            reader->msg->cap *= BYTE_POT;
            reader->msg->cap += (size_t) ic;
            reader->int_bytes_read++;

            if (reader->msg->cap > MAX_WORD_CNT) {
                reader->state = rs_error;
                break;
            }

            if (reader->int_bytes_read == WORD_CNT_BYTES) {
                reader->msg->words = malloc(reader->msg->cap * sizeof(char *));
                
                reader->int_bytes_read = -1;
                reader->state = rs_content;
                chars_read++;
                break;
            }
        }
    }

    return chars_read;
}

void store_reader_cur_word(p_message_reader *reader)
{
    if (reader->cur_word) {
        if (reader->msg->cnt >= reader->msg->cap) {
            reader->state = rs_error;
            return;
        }

        reader->cur_word[reader->wlen] = '\0';
        p_add_word_to_message(reader->msg, reader->cur_word);
        free(reader->cur_word);
    }

    reader->wcap = 0;
    reader->wlen = 0;
    reader->cur_word = NULL;
}

void process_delim_or_endc(p_message_reader *reader, char c)
{
    if (reader->int_bytes_read != -1) {
        reader->state = rs_error;
        return;
    }

    store_reader_cur_word(reader);
    
    if (c == ENDC)
        reader->state = reader->msg->cnt == reader->msg->cap ? rs_finished : rs_error;
    else if (c == DELIM)
        reader->int_bytes_read = 0;
}

size_t parse_content(p_message_reader *reader, const char *str, size_t len)
{
    size_t chars_read;
    if (len == 0)
        return 0;

    for (chars_read = 0; chars_read < len; chars_read++) {
        char c = str[chars_read];

        if (reader->int_bytes_read == -1) {
            if (c == ENDC || c == DELIM)
                process_delim_or_endc(reader, c);
            else
                reader->state = rs_error;

            if (reader->state != rs_content) {
                chars_read++;
                break;
            }
        } else if (reader->int_bytes_read < WORD_LEN_BYTES) {
            int ic = char_to_uint_byte_val(c);
            reader->wcap *= BYTE_POT;
            reader->wcap += (size_t) ic;
            reader->int_bytes_read++;

            if (reader->wcap > MAX_MSG_WORD_LEN) {
                reader->state = rs_error;
                break;
            }
        } else {
            if (reader->wlen >= reader->wcap) {
                reader->state = rs_error;
                break;
            } else if (!reader->cur_word) {
                reader->wcap++; // for '\0'
                reader->cur_word = malloc(reader->wcap * sizeof(char));
            }

            reader->cur_word[reader->wlen] = c;
            reader->wlen++;

            if (reader->wlen >= reader->wcap - 1)
                reader->int_bytes_read = -1;
        }
    }

    return chars_read;
}

int p_reader_process_str(p_message_reader *reader, 
                         const char *str, size_t len, size_t *chars_processed)
{
    if (reader->state == rs_empty || reader->state == rs_error)
        return -1;

    int result = 0;
    
    *chars_processed = 0;
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
                chars_read = parse_cnt(reader, str, len);
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

        str += chars_read;
        len -= chars_read;

        *chars_processed += chars_read;

        if (reader->state == rs_finished) {
            result = 1;
            break;
        }

        if (reader->state == rs_error) {
            result = -1;
            break;
        }
    }

    return result;
}
