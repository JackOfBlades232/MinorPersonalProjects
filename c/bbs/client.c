/* bbc/client.c */
#include "protocol.h"
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
    LOGIN_BUFSIZE = 64
};

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
    char *str = p_construct_sendable_message(msg);
    write(sock, str, strlen(str));
    free(str);
}

void disable_echo(struct termios *bkp_ts)
{ 
    struct termios ts;
    tcgetattr(STDIN_FILENO, &ts);
    memcpy(bkp_ts, &ts, sizeof(ts));
    ts.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);
}

int await_server_message()
{
    size_t read_res;
    int parse_res = 0;

    // @TODO: sort out trailing data read (do we really have to?)
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
    p_clear_reader(&reader);
    return result;
}

void strip_nl(char *str)
{
    for (; *str && *str != '\n' && *str != '\r'; str++) {}
    *str = '\0';
}

int check_spc(const char *str)
{
    for (; *str && *str != ' ' && *str != '\t'; str++) {}
    return *str;
}

int send_login_credentials() 
{
    p_message *msg;
    char usernm[LOGIN_BUFSIZE], passwd[LOGIN_BUFSIZE];
    struct termios ts;

    msg = p_create_message(r_client, tc_login);

    printf("Username: ");
    fgets(usernm, sizeof(usernm), stdin);
    strip_nl(usernm);    
    if (check_spc(usernm))
        return 0;
    p_add_word_to_message(msg, usernm);

    disable_echo(&ts);
    printf("Password: ");
    fgets(passwd, sizeof(passwd), stdin);
    strip_nl(passwd);    
    if (check_spc(passwd))
        return 0;
    p_add_word_to_message(msg, passwd);

    putchar('\n');
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);

    send_message(msg);
    p_free_message(msg);
    return 1;
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
    if (!send_login_credentials())
        return_defer(-1);

    // @TEST
    p_init_reader(&reader);
    int res = await_server_message();
    if (!res)
        return_defer(-1);

    if (
            reader.msg->role == r_server && 
            reader.msg->cnt == 0 &&
            (
                reader.msg->type == ts_login_success ||
                reader.msg->type == ts_login_failed
            )
       ) {
        printf(reader.msg->type == ts_login_success ? "Logged in\n" : "Invalid login/passwd\n");
    } else {
        // @PLACEHOLDER
        fprintf(stderr, "Some bullshit this is\n");
        return_defer(-1);
    }
    p_clear_reader(&reader);

defer:
    if (p_reader_is_live(&reader)) p_clear_reader(&reader);
    if (sock != -1) close(sock);
    return result;
}
