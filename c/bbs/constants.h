/* bbs/constants.h */
#ifndef CONSTANTS_SENTRY
#define CONSTANTS_SENTRY

enum {
    MAX_LOGIN_ITEM_LEN = 64,
    MAX_FILENAME_LEN = 64,
    MAX_DESCR_LEN = 128,
    MAX_USER_CNT = 32,
    MAX_NOTE_LEN = 256,

    MAX_WORD_LEN = 4096 // must be >= user_cnt * (login_len+1), see client post msg
};

#endif
