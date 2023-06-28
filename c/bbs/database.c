/* bbs/database.c */
#include "database.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "debug.h"

static const char passwd_rel_path[] = "/passwd.txt";
static const char data_rel_path[] = "/data/";
static const char notes_rel_path[] = "/notes.txt";

static const char metafile_extension[] = ".meta";

#define WORD_SEP ' '

enum {
    WORD_BUFSIZE = 256,

    METADATA_BASE_NAME_BUFSIZE = 16,
    METADATA_BASE_DESCR_BUFSIZE = 32,
    METADATA_BASE_USER_CAP = 4,
    METADATA_BASE_LOGIN_ITEM_CAP = 16,

    METADATA_MAX_NAME_LEN = MAX_FILENAME_LEN,
    METADATA_MAX_DESCR_LEN = MAX_DESCR_LEN,
    METADATA_MAX_USER_CNT = MAX_USER_CNT,
    METADATA_MAX_LOGIN_ITEM_LEN = MAX_LOGIN_ITEM_LEN,

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

typedef enum notes_file_parse_state_tag {
    mfps_user,
    mfps_note,
    mfps_delim
} notes_file_parse_state;

static const char metadata_file_alias[] = "file:";
static const char metadata_descr_alias[] = "description:";
static const char metadata_users_alias[] = "access:";

static file_metadata *create_metadata(int is_complete)
{
    file_metadata *fmd = malloc(sizeof(*fmd));
    fmd->name = NULL;
    fmd->descr = NULL;
    fmd->is_for_all_users = 0;
    fmd->cnt = 0;
    fmd->cap = METADATA_BASE_USER_CAP;
    fmd->users = malloc(fmd->cap * sizeof(*fmd->users));
    fmd->is_complete = is_complete;
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

static user_data *create_user_data(user_type utype)
{
    user_data *ud = malloc(sizeof(*ud));
    ud->usernm = NULL;
    ud->passwd = NULL;
    ud->type = utype;
    return ud;
}

static void free_user_data(user_data *ud)
{
    if (ud->usernm) free(ud->usernm);
    if (ud->passwd) free(ud->passwd);
    free(ud);
}

static int read_word_to_buf(FILE *f, char *buf, size_t bufsize, char sep)
{
    size_t i = 0;
    int c;

    if (bufsize == 0)
        return 0;

    while ((c = getc(f)) != EOF) {
        if (c == sep || is_nl(c))
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

    result =
        access(full_path, F_OK) == 0 && 
        access(full_path, R_OK) == 0 &&
        access(full_path, W_OK) == 0;

    free(full_path);
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

static int metafile_name_is_correct(const char *metafile_name, const char *filename)
{
    for (; *metafile_name == *filename; metafile_name++, filename++) {}
    if (*filename)
        return 0;

    return strings_are_equal(metafile_name, metafile_extension);
}

static file_metadata *parse_meta_file(const char *filename, const char *dirpath)
{
    char w_buf[WORD_BUFSIZE];
    int break_c;

    metadata_parse_state state = mps_file;
    int parsed_alias = 0;

    char *full_path = concat_strings(dirpath, filename, NULL);
    FILE *f = fopen(full_path, "r");
    free(full_path);
    if (!f)
        return NULL;

    file_metadata *fmd = create_metadata(1);

    for (;;) {
        // If reading description content, allow regular word sep (space)
        char sep = (!parsed_alias || state != mps_descr) ? WORD_SEP : '\n'; 

        break_c = read_word_to_buf(f, w_buf, sizeof(w_buf), sep);
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
                        len <= METADATA_MAX_NAME_LEN &&
                        filename_is_stripped(w_buf) &&
                        !filename_ends_with_meta(w_buf, len) && 
                        metafile_name_is_correct(filename, w_buf) &&
                        file_exists_and_is_available(w_buf, dirpath)
                   ) {
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
                if (
                        len > METADATA_MAX_LOGIN_ITEM_LEN ||
                        fmd->is_for_all_users
                   ) { 
                    goto defer;
                } else if (strings_are_equal(w_buf, all_users_symbol)) {
                    if (fmd->cnt != 0)
                        goto defer;

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
    if (f) fclose(f);
    return fmd;
}

static int parse_data_dir(database *db)
{
    int result = 1;
    DIR *data_dir = NULL;
    struct dirent *dent;

    db->metas_cnt = 0;
    db->metas_cap = METAFILES_BASE_CAP;

    file_metadata *fmd = NULL;

    data_dir = opendir(db->data_path);
    if (!data_dir)
        return_defer(0);

    db->file_metas = malloc(db->metas_cap * sizeof(*db->file_metas));

    while ((dent = readdir(data_dir)) != NULL) {
        if (dent->d_type != DT_REG && dent->d_type != DT_UNKNOWN)
            continue;

        size_t len = strlen(dent->d_name);
        if (!filename_ends_with_meta(dent->d_name, len))
            continue;

        fmd = parse_meta_file(dent->d_name, db->data_path);
        
        if (!fmd)
            return_defer(0);

        if (!resize_dynamic_arr(
                    (void **) &db->file_metas, sizeof(*(db->file_metas)),
                    &db->metas_cnt, &db->metas_cap, METAFILES_BASE_CAP, METAFILES_MAX_CNT, 1
                    )) {
            return_defer(0);
        }

        db->file_metas[db->metas_cnt] = fmd;
        db->metas_cnt++;
  
        fmd = NULL;
    }

    db->file_metas[db->metas_cnt] = NULL;

defer:
    if (fmd) free(fmd);
    if (!result && db->file_metas) {
        for (size_t i = 0; i < db->metas_cnt; i++)
            free_metadata(db->file_metas[i]);
        free(db->file_metas);
        db->file_metas = NULL;
        db->metas_cnt = 0;
        db->metas_cap = 0;
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

    db->users_cnt = 0;
    db->users_cap = USERS_BASE_CAP;

    user_data *ud = NULL;

    db->passwd_f = fopen(path, "r");
    if (!db->passwd_f)
        return_defer(0);

    db->user_datas = malloc(db->users_cap * sizeof(*db->file_metas));
    db->user_datas[0] = NULL;

    int break_c;
    for (;;) {
        break_c = read_word_to_buf(db->passwd_f, buf, sizeof(buf), WORD_SEP);
        if (break_c == 0)
            return_defer(0);
        
        size_t len = strlen(buf);
        if (len > MAX_LOGIN_ITEM_LEN)
            return_defer(0);

        if (len == 0)
            goto case_eof;
        if (!ud) {
            if (break_c != WORD_SEP)
                return_defer(0);

            user_type utype = ut_regular;
            if (strings_are_equal(buf, poster_mark))
                utype = ut_poster;
            else if (strings_are_equal(buf, admin_mark))
                utype = ut_admin;

            ud = create_user_data(utype);
            if (utype != ut_regular)
                continue;
        } 

        if (!ud->usernm) {
            if (
                    break_c != WORD_SEP ||
                    check_spc(buf) ||
                    !username_is_new(db->user_datas, buf)
               ) {
                return_defer(0);
            }

            ud->usernm = strndup(buf, len);
        } else if (!ud->passwd) {
            if ((!is_nl(break_c) && break_c != EOF) || check_spc(buf))
                return_defer(0);

            ud->passwd = strndup(buf, len);

            if (!resize_dynamic_arr(
                        (void **) &db->user_datas, sizeof(*(db->user_datas)),
                        &db->users_cnt, &db->users_cap, USERS_BASE_CAP, USERS_MAX_CNT, 1
                        )) {
                return_defer(0);
            }

            db->user_datas[db->users_cnt] = ud;
            db->users_cnt++;
            db->user_datas[db->users_cnt] = NULL;

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
    if (!result) {
        if (db->user_datas) {
            for (size_t i = 0; i < db->users_cnt; i++)
                free_user_data(db->user_datas[i]);
            free(db->user_datas);
            db->user_datas = NULL;
            db->users_cnt = 0;
            db->users_cap = 0;
        }
        if (db->passwd_f) fclose(db->passwd_f);
    } else if (db->passwd_f) {
        fclose(db->passwd_f);
        db->passwd_f = fopen(path, "a");
        if (!db->passwd_f)
            result = 0;
    }
    return result;
}

static int parse_notes_file(database *db)
{
    int result = 1;

    db->notes_f = fopen(db->notes_path, "r");
    if (!db->notes_f)
        return 0;

    char buf[MAX_NOTE_LEN];
    int break_c;
    notes_file_parse_state state = mfps_user;
    for (;;) {
        break_c = read_word_to_buf(db->notes_f, buf, sizeof(buf), '\n');

        if (break_c == 0)
            return_defer(0);

        size_t len = strlen(buf);
        
        switch (state) {
            case mfps_user:
                if (len == 0) {
                    if (break_c == EOF)
                        return_defer(1);
                    else
                        continue;
                } else if (len > MAX_LOGIN_ITEM_LEN || break_c != '\n')
                    return_defer(0);
                state = mfps_note;
                break;

            case mfps_note:
                if (len > MAX_NOTE_LEN || break_c != '\n')
                    return_defer(0);
                state = mfps_delim;
                break;

            case mfps_delim:
                if (len != 0 || break_c != '\n')
                    return_defer(0);
                state = mfps_user;
                break;
        }
    }

defer:
    if (!result && db->notes_f) {
        fclose(db->notes_f);
        db->notes_f = NULL;
    } else if (db->notes_f) {
        fclose(db->notes_f);
        db->notes_f = fopen(db->notes_path, "a");
        if (!db->notes_f)
            result = 0;
    }
    return result;    
}

int db_init(database* db, const char *path)
{ 
    int result = 1;

    char *path_copy = NULL,
         *passwd_path = NULL,
         *notes_path = NULL;
    size_t path_len = strlen(path);

    db->data_path = NULL;
    db->notes_path = NULL;
    db->passwd_f = NULL;
    db->notes_f = NULL;
    db->file_metas = NULL;
    db->user_datas = NULL;

    if (path[path_len-1] == '/')
        path_len--;
    path_copy = strndup(path, path_len);

    passwd_path = concat_strings(path_copy, passwd_rel_path, NULL);
    db->notes_path = concat_strings(path_copy, notes_rel_path, NULL);
    db->data_path = concat_strings(path_copy, data_rel_path, NULL);

    if (!parse_passwd_file(db, passwd_path))
        return_defer(0);
    if (!parse_notes_file(db))
        return_defer(0);
    if (!parse_data_dir(db))
        return_defer(0);

defer:
    if (!result)
        db_deinit(db);
    if (notes_path) free(notes_path);
    if (passwd_path) free(passwd_path);
    if (path_copy) free(path_copy);
    return result;
}

static int truncate_notes_file(database *db)
{
    int result = 1;

    char *new_notes_path = NULL;
    FILE *new_nf = NULL;

    FILE *nf = fopen(db->notes_path, "r");
    if (!nf)
        return 0;

    int c;
    int has_zeroes = 0;
    while ((c = fgetc(nf)) == '\0') { 
        if (!has_zeroes)
            has_zeroes = 1;
    }
    if (!has_zeroes)
        return_defer(1);

    new_notes_path = concat_strings(db->notes_path, ".new", NULL);
    new_nf = fopen(new_notes_path, "w");
    if (!new_nf)
        return_defer(0);

    if (c != '\n')
        fputc(c, new_nf);

    while ((c = fgetc(nf)) != EOF)
        fputc(c, new_nf);

    fclose(new_nf);
    fclose(nf);
    new_nf = NULL;
    nf = NULL;

    unlink(db->notes_path);
    rename(new_notes_path, db->notes_path);

defer:
    if (new_nf) fclose(new_nf);
    if (new_notes_path) free(new_notes_path);
    if (nf) fclose(nf);
    return result;
}

void db_deinit(database* db)
{
    if (db->notes_path) {
        if (!truncate_notes_file(db))
            debug_printf_err("Failed to truncate notes file\n");
    }
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
    if (db->passwd_f) fclose(db->passwd_f);
    if (db->notes_f) fclose(db->notes_f);
    if (db->notes_path) free(db->notes_path);
    if (db->data_path) free(db->data_path);

    db->data_path = NULL;
    db->notes_path = NULL;
    db->passwd_f = NULL;
    db->notes_f = NULL;
    db->file_metas = NULL;
    db->user_datas = NULL;
}

user_type db_try_match_credentials(database* db, const char *usernm, const char *passwd)
{
    for (user_data **udp = db->user_datas; *udp; udp++) {
        if (
                strings_are_equal((*udp)->usernm, usernm) && 
                strings_are_equal((*udp)->passwd, passwd)
           ) {
            return (*udp)->type;
        }
    }

    return ut_none;
}

int db_file_is_available_to_user(file_metadata *fmd, const char *username, user_type utype)
{
    if (utype == ut_admin || fmd->is_for_all_users)
        return 1;
    else if (!username)
        return 0;

    for (size_t i = 0; i < fmd->cnt; i++) {
        if (strings_are_equal(fmd->users[i], username))
            return 1;
    }

    return 0;
}

file_lookup_result db_lookup_file(database *db, const char *filename, 
                                  const char *username, user_type utype, 
                                  char **out, file_metadata **fmd_out)
{
    if (!filename_is_stripped(filename))
        return not_found;

    file_lookup_result res = not_found;

    for (file_metadata **fmdp = db->file_metas; *fmdp; fmdp++) {
        if (strings_are_equal((*fmdp)->name, filename)) {
            res = no_access;

            if (db_file_is_available_to_user(*fmdp, username, utype)) {
                if (out)
                    *out = concat_strings(db->data_path, (*fmdp)->name, NULL);
                if (fmd_out)
                    *fmd_out = *fmdp;
                return (*fmdp)->is_complete ? found : incomplete;
            }
        }
    }

    return res;
}

void db_store_note(database *db, const char *username, byte_arr note)
{
    fputs(username, db->notes_f);
    fputc('\n', db->notes_f);
    fputs(note.str, db->notes_f); // since '\0' is included
    fputs("\n\n", db->notes_f);
    fflush(db->notes_f);
}

add_file_result db_try_add_file(database *db, const char *filename, const char *descr,
                                const char **users, size_t users_cnt)
{
    add_file_result result = { dmod_fail, -1, NULL };
    FILE *meta_f = NULL;
    char *full_filename = NULL,
         *metafile_name = NULL;

    if (
            !filename || !descr ||
            strlen(filename) > MAX_FILENAME_LEN || 
            strlen(descr) > MAX_DESCR_LEN ||
            users_cnt > MAX_USER_CNT ||
            !filename_is_stripped(filename)
       ) {
        result.type = dmod_err;
        return result;
    }

    file_lookup_result lookup_res = db_lookup_file(db, filename, NULL, ut_admin, NULL, NULL);
    if (lookup_res != not_found)
        return result;

    result.fmd = create_metadata(0);
    result.fmd->name = strdup(filename);
    result.fmd->descr = strdup(descr);
    result.fmd->cap = users_cnt;
    result.fmd->cnt = users_cnt;
    result.fmd->users = result.fmd->cap == 0 ? NULL : malloc(result.fmd->cap * sizeof(*result.fmd->users));

    if (users_cnt == 1 && strings_are_equal(*users, all_users_symbol)) {
        result.fmd->is_for_all_users = 1;
        result.fmd->cnt = 0;
    } else {
        result.fmd->is_for_all_users = 0;
        size_t i;
        for (i = 0; i < users_cnt; i++) {
            if (
                    strings_are_equal(*users, all_users_symbol) ||
                    strlen(users[i]) > MAX_LOGIN_ITEM_LEN
               ) {
                result.type = dmod_err;
                goto defer;
            }
            
            result.fmd->users[i] = strdup(users[i]);
        }
    }

    full_filename = concat_strings(db->data_path, filename, NULL);
    metafile_name = concat_strings(full_filename, metafile_extension, NULL);

    if (
            access(full_filename, F_OK) == 0 ||
            access(metafile_name, F_OK) == 0
       ) {
        result.type = dmod_err;
        goto defer;
    }

    result.fd = open(full_filename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (result.fd == -1) {
        result.type = dmod_err;
        goto defer;
    }

    meta_f = fopen(metafile_name, "w");
    if (!meta_f) {
        result.type = dmod_err;
        goto defer;
    }

    if (!resize_dynamic_arr(
                (void **) &db->file_metas, sizeof(*(db->file_metas)),
                &db->metas_cnt, &db->metas_cap, METAFILES_BASE_CAP, METAFILES_MAX_CNT, 1
                )) {
        goto defer;
    }

    db->file_metas[db->metas_cnt] = result.fmd;
    db->metas_cnt++;
    db->file_metas[db->metas_cnt] = NULL;

    fprintf(meta_f, "%s %s\n", metadata_file_alias, filename);
    fprintf(meta_f, "%s %s\n", metadata_descr_alias, descr);
    fprintf(meta_f, "%s", metadata_users_alias);
    if (users_cnt > 0) {
        for (size_t i = 0; i < users_cnt; i++)
            fprintf(meta_f, " %s", users[i]);
    } else 
        fprintf(meta_f, " ");
    fputc('\n', meta_f);

    result.type = dmod_ok;

defer:
    if (meta_f) fclose(meta_f);
    if (metafile_name) free(metafile_name);
    if (full_filename) free(full_filename);
    if (result.type != dmod_ok) {
        if (result.fd != -1) {
            close(result.fd);
            result.fd = -1;
        }
        if (result.fmd) free_metadata(result.fmd);
    }
    return result;
}

int db_cleanup_incomplete_meta(database *db, file_metadata *fmd)
{
    file_metadata **fmdp;
    for (fmdp = db->file_metas; *fmdp; fmdp++) {
        if (*fmdp == fmd) 
            break;
    }

    debug_printf("Cleanup meta\n");

    if (!*fmdp) {
        debug_printf_err("Trying to cleanup meta not from the db array\n");
        return 0;
    }

    debug_printf("Found meta, cur cnt: %d\n", db->metas_cnt);

    size_t offset = fmdp - db->file_metas;
    memmove(fmdp, fmdp+1, db->metas_cnt-offset);
    db->metas_cnt--;
    db->file_metas[db->metas_cnt] = NULL;

    debug_printf("Del from arr, new cnt: %d\n", db->metas_cnt);

    char *full_filename = concat_strings(db->data_path, fmd->name, NULL);
    char *metafile_name = concat_strings(full_filename, metafile_extension, NULL);

    debug_printf("Files to unlink: %s, %s\n", full_filename, metafile_name);
    
    unlink(full_filename);
    unlink(metafile_name);
    free_metadata(fmd);

    free(metafile_name);
    free(full_filename);

    return 1;
}

int db_user_exists(database* db, const char *usernm)
{
    for (user_data **udp = db->user_datas; *udp; udp++) {
        if (strings_are_equal((*udp)->usernm, usernm))
            return 1;
    }

    return 0;
}

db_modification_result db_add_user(database* db, const char *usernm, const char *passwd, user_type ut)
{
    user_data *ud;

    if (db_user_exists(db, usernm))
        return dmod_fail;

    ud = create_user_data(ut);
    ud->usernm = strdup(usernm);
    ud->passwd = strdup(passwd);

    if (!resize_dynamic_arr(
                (void **) &db->user_datas, sizeof(*(db->user_datas)),
                &db->users_cnt, &db->users_cap, USERS_BASE_CAP, USERS_MAX_CNT, 1
                )) {
        free(ud);
        return dmod_fail;
    }

    db->user_datas[db->users_cnt] = ud;
    db->users_cnt++;
    db->user_datas[db->users_cnt] = NULL;

    if (ut > ut_regular)
        fprintf(db->passwd_f, "%s ", ut == ut_poster ? poster_mark : admin_mark);
    fprintf(db->passwd_f, "%s %s\n", usernm, passwd);
    fflush(db->passwd_f);

    return dmod_ok;
}

read_note_result db_read_and_rm_top_note(database *db)
{
    read_note_result res = { NULL, NULL };
    FILE *nf = fopen(db->notes_path, "r+");
    if (!nf)
        return res;

    long chars_read = 0;

    char buf[MAX_NOTE_LEN];
    int break_c;
    notes_file_parse_state state = mfps_user;
    for (;;) {
        break_c = read_word_to_buf(nf, buf, sizeof(buf), '\n');
        size_t len = strlen(buf);

        chars_read += len;
        if (break_c != EOF)
            chars_read++;
        
        switch (state) {
            case mfps_user:
                if (len == 0) {
                    if (break_c == EOF)
                        goto defer;
                    else
                        continue;
                }
                res.usernm = strdup(buf);
                state = mfps_note;
                break;

            case mfps_note:
                res.note = strdup(buf);
                state = mfps_delim;
                break;

            case mfps_delim:
                goto defer;
        }
    }

defer:
    if (chars_read > 0) {
        fseek(nf, -chars_read, SEEK_CUR);
        for (; chars_read > 1; chars_read--)
            fputc('\0', nf);
        fputc('\n', nf);
    }
    debug_printf("%s: %s\n", res.usernm, res.note);
    if (nf) fclose(nf);
    return res;
}

db_modification_result db_try_edit_metadata(database *db,
                                            const char *filename, const char *descr,
                                            const char **users, size_t users_cnt)
{
    db_modification_result result = dmod_ok;
    FILE *meta_f = NULL;
    char *metafile_name = NULL;
    
    if (
            !filename || !descr ||
            strlen(filename) > MAX_FILENAME_LEN || 
            strlen(descr) > MAX_DESCR_LEN ||
            users_cnt > MAX_USER_CNT ||
            !filename_is_stripped(filename)
       ) {
        return dmod_err;
    }

    file_metadata *fmd;
    file_lookup_result lookup_res = db_lookup_file(db, filename, NULL, ut_admin, NULL, &fmd);
    if (lookup_res == not_found)
        return dmod_fail;

    if (fmd->descr) free(fmd->descr);
    if (fmd->users) {
        for (size_t i = 0; i < fmd->cnt; i++)
            free(fmd->users[i]);
        free(fmd->users);
    }

    fmd->descr = strdup(descr);
    fmd->cap = users_cnt;
    fmd->cnt = users_cnt;
    fmd->users = fmd->cap == 0 ? NULL : malloc(fmd->cap * sizeof(*fmd->users));

    if (users_cnt == 1 && strings_are_equal(*users, all_users_symbol)) {
        fmd->is_for_all_users = 1;
        fmd->cnt = 0;
    } else {
        fmd->is_for_all_users = 0;
        size_t i;
        for (i = 0; i < users_cnt; i++) {
            if (
                    strings_are_equal(*users, all_users_symbol) ||
                    strlen(users[i]) > MAX_LOGIN_ITEM_LEN
               ) {
                return_defer(dmod_err);
            }
            
            fmd->users[i] = strdup(users[i]);
        }
    }

    metafile_name = concat_strings(db->data_path, filename, metafile_extension, NULL);

    if (access(metafile_name, W_OK) == -1)
        return_defer(dmod_err);

    meta_f = fopen(metafile_name, "w");
    if (!meta_f)
        return_defer(dmod_err);

    fprintf(meta_f, "%s %s\n", metadata_file_alias, filename);
    fprintf(meta_f, "%s %s\n", metadata_descr_alias, descr);
    fprintf(meta_f, "%s", metadata_users_alias);
    if (users_cnt > 0) {
        for (size_t i = 0; i < users_cnt; i++)
            fprintf(meta_f, " %s", users[i]);
    } else 
        fprintf(meta_f, " ");
    fputc('\n', meta_f);

defer:
    if (meta_f) fclose(meta_f);
    if (metafile_name) free(metafile_name);
    return result;
}

db_modification_result db_try_delete_file(database *db, const char *filename)
{
    db_modification_result result = dmod_ok;

    file_metadata *fmd = NULL;
    char *full_filename = NULL,
         *metafile_name = NULL;
    file_lookup_result lookup_res = db_lookup_file(db, filename, NULL, ut_admin, &full_filename, &fmd);
    if (lookup_res != found) {
        debug_printf("Can not delete the file\n");
        return_defer(dmod_fail);
    }

    file_metadata **fmdp;
    for (fmdp = db->file_metas; *fmdp; fmdp++) {
        if (*fmdp == fmd) 
            break;
    }

    if (!*fmdp) {
        debug_printf_err("Trying to delete file not from meta array\n");
        return_defer(dmod_err);
    }

    size_t offset = fmdp - db->file_metas;
    memmove(fmdp, fmdp+1, db->metas_cnt-offset);
    db->metas_cnt--;
    db->file_metas[db->metas_cnt] = NULL;

    metafile_name = concat_strings(full_filename, metafile_extension, NULL);
    
    unlink(full_filename);
    unlink(metafile_name);
    free_metadata(fmd);

defer:
    if (metafile_name) free(metafile_name);
    if (full_filename) free(full_filename);

    return result;
}
