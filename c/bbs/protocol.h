/* bbs/protocol.h */
#ifndef PROTOCOL_SENTRY
#define PROTOCOL_SENTRY

#include "utils.h"
#include "types.h"
#include <stddef.h>

/*
 * Message format: 
 * header:role_byte type_byte:word_cnt(byte):word:word:...:word,
 * where word is: len(2 bytes since <= 65535) content 
*/ 

typedef enum p_role_tag { 
    r_unknown,
    r_server, 
    r_client
} p_role;

typedef enum p_type_tag { 
    t_unknown,

    ts_init,
    tc_login,
    ts_login_success,
    ts_login_poster,
    ts_login_admin,
    ts_login_failed,

    tc_list_files,
    ts_file_list_response,

    tc_file_check,
    ts_file_exists,
    ts_file_not_found,

    tc_file_query,
    ts_file_restricted,
    ts_start_file_transfer,
    ts_file_packet,
    
    tc_leave_note,
    ts_note_done,

    tc_post_file,
    tc_post_file_packet,

    tc_user_check,
    ts_user_exists,
    ts_user_does_not_exist,
    tc_add_user,
    ts_user_added,

    tc_ask_for_next_note,
    ts_show_and_rm_note,
    ts_no_notes_left,

    tc_edit_file_meta,
    ts_file_edit_done,

    tc_delete_file,
    ts_file_deleted,
    ts_cant_delete_file
} p_type;

typedef struct p_message_tag {
    p_role role;
    p_type type;
    byte_arr *words;
    size_t cnt, cap;
    size_t tot_w_len;
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

typedef enum p_reader_processing_res_tag {
    rpr_done, rpr_in_progress, rpr_error
} p_reader_processing_res;

p_message *p_create_message(p_role role, p_type type);
void p_free_message(p_message *msg);

// Returns 0 if word too long or the word cnt is capped out, I do not use it
// since in this application I do not approach the limits
int p_add_word_to_message(p_message *msg, const char *word, size_t len);
int p_add_string_to_message(p_message *msg, const char *str);

p_sendable_message p_construct_sendable_message(p_message *msg);
void p_deinit_sendable_message(p_sendable_message *msg);

void p_init_reader(p_message_reader *reader);
void p_deinit_reader(p_message_reader *reader);
void p_reset_reader(p_message_reader *reader);
int p_reader_is_live(p_message_reader *reader);
p_reader_processing_res p_reader_process_str(p_message_reader *reader, 
                                             const char *str, size_t len,
                                             size_t *chars_processed);

p_type user_type_to_p_type(user_type ut);
user_type p_type_to_user_type(p_type pt);

#endif
