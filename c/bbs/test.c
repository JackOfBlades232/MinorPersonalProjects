/* bbs/test.c */
#include "database.h"
#include "protocol.h"
#include "utils.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    /*
    p_message *msg = p_create_message(r_client, tc_login);
    p_add_string_to_message(msg, "bruh");
    p_add_string_to_message(msg, "__");
    
    p_sendable_message smsg = p_construct_sendable_message(msg);
    debug_print_buf(smsg.str, smsg.len);

    p_message_reader reader;
    p_init_reader(&reader);

    size_t chars_processed = 0;
    p_reader_process_str(&reader, smsg.str, smsg.len, &chars_processed);

    debug_log_p_message(reader.msg);

    p_deinit_sendable_message(&smsg);
    p_deinit_reader(&reader);
    p_free_message(msg);
    */


    /*
    FILE *f;
    if (argc >= 2) 
        f = fopen(argv[1], "r");
    else
        f = fopen("./database_ex/data/humpty.txt.meta", "r");
    file_metadata *fmd = parse_meta_file(f, "./database_ex/data/");
    if (!fmd) {
        fprintf(stderr, "Parse failed\n");
    } else {
        puts(fmd->name);
        puts(fmd->descr);
        for (size_t i = 0; i < fmd->cnt; i++)
            printf("%s, ", fmd->users[i]);
        putchar('\n');
    }
    */

    if (argc < 2)
        return 0;
    database db;
    int init_res = db_init(&db, argv[1]);
    if (!init_res) 
        fprintf(stderr, "Parse failed\n");
    else {
        for (file_metadata **fmdp = db.file_metas; *fmdp; fmdp++) {
            file_metadata *fmd = *fmdp;
            puts(fmd->name);
            puts(fmd->descr);
            if (fmd->is_for_all_users) 
                printf("Free for all");
            else {
                for (size_t i = 0; i < fmd->cnt; i++)
                    printf("%s ", fmd->users[i]);
            }
            printf("\n\n");
        }
        for (user_data **udp = db.user_datas; *udp; udp++) {
            user_data *ud = *udp;
            printf("\n");
            puts(ud->usernm);
            puts(ud->passwd);
        }

        db_deinit(&db);
    }
    /*
    */

    /*
    debug_cat_file(stderr, "database_ex/data/humpty.txt");
    */

    return 0;
}
