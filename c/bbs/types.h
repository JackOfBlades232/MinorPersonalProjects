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

const user_type real_user_types[] = { ut_regular, ut_poster, ut_admin };
#define USER_TYPES_CNT sizeof(real_user_types)/sizeof(*real_user_types)
const char *user_type_names[USER_TYPES_CNT] = { "regular", "poster", "admin" };

const char poster_mark[] = "&";
const char admin_mark[] = "*";

#endif
