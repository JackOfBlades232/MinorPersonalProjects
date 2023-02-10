/* shootemup_v2/player.h */
#ifndef PLAYER_SENTRY
#define PLAYER_SENTRY

#include "geom.h"
#include "graphics.h"

typedef struct tag_player {
    point pos;
    int cur_hp, max_hp;
} player;

void init_player(player *p, int max_hp, term_state *ts);

void show_player(player *p);
void hide_player(player *p);
void move_player(player *p, int dx, int dy, term_state *ts);

int point_is_in_player(player *pl, point pt);

#endif
