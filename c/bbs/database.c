/* bbs/database.c */
#include "database.h"
#include "constants.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

static const char passwd_rel_path[] = "/passwd.txt";
static const char data_rel_path[] = "/data/";

static const char metafile_extension[] = ".meta";

#define WORD_SEP ' '

enum {
    METADATA_BASE_NAME_BUFSIZE = 16,
    METADATA_BASE_DESCR_BUFSIZE = 32,
    METADATA_BASE_USER_CAP = 4,
    METADATA_BASE_LOGIN_ITEM_CAP = 16,

    METADATA_MAX_NAME_LEN = MAX_FILENAME_LEN,
    METADATA_MAX_DESCR_LEN = 128,
    METADATA_MAX_USER_CNT = 4,
    METADATA_MAX_LOGIN_ITEM_LEN = MAX_LOGIN_ITEM_LEN,

    WORD_BUFSIZE = 256,

    METAFILES_BASE_CAP = 32,
    METAFILES_MAX_CNT = 2048,

    USERS_BASE_CAP = 64,
    USERS_MAX_CNT = 4096,

    METAFILE_EXT_LEN = sizeof(metafile_extension)-1
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

static const char all_users_symbol[] = "*";

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

static void free_metadata(file_metadata *fmd)
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

static user_data *create_user_data()
{
    user_data *ud = malloc(sizeof(*ud));
    ud->usernm = NULL;
    ud->passwd = NULL;
    return ud;
}

static void free_user_data(user_data *ud)
{
    if (ud->usernm) free(ud->usernm);
    if (ud->passwd) free(ud->passwd);
    free(ud);
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
                is_nl(c)
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
            return strings_are_equal(word, metadata_file_alias);
        case mps_descr:
            return strings_are_equal(word, metadata_descr_alias);
        case mps_users:
            return strings_are_equal(word, metadata_users_alias);
        default:
            return 1;
    }
}

static int file_exists_and_is_available(const char *filename, const char *dirname)
{
    int result = 1;
    char *full_path = concat_strings(dirname, filename, NULL);

    FILE *f = fopen(full_path, "r");
    if (!f)
        return_defer(0);

    fclose(f);
    f = fopen(full_path, "rw");
    if (!f)
        return_defer(0);

defer:
    if (f) fclose(f);
    if (full_path) free(full_path);
    return result;
}

static int filename_ends_with_meta(const char *filename, size_t len)
{
    if (len < METAFILE_EXT_LEN)
        return 0;

    const char *fnp = filename+(len-1);
    for (int i = METAFILE_EXT_LEN-1; i >= 0; i--) {
        if (*fnp != metafile_extension[i])
            return 0;

        fnp--;
    }

    return 1;
}

static file_metadata *parse_meta_file(FILE *f, const char *dirpath)
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

        size_t len = strlen(w_buf);
        switch (state) {
            case mps_file:
                if (!is_nl(break_c))
                    goto defer;

                if (
                        !filename_ends_with_meta(w_buf, len) && 
                        file_exists_and_is_available(w_buf, dirpath)
                   ) {
                    if (len > METADATA_MAX_NAME_LEN)
                        goto defer;

                    fmd->name = strndup(w_buf, len);

                    state = mps_descr;
                    parsed_alias = 0;
                } else
                    goto defer;
                break;

            case mps_descr:
                if (len > METADATA_MAX_DESCR_LEN)
                    goto defer;

                fmd->descr = strndup(w_buf, len);

                state = mps_users;
                parsed_alias = 0;
                break;

            case mps_users:
                if (len > METADATA_MAX_LOGIN_ITEM_LEN)
                    goto defer;

                if (fmd->is_for_all_users) {
                    state = mps_error;
                    goto defer;
                }

                if (strings_are_equal(w_buf, all_users_symbol)) {
                    if (fmd->cnt != 0) {
                        state = mps_error;
                        goto defer;
                    }

                    fmd->is_for_all_users = 1;
                } else {
                    int add_res = add_string_to_string_array(&fmd->users, w_buf,
                                                             &fmd->cnt, &fmd->cap,
                                                             METADATA_BASE_USER_CAP,
                                                             METADATA_MAX_USER_CNT);
                    if (!add_res)
                        goto defer;
                }

                if (is_nl(break_c))
                    state = mps_finished;

                break;
            
            case mps_finished:
                if (len > 0) {
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

static int parse_data_dir(database *db)
{
    int result = 1;
    DIR *data_dir = NULL;
    struct dirent *dent;

    size_t cnt = 0,
           cap = METAFILES_BASE_CAP;

    data_dir = opendir(db->data_path);
    if (!data_dir)
        return_defer(0);

    db->file_metas = malloc(cap * sizeof(*db->file_metas));

    file_metadata *fmd = NULL;
    while ((dent = readdir(data_dir)) != NULL) {
        if (dent->d_type != DT_REG && dent->d_type != DT_UNKNOWN)
            continue;

        size_t len = strlen(dent->d_name);
        if (!filename_ends_with_meta(dent->d_name, len))
            continue;

        char *full_path = concat_strings(db->data_path, dent->d_name, NULL);
        FILE *mf = fopen(full_path, "r");
        free(full_path);
        if (!mf)
            return_defer(0);

        fmd = parse_meta_file(mf, db->data_path);
        fclose(mf);
        
        if (!fmd)
            return_defer(0);

        int resize = 0;
        while (cnt >= cap-1) {
            if (!resize)
                resize = 1;
            cap += METAFILES_BASE_CAP;
        }
    
        if (cap-1 > METAFILES_MAX_CNT)
            return_defer(0);

        if (resize)
            db->file_metas = realloc(db->file_metas, cap * sizeof(*(db->file_metas)));

        db->file_metas[cnt] = fmd;
        cnt++;

        fmd = NULL;
    }

    db->file_metas[cnt] = NULL;

defer:
    if (fmd) free(fmd);
    if (!result && db->file_metas) {
        for (size_t i = 0; i < cnt; i++)
            free_metadata(db->file_metas[i]);
        free(db->file_metas);
        db->file_metas = NULL;
    }
    if (data_dir) closedir(data_dir);
    return result;
}

static int username_is_new(user_data **datas, const char *new_usernm)
{
    for (; *datas; datas++) {
        if (strings_are_equal(new_usernm, (*datas)->usernm))
            return 0;
    }

    return 1;
}

static int parse_passwd_file(database *db, const char *path)
{
    int result = 1;

    char buf[MAX_LOGIN_ITEM_LEN];

    FILE *passwd_f;
    size_t cnt = 0,
           cap = USERS_BASE_CAP;

    passwd_f = fopen(path, "r");
    if (!passwd_f)
        return_defer(0);

    db->user_datas = malloc(cap * sizeof(*db->file_metas));
    db->user_datas[0] = NULL;

    int break_c;
    user_data *ud = NULL;
    for (;;) {
        break_c = read_word_to_buf(passwd_f, buf, sizeof(buf), 1);
        if (break_c == 0)
            return_defer(0);
        
        size_t len = strlen(buf);
        if (len > MAX_LOGIN_ITEM_LEN)
            return_defer(0);

        if (len == 0)
            goto case_eof;
        if (!ud) {
            if (
                    break_c != WORD_SEP || 
                    check_spc(buf) ||
                    !username_is_new(db->user_datas, buf)
               ) {
                return_defer(0);
            }

            ud = create_user_data();
            ud->usernm = strndup(buf, len);
        } else {
            if ((!is_nl(break_c) && break_c != EOF)  || check_spc(buf))
                return_defer(0);

            ud->passwd =  strndup(buf, len);

            int resize = 0;
            while (cnt >= cap-1) {
                if (!resize)
                    resize = 1;
                cap += USERS_BASE_CAP;
            }

            if (cap-1 > USERS_MAX_CNT) {
                return_defer(0);
            }

            if (resize)
                db->user_datas = realloc(db->user_datas, cap * sizeof(*(db->user_datas)));

            db->user_datas[cnt] = ud;
            cnt++;
            db->user_datas[cnt] = NULL;

            ud = NULL;
        }

case_eof:
        if (break_c == EOF) {
            if (ud)
                return_defer(0);

            break;
        }
    }

defer:
    if (ud) free(ud);
    if (!result && db->user_datas) {
        for (size_t i = 0; i < cnt; i++)
            free_user_data(db->user_datas[i]);
        free(db->user_datas);
        db->user_datas = NULL;
    }
    if (passwd_f) fclose(passwd_f);
    return result;
}

int db_init(database* db, const char *path)
{ 
    int result = 1;

    char *path_copy = NULL,
         *passwd_path = NULL;
    size_t path_len = strlen(path);

    db->data_path = NULL;
    db->file_metas = NULL;
    db->user_datas = NULL;

    if (path[path_len-1] == '/')
        path_len--;
    path_copy = strndup(path, path_len);

    passwd_path = concat_strings(path_copy, passwd_rel_path, NULL);
    db->data_path = concat_strings(path_copy, data_rel_path, NULL);

    if (!parse_passwd_file(db, passwd_path))
        return_defer(0);
    if (!parse_data_dir(db))
        return_defer(0);

defer:
    if (!result)
        db_deinit(db);
    if (passwd_path) free(passwd_path);
    if (path_copy) free(path_copy);
    return result;
}

void db_deinit(database* db)
{
    if (db->file_metas) {
        for (file_metadata **fmdp = db->file_metas; *fmdp; fmdp++)
            free_metadata(*fmdp);
        free(db->file_metas);
    }
    if (db->user_datas) {
        for (user_data **udp = db->user_datas; *udp; udp++)
            free_user_data(*udp);
        free(db->user_datas);
    }
    if (db->data_path) free(db->data_path);

    db->data_path = NULL;
    db->file_metas = NULL;
    db->user_datas = NULL;
}

int try_match_credentials(database* db, const char *usernm, const char *passwd)
{
    for (user_data **udp = db->user_datas; *udp; udp++) {
        if (
                strings_are_equal((*udp)->usernm, usernm) && 
                strings_are_equal((*udp)->passwd, passwd)
           ) {
            return 1;
        }
    }

    return 0;
}

// @TODO: char *lookup_file(database *db, const char *filename, const char *username);
//      restrict access to user
char *lookup_file(database *db, const char *filename)
{
    for (file_metadata **fmdp = db->file_metas; *fmdp; fmdp++) {
        if (strings_are_equal((*fmdp)->name, filename))
            return concat_strings(db->data_path, (*fmdp)->name, NULL);
    }

    return NULL;
}
