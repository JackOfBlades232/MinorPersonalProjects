/* bbc/client.c */
#include "protocol.h"
#include "constants.h"
#include "utils.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>

#include "debug.h"

enum {
    SERV_READ_BUFSIZE = 2048,
    ACTION_BUFSIZE = 32,
    DOWNLOAD_PLACK_NUM_BUFSIZE = 128,

    SERV_AWAIT_TIMEOUT_SEC = 12
};

#define EPSILON 0.000001
#define DOWNLOAD_PROGRESS_BAR_VAL 0.1 + EPSILON

typedef enum client_action_tag {
    log_in,
    list_files,
    query_file,
    leave_note,
    exit_client,
    post_file,
    add_user,
    read_note,
    edit_file_meta,
    delete_file
} client_action;

static const client_action reg_actions[] = {
    log_in, list_files, query_file, leave_note, exit_client
};
#define NUM_REG_ACTIONS sizeof(reg_actions)/sizeof(*reg_actions)
static const char *reg_action_names[NUM_REG_ACTIONS] = {
    "login", "list", "query", "note", "exit"
};

static const client_action poster_actions[] = { post_file };
#define NUM_POSTER_ACTIONS sizeof(poster_actions)/sizeof(*poster_actions)
static const char *poster_action_names[NUM_POSTER_ACTIONS] = { "post" };

static const client_action admin_actions[] = {
    add_user, read_note, edit_file_meta, delete_file
};
#define NUM_ADMIN_ACTIONS sizeof(admin_actions)/sizeof(*admin_actions)
static const char *admin_action_names[NUM_ADMIN_ACTIONS] = {
    "add user", "read next note", "edit", "delete"
};

typedef enum perform_action_result_tag {
    par_ok,
    par_continue,
    par_error
} perform_action_result; 

typedef enum await_server_msg_result_tag {
    asr_ok, 
    asr_incomplete, 
    asr_process_err, 
    asr_read_err, 
    asr_timeout,
    asr_disconnected
} await_server_msg_result;

// Global client state
static int sock = -1;

static char serv_read_buf[SERV_READ_BUFSIZE];
static size_t serv_buf_used = 0;
static p_message_reader reader = {0};

static user_type login_user_type = ut_none;
static char *last_queried_filename = NULL;
static await_server_msg_result last_await_res = asr_ok;

static int cur_fd = -1;

int send_message(p_message *msg)
{
    p_sendable_message smsg = p_construct_sendable_message(msg);
    int wc = write(sock, smsg.str, smsg.len);
    p_deinit_sendable_message(&smsg);
    return wc;
}

void send_empty_message(p_type type) 
{
    p_message *msg = p_create_message(r_client, type);
    send_message(msg);
    p_free_message(msg);
}

void disable_echo(struct termios *bkp_ts)
{ 
    struct termios ts;
    tcgetattr(STDIN_FILENO, &ts);
    memcpy(bkp_ts, &ts, sizeof(ts));
    ts.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);
}

int search_word_arr(const char **arr, size_t size, const char *query)
{
    for (int i = 0; i < size; i++) {
        if (strings_are_equal(arr[i], query))
            return i;
    }

    return -1;
}

void output_word_arr(const char **arr, size_t size)
{
    for (int i = 0; i < size; i++) {
        if (i == 0)
            printf("%s", arr[i]);
        else
            printf(", %s", arr[i]);
    }
}

int try_get_client_action_by_name(const char *name, client_action *out)
{
    user_type req_type = ut_none;
    int i = search_word_arr(reg_action_names, NUM_REG_ACTIONS, name); 
    if (i >= 0)
        *out = reg_actions[i];

    if (i == -1) {
        req_type = ut_poster;
        i = search_word_arr(poster_action_names, NUM_POSTER_ACTIONS, name);
        if (i >= 0)
            *out = poster_actions[i];
    }

    if (i == -1) {
        req_type = ut_admin;
        i = search_word_arr(admin_action_names, NUM_ADMIN_ACTIONS, name);
        if (i >= 0)
            *out = admin_actions[i];
    }

    if (i != -1 && req_type <= login_user_type) {
        return 1;
    }

    return 0;
}

int await_server_message()
{
    int sel_res = 1;
    size_t read_res = 1;

    p_reader_processing_res pr_res = rpr_in_progress;

    while (pr_res == rpr_in_progress) {
        if (serv_buf_used == 0) {
            fd_set readfds;
            struct timeval timeout;

            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            
            timeout.tv_sec = SERV_AWAIT_TIMEOUT_SEC;
            timeout.tv_usec = 0;

            sel_res = select(sock+1, &readfds, NULL, NULL, &timeout);
            if (sel_res <= 0) // error or timeout
                break;

            read_res = read(sock, serv_read_buf, sizeof(serv_read_buf));
            if (read_res <= 0) // error or disconnect
                break;

            serv_buf_used += read_res;
        }

        size_t chars_processed;

        pr_res = p_reader_process_str(&reader, serv_read_buf, serv_buf_used, &chars_processed);
        if (chars_processed < serv_buf_used) {
            memmove(serv_read_buf, 
                    serv_read_buf + chars_processed, 
                    serv_buf_used - chars_processed);
        }

        serv_buf_used -= chars_processed;
    }

    if (sel_res == -1 || read_res == -1)
        last_await_res = asr_read_err;
    else if (sel_res == 0)
        last_await_res = asr_timeout;
    else if (read_res == 0)
        last_await_res = asr_disconnected;
    else if (pr_res == rpr_error)
        last_await_res = asr_process_err;
    else if (pr_res == rpr_in_progress)
        last_await_res = asr_incomplete;
    else
        last_await_res = asr_ok;

    return last_await_res == asr_ok;
}

void log_await_error()
{
    switch (last_await_res) {
        case asr_incomplete:
            printf_err("Recieved incomplete message\n");
            break;
        case asr_process_err:
            printf_err("Failed to parse message\n");
            break;
        case asr_read_err:
            printf_err("Failed to select/read socket\n");
            break;
        case asr_timeout:
            printf("\nConnection timeout\n");
            break;
        case asr_disconnected:
            printf("\nYou have been disconnected\n");
            break;
        default:
            break;
    }
}

void log_await_error_with_caption(const char *caption)
{
    if (last_await_res == asr_timeout || last_await_res == asr_disconnected)
        printf("%s\n", caption);
    else
        printf_err("%s\n", caption);

    log_await_error();
}

int connect_to_server(struct sockaddr_in serv_addr)
{
    int result = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return 0;

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        return 0;

    p_init_reader(&reader);
    if (!await_server_message()) {
        log_await_error();
        return_defer(0);
    }

    if (
            reader.msg->role == r_server && 
            reader.msg->type == ts_init && 
            reader.msg->cnt == 1
       ) {
        printf("%s\n", reader.msg->words[0].str); // title
        return_defer(1);
    }

defer:
    p_deinit_reader(&reader);
    return result;
}

int try_read_item_from_stdin(char *buf, size_t bufsize, 
                             const char* prompt, const char *overflow_msg)
{
    fputs(prompt, stdout);
    fgets(buf, bufsize-1, stdin);
    if (!strip_nl(buf)) {
        discard_stdin();
        puts(overflow_msg);
        return 0;
    }

    return 1;
}

int ask_for_credential_item(p_message *msg, const char *dialogue)
{
    char cred[MAX_LOGIN_ITEM_LEN+2];

    if (!try_read_item_from_stdin(cred, sizeof(cred), dialogue, "Login item too long"))
        return 0;

    if (check_spc(cred)) {
        printf("Spaces are not allowed in login item\n");
        return 0;
    }

    p_add_string_to_message(msg, cred);
    return 1;
}

perform_action_result login_dialogue() 
{
    perform_action_result result = par_ok;

    p_message *msg;
    struct termios ts;

    if (login_user_type != ut_none) {
        printf("\nYou are already logged in\n");
        return par_continue;
    }

    msg = p_create_message(r_client, tc_login);

    if (!ask_for_credential_item(msg, "\nUsername: "))
        return_defer(par_continue);

    disable_echo(&ts);
    int passwd_res = ask_for_credential_item(msg, "Password: ");
    putchar('\n');
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);

    if (!passwd_res)
        return_defer(par_continue);

    send_message(msg);

defer:
    p_free_message(msg);
    return result;
}

perform_action_result query_file_dialogue()
{
    perform_action_result result = par_ok;
    char filename[MAX_FILENAME_LEN+2];

    if (!try_read_item_from_stdin(filename, sizeof(filename), "\nInput file name: ", "Filename is too long"))
        return par_continue;

    if (access(filename, W_OK) == -1) {
        int fd = open(filename, O_WRONLY | O_CREAT);
        if (fd == -1) {
            printf("Can't write to this file\n");
            return par_continue;
        } else {
            close(fd);
            unlink(filename);
        }
    }

    p_message *msg = p_create_message(r_client, tc_file_query);
    p_add_string_to_message(msg, stripped_filename(filename));

    if (last_queried_filename) free(last_queried_filename);
    last_queried_filename = strdup(filename);

    send_message(msg);

    if (msg) p_free_message(msg);
    return result;
}

perform_action_result leave_note_dialogue()
{
    perform_action_result result = par_ok;
    
    char note[MAX_NOTE_LEN+2];

    if (login_user_type == ut_none) {
        printf("\nLog in to leave notes\n");
        return par_continue;
    }

    if (!try_read_item_from_stdin(note, sizeof(note), "\nInput note: ", "Note is too long"))
        return par_continue;

    p_message *msg = p_create_message(r_client, tc_leave_note);
    p_add_string_to_message(msg, note);

    send_message(msg);

    p_free_message(msg);
    return result;
}

int draw_load_progress_bar(const char *prefix,
        long tot_packets, long packets_left, int prev_chars)
{
    for (int i = 0; i < prev_chars; i++)
        putchar('\b');

    int new_chars = printf("%s [", prefix);
    long packets_downloaded = tot_packets-packets_left;
    float fraction = (float) packets_downloaded / tot_packets;
    for (float pr = 0.; pr < 1.; pr += DOWNLOAD_PROGRESS_BAR_VAL) {
        if (pr < fraction)
            putchar('=');
        else
            putchar(' ');
        new_chars++;
    }

    new_chars += printf("] %ld/%ld packets", packets_downloaded, tot_packets);

    fflush(stdout);
    return new_chars;
}

perform_action_result post_file_dialogue()
{
    perform_action_result result = par_continue;

    char filename[MAX_FILENAME_LEN+2];
    if (!try_read_item_from_stdin(filename, sizeof(filename), "\nInput file name: ", "Filename is too long"))
        return par_continue;

    if (
            access(filename, F_OK) == -1 || // doesn't exist
            access(filename, R_OK) == -1    // or is not readable
       ) {
        printf("Can't read this file\n");
        return par_continue;
    }

    p_message *msg = p_create_message(r_client, tc_file_check);
    p_add_string_to_message(msg, stripped_filename(filename));
    send_message(msg);
    p_free_message(msg);

    if (!await_server_message()) {
        log_await_error_with_caption("Failed to check file existence");
        return par_error; // To terminate in ask for action
    }

    if (
            reader.msg->role != r_server ||
            (
             reader.msg->type != ts_file_exists &&
             reader.msg->type != ts_file_not_found
            ) ||
            reader.msg->cnt != 0
       ) {
        printf_err("Invalid file-check server response");
        return par_error; // To terminate in ask for action
    }

    if (reader.msg->type == ts_file_exists) {
        printf("A file with this name already exists\n");
        return par_continue;
    }

    msg = p_create_message(r_client, tc_post_file);
    p_add_string_to_message(msg, stripped_filename(filename));

    char descr[MAX_DESCR_LEN+2];
    if (!try_read_item_from_stdin(descr, sizeof(descr), "Input description: ", "Description is too long"))
        return_defer(par_continue);
    p_add_string_to_message(msg, descr);

    char users[MAX_USER_CNT*(MAX_LOGIN_ITEM_LEN+2)];
    if (!try_read_item_from_stdin(users, sizeof(users), "Input users that will have access to the file: ", "The list is too long"))
        return_defer(par_continue);

    char *users_p = users;
    size_t users_rem_len = strlen(users);
    char *usernm = NULL;
    size_t chars_read;
    int added_a_user = 0,
        for_all_users = 0;
    while ((usernm = extract_word_from_buf(users_p, users_rem_len, &chars_read)) != NULL) {
        if (strings_are_equal(usernm, all_users_symbol)) {
            if (added_a_user) 
                goto loop_err;
            for_all_users = 1;
        } else {
            if (for_all_users)
                goto loop_err;
            added_a_user = 1;
        }

        users_p += chars_read;
        users_rem_len -= chars_read;

        if (msg->cnt-2 >= MAX_USER_CNT)
            goto loop_err;

        p_add_string_to_message(msg, usernm);
        free(usernm);
        continue;

loop_err:
        printf("Invalid user access list\n");
        free(usernm);
        return_defer(par_continue);
    }

    cur_fd = open(filename, O_RDONLY);
    if (cur_fd == -1) {
        printf_err("failed to open file\n");
        return_defer(par_continue);
    }

    long len = lseek(cur_fd, 0, SEEK_END);
    lseek(cur_fd, 0, SEEK_SET);
    long packets_left = ((len-1) / MAX_WORD_LEN) + 1;

    char packets_left_digits[LONG_MAX_DIGITS+2];
    snprintf(packets_left_digits, sizeof(packets_left_digits)-1, "%ld", packets_left);
    packets_left_digits[sizeof(packets_left_digits)-1] = '\0';

    p_add_string_to_message(msg, packets_left_digits);
    send_message(msg);
    p_free_message(msg);
    msg = NULL;

    long tot_packets = packets_left;
    long last_draw_res = draw_load_progress_bar("Uploading", tot_packets, packets_left, 0);

    while (packets_left > 0) {
        p_message *out_msg = p_create_message(r_client, tc_post_file_packet);
        out_msg->cnt = 1;

        // Create the chunk directly in the message to avoid more copying;
        byte_arr *content = out_msg->words;
        size_t cap = MAX_WORD_LEN * sizeof(*content->str);
        content->str = malloc(cap);

        content->len = read(cur_fd, content->str, cap);
        out_msg->tot_w_len = content->len;

        int sr = send_message(out_msg);
        p_free_message(out_msg);

        if (sr <= 0) {
            printf("\nError while sending packet, might be disconnected\n");
            return_defer(par_error);
        }

        packets_left--;
        last_draw_res = draw_load_progress_bar("Uploading", tot_packets, packets_left, last_draw_res);
    }
    
    printf("\nUpload complete\n");

defer:
    if (msg) p_free_message(msg);
    if (cur_fd != -1) {
        close(cur_fd);
        cur_fd = -1;
    }
    return result;
}

perform_action_result add_user_dialogue()
{
    char usernm[MAX_LOGIN_ITEM_LEN+2];
    if (!try_read_item_from_stdin(usernm, sizeof(usernm), "\nInput the new user's username: ", "Username is too long"))
        return par_continue;
    if (check_spc(usernm)) {
        printf("Invalid username\n");
        return par_continue;
    }

    p_message *msg = p_create_message(r_client, tc_user_check);
    p_add_string_to_message(msg, usernm);
    send_message(msg);
    p_free_message(msg);

    if (!await_server_message()) {
        log_await_error_with_caption("Failed to check user existence");
        return par_error; // To terminate in ask for action
    }

    if (
            reader.msg->role != r_server ||
            (
             reader.msg->type != ts_user_exists &&
             reader.msg->type != ts_user_does_not_exist
            ) ||
            reader.msg->cnt != 0
       ) {
        printf_err("Invalid user-check server response");
        return par_error; // To terminate in ask for action
    }

    if (reader.msg->type == ts_user_exists) {
        printf("This user already exists\n");
        return par_continue;
    }

    char passwd[MAX_LOGIN_ITEM_LEN+2];
    if (!try_read_item_from_stdin(passwd, sizeof(passwd), "Input the new user's password: ", "Password is too long"))
        return par_continue;
    if (check_spc(passwd)) {
        printf("Invalid password\n");
        return par_continue;
    }

    printf("Input user type, one of [");
    output_word_arr(user_type_names, USER_TYPES_CNT);
    printf("]: ");

    char ut_name[MAX_USER_TYPE_LEN+2];
    if (!try_read_item_from_stdin(ut_name, sizeof(ut_name), "", "User type is too long"))
        return par_continue;
    int i = search_word_arr(user_type_names, USER_TYPES_CNT, ut_name);
    if (i == -1) {
        printf("Invalid user type\n");
        return par_continue;
    }

    msg = p_create_message(r_client, tc_add_user);
    p_add_string_to_message(msg, usernm);
    p_add_string_to_message(msg, passwd);
    if (real_user_types[i] == ut_poster)
        p_add_string_to_message(msg, poster_mark);
    else if (real_user_types[i] == ut_admin)
        p_add_string_to_message(msg, admin_mark);
    send_message(msg);
    p_free_message(msg);

    return par_ok;
}

perform_action_result perform_action(client_action action)
{
    switch (action) {
        case log_in:
            return login_dialogue();

        case list_files:
            send_empty_message(tc_list_files);
            return par_ok;

        case query_file:
            return query_file_dialogue();

        case leave_note:
            return leave_note_dialogue();

        case post_file:
            return post_file_dialogue();

        case add_user:
            return add_user_dialogue();

        default:
            printf_err("Not implemented\n");
    }

    return par_continue;
}

int process_login_response()
{
    if (
            reader.msg->cnt != 0 || 
            (
             reader.msg->type != ts_login_success &&
             reader.msg->type != ts_login_poster &&
             reader.msg->type != ts_login_admin &&
             reader.msg->type != ts_login_failed
            )
       ) {
        printf_err("Invalid log_in server response\n");
        return 0;
    }

    login_user_type = p_type_to_user_type(reader.msg->type);
    if (login_user_type == ut_none)
        printf("Invalid username/password, please try again\n");
    else 
        printf("\nLogged in\n");

    return 1;
}

int process_file_list_response()
{
    if (reader.msg->type != ts_file_list_response) {
        printf_err("Invalid list_files server response\n");
        return 0;
    }

    if (reader.msg->cnt > 0) {
        printf("\n%s", reader.msg->words[0].str);
        for (size_t i = 1; i < reader.msg->cnt; i++) {
            printf("\n\n%s", reader.msg->words[i].str);
        }
        putchar('\n');
    } else
        printf("\nNo available files\n");

    return 1;
}

int process_query_file_response()
{
    int result = 1;
    int fd = -1;

    if (reader.msg->type == ts_file_not_found) {
        printf("File not found\n");
        return_defer(1);
    } else if (reader.msg->type == ts_file_restricted) {
        printf("File is restricted now\n");
        return_defer(1);
    }

    if (
            reader.msg->type != ts_start_file_transfer ||
            reader.msg->cnt != 1
       ) {
        printf_err("Invalid query_file server response\n");
        return_defer(0);
    }

    char *e;
    long packets_left = strtol(reader.msg->words[0].str, &e, 10);
    if (*e != '\0' || packets_left <= 0) {
        printf_err("Invalid num packets in query_file server response\n");
        return_defer(0);
    }

    if (!last_queried_filename) {
        printf_err("Last queried filename is NULL, this is a bug\n");
        return_defer(0);
    }

    fd = open(last_queried_filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (fd == -1) {
        printf_err("Can't save the file\n");
        return_defer(0);
    }

    p_reset_reader(&reader);

    long tot_packets = packets_left;
    int last_draw_res = draw_load_progress_bar("Downloading", tot_packets, packets_left, 0);

    while (packets_left > 0 && await_server_message()) {
        if (
                reader.msg->type != ts_file_packet ||
                reader.msg->cnt != 1
           ) {
            printf_err("Recieved invalid packet, file is incomplete\n");
            return_defer(0);
        }

        byte_arr content = reader.msg->words[0];
        write(fd, content.str, content.len * sizeof(*content.str));
        packets_left--;

        last_draw_res = draw_load_progress_bar("Downloading", tot_packets, packets_left, last_draw_res);

        p_reset_reader(&reader);
    }

    putchar('\n');

    if (packets_left > 0) {
        log_await_error_with_caption("Failed to recieve all packets, file is incomplete");
        return_defer(0);
    } else
        printf("Download complete\n");

defer:
    if (fd != -1) close(fd);
    return result;
}

int process_leave_note_response()
{
    if (reader.msg->type == ts_note_done && reader.msg->cnt == 0) {
        printf("Note sent\n");
        return 1;
    } else {
        printf_err("Invalid leave_note server response\n");
        return 0;
    }
}

int process_add_user_response()
{
    if (reader.msg->type == ts_user_added && reader.msg->cnt == 0) {
        printf("\nUser added\n");
        return 1;
    } else {
        printf_err("Invalid add_user server response\n");
        return 0;
    }
}

int process_action_response(client_action action)
{
    if (reader.msg->role != r_server) 
        return 0;

    switch (action) {
        case log_in:
            return process_login_response();
        case list_files:
            return process_file_list_response();
        case query_file:    
            return process_query_file_response();
        case leave_note:
            return process_leave_note_response();
        case add_user:
            return process_add_user_response();

        default:
            printf("Not implemented\n");
            return 1;
    }

    return 0;
}

void output_available_actions()
{
    printf("\nAvailable actions: ");
    output_word_arr(reg_action_names, NUM_REG_ACTIONS);
    if (login_user_type >= ut_poster) {
        printf(" | ");
        output_word_arr(poster_action_names, NUM_POSTER_ACTIONS);
    }
    if (login_user_type >= ut_admin) {
        printf(" | ");
        output_word_arr(admin_action_names, NUM_ADMIN_ACTIONS);
    }

    putchar('\n');
}

int ask_for_action()
{
    int result = 1;

    char action_buf[ACTION_BUFSIZE];    
    client_action action;

    output_available_actions();

    if (!try_read_item_from_stdin(action_buf, sizeof(action_buf), "Input action: ", "Action name is too long"))
        return_defer(1);

    if (!try_get_client_action_by_name(action_buf, &action)) {
        printf("Invalid action name\n");
        return_defer(1);
    }
    
    if (action == exit_client) {
        printf("\nExiting\n");
        return_defer(0);
    }

    p_init_reader(&reader);

    // If could not perform action in dialogue, continue
    // If there was a critical error, break
    int perf_res = perform_action(action);
    if (perf_res == par_continue)
        return_defer(1);
    else if (perf_res == par_error)
        return_defer(0);

    p_reset_reader(&reader);

    // If server message was invalid or unparsable, stop client
    if (!await_server_message()) {
        log_await_error();
        return_defer(0);
    }
    if (!process_action_response(action)) {
        printf_err("Error while processing server response\n");
        return_defer(0);
    }

defer:
    p_deinit_reader(&reader);
    fflush(stdin);
    return result;
}

int main(int argc, char **argv)
{
    int result = 0;

    char *endptr;
    long port;
    struct sockaddr_in serv_addr;

    if (!isatty(STDIN_FILENO)) {
        printf_err("Launch this from a tty\n");
        return_defer(1);
    }

    if (argc != 3) {
        printf_err("Usage: <ip> <port>\n");
        return_defer(1);
    }

    serv_addr.sin_family = AF_INET;
    port = strtol(argv[2], &endptr, 10);
    if (!*argv[2] || *endptr) {
        printf_err("Invalid port number\n");
        return_defer(1);
    }
    serv_addr.sin_port = htons(port);
    if (!inet_aton(argv[1], &serv_addr.sin_addr)) {
        printf_err("Provide valid server ip-address\n");
        return_defer(1);
    }

    if (!connect_to_server(serv_addr)) {
        printf_err("Failed to connect to server\n");
        return_defer(2);
    }

    while (ask_for_action()) {}

defer:
    if (sock != -1) close(sock);
    return result;
}

