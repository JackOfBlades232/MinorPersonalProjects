/* bbs/server.c */
#include "database.h"
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

#include "debug.h"

enum { 
    LISTEN_QLEN = 16,
    INIT_SESS_ARR_SIZE = 32,
    INBUFSIZE = 128
};

typedef enum session_state_tag { 
    sstate_init, 
    sstate_await, 
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
static database db = {0};

void session_send_msg(session *sess, p_message *msg)
{
    // @TODO: will there be long messages so as to add writes to select?
    p_sendable_message smsg = p_construct_sendable_message(msg);
    write(sess->fd, smsg.str, smsg.len);
    p_deinit_sendable_message(&smsg);
}

void session_send_empty_message(session *sess, p_type type) 
{
    p_message *msg = p_create_message(r_server, type);
    session_send_msg(sess, msg);
    p_free_message(msg);
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
    session_send_empty_message(sess, ts_init);

    p_init_reader(&sess->in_reader); // For login recieving

    return sess;
}

void session_handle_login(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    if (msg->role != r_client || msg->type != tc_login || msg->cnt != 2) {
        sess->state = sstate_error;
        return;
    }

    int matched = try_match_credentials(&db, msg->words[0], msg->words[1]);
    session_send_empty_message(sess, matched ? ts_login_success : ts_login_failed);

    if (matched)
        sess->state = sstate_await; 
        
    p_reset_reader(&sess->in_reader);
    return;
}

void session_parse_regular_message(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    p_message *response = NULL;
    if (msg->role != r_client) {
        sess->state = sstate_error;
        return;
    }

    switch (msg->type) {
        case tc_list_files:
            response = p_create_message(r_server, ts_file_list_response);

            // @TEST
            for (file_metadata **fmd = db.file_metas; *fmd; fmd++) {
                char *name_and_descr = concat_strings((*fmd)->name, "\n", (*fmd)->descr, NULL);
                p_add_word_to_message(response, name_and_descr);
                free(name_and_descr);
            }

            break;

        case tc_file_query:
            if (msg->cnt != 1) {
                sess->state = sstate_error;
                break;
            }

            char *filename = lookup_file(&db, msg->words[0]);
            debug_cat_file(stderr, filename);

            // @TODO: check user access
            if (filename) {
                response = p_create_message(r_server, ts_start_file_transfer);
                free(filename);
            } else
                response = p_create_message(r_server, ts_file_not_found);

            break;

        default:
            sess->state = sstate_error;
    }

    if (response) {
        session_send_msg(sess, response);
        p_free_message(response);
    }

    p_reset_reader(&sess->in_reader);
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

        case sstate_await:
            session_parse_regular_message(sess);
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

    if (!db_init(&db, argv[2])) {
        fprintf(stderr, "Could not parse database in folder\n");
        return 1;
    }
    if (!server_init(port)) {
        fprintf(stderr, "Server init failed\n");
        return_defer(2);
    }

    // @TODO: should I daemonize the server?

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
    db_deinit(&db);
    return result;
}
