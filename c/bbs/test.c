/* bbs/test.c */
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
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
    return 0;
}
