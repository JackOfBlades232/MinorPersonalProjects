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
    MSG_BUFSIZE = 128
};

static const char title[] = 
    "/////////////////////////////////////////////////\n"
    "///////////   WELCOME TO DUMMY-BBS!   ///////////\n"
    "/////////////////////////////////////////////////\n";

void send_message(int fd, const char *msg)
{
    write(fd, msg, strlen(msg));
}

int match_message_from_fd(int fd, const char *req_msg)
{
    int read_res;
    int msg_comp_idx;
    char msg_buf[MSG_BUFSIZE];

    msg_comp_idx = 0;
    while ((read_res = read(fd, msg_buf, sizeof(msg_buf))) > 0) {
        for (int i = 0; i < read_res; i++) {
            if (msg_buf[i] != req_msg[msg_comp_idx])
                break;

            msg_comp_idx++;
            if (req_msg[msg_comp_idx] == '\0')
                return 1;
        }
    }

    return 0;
}

void disable_echo(struct termios *bkp_ts)
{ 
    struct termios ts;
    tcgetattr(STDIN_FILENO, &ts);
    memcpy(bkp_ts, &ts, sizeof(ts));
    ts.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);
}

int connect_to_server(struct sockaddr_in serv_addr)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return -1;

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        return -1;

    // Init protocol steps (to establish that it is indeed my bbs)
    if (match_message_from_fd(sock, server_init_msg))
        return sock;

    return -1;
}

void strip_nl(char *str)
{
    for (; *str && *str != '\n' && *str != '\r'; str++) {}
    *str = '\0';
}

int log_in(int sock) 
{
    struct login_info li;
    struct termios ts;

    printf("Username: ");
    fgets(li.usernm, sizeof(li.usernm), stdin);
    strip_nl(li.usernm);    

    disable_echo(&ts);
    printf("Password: ");
    fgets(li.passwd, sizeof(li.passwd), stdin);
    strip_nl(li.passwd);    
    putchar('\n');
    tcsetattr(STDIN_FILENO, TCSANOW, &ts);

    // @TEST
    printf("%s, %s\n", li.usernm, li.passwd);
    write(sock, client_init_response, strlen(client_init_response));
    return 1;
}

int main(int argc, char **argv)
{
    char *endptr;
    long port;
    struct sockaddr_in serv_addr;
    
    int sock = -1;

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

    sock = connect_to_server(serv_addr);
    if (sock == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        return 2;
    }

    printf("%s\n", title);
    log_in(sock);    

defer:
    if (sock != -1) close(sock);
    return 0;
}
