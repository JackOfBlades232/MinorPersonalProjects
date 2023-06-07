/* bbs/test.c */
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    p_message *msg = p_create_message(r_server, ts_init);
    p_add_word_to_message(msg, "11122");
    p_add_word_to_message(msg, "21122");
    p_add_word_to_message(msg, "dnfa");
    p_add_word_to_message(msg, "d::a");
    p_add_word_to_message(msg, "lkasf,,,afnk");
    p_add_word_to_message(msg, "lkasfnlkafnk");
    p_add_word_to_message(msg, "lkasfnlkafnk");

    printf("asd\014asd\n");

    p_sendable_message smsg = p_construct_sendable_message(msg);
    for (size_t i = 0; i < smsg.len; i++) {
        putchar(smsg.str[i]);
    }
    putchar('\n');

    p_message_reader reader;
    p_init_reader(&reader);

    int pr_res = p_reader_process_str(&reader, smsg.str, 10);
    if (pr_res != 0) {
        printf("1");
        return 1;
    }
    pr_res = p_reader_process_str(&reader, smsg.str + 10, 15);
    if (pr_res != 0) {
        printf("2");
        return 1;
    }
    pr_res = p_reader_process_str(&reader, smsg.str + 25, smsg.len - 25);

    putchar('\n');
    printf("%d %d\n", pr_res, reader.state);
    if (pr_res == 1) {
        printf("%d %d | %d %d\n", reader.msg->role, reader.msg->type,
                                  reader.msg->cnt, reader.msg->cap);
        for (size_t i = 0; i < reader.msg->cnt; i++)
            puts(reader.msg->words[i]);
    }

    // No frees cuz lazy
    return 0;
}
