/* bbc/client.c */
#include "protocol.h"
#include "constants.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "debug.h"

enum {
    SERV_READ_BUFSIZE = 2048,
    ACTION_BUFSIZE = 32,
    NUM_ACTIONS = 4
};

typedef enum client_action_tag {
    log_in,
    list_files,
    query_file,
    leave_message
} client_action;

static const client_action all_actions[NUM_ACTIONS] = {
    log_in, list_files, query_file, leave_message
};

static const char *action_names[NUM_ACTIONS] = {
    "login", "list", "query", "message"
};

// @TODO: add privileged client actions

// Global client state
static int sock = -1;
static char serv_read_buf[SERV_READ_BUFSIZE];
static size_t serv_buf_used = 0;
static p_message_reader reader = {0};
static int logged_in = 0;

static char *last_queried_filename = NULL;

void send_message(p_message *msg)
{
    p_sendable_message smsg = p_construct_sendable_message(msg);
    write(sock, smsg.str, smsg.len);
    p_deinit_sendable_message(&smsg);
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

void discard_stdin()
{
    while (getchar() != '\n') {};
}

int try_get_client_action_by_name(const char *name, client_action *out)
{
    int result = 0;
    int i;
    for (i = 0; i < NUM_ACTIONS; i++) {
        if (strings_are_equal(action_names[i], name)) {
            result = 1;
            break;
        }
    }

    if (result)
        *out = all_actions[i];
    return result;
}

int action_to_p_type(client_action action)
{
    switch (action) {
        case log_in:
            return tc_login;
        case list_files:
            return tc_list_files;
        case query_file:
            return tc_file_query;
        case leave_message:
            return tc_leave_message;
        default:
            return t_unknown;
    }
}

int await_server_message()
{
    size_t read_res = 0;
    int parse_res = 0;

    while (
            serv_buf_used > 0 ||
            (read_res = read(sock, serv_read_buf, sizeof(serv_read_buf))) > 0
          ) {
        size_t chars_processed;
        serv_buf_used += read_res;
        parse_res = p_reader_process_str(&reader, 
                                         serv_read_buf, serv_buf_used, 
                                         &chars_processed);
        if (chars_processed < serv_buf_used) {
            memmove(serv_read_buf, 
                    serv_read_buf + chars_processed, 
                    serv_buf_used - chars_processed);
        }

        serv_buf_used -= chars_processed;
        read_res = 0;

        if (parse_res != 0)
            break;
    }

    return parse_res == 1;
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
    if (!await_server_message())
        return_defer(0);

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

int ask_for_credential_item(p_message *msg, const char *dialogue)
{
    char cred[MAX_LOGIN_ITEM_LEN];

    fputs(dialogue, stdout);
    fgets(cred, sizeof(cred)-1, stdin);
    if (!strip_nl(cred)) {
        printf("Login item loo long\n");
        return 0;
    }
    if (check_spc(cred)) {
        printf("Spaces are not allowed in login item\n");
        return 0;
    }

    p_add_string_to_message(msg, cred);
    return 1;
}

int login_dialogue() 
{
    int result = 1;

    p_message *msg;
    struct termios ts;

    if (logged_in) {
        printf("\nYou are already logged in\n");
        return 0;
    }

    msg = p_create_message(r_client, tc_login);

    if (!ask_for_credential_item(msg, "\nUsername: "))
        return_defer(0);

    disable_echo(&ts);
    int passwd_res = ask_for_credential_item(msg, "Password: ");
    putchar('\n');
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);

    if (!passwd_res)
        return_defer(0);

    send_message(msg);

defer:
    p_free_message(msg);
    return result;
}

int query_file_dialogue()
{
    int result = 1;
    
    char filename[MAX_FILENAME_LEN+1];
    printf("\nInput file name: ");
    fgets(filename, sizeof(filename)-1, stdin);
    if (!strip_nl(filename)) {
        printf("Filename is too long\n");
        return 0;
    }

    p_message *msg = p_create_message(r_client, tc_file_query);
    p_add_string_to_message(msg, filename);

    if (last_queried_filename) free(last_queried_filename);
    last_queried_filename = strdup(filename);

    send_message(msg);

    if (msg) p_free_message(msg);
    return result;
}

int leave_message_dialogue()
{
    int result = 1;
    
    char message[MAX_MESSAGE_LEN+1];

    if (!logged_in) {
        printf("\nLog in to leave messages\n");
        return 0;
    }

    printf("\nInput message: ");
    fgets(message, sizeof(message)-1, stdin);
    if (!strip_nl(message)) {
        printf("Message is too long\n");
        return 0;
    }

    p_message *msg = p_create_message(r_client, tc_leave_message);
    p_add_string_to_message(msg, message);

    send_message(msg);

    if (msg) p_free_message(msg);
    return result;
}

int perform_action(client_action action)
{
    p_type type = action_to_p_type(action);

    switch (action) {
        case log_in:
            return login_dialogue();

        case list_files:
            send_empty_message(type);
            return 1;

        case query_file:
            return query_file_dialogue();

        case leave_message:
            return leave_message_dialogue();

        default:
            fprintf(stderr, "Not implemented\n");
    }

    return 0;
}

// @TODO: factor this apart
int parse_action_response(client_action action)
{
    if (reader.msg->role != r_server) 
        return 0;

    switch (action) {
        case log_in:
            if (
                    reader.msg->cnt != 0 || 
                    (
                     reader.msg->type != ts_login_success &&
                     reader.msg->type != ts_login_failed
                    )
               ) {
                fprintf(stderr, "Invalid log_in server response\n");
                return 0;
            }
            
            if (reader.msg->type == ts_login_success) {
                logged_in = 1;
                printf("\nLogged in\n");
            } else
                printf("Invalid username/password, please try again\n");

            return 1;

        case list_files:
            if (reader.msg->type != ts_file_list_response) {
                fprintf(stderr, "Invalid list_files server response\n");
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

        case query_file:
            if (reader.msg->type == ts_file_not_found)
                printf("File not found\n");
            else if (reader.msg->type == ts_file_restricted)
                printf("File is restricted for this account\n");
            else if (
                    reader.msg->type == ts_start_file_transfer &&
                    reader.msg->cnt == 1
                    ) {
                char *e;
                long packets_left = strtol(reader.msg->words[0].str, &e, 10);
                if (*e != '\0' || packets_left <= 0) {
                    fprintf(stderr, "Invalid num packets in query_file server response\n");
                    return 0;
                }

                p_reset_reader(&reader);

                FILE *f = fopen(last_queried_filename, "w"); // @TODO: check for null? again, should not happen
                if (!f) {
                    // @TODO: if can't save, send something back as a rejection
                    fprintf(stderr, "Can't save the file\n");
                    return 0;
                }
                while (packets_left > 0 && await_server_message()) {
                    if (
                            reader.msg->type != ts_file_packet ||
                            reader.msg->cnt != 1
                       ) {
                        fprintf(stderr, "Recieved invalid packet\n");
                        break;
                    }
                        
                    byte_arr content = reader.msg->words[0];
                    fwrite(content.str, sizeof(*content.str), content.len, f);
                    packets_left--;

                    p_reset_reader(&reader);
                }


                fclose(f);
                if (packets_left > 0)
                    fprintf(stderr, "Failed to recieve all packets\n");
                else
                    printf("Download complete\n");
                return packets_left == 0;
            } else {
                fprintf(stderr, "Invalid query_file server response\n");
                return 0;
            }

            return 1;

        case leave_message:
            if (reader.msg->type == ts_message_done && reader.msg->cnt == 0) {
                printf("Message sent\n");
                return 1;
            } else {
                fprintf(stderr, "Invalid leave_message server response\n");
                return 0;
            }

        default:
            printf("Not implemented\n");
            return 1;
    }

    return 0;
}

int ask_for_action()
{
    int result = 1;

    char action_buf[ACTION_BUFSIZE];    
    client_action action;

    printf("\nAvailable actions: ");
    for (int i = 0; i < NUM_ACTIONS; i++) {
        if (i == 0)
            printf("%s", action_names[i]);
        else
            printf(", %s", action_names[i]);
    }
    putchar('\n');
    printf("Input action: ");

    fgets(action_buf, sizeof(action_buf)-1, stdin);
    if (!strip_nl(action_buf)) {
        discard_stdin();
        printf("Action name is too long\n");
        return 0;
    }
    if (!try_get_client_action_by_name(action_buf, &action)) {
        printf("Invalid action name\n");
        return 0;
    }

    p_init_reader(&reader);
    if (perform_action(action)) { // If could not perform action in dialogue, continue
        if (                      // If server message was invalid or unparsable, stop client
                !await_server_message() ||
                !parse_action_response(action)
           ) {
            result = 0;
        }
    }
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
        fprintf(stderr, "Launch this from a tty\n");
        return_defer(1);
    }

    if (argc != 3) {
        fprintf(stderr, "Usage: <ip> <port>\n");
        return_defer(1);
    }

    serv_addr.sin_family = AF_INET;
    port = strtol(argv[2], &endptr, 10);
    if (!*argv[2] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return_defer(1);
    }
    serv_addr.sin_port = htons(port);
    if (!inet_aton(argv[1], &serv_addr.sin_addr)) {
        fprintf(stderr, "Provide valid server ip-address\n");
        return_defer(1);
    }

    if (!connect_to_server(serv_addr)) {
        fprintf(stderr, "Failed to connect to server\n");
        return_defer(2);
    }

    // @TEST
    int action_res;
    while ((action_res = ask_for_action()) != -1) {}

defer:
    if (sock != -1) close(sock);
    return result;
}

