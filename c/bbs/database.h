/* bbs/database.h */
#ifndef DATABASE_SENTRY
#define DATABASE_SENTRY

#include "utils.h"
#include "types.h"
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
    user_type type;
} user_data;

typedef struct database_tag {
    char *data_path;
    file_metadata **file_metas;
    size_t metas_cnt, metas_cap;
    user_data **user_datas;
    size_t users_cnt, users_cap;
    FILE *notes_f;
} database;

typedef enum file_lookup_result_tag {
    found, not_found, no_access 
} file_lookup_result;

int db_init(database* db, const char *path);
void db_deinit(database* db);

user_type db_try_match_credentials(database* db, const char *usernm, const char *passwd);

int db_file_is_available_to_user(file_metadata *fmd, const char *username, user_type utype);
file_lookup_result db_lookup_file(database *db, const char *filename, 
                                  const char *username, user_type utype, 
                                  char **out);

void db_store_note(database *db, const char *username, byte_arr note);

int db_try_add_file(database *db, const char *filename, const char *descr,
                    const char **users, size_t users_cnt);

#endif
