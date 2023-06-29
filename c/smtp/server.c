/* smtp/server.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

// Listen queue len for TCP
#define LISTEN_QLEN 16
// Init size (and size step for resizing) of the array of session structs
#define INIT_SESS_ARR_SIZE 32
// Max SMTP line len, which is canonically 998 bytes + <CR><LF> = 1000
#define INBUFSIZE 1000
// All server return codes must have three digits
#define CODE_DIGITS 3

// Single client session states
typedef enum fsm_state_tag {
    // @TODO: add states
    fsm_init,
    fsm_finish,
    fsm_error
} fsm_state;

// Session struct, one per connenction, contains the socket fd,\
// buffer for accumulating lines and current state of the session
typedef struct session_tag {
    int fd;
    char buf[INBUFSIZE];
    int buf_used;
    fsm_state state;
} session;

// Server struct, contains a listening socket for accepting connections,
// and an array of current sessions, where the index of a session is equal
// to the fd of it's socket
typedef struct server_tag {
    int ls;
    session **sessions;
    int sessions_size;
} server;

// Hacky macro to enable "goto cleanup" in functions with return codes
#define return_defer(code) do { result = code; goto defer; } while (0)

// Sending standard server response: "code contents<CR><LF>", where
// code has an exact number of digits
int session_send_msg(session *sess, int code, const char *str)
{
    int result = 1;

    // @TODO: add <CR><LF>
    char *msg = malloc((CODE_DIGITS+1+strlen(str)+2) * sizeof(*str));

    int pr_res = sprintf(msg, "%d ", code);
    if (pr_res != CODE_DIGITS+1) {
        fprintf(stderr, "ERR: Invalid code (3 digits max)\n");
        return_defer(0);
    }

    memcpy(msg+pr_res, str, strlen(str));
    
    // we send small strings => no need to wait for writefds, just send
    write(sess->fd, msg, strlen(msg));

defer:
    free(msg);
    return result;
}

session *make_session(int fd,
        unsigned int from_ip, unsigned short from_port)
{
    session *sess = malloc(sizeof(session));
    sess->fd = fd;
    sess->from_ip = ntohl(from_ip);
    sess->from_port = ntohs(from_port);
    sess->buf_used = 0;
    sess->state = fsm_init;

    return sess;
}

void cleanup_session(session *sess)
{
    // @TODO: cleanup whatever data
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
    if (pos == -1) 
        return;

    line = malloc(pos+1);
    memcpy(line, sess->buf, pos);
    line[pos] = '\0';
    sess->buf_used -= pos+1;
    memmove(sess->buf, sess->buf+pos+1, sess->buf_used);
    if (line[pos-1] == '\r')
        line[pos-1] = '\0';

    // @TODO: logical reaction to line

    // @TEST
    fprintf(stderr, "Line: %s\n", line);
    sess->state = fsm_finish;
}

int session_do_read(session *sess)
{
    int rc, bufp = sess->buf_used;
    rc = read(sess->fd, sess->buf + bufp, INBUFSIZE-bufp);
    if (rc <= 0) {
        sess->state = fsm_error;
        return 0;
    }

    sess->buf_used += rc;

    // @TODO: organize a cycle?
    session_check_lf(sess);

    return sess->state != fsm_finish &&
           sess->state != fsm_error;
}

int server_init(server *serv, int port)
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
    serv->ls = sock;

    serv->sessions = calloc(INIT_SESS_ARR_SIZE, sizeof(*serv->sessions));
    serv->sessions_size = INIT_SESS_ARR_SIZE;

    return 1;
}

void server_accept_client(server *serv)
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

void server_close_session(server *serv, int sd)
{
    close(sd);
    cleanup_session(serv->sessions[sd]);
    free(serv->sessions[sd]);
    serv->sessions[sd] = NULL;
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
    server serv;
    long port;
    char *endptr;

    if (argc != 2) {
        fprintf(stderr, "Args: <port>\n");
        return -1;
    }

    port = strtol(argv[1], &endptr, 10);
    if (!*argv[1] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return -1;
    }
        
    if (!server_init(&serv, port))
        return -1;

#ifndef DEBUG
    daemonize_self();
#endif

    for (;;) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serv.ls, &readfds);

        int maxfd = serv.ls;
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i]) {
                FD_SET(i, &readfds);
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
            server_accept_client(&serv);
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i] && FD_ISSET(i, &readfds)) {
                int ssr = session_do_read(serv.sessions[i]);
                if (!ssr)
                    server_close_session(&serv, i);
            }
        }
    }

    return 0;
}
