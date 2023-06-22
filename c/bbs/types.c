/* bbs/types.c */
#include "types.h"

const user_type real_user_types[USER_TYPES_CNT] = { ut_regular, ut_poster, ut_admin };
const char *user_type_names[USER_TYPES_CNT] = { "regular", "poster", "admin" };

const char poster_mark[] = "&";
const char admin_mark[] = "*";
