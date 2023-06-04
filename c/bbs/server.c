/* bbs/server.c */
#include "protocol.h"
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

static const char passwd_rel_path[] = "/passwd.txt";

typedef enum session_state_tag { 
    sstate_init, 
    sstate_idle, 
    sstate_finish, 
    sstate_error 
} session_state;

typedef struct session_tag {
    int fd;
    unsigned int from_ip;
    unsigned short from_port;
    char buf[INBUFSIZE];
    int buf_used;
    p_message_reader in_reader;
    session_state state;
} session;

typedef struct server_tag {
    int ls;
    session **sessions;
    int sessions_size;
} server;

// Global server vars
static server serv = {0};
static FILE *passwd_f = NULL;

void session_send_msg(session *sess, p_message *msg)
{
    // @TODO: will there be long messages so as to add writes to select?
    char *msg_str = p_construct_sendable_message(msg);
    write(sess->fd, msg_str, strlen(msg_str));
    free(msg_str);
}

session *make_session(int fd,
        unsigned int from_ip, unsigned short from_port)
{
    session *sess = malloc(sizeof(session));
    sess->fd = fd;
    sess->from_ip = ntohl(from_ip);
    sess->from_port = ntohs(from_port);
    sess->buf_used = 0;
    sess->state = sstate_init;

    // Protocol step one: state that it is a bbs server
    p_message *msg = p_create_message(r_server, ts_init);
    session_send_msg(sess, msg);
    p_free_message(msg);

    p_init_reader(&sess->in_reader); // For login recieving

    return sess;
}

int try_match_passwd_line(const char *usernm, const char *passwd, int *last_c)
{
    int c;
    int failed = 0;
    int matched_usernm = 0,
        matched_passwd = 0;
    size_t usernm_len = strlen(usernm),
           passwd_len = strlen(passwd);
    size_t usernm_idx = 0,
           passwd_idx = 0;

    while ((c = fgetc(passwd_f)) != EOF) {
        if (c == '\n')
            break;
        if (failed)
            continue;

        if (!matched_usernm) {
            if (usernm_idx >= usernm_len) {
                if (c == ' ')
                    matched_usernm = 1;
                else
                    failed = 1;
            } else if (usernm[usernm_idx] == c)
                usernm_idx++;
            else
                failed = 1;
        } else if (!matched_passwd) {
            if (passwd_idx < passwd_len && passwd[passwd_idx] == c)
                passwd_idx++;
            else
                failed = 1;
        }
    }

    if (!failed && matched_usernm && passwd_idx == passwd_len)
        matched_passwd = 1;

    *last_c = c;
    return matched_usernm && matched_passwd;
}

void session_handle_login(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    if (msg->role != r_client || msg->type != tc_login || msg->cnt != 2) {
        sess->state = sstate_error;
        return;
    }

    int matched = 0;
    int last_c = '\n';
    while (last_c != EOF) {
        matched = try_match_passwd_line(msg->words[0], msg->words[1], &last_c);
        if (matched)
            break;
    }

    msg = p_create_message(r_server, matched ? ts_login_success : ts_login_failed);
    session_send_msg(sess, msg);
    p_free_message(msg);
        
    p_clear_reader(&sess->in_reader);
    sess->state = sstate_finish; // @TODO: Idle/finished

    rewind(passwd_f);
    return;
}

int session_read(session *sess)
{
    int rc, bufp = sess->buf_used;
    rc = read(sess->fd, sess->buf + bufp, INBUFSIZE-bufp);
    if (rc <= 0) {
        sess->state = sstate_error;
        return 0;
    }
    sess->buf_used += rc;
    
    // @TEST
    int parse_res = p_reader_process_str(&sess->in_reader, sess->buf, sess->buf_used);
    sess->buf_used = 0;

    if (parse_res == -1)
        sess->state = sstate_error;
    else if (parse_res == 0)
        goto defer;

    switch (sess->state) {
        case sstate_init:
            session_handle_login(sess);
            break;

        case sstate_idle:
            break;

        default:
            break;
    }

defer:
    return sess->state != sstate_finish &&
           sess->state != sstate_error;
}

int server_init(int port)
{
    int sock, opt;
    struct sockaddr_in addr;

    serv.ls = -1;
    serv.sessions = NULL;

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
    serv.ls = sock;

    serv.sessions = calloc(INIT_SESS_ARR_SIZE, sizeof(*serv.sessions));
    serv.sessions_size = INIT_SESS_ARR_SIZE;

    return 1;
}

void server_accept_client()
{
    int sd, i;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    sd = accept(serv.ls, (struct sockaddr *) &addr, &len);
    if (sd == -1) {
        perror("accept");
        return;
    }

    if (sd >= serv.sessions_size) { // resize if needed
        int newsize = serv.sessions_size;
        while (newsize <= sd)
            newsize += INIT_SESS_ARR_SIZE;
        serv.sessions = 
            realloc(serv.sessions, newsize * sizeof(*serv.sessions));
        for (i = serv.sessions_size; i < newsize; i++)
            serv.sessions[i] = NULL;
        serv.sessions_size = newsize;
    }

    serv.sessions[sd] = make_session(sd, addr.sin_addr.s_addr, addr.sin_port);
}

void server_close_session(int sd)
{
    close(sd);
    free(serv.sessions[sd]);
    serv.sessions[sd] = NULL;
}

void server_deinit()
{
    if (serv.ls != -1) 
        close(serv.ls);
    if (serv.sessions) {
        for (int sd = 0; sd < serv.sessions_size; sd++) {
            if (serv.sessions[sd])
                server_close_session(sd);
        }
        free(serv.sessions);
    }
}

int db_init(const char *path)
{
    size_t path_len = strlen(path);
    if (path[path_len-1] == '/')
        path_len--;

    size_t passwd_rel_path_len = strlen(passwd_rel_path);
    size_t full_path_len = path_len+passwd_rel_path_len;
    char *passwd_path = malloc((full_path_len+1) * sizeof(char));

    memcpy(passwd_path, path, path_len);
    memcpy(passwd_path+path_len, passwd_rel_path, passwd_rel_path_len);
    passwd_path[full_path_len] = '\0';

    passwd_f = fopen(passwd_path, "r");
    if (!passwd_f)
        return 0;

    // @TODO: database format check

    return 1;
}

void db_deinit()
{
    if (passwd_f) fclose(passwd_f);
}

int main(int argc, char **argv)
{
    int result = 0;

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

    if (!db_init(argv[2])) {
        fprintf(stderr, "Invalid db folder\n");
        return 1;
    }
    if (!server_init(port)) {
        fprintf(stderr, "Server init failed\n");
        return_defer(2);
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
            return_defer(-1);
        }

        if (FD_ISSET(serv.ls, &readfds))
            server_accept_client();
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i] && FD_ISSET(i, &readfds)) {
                int ssr = session_read(serv.sessions[i]);
                if (!ssr)
                    server_close_session(i);
            }
        }
    }

defer:
    server_deinit();
    db_deinit();
    return result;
}
