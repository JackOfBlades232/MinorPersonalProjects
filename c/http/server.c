/* http/server.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

// @TODO: fix the multi-chunk file bug

// A simple HTTP/1.1 server which only processes GET requests for a given
// html file

// Listen queue len for TCP
#define LISTEN_QLEN 16
// Init size (and size step for resizing) of the array of session structs
#define INIT_SESS_ARR_SIZE 32
// Input bufsize, also treated as max line len for request, since there
// will be no body in GET requests, and out bufsize
#define INBUFSIZE 1000
#define OUTBUFSIZE 1000
// All server return codes must have three digits
#define CODE_DIGITS 3

// Single client session states
typedef enum fsm_state_tag {
    fsm_await,
    fsm_headers_and_body,
    fsm_response_body,
    fsm_finish,
    fsm_error
} fsm_state;

// Session struct, one per connenction, contains the socket fd,
// buffer for accumulating lines and current state of the session
typedef struct session_tag {
    int fd;
    unsigned int from_ip;
    unsigned short from_port;

    char buf[INBUFSIZE];
    size_t buf_used;

    char out_buf[OUTBUFSIZE];
    size_t out_buf_used;

    int html_fd;
    int last_read_res;

    fsm_state state;
} session;

// Server struct, contains a listening socket for accepting connections,
// and an array of current sessions, where the index of a session is equal
// to the fd of it's socket
typedef struct server_tag {
    int ls;
    session **sessions;
    size_t sessions_size;
} server;

// Hacky macro to enable "goto cleanup" in functions with return codes
#define return_defer(code) do { result = code; goto defer; } while (0)

#define streq(s1, s2) (strcmp(s1, s2) == 0)

// Server codes
#define OK_CD 200
#define BAD_CD 400
#define NOT_FOUND_CD 404
#define NOT_IMPLEMENTED_CD 501

// Standard server commands and text messages
static const char http_version[] = "HTTP/1.1";
static const char get_cmd[] = "GET";
static const char ok_resp[] = "OK";
static const char bad_resp[] = "BAD REQUEST";
static const char not_found_resp[] = "RESOURCE NOT FOUND";
static const char not_implemented_resp[] = "NOT IMPLEMENTED";

static const char server_header[] = "Server: some pc somewhere";
static const char connection_header[] = "Connection: close";
static const char content_length_header[] = "Content-Length:";
static const char content_type_header[] = "Content-Type: text/html";

// Other constant strings
static const char html_extension[] = ".html";

// Global state: server struct and path to mail storage
static server serv;
static char *file_path;
static off_t file_size;

// Dyn arr funcs

void resize_dynamic_pointer_arr(void ***arr, size_t i, 
                                size_t *cap, size_t cap_step) 
{
    if (i >= *cap) { // resize if needed
        int newcap = *cap;
        while (newcap <= i)
            newcap += cap_step;
        *arr = realloc(*arr, newcap * sizeof(**arr));
        for (i = *cap; i < newcap; i++)
            (*arr)[i] = NULL;
        *cap = newcap;
    }
}

// Inbuf line manipulation functions

// Match line prefix with given command, and return the pointer to
// the next char if matched
const char *match_prefix_and_advance(const char *line, const char *cmd)
{
    for (; *cmd && *cmd == *line; cmd++, line++) {}
    return *cmd == '\0' ? line : NULL;
}

const char *skip_spc(const char *line)
{
    for (; *line && (*line == ' ' || *line == '\t'); line++) {}
    return line;
}

// Session helper functions

// Sending standard server response: "protocol code contents<CR><LF>", where
// code has an exact number of digits
void session_send_str(session *sess, const char *str)
{
    write(sess->fd, str, strlen(str));
}

void session_add_header(session *sess, const char *header)
{
    sess->out_buf_used += sprintf(sess->out_buf+sess->out_buf_used, 
                                  "%s\r\n", header);
}

void session_add_content_len(session *sess)
{
    sess->out_buf_used += sprintf(sess->out_buf+sess->out_buf_used,
                                  "%s %ld\r\n", 
                                  content_length_header, file_size);
}

void session_add_headers_end(session *sess)
{
    sess->out_buf_used += sprintf(sess->out_buf+sess->out_buf_used, "\r\n");
}

int session_send_response_headers(session *sess, int code)
{
    // Bypass posting, since in this state write buffer is empty and
    // the headers will surely fit
    if (sess->out_buf_used != 0)
        return 0;

    sess->out_buf_used += sprintf(sess->out_buf, "%s %d ", http_version, code);

    time_t t;
    time(&t);

    switch (code) {
        case OK_CD:
            session_add_header(sess, ok_resp);
            session_add_header(sess, server_header);
            session_add_header(sess, connection_header);
            session_add_content_len(sess);
            session_add_header(sess, content_type_header);
            break;
        case BAD_CD:
            session_add_header(sess, bad_resp);
            session_add_header(sess, server_header);
            session_add_header(sess, connection_header);
            break;
        case NOT_FOUND_CD:
            session_add_header(sess, not_found_resp);
            session_add_header(sess, server_header);
            session_add_header(sess, connection_header);
            break;
        case NOT_IMPLEMENTED_CD:
            session_add_header(sess, not_implemented_resp);
            session_add_header(sess, server_header);
            session_add_header(sess, connection_header);
            break;
        default:
            return 0;
    }

    session_add_headers_end(sess);

    write(sess->fd, sess->out_buf, sess->out_buf_used);
    sess->out_buf_used = 0;
    return 1;
}

void session_queue_file_chunk(session *sess)
{
    sess->last_read_res = read(sess->html_fd, sess->out_buf, OUTBUFSIZE-sess->out_buf_used);
    if (sess->last_read_res == -1)
        sess->state = fsm_error;
    else
        sess->out_buf_used += sess->last_read_res;
}

session *make_session(int fd, unsigned int from_ip, unsigned short from_port)
{
    session *sess = malloc(sizeof(*sess));
    sess->fd = fd;
    sess->from_ip = ntohl(from_ip);
    sess->from_port = ntohs(from_port);
    sess->buf_used = 0;
    sess->out_buf_used = 0;
    sess->html_fd = -1;
    sess->last_read_res = 0;
    sess->state = fsm_await;

    return sess;
}

void cleanup_session(session *sess)
{
    if (sess->html_fd != -1) close(sess->html_fd);
    return;
}

// Session logic functions

void session_fms_await_step(session *sess, const char *line)
{
    line = match_prefix_and_advance(line, get_cmd);
    if (!line || *line != ' ') {
        int code = !line ? NOT_IMPLEMENTED_CD : BAD_CD;
        session_send_response_headers(sess, code);
        if (code == BAD_CD)
            sess->state = fsm_error;
        return;
    }

    line++;
    if (*line != '/') {
        session_send_response_headers(sess, BAD_CD);
        sess->state = fsm_error;
        return;
    }

    line++;
    if (*line == ' ' || (line = match_prefix_and_advance(line, file_path)))
        line++;
    else {
        session_send_response_headers(sess, NOT_FOUND_CD);
        return;
    }

    line  = match_prefix_and_advance(line, http_version);
    sess->state = (line && *line == '\0') ? fsm_headers_and_body : fsm_error;
}

void session_fsm_input_step(session *sess, const char *line)
{
    switch (sess->state) {
        case fsm_await:
            session_fms_await_step(sess, line);
            break;
        case fsm_headers_and_body:
            if (*line == '\0') {
                session_send_response_headers(sess, OK_CD);
                sess->html_fd = open(file_path, O_RDONLY);
                sess->state = fsm_response_body;
                session_queue_file_chunk(sess);
            }
            break;

        case fsm_response_body:
        case fsm_finish:
        case fsm_error:
            break;
    }
}

void session_check_lf(session *sess)
{
    int pos = -1;
    char *line;
    for (int i = 0; i < sess->buf_used; i++) {
        if (sess->buf[i] == '\n') {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        sess->state = fsm_error;
        return;
    }

    line = strndup(sess->buf, pos);
    sess->buf_used -= pos+1;
    memmove(sess->buf, sess->buf+pos+1, sess->buf_used);
    if (line[pos-1] == '\r')
        line[pos-1] = '\0';

    session_fsm_input_step(sess, line);
    free(line);
}

int session_do_read(session *sess)
{
    int rc, bufp = sess->buf_used;
    if (sess->state == fsm_response_body)
        return 1;

    rc = read(sess->fd, sess->buf + bufp, INBUFSIZE-bufp);
    if (rc <= 0) {
        sess->state = fsm_error;
        return 0;
    }

    sess->buf_used += rc;
    while (sess->buf_used > 0)
        session_check_lf(sess);

    return sess->state != fsm_finish &&
           sess->state != fsm_error;
}

int session_do_write(session *sess)
{
    int wc;
    if (sess->out_buf_used <= 0)
        return 1;

    wc = write(sess->fd, sess->out_buf, sess->out_buf_used);
    if (wc <= 0) {
        sess->state = fsm_error;
        return 0;
    }

    sess->out_buf_used -= wc;
    if (sess->out_buf_used <= 0) {
        if (sess->last_read_res > 0)
            session_queue_file_chunk(sess);
        else {
            session_send_str(sess, "\r\n");
            close(sess->html_fd);
            sess->html_fd = -1;
            sess->state = fsm_await;
        }
    } else
        memmove(sess->out_buf, sess->out_buf+wc, sess->out_buf_used);

    return sess->state != fsm_finish &&
           sess->state != fsm_error;
}

// Server functions

int server_init(int port)
{
    int sock, opt;
    struct sockaddr_in addr;

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
    int sd;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    sd = accept(serv.ls, (struct sockaddr *) &addr, &len);
    if (sd == -1) {
        perror("accept");
        return;
    }

    int flags = fcntl(sd, F_GETFL);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    resize_dynamic_pointer_arr((void ***) &serv.sessions, sd,
                               &serv.sessions_size, INIT_SESS_ARR_SIZE);
    serv.sessions[sd] = make_session(sd, addr.sin_addr.s_addr, addr.sin_port);
}

void server_close_session(int sd)
{
    close(sd);
    cleanup_session(serv.sessions[sd]);
    free(serv.sessions[sd]);
    serv.sessions[sd] = NULL;
}

// Other stuff

int check_file(const char *path)
{
    size_t path_len = strlen(path);
    size_t html_ext_len = strlen(html_extension);
    if (path_len < html_ext_len) {
        fprintf(stderr, "File must have .html extension\n");
        return 0;
    }

    size_t offset = path_len - html_ext_len;
    const char *match_p = path + offset;
    const char *ext_p = html_extension;

    for (; *match_p && *ext_p && *match_p == *ext_p; match_p++, ext_p++) {}
    if (*match_p != *ext_p) {
        fprintf(stderr, "File must have .html extension\n");
        return 0;
    }

    if (access(path, R_OK) == -1) {
        fprintf(stderr, "The given file is not readable\n");
        return 0;
    }

    return 1;
}

off_t calculate_file_size(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1;

    off_t len = lseek(fd, 0, SEEK_END);
    close(fd);
    return len;
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

int main(int argc, char **argv) 
{
    long port;
    char *endptr;

    if (argc != 3) {
        fprintf(stderr, "Args: <port> <file path>\n");
        return -1;
    }

    port = strtol(argv[1], &endptr, 10);
    if (!*argv[1] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return -1;
    }

    file_path = argv[2];
    if (!check_file(file_path))
        return -1;

    file_size = calculate_file_size(file_path);
    if (file_size == -1) {
        fprintf(stderr, "Failed to calculate file length\n");
        return -1;
    }
        
    if (!server_init(port))
        return -1;

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
            if (serv.sessions[i]) {
                FD_SET(i, &readfds);
                FD_SET(i, &writefds);
                if (i > maxfd)
                    maxfd = i;
            }
        }

        int sr = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if (sr == -1) {
            perror("select");
            return -1;
        }

        if (FD_ISSET(serv.ls, &readfds))
            server_accept_client();
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i]) {
                if (
                        (
                         FD_ISSET(i, &readfds) &&
                         !session_do_read(serv.sessions[i])
                        ) ||
                        (
                         FD_ISSET(i, &writefds) &&
                         !session_do_write(serv.sessions[i])
                        )
                   ) {
                    server_close_session(i);
                }
            }
        }
    }

    return 0;
}
