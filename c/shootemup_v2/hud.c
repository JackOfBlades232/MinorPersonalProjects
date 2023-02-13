/* shootemup_v2/hud.c */
#include "hud.h"

#include <curses.h>

void draw_hud_plack(player_state *ps)
{
    int i;
    
    move(0, 0);
    addstr("                                            "); /* erase old */
    
    move(0, 0);
    addstr("HP: ");

    for (i = 0; i < ps->cur_hp; i++)
        addch('o');
    for (i = ps->cur_hp; i < ps->max_hp; i++)
        addch('-');

    printw(" | Ammo: %d/%d | Score: %d", ps->cur_ammo, ps->max_ammo, ps->score);
}
