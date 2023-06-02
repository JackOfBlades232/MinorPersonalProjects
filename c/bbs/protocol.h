/* bbs/protocol.h */

#define DELIM ':'

const char msg_prefix[] = "BBS232:";

enum {
    LOGIN_BUFSIZE = 256
};

struct login_info {
    char usernm[LOGIN_BUFSIZE], passwd[LOGIN_BUFSIZE];
};

// @TEST
const char server_init_msg[] = "BBS232:CONNECTED";
const char client_init_response[] = "BBS232:CLIENT:CONFIRM";
