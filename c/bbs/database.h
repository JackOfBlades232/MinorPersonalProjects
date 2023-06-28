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
    int is_complete;
} file_metadata;

typedef struct user_data_tag {
    char *usernm;
    char *passwd;
    user_type type;
} user_data;

typedef struct database_tag {
    char *data_path;
    char *notes_path;
    FILE *passwd_f;
    FILE *notes_f;
    file_metadata **file_metas;
    size_t metas_cnt, metas_cap;
    user_data **user_datas;
    size_t users_cnt, users_cap;
} database;

typedef enum file_lookup_result_tag {
    found, not_found, no_access, incomplete
} file_lookup_result;

typedef enum db_modification_result_tag {
    dmod_ok, dmod_fail, dmod_err
} db_modification_result;

typedef struct add_file_result_tag {
    db_modification_result type;
    int fd;
    file_metadata *fmd;
} add_file_result;

typedef struct read_note_result_tag {
    char *usernm;
    char *note;
} read_note_result;

int db_init(database* db, const char *path);
void db_deinit(database* db);

user_type db_try_match_credentials(database* db, const char *usernm, const char *passwd);

int db_file_is_available_to_user(file_metadata *fmd, const char *username, user_type utype);
file_lookup_result db_lookup_file(database *db, const char *filename, 
                                  const char *username, user_type utype, 
                                  char **out, file_metadata **fmd_out);

void db_store_note(database *db, const char *username, byte_arr note);

add_file_result db_try_add_file(database *db, const char *filename, const char *descr,
                                const char **users, size_t users_cnt);

int db_user_exists(database* db, const char *usernm);
db_modification_result db_add_user(database* db, const char *usernm, const char *passwd, user_type ut);

read_note_result db_read_and_rm_top_note(database *db);

db_modification_result db_try_edit_metadata(database *db,
                                            const char *filename, const char *descr,
                                            const char **users, size_t users_cnt);

int db_delete_meta(database *db, file_metadata *fmd);
db_modification_result db_try_delete_file(database *db, const char *filename);

#endif
