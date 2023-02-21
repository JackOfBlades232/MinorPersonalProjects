/* shootemup_v2/boss_ai.h */
#ifndef BOSS_AI_SENTRY
#define BOSS_AI_SENTRY

#include "boss.h"
#include "player.h"
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
} boss_movement;

typedef enum tag_boss_attack_type {
    no_attack, bullet_burst, gun_volley, mine_field, force_blast
} boss_attack_type;

typedef struct tag_boss_attack {
    boss_attack_type type;
    int frames; 
    int completed;
} boss_attack;

typedef struct tag_boss_behaviour_state {
    boss_movement movement;
    boss_attack attack;
} boss_behaviour_state;

struct tag_boss_behaviour;

typedef int (*boss_movement_func)(
        struct tag_boss_behaviour *, boss *, player *, term_state *
        );

typedef int (*boss_attack_func)(struct tag_boss_behaviour *, boss *, player *);

typedef struct tag_boss_sequence_elem {
    boss_movement_func move;
    boss_attack_func atk;
} boss_sequence_elem;

typedef struct tag_boss_behaviour {
    const boss_sequence_elem *current_sequence;
    boss_behaviour_state current_state;
} boss_behaviour;

void init_boss_ai(boss_behaviour *beh);

/* TODO: these, too, are to become static */
int perform_bullet_burst(boss_behaviour *beh);
int perform_battering_ram(boss_behaviour *beh);
int perform_mine_plant(boss_behaviour *beh);
int perform_force_blast(boss_behaviour *beh);
int perform_sliding_volley(boss_behaviour *beh);

/* Main function */
void tick_boss_ai(boss_behaviour *beh, boss *bs, player *p,
        boss_projectile_buf proj_buf, explosion_buf expl_buf, term_state *ts);

#endif
