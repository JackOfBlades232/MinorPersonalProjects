/* bbs/protocol.h */
#ifndef PROTOCOL_SENTRY
#define PROTOCOL_SENTRY

#include "utils.h"
#include <stddef.h>

/*
 * Message format: 
 * header:role_byte type_byte:word_cnt(byte):word:word:...:word,
 * where word is: len(2 bytes since <= 65535) content 
*/ 

typedef enum p_role_tag { 
    r_unknown = 0,
    r_server = 1, 
    r_client = 2
} p_role;

typedef enum p_type_tag { 
    t_unknown = 0,
    ts_init = 1,
    tc_login = 2,
    ts_login_success = 3,
    ts_login_failed = 4,
    tc_list_files = 5,
    ts_file_list_response = 6,
    tc_file_query = 7,
    ts_file_not_found = 8,
    ts_file_restricted = 9,
    ts_start_file_transfer = 10,
    ts_file_packet = 11,
    /* ts_last_file_packet // should this exist? */
    tc_leave_message = 12,
    ts_message_done = 13
} p_type;

typedef struct p_message_tag {
    p_role role;
    p_type type;
    char **words;
    size_t cnt, cap;
} p_message;

typedef byte_arr p_sendable_message;

typedef enum p_read_state_tag { 
    rs_empty,
    rs_error,
    rs_header,
    rs_role,
    rs_type,
    rs_cnt,
    rs_content,
    rs_finished
} p_read_state;

typedef struct p_message_reader_tag {
    p_read_state state;
    size_t header_match_idx;
    int int_bytes_read; // for cnt/word len, -1 : not read delim yet
    char *cur_word;
    size_t wlen, wcap;
    p_message *msg;
} p_message_reader;

p_message *p_create_message(p_role role, p_type type);
void p_free_message(p_message *msg);
int p_add_word_to_message(p_message *msg, const char *word);
p_sendable_message p_construct_sendable_message(p_message *msg);
void p_deinit_sendable_message(p_sendable_message *msg);

void p_init_reader(p_message_reader *reader);
void p_deinit_reader(p_message_reader *reader);
void p_reset_reader(p_message_reader *reader);
int p_reader_is_live(p_message_reader *reader);
int p_reader_process_str(p_message_reader *reader, 
                         const char *str, size_t len, size_t *chars_processed);

#endif
