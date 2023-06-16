/* bbs/debug.c */
#include "debug.h"

#include <string.h>

enum {
    MAX_FILE_LEN_FOR_DISPLAY = 65535
};

void debug_log_p_role(p_role role)
{
    switch (role) {
        case r_unknown:
            fprintf(stderr, "unknown");
            break;
        case r_server:
            fprintf(stderr, "server");
            break;
        case r_client:
            fprintf(stderr, "client");
            break;
        default:
            fprintf(stderr, "ROLE %d MISSING, IMPLEMENT", role);
    }
}

void debug_log_p_type(p_type type)
{
    switch (type) {
        case t_unknown:
            fprintf(stderr, "unknown");
            break;
        case ts_init:
            fprintf(stderr, "s-init");
            break;
        case tc_login:
            fprintf(stderr, "c-login");
            break;
        case ts_login_success:
            fprintf(stderr, "s-login-success");
            break;
        case ts_login_failed:
            fprintf(stderr, "s-login-failed");
            break;
        case tc_list_files:
            fprintf(stderr, "c-list-files");
            break;
        case ts_file_list_response:
            fprintf(stderr, "s-file-list-response");
            break;
        case tc_file_query:
            fprintf(stderr, "c-file-query");
            break;
        case ts_file_not_found:
            fprintf(stderr, "s-file-not-found");
            break;
        case ts_file_restricted:
            fprintf(stderr, "s-file-restricted");
            break;
        case ts_start_file_transfer:
            fprintf(stderr, "s-start-file-transfer");
            break;
        case ts_file_packet:
            fprintf(stderr, "s-file-packet");
            break;
        case tc_leave_message:
            fprintf(stderr, "c-leave-message");
            break;
        case ts_message_done:
            fprintf(stderr, "s-message-done");
            break;
        default:
            fprintf(stderr, "TYPE %d MISSING, IMPLEMENT", type);
    }
}

void debug_log_p_message(p_message *msg)
{
    if (!msg) {
        fprintf(stderr, "DEBUG: message is null\n");
        return;
    }

    fprintf(stderr, "DEBUG: message\n");

    fprintf(stderr, "DEBUG: role: ");
    debug_log_p_role(msg->role);
    fprintf(stderr, " type: ");
    debug_log_p_type(msg->type);
    fputc('\n', stderr);

    fprintf(stderr, "DEBUG: words[%ld]:", msg->cnt);
    for (size_t i = 0; i < msg->cnt; i++)
        fprintf(stderr, " [%ld] %s", msg->words[i].len, msg->words[i].str);
    fputc('\n', stderr);
}

void debug_cat_file(const char *filename)
{
    FILE *file;
    if (!filename) {
        fprintf(stderr, "DEBUG: filename is null\n");
        return;
    }

    fprintf(stderr, "DEBUG: cat file %s\n", filename);

    file = fopen(filename, "r");
    if (!file) 
        fprintf(stderr, "DEBUG: can't open file\n");
    else {
        fseek(file, 0, SEEK_END);
        long len = ftell(file);
        rewind(file);
        fprintf(stderr, "DEBUG: byte length: %ld\n", len);

        if (len > MAX_FILE_LEN_FOR_DISPLAY)
            fprintf(stderr, "DEBUG: too long to display\n");
        else {
            int c;
            while ((c = fgetc(file)) != EOF)
                fputc(c, stderr);
            fprintf(stderr, "DEBUG: eof\n");
        }
    }
}

void debug_print_buf(const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        fputc(buf[i], stderr);
    fputc('\n', stderr);
}
