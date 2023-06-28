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
#include <signal.h>
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
    sstate_file_recv, 
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
    user_type ut;

    p_message_reader in_reader;

    int cur_query_fd;
    long out_packets_left;

    int cur_post_fd;
    file_metadata *cur_post_meta;
    long in_packets_left;
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

static volatile sig_atomic_t recieved_int = 0;
static sigset_t orig_mask;

void int_handler(int s)
{
    signal(SIGINT, int_handler);
    recieved_int = 1;
}

void session_post_msg(session *sess, p_message *msg)
{
    p_sendable_message smsg = p_construct_sendable_message(msg);
    
    // We presume that whenever we post a message, the previous one has
    // already been posted. It is a safe assumption, since all regular messages
    // are small, and the only big messages are file packets, which
    // we are guaranteed to send in sucession without sending anything else
    // inbetween
    sess->out_buf = smsg.str;
    sess->out_buf_len = smsg.len;
    sess->out_buf_sent = 0;

    // Transferring ownership of smsg.str, thus not freeing now
}

void session_post_empty_message(session *sess, p_type type) 
{
    p_message *msg = p_create_message(r_server, type);
    session_post_msg(sess, msg);
    p_free_message(msg);
}

void session_post_init_message(session *sess)
{
    p_message *msg = p_create_message(r_server, ts_init);
    p_add_string_to_message(msg, title);

    debug_log_p_message(msg);

    session_post_msg(sess, msg);
    p_free_message(msg);
}

void session_free_out_buf(session *sess)
{
    free(sess->out_buf);
    sess->out_buf = NULL;
    sess->out_buf_sent = 0;
    sess->out_buf_len = 0;
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
    sess->ut = ut_none;
    sess->cur_query_fd = -1;
    sess->out_packets_left = 0;
    sess->cur_post_fd = -1;
    sess->cur_post_meta = NULL;
    sess->in_packets_left = 0;

    p_init_reader(&sess->in_reader); // For login recieving

    // Protocol step one: state that it is a bbs server
    session_post_init_message(sess);

    return sess;
}

void session_handle_login(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    if (msg->role != r_client || msg->type != tc_login || msg->cnt != 2) {
        sess->state = sstate_error;
        return;
    }

    char *usernm = msg->words[0].str,
         *passwd = msg->words[1].str;

    sess->ut = db_try_match_credentials(&db, usernm, passwd);
    session_post_empty_message(sess, user_type_to_p_type(sess->ut));

    if (sess->ut != ut_none)
        sess->usernm = strdup(usernm);

    return;
}

void session_post_file_list(session *sess)
{
    p_message *response = p_create_message(r_server, ts_file_list_response);

    for (file_metadata **fmd = db.file_metas; *fmd; fmd++) {
        if (db_file_is_available_to_user(*fmd, sess->usernm, sess->ut)) {
            char *name_and_descr = concat_strings((*fmd)->name, "\n", (*fmd)->descr, NULL);
            p_add_string_to_message(response, name_and_descr);
            free(name_and_descr);
        }
    }

    session_post_msg(sess, response);
    p_free_message(response);
}

p_message *construct_num_packets_response(session *sess, const char *filename)
{
    p_message *response;

    sess->cur_query_fd = open(filename, O_RDONLY);
    if (sess->cur_query_fd == -1) {
        sess->state = sstate_error;
        return NULL;
    }

    long len = lseek(sess->cur_query_fd, 0, SEEK_END);
    lseek(sess->cur_query_fd, 0, SEEK_SET);
    sess->out_packets_left = ((len-1) / MAX_WORD_LEN) + 1;

    char packets_left_digits[LONG_MAX_DIGITS+1];
    snprintf(packets_left_digits, sizeof(packets_left_digits)-1, "%ld", sess->out_packets_left);
    packets_left_digits[sizeof(packets_left_digits)-1] = '\0';

    response = p_create_message(r_server, ts_start_file_transfer);
    p_add_string_to_message(response, packets_left_digits);

    return response;
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
    file_lookup_result lookup_res = db_lookup_file(&db, msg->words[0].str, 
                                                   sess->usernm, sess->ut,
                                                   &filename, NULL);

    if (lookup_res == found) {
        response = construct_num_packets_response(sess, filename);
        if (!response)
            goto defer;

        sess->state = sstate_file_transfer;
    } else {
        response = p_create_message(r_server, 
                                    lookup_res == no_access ? 
                                    ts_file_restricted : 
                                    ts_file_not_found);
    }

    session_post_msg(sess, response);
    p_free_message(response);

defer:
    if (filename) free(filename);
}

void session_store_note(session *sess) {
    p_message *msg = sess->in_reader.msg;
    if (msg->cnt != 1 || !sess->usernm) {
        sess->state = sstate_error;
        return;
    }

    db_store_note(&db, sess->usernm, msg->words[0]);
    session_post_empty_message(sess, ts_note_done);
}

void session_process_file_check(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt != 1 || sess->ut < ut_poster) {
        sess->state = sstate_error;
        return;
    }

    file_lookup_result lookup_res = db_lookup_file(&db, msg->words[0].str, 
                                                   sess->usernm, sess->ut, NULL, NULL);
    
    session_post_empty_message(sess, lookup_res == not_found ? ts_file_not_found : ts_file_exists);
}

void session_process_file_post(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt < 3 || sess->ut < ut_poster) { // name, descr, num packets
        sess->state = sstate_error;
        return;
    }

    char *filename = msg->words[0].str;
    char *descr = msg->words[1].str;
    size_t users_cnt = msg->cnt-3;
    char **users = malloc(users_cnt * sizeof(*users));
    for (size_t i = 0; i < users_cnt; i++)
        users[i] = msg->words[i+2].str;

    add_file_result add_res = db_try_add_file(&db, filename, descr, (const char **) users, users_cnt);
    if (add_res.type == dmod_err) {
        sess->state = sstate_error;
        return;
    } else if (add_res.type == dmod_fail) {
        session_post_empty_message(sess, ts_mod_fail);
        return;
    } else if (add_res.fd == -1) {
        sess->state = sstate_error;
        return;
    }

    sess->cur_post_fd = add_res.fd;
    sess->cur_post_meta = add_res.fmd;
    free(users);

    char *e;
    sess->in_packets_left = strtol(msg->words[msg->cnt-1].str, &e, 10);
    if (*e != '\0' || sess->in_packets_left <= 0) {
        sess->state = sstate_error;
        return;
    }

    session_post_empty_message(sess, ts_ready_to_recv);
    sess->state = sstate_file_recv;
}

void session_process_check_user(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt != 1 || sess->ut < ut_admin) {
        sess->state = sstate_error;
        return;
    }

    int exists = db_user_exists(&db, msg->words[0].str);
    session_post_empty_message(sess, exists ? ts_user_exists : ts_user_does_not_exist);
}

void session_add_user(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt != 2 && msg->cnt != 3) {
        sess->state = sstate_error;
        return;
    }

    if (sess->ut < ut_admin) {
        sess->state = sstate_error;
        return;
    }

    user_type ut;
    if (msg->cnt == 2)
        ut = ut_regular;
    else if (strings_are_equal(msg->words[2].str, poster_mark))
        ut = ut_poster;
    else if (strings_are_equal(msg->words[2].str, admin_mark))
        ut = ut_admin;
    else {
        sess->state = sstate_error;
        return;
    }

    db_modification_result res = db_add_user(&db, msg->words[0].str, msg->words[1].str, ut);

    if (res == dmod_err)
        sess->state = sstate_error;
    else if (res == dmod_fail)
        session_post_empty_message(sess, ts_mod_fail);
    else
        session_post_empty_message(sess, ts_user_added);
}

void session_try_pop_next_note(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt != 0 || sess->ut < ut_admin) {
        sess->state = sstate_error;
        return;
    }

    read_note_result rn_res = db_read_and_rm_top_note(&db);
    if (!rn_res.usernm || !rn_res.note)
        session_post_empty_message(sess, ts_no_notes_left);
    else {
        p_message *response = p_create_message(r_server, ts_show_and_rm_note);
        p_add_string_to_message(response, rn_res.usernm);
        p_add_string_to_message(response, rn_res.note);
        session_post_msg(sess, response);
        p_free_message(response);
    }

    if (rn_res.usernm) free(rn_res.usernm);
    if (rn_res.note) free(rn_res.note);
}

void session_edit_file_meta(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt < 2 || sess->ut < ut_admin) {
        sess->state = sstate_error;
        return;
    }

    char *filename = msg->words[0].str;
    char *descr = msg->words[1].str;
    size_t users_cnt = msg->cnt-2;
    char **users = malloc(users_cnt * sizeof(*users));
    for (size_t i = 0; i < users_cnt; i++)
        users[i] = msg->words[i+2].str;

    db_modification_result res = db_try_edit_metadata(&db, filename, descr, (const char **) users, users_cnt);
    free(users);
    
    if (res == dmod_err)
        sess->state = sstate_error;
    else if (res == dmod_fail)
        session_post_empty_message(sess, ts_mod_fail);
    else
        session_post_empty_message(sess, ts_file_edit_done);
}

void session_try_delete_file(session *sess)
{
    p_message *msg = sess->in_reader.msg;

    if (msg->cnt != 1 || sess->ut < ut_admin) {
        sess->state = sstate_error;
        return;
    }
    
    db_modification_result res = db_try_delete_file(&db, msg->words[0].str);

    if (res == dmod_err)
        sess->state = sstate_error;
    else if (res == dmod_fail)
        session_post_empty_message(sess, ts_mod_fail);
    else
        session_post_empty_message(sess, ts_file_deleted);

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
            session_post_file_list(sess);
            break;
        case tc_file_query:
            session_process_file_query(sess);
            break;
        case tc_leave_note:
            session_store_note(sess);
            break;
        case tc_file_check:
            session_process_file_check(sess);
            break;
        case tc_post_file:
            session_process_file_post(sess);
            break;
        case tc_user_check:
            session_process_check_user(sess);
            break;
        case tc_add_user:
            session_add_user(sess);
            break;
        case tc_ask_for_next_note:
            session_try_pop_next_note(sess);
            break;
        case tc_edit_file_meta:
            session_edit_file_meta(sess);
            break;
        case tc_delete_file:
            session_try_delete_file(sess);
            break;
        default:
            sess->state = sstate_error;
    }
}

void session_parse_recv_packet(session *sess)
{
    p_message *msg = sess->in_reader.msg;
    if (
            msg->role != r_client ||
            msg->type != tc_post_file_packet ||
            msg->cnt != 1
       ) {
        sess->state = sstate_error;
        return;
    }

    byte_arr content = msg->words[0];
    write(sess->cur_post_fd, content.str, content.len * sizeof(*content.str));
    sess->in_packets_left--;

    if (sess->in_packets_left <= 0) {
        close(sess->cur_post_fd);
        sess->cur_post_fd = -1;
        sess->cur_post_meta->is_complete = 1;
        sess->cur_post_meta = NULL;
        sess->state = sstate_await;
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
    
    p_reader_processing_res pr_res = rpr_in_progress;
    while (sess->in_buf_used > 0) {
        size_t chars_processed = 0;
        pr_res = p_reader_process_str(&sess->in_reader, 
                sess->in_buf, sess->in_buf_used, &chars_processed);
        if (chars_processed < sess->in_buf_used) {
            memmove(sess->in_buf, 
                    sess->in_buf + chars_processed, 
                    sess->in_buf_used - chars_processed);
        }

        sess->in_buf_used -= chars_processed;

        if (pr_res == rpr_in_progress)
            continue;
        else if (pr_res == rpr_error || sess->in_reader.msg->role != r_client) {
            sess->state = sstate_error;
            break;
        } 

        switch (sess->state) {
            case sstate_await:
                session_parse_regular_message(sess);
                break;

            case sstate_file_recv:
                session_parse_recv_packet(sess);
                break;

            case sstate_file_transfer:
                sess->state = sstate_error;
                break;

            default:
                break;
        }

        p_reset_reader(&sess->in_reader);
    }

    if (pr_res != rpr_in_progress)
        p_reset_reader(&sess->in_reader);

    return sess->state != sstate_finish &&
           sess->state != sstate_error;
}

void prepare_next_file_chunk_for_output(session *sess)
{
    p_message *out_msg = p_create_message(r_server, ts_file_packet);
    out_msg->cnt = 1;

    // Create the chunk directly in the message to avoid more copying;
    byte_arr *content = out_msg->words;
    size_t cap = MAX_WORD_LEN * sizeof(*sess->out_buf);
    content->str = malloc(cap);

    content->len = read(sess->cur_query_fd, content->str, cap);
    out_msg->tot_w_len = content->len;

    session_post_msg(sess, out_msg);
    p_free_message(out_msg);
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
        session_free_out_buf(sess);

        switch (sess->state) {
            case sstate_file_transfer:
                prepare_next_file_chunk_for_output(sess);
                sess->out_packets_left--;
                if (sess->out_packets_left <= 0) {
                    close(sess->cur_query_fd);
                    sess->cur_query_fd = -1;
                    p_reset_reader(&sess->in_reader);
                    sess->state = sstate_await;
                }
                break;

            default:
                break;
        }
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
    if (sess->cur_query_fd != -1) close(sess->cur_query_fd);
    if (sess->cur_post_fd != -1) close(sess->cur_post_fd);
    if (sess->cur_post_meta) {
        if (sess->cur_post_meta->is_complete)
            debug_printf_err("Mem leak, complete metadata in closing sess\n");
        else
            db_delete_meta(&db, sess->cur_post_meta);
    }
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

#ifndef DEBUG
void daemonize_self()
{
    int pid;
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
    chdir("/");
    pid = fork();
    if (pid > 0)
        exit(0);
    setsid();
    pid = fork();
    if (pid > 0)
        exit(0);
}
#endif

void set_up_sigint()
{
    sigset_t mask;
    signal(SIGINT, int_handler);
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &orig_mask);
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

    set_up_sigint();

#ifndef DEBUG
    daemonize_self();
#endif

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

        int sr = pselect(maxfd+1, &readfds, &writefds, NULL, NULL, &orig_mask);
        if (sr == -1 && errno != EINTR) {
            perror("select");
            return_defer(-1);
        }

        if (recieved_int) {
            debug_printf("Shutting down server\n");
            return_defer(0);
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
