/* bbs/test.c */
#include "database.h"
#include "protocol.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    /*
    p_message *msg = p_create_message(r_server, ts_init);
    p_add_word_to_message(msg, "hhf");
    p_add_word_to_message(msg, "232");
    
    p_sendable_message smsg = p_construct_sendable_message(msg);
    for (size_t i = 0; i < smsg.len; i++) {
        putchar(smsg.str[i]);
    }
    putchar('\n');

    p_message_reader reader;
    p_init_reader(&reader);

    int pr_res = p_reader_process_str(&reader, smsg.str, smsg.len);

    putchar('\n');
    printf("%d %d\n", pr_res, reader.state);
    if (pr_res == 1) {
        printf("%d %d | %ld %ld\n", reader.msg->role, reader.msg->type,
                                  reader.msg->cnt, reader.msg->cap);
        for (size_t i = 0; i < reader.msg->cnt; i++)
            puts(reader.msg->words[i]);
    }

    p_deinit_sendable_message(&smsg);
    p_deinit_reader(&reader);
    p_free_message(msg);
    */


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

    return 0;
}
