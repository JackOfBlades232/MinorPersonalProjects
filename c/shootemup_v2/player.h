/* shootemup_v2/player.h */
#ifndef PLAYER_SENTRY
#define PLAYER_SENTRY

#include "geom.h"
#include "graphics.h"

enum { player_width = 8, player_height = 4 };
enum { player_bullet_bufsize = 64 };

typedef struct tag_player_state {
    int cur_hp, max_hp;
    int cur_ammo, max_ammo;
    int frames_since_shot;
    int bullet_dmg;
    int score;
} player_state;

typedef struct tag_player {
    point pos;
    player_state state;
} player;

typedef struct tag_player_bullet {
    point pos;
    int dx, dy;
    int damage;
    int is_alive;
    int frames_until_move;
} player_bullet;

typedef player_bullet player_bullet_buf[player_bullet_bufsize];

void init_player(player *p, 
        int max_hp, int max_ammo, int bullet_dmg,
        term_state *ts);

void show_player(player *p);
void hide_player(player *p);
void move_player(player *p, int dx, int dy, term_state *ts);

int point_is_in_player(player *pl, point pt);

int damage_player(player *p, int damage);
int player_is_dead(player *p);

void add_score(player *p, int d_score);

void init_player_bullet_buf(player_bullet_buf bullet_buf);

int player_shoot(player *p, player_bullet_buf bullet_buf);
void handle_player_ammo_replenish(player *p);
void update_live_bullets(player_bullet_buf bullet_buf);
int kill_bullet(player_bullet *b);

#endif
