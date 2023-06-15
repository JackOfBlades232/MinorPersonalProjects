/* bbs/server.c */
#include "database.h"
#include "protocol.h"
#include "constants.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
    sstate_await, 
    sstate_file_transfer, 
    sstate_finish, 
    sstate_error 
} session_state;

typedef struct session_tag {
    int fd;
    unsigned int from_ip;
    unsigned short from_port;

    char in_buf[INBUFSIZE];
    size_t in_buf_used;

    char *out_buf;
    size_t out_buf_sent, out_buf_len;

    session_state state;
    char *usernm;

    p_message_reader in_reader;

    FILE *cur_f;
    int packets_left;
} session;

typedef struct server_tag {
    int ls;
    session **sessions;
    size_t sessions_size;
} server;

static const char title[] = 
    "/////////////////////////////////////////////////\n"
    "///////////   WELCOME TO DUMMY-BBS!   ///////////\n"
    "/////////////////////////////////////////////////\n";

// Global server vars
static server serv = {0};
static database db = {0};

void session_send_msg(session *sess, p_message *msg)
{
    p_sendable_message smsg = p_construct_sendable_message(msg);
    //write(sess->fd, smsg.str, smsg.len);
    //p_deinit_sendable_message(&smsg);
    
    // @TODO: what if last out_buf was not sent fully? (though it is my responsibility not to let this happen)
    sess->out_buf = smsg.str;
    sess->out_buf_len = smsg.len;
    sess->out_buf_sent = 0;
}

void session_send_empty_message(session *sess, p_type type) 
{
    p_message *msg = p_create_message(r_server, type);
    session_send_msg(sess, msg);
    p_free_message(msg);
}

void session_send_init_message(session *sess)
{
    p_message *msg = p_create_message(r_server, ts_init);
    p_add_word_to_message(msg, title);
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
    sess->in_buf_used = 0;
    sess->out_buf = NULL;
    sess->out_buf_sent = 0;
    sess->out_buf_len = 0;
    sess->state = sstate_await;
    sess->usernm = NULL; // Not logged in
    sess->cur_f = NULL;
    sess->packets_left = 0; // Not logged in

    // Protocol step one: state that it is a bbs server
    session_send_init_message(sess);
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

    char *usernm = msg->words[0],
         *passwd = msg->words[1];

    int matched = try_match_credentials(&db, usernm, passwd);
    session_send_empty_message(sess, matched ? ts_login_success : ts_login_failed);

    if (matched)
        sess->usernm = strdup(usernm);
        
    p_reset_reader(&sess->in_reader);
    return;
}

void session_send_file_list(session *sess)
{
    p_message *response = p_create_message(r_server, ts_file_list_response);

    for (file_metadata **fmd = db.file_metas; *fmd; fmd++) {
        if (file_is_available_to_user(*fmd, sess->usernm)) {
            char *name_and_descr = concat_strings((*fmd)->name, "\n", (*fmd)->descr, NULL);
            p_add_word_to_message(response, name_and_descr);
            free(name_and_descr);
        }
    }

    session_send_msg(sess, response);
    p_free_message(response);
}

void session_process_file_query(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    p_message *response;

    if (msg->cnt != 1) {
        sess->state = sstate_error;
        return;
    }

    char *filename = NULL;
    file_lookup_result lookup_res = lookup_file(&db, msg->words[0], 
                                                sess->usernm, &filename);
    
    // @TEST
    debug_cat_file(stderr, filename);

    if (lookup_res == found) {
        sess->cur_f = fopen(filename, "r");
        if (!sess->cur_f) {
            sess->state = sstate_error;
            goto defer;
        }

        fseek(sess->cur_f, 0, SEEK_END);
        long len = ftell(sess->cur_f);
        rewind(sess->cur_f);
        sess->packets_left = ((len-1) / MAX_WORD_LEN) + 1;

        char word[sizeof(len)+1];
        *((long *) word) = len;
        word[sizeof(len)] = '\0';

        response = p_create_message(r_server, ts_start_file_transfer);
        p_add_word_to_message(response, word);

        sess->state = sstate_file_transfer;
    } else {
        response = p_create_message(r_server, 
                                    lookup_res == no_access ? 
                                    ts_file_restricted : 
                                    ts_file_not_found);
    }

    session_send_msg(sess, response);
    p_free_message(response);

defer:
    if (filename) free(filename);
}

void session_parse_regular_message(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    if (msg->role != r_client) {
        sess->state = sstate_error;
        return;
    }

    switch (msg->type) {
        case tc_login:
            session_handle_login(sess);
            break;
        case tc_list_files:
            session_send_file_list(sess);
            break;
        case tc_file_query:
            session_process_file_query(sess);
            break;
        default:
            sess->state = sstate_error;
    }
}

int session_read(session *sess)
{
    int rc, bufp = sess->in_buf_used;
    rc = read(sess->fd, sess->in_buf + bufp, INBUFSIZE-bufp);
    if (rc <= 0) {
        sess->state = sstate_error;
        return 0;
    }
    sess->in_buf_used += rc;
    
    while (sess->in_buf_used > 0) {
        int parse_res = p_reader_process_str(&sess->in_reader, 
                                             sess->in_buf, &sess->in_buf_used);

        if (parse_res == -1) {
            sess->state = sstate_error;
            break;
        } else if (parse_res == 0)
            continue;

        switch (sess->state) {
            case sstate_await:
                session_parse_regular_message(sess);
                break;

            case sstate_file_transfer:
                // @TEST
                session_parse_regular_message(sess);
                break;

            default:
                break;
        }

        p_reset_reader(&sess->in_reader);
    }

    return sess->state != sstate_finish &&
           sess->state != sstate_error;
}

int session_write(session *sess)
{
    int wc;
    wc = write(sess->fd, 
               sess->out_buf + sess->out_buf_sent, 
               sess->out_buf_len - sess->out_buf_sent);
    if (wc <= 0) {
        sess->state = sstate_error;
        return 0;
    }
    sess->out_buf_sent += wc;
    
    if (sess->out_buf_sent >= sess->out_buf_len) {
        free(sess->out_buf);
        sess->out_buf = NULL;
        sess->out_buf_sent = 0;
        sess->out_buf_len = 0;
    }

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

    int flags = fcntl(sd, F_GETFL);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK); // for writing big data

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

    session *sess = serv.sessions[sd];
    p_deinit_reader(&sess->in_reader);
    if (sess->out_buf) free(sess->out_buf);
    if (sess->usernm) free(sess->usernm);
    if (sess->cur_f) fclose(sess->cur_f);
    free(sess);
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
        return_defer(1);
    }

    port = strtol(argv[1], &endptr, 10);
    if (!*argv[1] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return_defer(1);
    }

    if (!db_init(&db, argv[2])) {
        fprintf(stderr, "Could not parse database in folder\n");
        return_defer(1);
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
            session *sess = serv.sessions[i];
            if (sess) {
                FD_SET(i, &readfds);
                if (sess->out_buf)
                    FD_SET(i, &writefds);

                if (i > maxfd)
                    maxfd = i;
            }
        }

        // @TODO: impl writefds, since we may be sending large files
        int sr = select(maxfd+1, &readfds, &writefds, NULL, NULL);
        if (sr == -1) {
            perror("select");
            return_defer(-1);
        }

        if (FD_ISSET(serv.ls, &readfds))
            server_accept_client();
        for (int i = 0; i < serv.sessions_size; i++) {
            session *sess = serv.sessions[i];
            if (sess) {
                if (
                        (
                         FD_ISSET(i, &readfds) && 
                         !session_read(sess)
                        ) || 
                        (
                         FD_ISSET(i, &writefds) && 
                         !session_write(sess)
                        )
                   ) {
                    server_close_session(i);
                }
            }
        }
    }

defer:
    server_deinit();
    db_deinit(&db);
    return result;
}
