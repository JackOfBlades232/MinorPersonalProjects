/* bbs/debug.c */
#include "debug.h"

#include <string.h>

enum {
    MAX_FILE_LEN_FOR_DISPLAY = 65535
};

void debug_log_p_role(FILE* f, p_role role)
{
    switch (role) {
        case r_unknown:
            fprintf(f, "unknown");
            break;
        case r_server:
            fprintf(f, "server");
            break;
        case r_client:
            fprintf(f, "client");
            break;
        default:
            fprintf(f, "ROLE %d MISSING, IMPLEMENT", role);
    }
}

void debug_log_p_type(FILE* f, p_type type)
{
    switch (type) {
        case t_unknown:
            fprintf(f, "unknown");
            break;
        case ts_init:
            fprintf(f, "s-init");
            break;
        case tc_login:
            fprintf(f, "c-login");
            break;
        case ts_login_success:
            fprintf(f, "s-login-success");
            break;
        case ts_login_failed:
            fprintf(f, "s-login-failed");
            break;
        case tc_list_files:
            fprintf(f, "c-list-files");
            break;
        case ts_file_list_response:
            fprintf(f, "s-file-list-response");
            break;
        case tc_file_query:
            fprintf(f, "c-file-query");
            break;
        case ts_file_not_found:
            fprintf(f, "s-file-not-found");
            break;
        case ts_file_restricted:
            fprintf(f, "s-file-restricted");
            break;
        case ts_start_file_transfer:
            fprintf(f, "s-start-file-transfer");
            break;
        case ts_file_packet:
            fprintf(f, "s-file-packet");
            break;
        case tc_leave_message:
            fprintf(f, "c-leave-message");
            break;
        case ts_message_done:
            fprintf(f, "s-message-done");
            break;
        default:
            fprintf(f, "TYPE %d MISSING, IMPLEMENT", type);
    }
}

void debug_log_p_message(FILE* f, p_message *msg)
{
    if (!msg) {
        fprintf(f, "DEBUG: message is null\n");
        return;
    }

    fprintf(f, "DEBUG: message\n");

    fprintf(f, "DEBUG: role: ");
    debug_log_p_role(f, msg->role);
    fprintf(f, " type: ");
    debug_log_p_type(f, msg->type);
    fputc('\n', f);

    fprintf(f, "DEBUG: words[%ld]: ", msg->cnt);
    for (size_t i = 0; i < msg->cnt; i++)
        fprintf(f, "[%ld] %s", strlen(msg->words[i]), msg->words[i]);
    fputc('\n', f);
}

void debug_cat_file(FILE* f, const char *filename)
{
    FILE *file;
    if (!filename) {
        fprintf(f, "DEBUG: filename is null\n");
        return;
    }

    fprintf(f, "DEBUG: cat file %s\n", filename);

    file = fopen(filename, "r");
    if (!file) 
        fprintf(f, "DEBUG: can't open file\n");
    else {
        fseek(file, 0, SEEK_END);
        long len = ftell(file);
        rewind(file);
        fprintf(f, "DEBUG: byte length: %ld\n", len);

        if (len > MAX_FILE_LEN_FOR_DISPLAY)
            fprintf(f, "DEBUG: too long to display\n");
        else {
            int c;
            while ((c = fgetc(file)) != EOF)
                fputc(c, f);
            fprintf(f, "DEBUG: eof\n");
        }
    }
}

void debug_print_buf(FILE *f, const char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        fputc(buf[i], f);
    fputc('\n', f);
}
