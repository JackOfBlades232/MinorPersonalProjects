/* bbs/server.c */
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

enum { 
    LISTEN_QLEN = 16,
    INIT_SESS_ARR_SIZE = 32,
    INBUFSIZE = 128
};

enum session_state { sstate_idle, sstate_finish, sstate_error };

struct session {
    int fd;
    unsigned int from_ip;
    unsigned short from_port;
    char buf[INBUFSIZE];
    int buf_used;
    enum session_state state;
};

void session_send_msg(struct session *sess, const char *str)
{
    // messages are small strings => no need to wait for writefds, just send
    write(sess->fd, str, strlen(str));
}

struct session *make_session(int fd,
        unsigned int from_ip, unsigned short from_port)
{
    struct session *sess = malloc(sizeof(struct session));
    sess->fd = fd;
    sess->from_ip = ntohl(from_ip);
    sess->from_port = ntohs(from_port);
    sess->buf_used = 0;
    sess->state = sstate_idle;

    // @TODO: do something to start dialogue? Placeholder for now
    session_send_msg(sess, "Hullo\n");

    return sess;
}

int session_read(struct session *sess)
{
    int rc, bufp = sess->buf_used;
    rc = read(sess->fd, sess->buf + bufp, INBUFSIZE-bufp);
    if (rc <= 0) {
        sess->state = sstate_error;
        return 0;
    }
    sess->buf_used += rc;
    
    // @TODO: impl msg parsing logic with states, now placeholder
    for (; sess->buf_used > 0; sess->buf_used--) {
        if (sess->buf[sess->buf_used] == '\n') {
            session_send_msg(sess, "NL Boi\n");
            sess->state = sstate_finish;
        }
    }

    return sess->state != sstate_finish;
}

struct server {
    int ls;
    struct session **sessions;
    int sessions_size;
};

int server_init(struct server *serv, int port)
{
    int sock, opt;
    struct sockaddr_in addr;

    serv->ls = -1;
    serv->sessions = NULL;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 0;
    }

    opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(&opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        return 0;
    }

    listen(sock, LISTEN_QLEN);
    serv->ls = sock;

    serv->sessions = calloc(INIT_SESS_ARR_SIZE, sizeof(*serv->sessions));
    serv->sessions_size = INIT_SESS_ARR_SIZE;

    return 1;
}

void server_accept_client(struct server *serv)
{
    int sd, i;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    sd = accept(serv->ls, (struct sockaddr *) &addr, &len);
    if (sd == -1) {
        perror("accept");
        return;
    }

    if (sd >= serv->sessions_size) { // resize if needed
        int newsize = serv->sessions_size;
        while (newsize <= sd)
            newsize += INIT_SESS_ARR_SIZE;
        serv->sessions = 
            realloc(serv->sessions, newsize * sizeof(*serv->sessions));
        for (i = serv->sessions_size; i < newsize; i++)
            serv->sessions[i] = NULL;
        serv->sessions_size = newsize;
    }

    serv->sessions[sd] = make_session(sd, addr.sin_addr.s_addr, addr.sin_port);
}

void server_close_session(struct server *serv, int sd)
{
    close(sd);
    free(serv->sessions[sd]);
    serv->sessions[sd] = NULL;
}

void server_deinit(struct server *serv)
{
    if (serv->ls != -1) 
        close(serv->ls);
    if (serv->sessions) {
        for (int sd = 0; sd < serv->sessions_size; sd++) {
            if (serv->sessions[sd])
                server_close_session(serv, sd);
        }
        free(serv->sessions);
    }
}

int main(int argc, char **argv)
{
    int result = 0;

    struct server serv;
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

    if (!server_init(&serv, port)) {
        fprintf(stderr, "Server init failed\n");
        return 2;
    }

    // @TODO: should I daemonize the server?
    // @TODO: init database from folder
    // @TODO: data structure for all sessions

    for (;;) {
        fd_set readfds, writefds;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(serv.ls, &readfds);

        int maxfd = serv.ls;
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i]) {
                FD_SET(i, &readfds);
                if (i > maxfd)
                    maxfd = i;
            }
        }

        // @TODO: impl writefds, since we may be sending large files
        int sr = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if (sr == -1) {
            perror("select");
            return -1;
        }

        if (FD_ISSET(serv.ls, &readfds))
            server_accept_client(&serv);
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i] && FD_ISSET(i, &readfds)) {
                int ssr = session_read(serv.sessions[i]);
                if (!ssr)
                    server_close_session(&serv, i);
            }
        }
    }

defer:
    server_deinit(&serv);
    return result;
}
