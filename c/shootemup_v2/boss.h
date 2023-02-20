/* shootemup_v2/boss.h */
#ifndef BOSS_SENTRY
#define BOSS_SENTRY

#include "explosion.h"
#include "graphics.h"
#include "geom.h"

enum { boss_width = 19, boss_height = 8 };
enum { boss_projectile_bufsize = 128 };

typedef struct tag_boss_state {
    int cur_hp, max_hp;
    int frames_since_moved, cur_movement_frames;
    int frames_since_shot;
    int bullet_dmg, gunshot_dmg, mine_dmg;
} boss_state;

typedef struct tag_boss {
    point pos;
    boss_state state;
} boss;

typedef enum tag_boss_projectile_type {
    bullet, gunshot, mine
} boss_projectile_type;

typedef struct tag_boss_projectile {
    boss_projectile_type type;
    point pos;
    int dx, dy;
    int damage;
    int is_alive;
    int frames_since_moved;
    int mine_frames_to_expl;
} boss_projectile;

typedef boss_projectile boss_projectile_buf[boss_projectile_bufsize];

void init_boss(boss *bs, 
        int max_hp, int bullet_dmg, int gunshot_dmg, int mine_dmg,
        term_state *ts);

void show_boss(boss *bs);
void hide_boss(boss *bs);
void move_boss(boss *bs, int dx, int dy, term_state *ts);

int point_is_in_boss(boss *bs, point pt);

int damage_boss(boss *bs, int damage);
int boss_is_dead(boss *bs);

void init_boss_projectile_buf(boss_projectile_buf projectile_buf);

int boss_shoot_bullet(boss *bs, boss_projectile_buf projectile_buf);
int boss_shoot_gun(boss *bs, boss_projectile_buf projectile_buf);
int boss_plant_mines(boss *bs, boss_projectile_buf projectile_buf,
        term_state *ts);
int boss_emit_force_field(boss *bs, explosion_buf expl_buf, term_state *ts);

int distance_is_in_gunshot_expl_reach(int dist, boss_projectile *pr);

void set_gunshot_off(boss_projectile *pr, explosion_buf expl_buf, 
        term_state *ts);
void set_mine_off(boss_projectile *pr, explosion_buf expl_buf, term_state *ts);

void update_live_boss_projectiles(boss_projectile_buf projectile_buf,
        explosion_buf expl_buf, term_state *ts);
int kill_boss_projectile(boss_projectile *pr);

void update_boss_frame_counters(boss *bs);

#endif
