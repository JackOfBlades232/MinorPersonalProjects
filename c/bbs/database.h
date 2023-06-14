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

typedef struct user_data_tag {
    char *usernm;
    char *passwd;
} user_data;

typedef struct database_tag {
    // @TODO: clean up redundant data
    char *data_path;
    file_metadata **file_metas;
    user_data **user_datas;
} database;

typedef enum file_lookup_result_tag {
    found, not_found, no_access 
}file_lookup_result;

int db_init(database* db, const char *path);
void db_deinit(database* db);

int try_match_credentials(database* db, const char *usernm, const char *passwd);
int file_is_available_to_user(file_metadata *fmd, const char *username);
file_lookup_result lookup_file(database *db, const char *filename, const char *username, char **out);

#endif
