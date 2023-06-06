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

    p_sendable_message smsg = p_construct_sendable_message(msg);
    for (size_t i = 0; i < smsg.len; i++) {
        if (smsg.str[i] >= '!' && smsg.str[i] <= '~')
            putchar(smsg.str[i]);
    }

    p_deinit_sendable_message(&smsg);
    p_free_message(msg);
    return 0;
}
