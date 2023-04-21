/* bbs/server.c */
#include "utils.h"
#include <sys/select.h>
#include <stdio.h>
#include <errno.h>

enum { listen_queue_len = 16 };

int main(int argc, char **argv)
{
    int ls = -1;
    struct sockaddr_in addr;
    char *endptr;
    long port;

    if (argc != 3) {
        fprintf(stderr, "Usage: <port> <database directory>\n");
        return 1;
    }

    port = strtol(argv[1], &endptr, 10);
    if (!*argv[1] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return 1;
    }

    ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == -1) {
        perror("socket");
        return 2;
    }

    // For debugging purposes
    int opt = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int ok = bind(ls, (struct sockaddr *) &addr, sizeof(addr));
    if (ok == -1) {
        perror("bind");
        return_defer(2);
    }

    listen(ls, listen_queue_len);

    // @TODO: should I daemonize the server?
    // @TODO: data structure for all sessions

    for (;;) {
        fd_set readfds, writefds;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(ls, &readfds);

        // @TODO: select-based serving cycle
    }

defer:
    if (ls == -1) close(ls);
    return 0;
}
