/* shootemup_v2/boss_ai.h */
#ifndef BOSS_AI_SENTRY
#define BOSS_AI_SENTRY

#include "boss.h"
#include "geom.h"
#include "graphics.h"

typedef enum tag_boss_movement_type {
    not_moving, horizontal, vertical, teleport
} boss_movement_type;

typedef struct tag_boss_movement {
    boss_movement_type type;
    union {
        point pos; /* for teleport */
        int x; /* for horiz/vert movement */
        int y;
    } goal;
    int completed;
    boss_movement_type next;
} boss_movement;

typedef enum tag_boss_attack_type {
    no_attack, bullet_burst, gun_volley, mine_field, force_blast
} boss_attack_type;

typedef struct tag_boss_attack {
    boss_attack_type type;
    int frames; 
    int completed;
    boss_attack_type next;
} boss_attack;

typedef enum tag_boss_sequence_type {
    no_sequence,
    side_bullet_spray, side_to_side_shooting, 
    battering_ram, mine_field_plant,
    force_field_emission
} boss_sequence_type;

typedef struct tag_boss_behaviour_state {
    boss_movement current_movement;
    boss_attack current_attack;
} boss_behaviour_state;

/* TODO: make static later */
int move_boss_to_x(boss_behaviour_state *bh, 
        boss *bs, int x, int frames, term_state *ts);
int move_boss_to_y(boss_behaviour_state *bh, 
        boss *bs, int y, int frames, term_state *ts);
int teleport_boss_to_pos(boss_behaviour_state *bh, 
        boss *bs, point pos, term_state *ts);

int perform_boss_bullet_burst(boss_behaviour_state *bh, boss *bs, int frames);
int perform_boss_gun_volley(boss_behaviour_state *bh, boss *bs, int frames);
int perform_boss_mine_plant(boss_behaviour_state *bh, boss *bs);
int perform_boss_force_blast(boss_behaviour_state *bh, boss *bs, int frames);

/* Main function */
void tick_boss_ai(boss_behaviour_state *bh, boss *bs,
        boss_projectile_buf proj_buf, explosion_buf expl_buf, term_state *ts);

#endif
