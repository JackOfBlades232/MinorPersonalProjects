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

enum {
    SERV_READ_BUFSIZE = 128,
    ACTION_BUFSIZE = 32,
    NUM_ACTIONS = 3
};

typedef enum client_action_tag { // @TODO: add log in action
    list_files,
    query_file,
    leave_message
} client_action;

static const client_action all_actions[NUM_ACTIONS] = {
    list_files, query_file, leave_message
};

static const char *action_names[NUM_ACTIONS] = {
    "list", "query", "message"
};

// @TODO: add privileged client actions

static const char title[] = 
    "/////////////////////////////////////////////////\n"
    "///////////   WELCOME TO DUMMY-BBS!   ///////////\n"
    "/////////////////////////////////////////////////\n";

// Global client state
int sock = -1;
static char serv_read_buf[SERV_READ_BUFSIZE];
static p_message_reader reader = {0};

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
        if (strcmp(action_names[i], name) == 0) {
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
    size_t read_res;
    int parse_res = 0;

    while ((read_res = read(sock, serv_read_buf, sizeof(serv_read_buf))) > 0) {
        parse_res = p_reader_process_str(&reader, serv_read_buf, read_res);
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
    int res = await_server_message();
    if (!res)
        return_defer(0);

    if (
            reader.msg->role == r_server && 
            reader.msg->type == ts_init && 
            reader.msg->cnt == 0
       ) {
        return_defer(1);
    }

defer:
    p_deinit_reader(&reader);
    return result;
}

int strip_nl(char *str)
{
    int result;
    for (; *str && *str != '\n' && *str != '\r'; str++) {}
    result = *str != '\0';
    if (result) *str = '\0';
    return result;
}

int check_spc(const char *str)
{
    for (; *str && *str != ' ' && *str != '\t'; str++) {}
    return *str;
}

int ask_for_credential_item(p_message *msg, const char *dialogue)
{
    char cred[MAX_LOGIN_ITEM_LEN];

    fputs(dialogue, stdout);
    fgets(cred, sizeof(cred), stdin);
    if (!strip_nl(cred) || check_spc(cred))
        return 0;

    return p_add_word_to_message(msg, cred);
}

int send_login_credentials() 
{
    int result = 1;

    p_message *msg;
    struct termios ts;

    msg = p_create_message(r_client, tc_login);

    if (!ask_for_credential_item(msg, "Username: "))
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

int log_in()
{
    int result;

    p_init_reader(&reader);
    if (!send_login_credentials() || !await_server_message())
        return_defer(-1);

    if (
            reader.msg->role == r_server && 
            reader.msg->cnt == 0 &&
            (
             reader.msg->type == ts_login_success ||
             reader.msg->type == ts_login_failed
            )
       ) {
        result = reader.msg->type == ts_login_success ? 1 : 0;
    } else
        result = -1;

defer:
    p_deinit_reader(&reader);
    return result;
}

int perform_action(client_action action)
{
    p_type type = action_to_p_type(action);

    switch (action) {
        case list_files:
            send_empty_message(type);
            return 1;
        default:
            printf("Not implemented\n");
    }

    return 0;
}

int parse_action_response(client_action action)
{
    p_type type = action_to_p_type(action);

    if (reader.msg->role != r_server || reader.msg->type != type) 
        return 0;

    switch (action) {
        case list_files:
            // @TEST
            printf("Response:");
            for (size_t i = 0; i < reader.msg->cnt; i++)
                printf(" %s", reader.msg->words[i]);
            putchar('\n');

            return 1;
        default:
            printf("Not implemented\n");
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
        return 0;
    }
    if (!try_get_client_action_by_name(action_buf, &action))
        return 0;

    p_init_reader(&reader);
    if (
            !perform_action(action) ||
            !await_server_message() ||
            !parse_action_response(action)
       ) {
        result = 0;
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
        return 1;
    }

    if (argc != 3) {
        fprintf(stderr, "Usage: <ip> <port>\n");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    port = strtol(argv[2], &endptr, 10);
    if (!*argv[2] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return 1;
    }
    serv_addr.sin_port = htons(port);
    if (!inet_aton(argv[1], &serv_addr.sin_addr)) {
        fprintf(stderr, "Provide valid server ip-address\n");
        return 1;
    }

    if (!connect_to_server(serv_addr)) {
        fprintf(stderr, "Failed to connect to server\n");
        return 2;
    }

    printf("%s\n", title);

    int login_res;
    while ((login_res = log_in()) == 0)
        printf("\nInvalid username/password, please try again.\n\n");
    if (login_res == -1) {
        fprintf(stderr, "Login failed\n");
        return_defer(-1);
    }

    printf("\nLogged in\n");

    // @TEST
    for (;;)
        ask_for_action();

defer:
    if (sock != -1) close(sock);
    return result;
}

