/* shootemup_v2/colors.h */
#ifndef COLORS_SENTRY
#define COLORS_SENTRY

enum { 
    player_color_pair = 1,
    asteroid_color_pair = 2,
    pbullet_color_pair_1 = 3,
    pbullet_color_pair_2 = 4,
    hcrate_color_pair = 5,
    hud_color_pair = 6,
    menu_color_pair = 7,
};

void init_color_pairs();
int get_color_pair(int pair_idx);
int switch_and_get_pbullet_color_pair();

#endif
