/* bbs/database.c */
#include "database.h"
#include "constants.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

static const char passwd_rel_path[] = "/passwd.txt";
static const char data_rel_path[] = "/data/";

#define WORD_SEP ' '

enum {
    METADATA_BASE_NAME_BUFSIZE = 16,
    METADATA_BASE_DESCR_BUFSIZE = 32,
    METADATA_BASE_USER_CAP = 4,
    METADATA_BASE_LOGIN_ITEM_CAP = 16,

    METADATA_MAX_NAME_LEN = 16,
    METADATA_MAX_DESCR_LEN = 128,
    METADATA_MAX_USER_CNT = 4,
    METADATA_MAX_LOGIN_ITEM_LEN = MAX_LOGIN_ITEM_LEN,

    WORD_BUFSIZE = 256
};

typedef enum metadata_parse_state_tag {
    mps_file,
    mps_descr,
    mps_users,
    mps_finished,
    mps_error
} metadata_parse_state;

static const char metadata_file_alias[] = "file:";
static const char metadata_descr_alias[] = "description:";
static const char metadata_users_alias[] = "access:";

static char *get_full_path(const char *rel_path, const char *db_path, size_t db_path_len)
{
    size_t rel_path_len = strlen(rel_path);
    size_t full_path_len = db_path_len+rel_path_len;
    char *path = malloc((full_path_len+1) * sizeof(char));

    memcpy(path, db_path, db_path_len);
    memcpy(path+db_path_len, rel_path, rel_path_len);
    path[full_path_len] = '\0';
    return path;
}

int db_init(database* db, const char *path)
{ 
    int result = 1;

    char *passwd_path = NULL,
         *data_path = NULL;
    size_t path_len = strlen(path);

    if (path[path_len-1] == '/')
        path_len--;

    passwd_path = get_full_path(passwd_rel_path, path, path_len);
    data_path = get_full_path(data_rel_path, path, path_len);

    db->passwd_f = fopen(passwd_path, "r");
    if (!db->passwd_f)
        return_defer(0);

    db->data_dir = opendir(data_path);
    if (!db->data_dir) 
        return_defer(0);

    // @TEST
    struct dirent *dent;
    size_t cnt = 0, cap = 32, cap_step = 1, max_cnt = 30;
    db->file_names = malloc(cap * sizeof(char *));
    while ((dent = readdir(db->data_dir)) != NULL) {
        if (dent->d_type == DT_REG || dent->d_type == DT_UNKNOWN) { 
            add_string_to_string_array(&db->file_names, dent->d_name,
                                       &cnt, &cap, cap_step, max_cnt);
        }
    }
    db->file_names[cnt] = NULL;
    rewinddir(db->data_dir);

defer:
    if (passwd_path) free(passwd_path);
    if (data_path) free(data_path);
    if (!result) {
        if (db->passwd_f) fclose(db->passwd_f);
        if (db->data_dir) closedir(db->data_dir);
    }
    return result;
}

void db_deinit(database* db)
{
    if (db->passwd_f) fclose(db->passwd_f);
    if (db->data_dir) closedir(db->data_dir);
}

static int try_match_passwd_line(database *db, const char *usernm, 
                                 const char *passwd, int *last_c)
{
    int c;
    int failed = 0;
    int matched_usernm = 0,
        matched_passwd = 0;
    size_t usernm_len = strlen(usernm),
           passwd_len = strlen(passwd);
    size_t usernm_idx = 0,
           passwd_idx = 0;

    while ((c = fgetc(db->passwd_f)) != EOF) {
        if (c == '\n')
            break;
        if (failed)
            continue;

        if (!matched_usernm) {
            if (usernm_idx >= usernm_len) {
                if (c == ' ')
                    matched_usernm = 1;
                else
                    failed = 1;
            } else if (usernm[usernm_idx] == c)
                usernm_idx++;
            else
                failed = 1;
        } else if (!matched_passwd) {
            if (passwd_idx < passwd_len && passwd[passwd_idx] == c)
                passwd_idx++;
            else
                failed = 1;
        }
    }

    if (!failed && matched_usernm && passwd_idx == passwd_len)
        matched_passwd = 1;

    *last_c = c;
    return matched_usernm && matched_passwd;
}

int try_match_credentials(database* db, const char *usernm, const char *passwd)
{
    int last_c = '\n';
    while (last_c != EOF) {
        if (try_match_passwd_line(db, usernm, passwd, &last_c))
            return 1;
    }
    return 0;
}

static file_metadata *create_metadata()
{
    file_metadata *fmd = malloc(sizeof(*fmd));
    fmd->name = NULL;
    fmd->descr = NULL;
    fmd->is_for_all_users = 0;
    fmd->cnt = 0;
    fmd->cap = METADATA_BASE_USER_CAP;
    fmd->users = malloc(fmd->cap * sizeof(*fmd->users));
    return fmd;
}

//static void free_metadata(file_metadata *fmd)
void free_metadata(file_metadata *fmd)
{
    if (fmd->name) free(fmd->name);
    if (fmd->descr) free(fmd->descr);
    if (fmd->users) {
        for (size_t i = 0; i < fmd->cnt; i++)
            free(fmd->users[i]);
        free(fmd->users);
    }
    free(fmd);
}

static int read_word_to_buf(FILE *f, char *buf, size_t bufsize, int stop_at_sep)
{
    size_t i = 0;
    int c;

    if (bufsize == 0)
        return 0;

    while ((c = getc(f)) != EOF) {
        if (
                (stop_at_sep && c == WORD_SEP) ||
                c == '\n' || c == '\r'
           )
            break;
        else if (i >= bufsize-1)
            return 0;
        
        buf[i++] = c;
    }

    buf[i] = '\0';
    return c;
}

static int match_alias(char *word, metadata_parse_state cur_state)
{
    switch (cur_state) {
        case mps_file:
            return strcmp(word, metadata_file_alias) == 0;
        case mps_descr:
            return strcmp(word, metadata_descr_alias) == 0;
        case mps_users:
            return strcmp(word, metadata_users_alias) == 0;
        default:
            return 1;
    }
}

static int file_exists_and_is_available(const char *filename, const char *dirname)
{
    int result = 1;
    char *full_path = get_full_path(filename, dirname, strlen(dirname));

    FILE *f = fopen(full_path, "r");
    if (!f)
        return_defer(0);
    fclose(f);

    f = fopen(full_path, "w");
    if (!f)
        return_defer(0);

defer:
    if (f) fclose(f);
    if (full_path) free(full_path);
    return result;
}

//static file_metadata *parse_meta_file(FILE *f)
file_metadata *parse_meta_file(FILE *f, const char *dirpath)
{
    file_metadata *fmd;

    char w_buf[WORD_BUFSIZE];
    int break_c;

    metadata_parse_state state = mps_file;
    int parsed_alias = 0;

    if (!f)
        return NULL;

    fmd = create_metadata();

    for (;;) {
        break_c = read_word_to_buf(
                f, w_buf, sizeof(w_buf),
                !parsed_alias || state != mps_descr // so as to read the whole 
                );                                  // description content in one go 
        if (break_c == 0)
            goto defer;

        if (!parsed_alias) {
            if (break_c != WORD_SEP)
                goto defer;

            if (match_alias(w_buf, state)) {
                parsed_alias = 1;
                continue;
            } else 
                goto defer;
        }

        size_t len;
        switch (state) {
            case mps_file:
                if (break_c != '\n' && break_c != '\r')
                    goto defer;

                if (file_exists_and_is_available(w_buf, dirpath)) {
                    len = strlen(w_buf);
                    if (len > METADATA_MAX_NAME_LEN)
                        goto defer;

                    fmd->name = malloc((len+1) * sizeof(*fmd->name));
                    strncpy(fmd->name, w_buf, len);

                    state = mps_descr;
                    parsed_alias = 0;
                } else
                    goto defer;
                break;

            case mps_descr:
                len = strlen(w_buf);
                if (len > METADATA_MAX_DESCR_LEN)
                    goto defer;

                fmd->descr = malloc((len+1) * sizeof(*fmd->descr));
                strncpy(fmd->descr, w_buf, len);

                state = mps_users;
                parsed_alias = 0;
                break;

            case mps_users:
                if (strlen(w_buf) > METADATA_MAX_LOGIN_ITEM_LEN)
                    goto defer;

                int add_res = add_string_to_string_array(&fmd->users, w_buf,
                                                         &fmd->cnt, &fmd->cap,
                                                         METADATA_BASE_USER_CAP,
                                                         METADATA_MAX_USER_CNT);
                if (!add_res)
                    goto defer;

                if (break_c == '\n' || break_c == '\r')
                    state = mps_finished;

                break;
            
            case mps_finished:
                if (strlen(w_buf) > 0) {
                    state = mps_error;
                    goto defer;
                }
                break;

            default:
                state = mps_error;
                goto defer;
        }
        
        if (state == mps_error || break_c == EOF)
            break;
    }

defer:
    if (break_c == 0 || state != mps_finished) {
        free_metadata(fmd);
        fmd = NULL;
    }
    return fmd;
}
