/* bbs/types.h */
#ifndef TYPES_SENTRY
#define TYPES_SENTRY

enum {
    MAX_USER_TYPE_LEN = 10
};

typedef enum user_type_tag { // used in increasing order
    ut_none = 0,
    ut_regular = 1,
    ut_poster = 2,
    ut_admin = 3
} user_type;

#define USER_TYPES_CNT 3
extern const user_type real_user_types[USER_TYPES_CNT];
extern const char *user_type_names[USER_TYPES_CNT];

extern const char poster_mark[];
extern const char admin_mark[];

#endif
