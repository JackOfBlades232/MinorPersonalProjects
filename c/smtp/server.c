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
// Init size (and size step for resizing) of the array of rcptto email addresses
#define INIT_TO_ARR_SIZE 4
// Size of the mail storate filename (including data dir)
#define FILENAME_BUFSIZE 256
// Borders for filename/mail id
#define ID_MIN 10001
#define ID_MAX 999999
// Email parts length limits
#define MAX_LOCPART_LEN 64
#define MAX_DOMAIN_PART_LEN 63
#define MAX_DOMAIN_LEN 255

// Single client session states
typedef enum fsm_state_tag {
    fsm_init,
    fsm_mail_from,
    fsm_rcpt_to,
    fsm_data_header,
    fsm_data_body,
    fsm_finish,
    fsm_error
} fsm_state;

// A struct for a mail letter to be filled out during smtp dialogue,
// then an id is generated, an id.mail file is created and 
// the letter is stored there
typedef struct mail_letter_tag {
    char *from;
    char **to;
    size_t to_cnt, to_cap;
    int id;
    FILE *storage_f;
} mail_letter;

// Session struct, one per connenction, contains the socket fd,
// buffer for accumulating lines and current state of the session
typedef struct session_tag {
    int fd;
    unsigned int from_ip;
    unsigned short from_port;

    char buf[INBUFSIZE];
    size_t buf_used;

    fsm_state state;
    mail_letter *letter;
} session;

// Server struct, contains a listening socket for accepting connections,
// and an array of current sessions, where the index of a session is equal
// to the fd of it's socket
typedef struct server_tag {
    int ls;
    session **sessions;
    size_t sessions_size;
    char *storage_path;
} server;

// Hacky macro to enable "goto cleanup" in functions with return codes
#define return_defer(code) do { result = code; goto defer; } while (0)

#define streq(s1, s2) (strcmp(s1, s2) == 0)

// Server codes
#define INIT_CD 220
#define OK_CD 250
#define QUIT_CD 221
#define DATA_CD 354
#define SYNTAX_CD 500
#define NOIMPL_CD 502
#define NORCPT_CD 503

// Standard server commands and text messages
static char helo_cmd[] = "HELO";
static char ehlo_cmd[] = "EHLO";
static char starttls_cmd[] = "STARTTLS";
static char mail_from_cmd[] = "MAIL FROM:";
static char rcpt_to_cmd[] = "RCPT TO:";
static char data_cmd[] = "DATA";
static char quit_cmd[] = "QUIT";
static char header_end[] = "";
static char data_end[] = ".";
static char single_dot_pattern[] = "..";

static char init_msg[] = "smtp.example.com SMTP";
static char helo_msg[] = "Greetings, I am glad to meet you";
static char ok_msg[] = "Ok";
static char data_start_msg[] = "End data with <CR><LF>.<CR><LF>";
static char mail_store_ok_msg[] = "Ok: stored";
static char bye_msg[] = "Bye";
static char syntax_msg[] = "Syntax error";
static char too_long_msg[] = "Line too long";
static char noimpl_msg[] = "Command not implemented";
static char norcpt_msg[] = "Valid RCPT command must precede DATA";

// Email matching data
static char legal_locpart_symbols[] = "!#$%&'*+-/=?^_`{|}~";

// Global state: server struct and path to mail storage
static server serv;
static char *storage_path = NULL;

// Utils

int randint(int min, int max) 
{
    return min + (int) (((float) max) * rand() / (RAND_MAX+1.0));
}

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

// Mail letter functions

void cleanup_letter(mail_letter *letter)
{
    if (letter->from)
        free(letter->from);
    if (letter->to) {
        for (size_t i = 0; i < letter->to_cnt; i++)
            free(letter->to[i]);
        free(letter->to);
    }
    if (letter->storage_f)
        fclose(letter->storage_f);
}

void add_to_address_to_letter(mail_letter *letter, 
                              const char *addr, size_t addr_len)
{
    resize_dynamic_pointer_arr((void ***) &letter->to, letter->to_cnt,
                               &letter->to_cap, INIT_TO_ARR_SIZE);
    letter->to[letter->to_cnt] = strndup(addr, addr_len);
    letter->to_cnt++;
}

// Inbuf line manipulation functions

// Match line prefix with given command, and return the pointer to
// the next char if matched
const char *match_prefix_advanced(const char *line, const char *cmd)
{
    for (; *cmd && *cmd == *line; cmd++, line++) {}
    return *cmd == '\0' ? line : NULL;
}

const char *skip_spc(const char *line)
{
    for (; *line && (*line == ' ' || *line == '\t'); line++) {}
    return line;
}

// Email address format checking

int char_is_alphanumeric(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

// Ok if alphanumeric, in !#$%&'*+-/=?^_`{|}~ or is a ',', but
// a dot can not be first, last, and two dots can't come consecutively
int char_is_ok_for_locpart(char c, char prev_c, int is_first_or_last)
{
    if (
            char_is_alphanumeric(c) ||
            (c == '.' && !is_first_or_last && prev_c != '.')
       ) {
        return 1;
    }

    for (size_t i = 0; i < sizeof(legal_locpart_symbols)-1; i++) {
        if (c == legal_locpart_symbols[i])
            return 1;
    }

    return 0;
}

int char_is_ok_for_domain(char c, int is_first_or_last)
{
    return char_is_alphanumeric(c) || (c == '-' && !is_first_or_last);
}

// Following funcs check if mail addr segment is correct and advance line

const char *check_and_skip_locpart(const char *line, char break_c)
{
    size_t chars_read = 0;
    char prev_c = '\0';
    int is_first_or_last = 1;
    for (; *line && *line != break_c; line++) {
        if (!is_first_or_last)
            is_first_or_last = *(line+1) == break_c;

        if (!char_is_ok_for_locpart(*line, prev_c, is_first_or_last))
            return NULL;

        chars_read++;
        if (chars_read > MAX_LOCPART_LEN)
            return NULL;

        prev_c = *line;
        is_first_or_last = 0;
    }

    if (chars_read == 0)
        return NULL;

    return *line == break_c ? line : NULL;
}

const char *check_and_skip_domain_part(const char *line, char break_c,
                                       char fin_c, size_t *domain_tot_len)
{
    size_t chars_read = 0;
    int is_first_or_last = 1;
    for (; *line && *line != break_c && *line != fin_c; line++) {
        if (!is_first_or_last) {
            char next_c = line[1];
            is_first_or_last = next_c == break_c || next_c == fin_c;
        }

        if (!char_is_ok_for_domain(*line, is_first_or_last))
            return NULL;

        chars_read++;
        if (chars_read > MAX_DOMAIN_PART_LEN)
            return NULL;

        is_first_or_last = 0;
    }

    if (chars_read == 0)
        return NULL;

    *domain_tot_len += chars_read;
    return *line == break_c || *line == fin_c ? line : NULL;
}

// Checks mail address correctness and may return a pointer to it + length
int extract_mail_address(const char *line, char **mailp, size_t *mail_len)
{
    line = skip_spc(line);
    if (*line != '<')
        return 0;
    line++;
    *mailp = (char *) line;
    if ((line = check_and_skip_locpart(line, '@')) == NULL)
        return 0;
    line++;
    
    size_t domain_tot_len = 0;
    while ((line = check_and_skip_domain_part(line, '.', '>', &domain_tot_len)) != NULL) {
        if (*line == '>')
            break;

        line++;
        domain_tot_len++;
    }

    if (!line || *line != '>' || domain_tot_len > MAX_DOMAIN_LEN)
        return 0;
    line++;

    *mail_len = line - *mailp - 1;
    line = skip_spc(line);
    return *line == '\0';
}

// Session helper functions

// Sending standard server response: "code contents<CR><LF>", where
// code has an exact number of digits
int session_send_msg(session *sess, int code, const char *str)
{
    int result = 1;

    size_t str_len = strlen(str);
    // code + ' ' + str + <CR><LF>
    size_t tot_len = CODE_DIGITS+1+str_len+2;

    char *msg = malloc(tot_len * sizeof(*str));

    int pr_res = sprintf(msg, "%d ", code);
    if (pr_res != CODE_DIGITS+1) {
        fprintf(stderr, "ERR: Invalid code (3 digits max)\n");
        return_defer(0);
    }

    memcpy(msg+pr_res, str, str_len);
    msg[tot_len-2] = '\r';
    msg[tot_len-1] = '\n';
    
    // we send small strings => no need to wait for writefds, just send
    write(sess->fd, msg, tot_len);

defer:
    free(msg);
    return result;
}

void session_remake_letter(session *sess)
{
    if (sess->letter)
        cleanup_letter(sess->letter);
    else
        sess->letter = malloc(sizeof(*sess->letter));

    sess->letter->from = NULL;
    sess->letter->to_cnt = 0;
    sess->letter->to_cap = INIT_TO_ARR_SIZE;
    sess->letter->to = calloc(sess->letter->to_cap, sizeof(*sess->letter->to));
    sess->letter->id = -1;
    sess->letter->storage_f = NULL;
}

session *make_session(int fd, unsigned int from_ip, unsigned short from_port)
{
    session *sess = malloc(sizeof(*sess));
    sess->fd = fd;
    sess->from_ip = ntohl(from_ip);
    sess->from_port = ntohs(from_port);
    sess->buf_used = 0;
    sess->state = fsm_init;
    sess->letter = NULL;

    session_send_msg(sess, INIT_CD, init_msg);

    return sess;
}

void cleanup_session(session *sess)
{
    if (sess->letter) {
        cleanup_letter(sess->letter);
        free(sess->letter);
    }
}

// Mail storage i/o

void session_setup_mail_storage_file(session *sess)
{
    char full_path[FILENAME_BUFSIZE];

    while (!sess->letter->storage_f) {
        sess->letter->id = randint(ID_MIN, ID_MAX);
        snprintf(full_path, sizeof(full_path)-1,
                 "%s/%d.mail", storage_path, sess->letter->id);

        if (access(full_path, F_OK) == -1)
            sess->letter->storage_f = fopen(full_path, "w");
    }

    fprintf(sess->letter->storage_f, "FROM: %s\n", sess->letter->from);
    fprintf(sess->letter->storage_f, "TO:");
    for (size_t i = 0; i < sess->letter->to_cnt; i++)
        fprintf(sess->letter->storage_f, " %s", sess->letter->to[i]);
    fprintf(sess->letter->storage_f, "\n\nHEADER:\n");
}

// Session logic functions

void session_fsm_init_step(session *sess, const char *line)
{
    // on HELO command start dialogue, on EHLO/STARTTLS send noimpl,
    // and syntax error all else
    if (match_prefix_advanced(line, helo_cmd)) {
        session_send_msg(sess, OK_CD, helo_msg);
        session_remake_letter(sess);
        sess->state = fsm_mail_from;
    } else if (
            match_prefix_advanced(line, ehlo_cmd) ||
            match_prefix_advanced(line, starttls_cmd)
            ) {
        session_send_msg(sess, NOIMPL_CD, noimpl_msg);
    } else {
        session_send_msg(sess, SYNTAX_CD, syntax_msg);
        sess->state = fsm_error;
    }
}

void session_fsm_mail_from_step(session *sess, const char *line)
{
    line = match_prefix_advanced(line, mail_from_cmd); 
    if (!line) {
        session_send_msg(sess, SYNTAX_CD, syntax_msg);
        sess->state = fsm_error;
        return;
    }

    char *mail_adr;
    size_t mail_len;
    if (!extract_mail_address(line, &mail_adr, &mail_len)) {
        session_send_msg(sess, SYNTAX_CD, syntax_msg);
        sess->state = fsm_error;
        return;
    }

    session_send_msg(sess, OK_CD, ok_msg);
    sess->letter->from = strndup(mail_adr, mail_len);
    sess->state = fsm_rcpt_to;
}

void session_fsm_rcpt_to_step(session *sess, const char *line)
{
    if (match_prefix_advanced(line, data_cmd)) {
        // Only proceed to data if a reciever was specified with RCPT TO:
        if (sess->letter->to_cnt == 0) {
            session_send_msg(sess, NORCPT_CD, norcpt_msg);
            sess->state = fsm_error;
        } else {
            session_send_msg(sess, DATA_CD, data_start_msg);
            session_setup_mail_storage_file(sess);
            sess->state = fsm_data_header;
        }

        return;
    }

    line = match_prefix_advanced(line, rcpt_to_cmd); 
    if (!line) {
        session_send_msg(sess, SYNTAX_CD, syntax_msg);
        sess->state = fsm_error;
        return;
    }

    char *mail_adr;
    size_t mail_len;
    if (!extract_mail_address(line, &mail_adr, &mail_len)) {
        session_send_msg(sess, SYNTAX_CD, syntax_msg);
        sess->state = fsm_error;
        return;
    }

    session_send_msg(sess, OK_CD, ok_msg);
    add_to_address_to_letter(sess->letter, mail_adr, mail_len);
}

void session_fsm_data_body_step(session *sess, const char *line)
{
    int data_end_ptr_is_first = 0;
    if (streq(line, data_end)) {
        session_send_msg(sess, OK_CD, mail_store_ok_msg);
        session_remake_letter(sess);
        // Assuming that if the client wants to send multiple emails,
        // he will resend the MAIL FROM: every time
        sess->state = fsm_mail_from;
        return;
    } else if (match_prefix_advanced(line, single_dot_pattern)) {
        line += sizeof(single_dot_pattern)-1;
        data_end_ptr_is_first = 1;
    }

    if (data_end_ptr_is_first)
        fprintf(sess->letter->storage_f, "%s", data_end);
    fprintf(sess->letter->storage_f, "%s\n", line);
}

void session_fsm_data_header_step(session *sess, const char *line)
{
    if (streq(line, header_end)) {
        fprintf(sess->letter->storage_f, "\nDATA:\n");
        sess->state = fsm_data_body;
    } else 
        session_fsm_data_body_step(sess, line);
}

void session_fsm_step(session *sess, const char *line)
{
    if (match_prefix_advanced(line, quit_cmd)) {
        session_send_msg(sess, QUIT_CD, bye_msg);
        sess->state = fsm_finish;
    }

    switch (sess->state) {
        case fsm_init:
            session_fsm_init_step(sess, line);
            break;
        case fsm_mail_from:
            session_fsm_mail_from_step(sess, line);
            break;
        case fsm_rcpt_to:
            session_fsm_rcpt_to_step(sess, line);
            break;
        case fsm_data_header:
            session_fsm_data_header_step(sess, line);
            break;
        case fsm_data_body:
            session_fsm_data_body_step(sess, line);
            break;

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
        session_send_msg(sess, SYNTAX_CD, too_long_msg);
        sess->state = fsm_error;
        return;
    }

    line = strndup(sess->buf, pos);
    sess->buf_used -= pos+1;
    memmove(sess->buf, sess->buf+pos+1, sess->buf_used);
    if (line[pos-1] == '\r')
        line[pos-1] = '\0';

    session_fsm_step(sess, line);
    free(line);
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
    while (sess->buf_used > 0)
        session_check_lf(sess);

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

// OS functions

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

#ifdef TEST
void test_email_format_checking()
{
    char *emails[] = {
        "<simple@example.com>",
        "<very.common@example.com>",
        "<disposable.style.email.with+symbol@example.com>",
        "<other.email-with-hyphen@and.subdomains.example.com>",
        "<fully-qualified-domain@example.com>",
        "<user.name+tag+sorting@example.com>",
        "<x@example.com>",
        "<example-indeed@strange-example.com>",
        "<test/test@test.com>",
        "<admin@mailserver1>",
        "<example@s.example>",
        "<mailhost!username@example.org>",
        "<user%example.com@example.org>",
        "<user-@example.org>",
        "<Abc.example.com>",
        "<A@b@c@example.com>",
        "<a\"b(c)d,e:f;g<h>i[j\k]l@example.com>",
        "<just\"not\"right@example.com>",
        "<this is\"not\allowed@example.com>",
        "<this\ still\"not\\allowed@example.com>",
        "<1234567890123456789012345678901234567890123456789012345678901234+x@example.com>",
        "<i_like_underscore@but_its_not_allowed_in_this_part.example.com>",
        "<QA[icon]CHOCOLATE[icon]@test.com>"
    };
    int test_results[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    for (int i = 0; i < 23; i++) {
        char *mail = NULL;
        size_t len = 0;
        int res = extract_mail_address(emails[i], &mail, &len);
        if (res == test_results[i])
            printf("PASSED: %s res: %d\n", emails[i], res); 
        else 
            printf("\nTEST FAILED: %s expected: %d res: %d\n", emails[i], test_results[i], res); 
    }
}
#endif

int check_and_prep_dir(char *path)
{
    char buf[FILENAME_BUFSIZE];

    size_t plen = strlen(path);
    if (path[plen-1] == '/')
        path[plen-1] = '\0';

    snprintf(buf, sizeof(buf)-1, "%s/test.test", path);
    FILE *f = fopen(buf, "w");
    if (!f)
        return 0;

    fclose(f);
    unlink(buf);
    return 1;
}

int main(int argc, char **argv) 
{
#ifdef TEST
    test_email_format_checking();
    return 0;
#endif

    long port;
    char *endptr;

    if (argc != 3) {
        fprintf(stderr, "Args: <port> <storage dir>\n");
        return -1;
    }

    port = strtol(argv[1], &endptr, 10);
    if (!*argv[1] || *endptr) {
        fprintf(stderr, "Invalid port number\n");
        return -1;
    }

    storage_path = argv[2];
    if (!check_and_prep_dir(storage_path))
        return -1;
        
    if (!server_init(port))
        return -1;

#ifndef DEBUG
    daemonize_self();
#endif

    srand(time(NULL));

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
            server_accept_client();
        for (int i = 0; i < serv.sessions_size; i++) {
            if (serv.sessions[i] && FD_ISSET(i, &readfds)) {
                int ssr = session_do_read(serv.sessions[i]);
                if (!ssr)
                    server_close_session(i);
            }
        }
    }

    return 0;
}
