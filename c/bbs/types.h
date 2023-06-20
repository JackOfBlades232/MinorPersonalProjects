/* bbs/types.h */
#ifndef TYPES_SENTRY
#define TYPES_SENTRY

typedef enum user_type_tag { // used in increasing order
    ut_none = 0,
    ut_regular = 1,
    ut_poster = 2,
    ut_admin = 3
} user_type;

#endif
