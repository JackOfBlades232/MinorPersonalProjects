/* shootemup_v2/colors.c */
#include "colors.h"

#include <curses.h>

static int pbullet_color_pair;

void init_color_pairs()
{
    init_pair(player_color_pair, COLOR_WHITE, COLOR_BLACK);
    init_pair(asteroid_color_pair, COLOR_YELLOW, COLOR_BLACK);
    init_pair(pbullet_color_pair_1, COLOR_BLUE, COLOR_BLACK);
    init_pair(pbullet_color_pair_2, COLOR_GREEN, COLOR_BLACK);
    init_pair(hcrate_color_pair, COLOR_WHITE, COLOR_RED);
    init_pair(hud_color_pair, COLOR_BLACK, COLOR_WHITE);
    init_pair(menu_color_pair, COLOR_WHITE, COLOR_BLACK);

    pbullet_color_pair = pbullet_color_pair_1;
}

int get_color_pair(int pair_idx)
{
    return COLOR_PAIR(pair_idx);
}

int switch_and_get_pbullet_color_pair()
{
    pbullet_color_pair = pbullet_color_pair == pbullet_color_pair_1 ?
        pbullet_color_pair_2 :
        pbullet_color_pair_1;

    return COLOR_PAIR(pbullet_color_pair);
}
