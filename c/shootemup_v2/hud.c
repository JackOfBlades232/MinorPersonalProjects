/* shootemup_v2/hud.c */
#include "hud.h"
#include "colors.h"

#include <curses.h>
#include <string.h>

enum { boss_hp_per_symbol = 5 };

static void erase_old_plack(term_state *ts)
{
    int i;
    attrset(get_color_pair(0));
    move(0, 0);
    for (i = 0; i < ts->col; i++)
        addch(' ');
}

void draw_player_hud_plack(player_state *ps, term_state *ts)
{
    int i;
    
    erase_old_plack(ts);
    
    attrset(get_color_pair(hud_color_pair));
    move(0, 0);
    addstr("HP: ");

    for (i = 0; i < ps->cur_hp; i++)
        addch('o');
    for (i = ps->cur_hp; i < ps->max_hp; i++)
        addch('-');

    printw(" | Ammo: %d/%d | Score: %d", ps->cur_ammo, ps->max_ammo, ps->score);
}

void draw_boss_fight_hud_plack(player_state *ps, boss_state *bss,
        term_state *ts)
{
    int i, x, cur_fill, max_fill;

    draw_player_hud_plack(ps, ts);

    cur_fill = (bss->cur_hp + boss_hp_per_symbol - 1) / boss_hp_per_symbol;
    max_fill = (bss->max_hp + boss_hp_per_symbol - 1) / boss_hp_per_symbol;
    
    x = ts->col - (strlen("BOSS HP: ") + max_fill + 1);

    move(0, x);

    addstr("BOSS HP: ");
    for (i = 0; i < cur_fill; i++)
        addch('o');
    for (i = cur_fill; i < max_fill; i++)
        addch('-');
    addch(' ');
}
