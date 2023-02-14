/* shootemup_v2/hud.c */
#include "hud.h"
#include "colors.h"

#include <curses.h>

enum { max_hud_plack_len = 72 };

static void erase_old_plack()
{
    int i;
    attrset(get_color_pair(0));
    move(0, 0);
    for (i = 0; i < max_hud_plack_len; i++)
        addch(' ');
}

void draw_hud_plack(player_state *ps)
{
    int i;
    
    erase_old_plack();
    
    attrset(get_color_pair(hud_color_pair));
    move(0, 0);
    addstr("HP: ");

    for (i = 0; i < ps->cur_hp; i++)
        addch('o');
    for (i = ps->cur_hp; i < ps->max_hp; i++)
        addch('-');

    printw(" | Ammo: %d/%d | Score: %d", ps->cur_ammo, ps->max_ammo, ps->score);
}
