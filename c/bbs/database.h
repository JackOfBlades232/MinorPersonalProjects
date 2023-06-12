/* bbs/database.h */
#ifndef DATABASE_SENTRY
#define DATABASE_SENTRY

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

typedef struct file_metadata_tag {
    char *name;
    char *descr;
    int is_for_all_users;
    char **users;
    size_t cnt, cap;
} file_metadata;

typedef struct database_tag {
    FILE *passwd_f;
    DIR *data_dir;
    file_metadata **file_metas;
} database;

int db_init(database* db, const char *path);
void db_deinit(database* db);

int try_match_credentials(database* db, const char *usernm, const char *passwd);

#endif
