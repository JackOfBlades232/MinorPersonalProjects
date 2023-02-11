/* shootemup_v2/player.h */
#ifndef PLAYER_SENTRY
#define PLAYER_SENTRY

#include "geom.h"
#include "graphics.h"

enum { player_bullet_bufsize = 64 };

typedef struct tag_player {
    point pos;
    int cur_hp, max_hp;
} player;

typedef struct tag_player_bullet {
    point pos;
    int dx, dy;
    int damage;
    int is_alive;
    int frames_until_move;
} player_bullet;

typedef player_bullet player_bullet_buf[player_bullet_bufsize];

void init_player(player *p, int max_hp, term_state *ts);

void show_player(player *p);
void hide_player(player *p);
void move_player(player *p, int dx, int dy, term_state *ts);

int point_is_in_player(player *pl, point pt);

void init_player_bullet_buf(player_bullet_buf bullet_buf);

int player_shoot(player *p, player_bullet_buf bullet_buf);
void update_live_bullets(player_bullet_buf bullet_buf);
int kill_bullet(player_bullet *b);

#endif
