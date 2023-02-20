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
        struct tag_boss_behaviour *, boss *, term_state *
        );

typedef int (*boss_attack_func)(struct tag_boss_behaviour *, boss *);

typedef struct tag_boss_sequence_elem {
    boss_movement_func move;
    boss_attack_func atk;
} boss_sequence_elem;

typedef struct tag_boss_behaviour {
    boss_sequence_elem *current_sequence;
    boss_behaviour_state current_state;
} boss_behaviour;

void init_boss_ai(boss_behaviour *beh);

/* TODO: make static later */
int perform_boss_attack(boss_behaviour *beh, boss *bs,
                        boss_attack_type type, int frames);

/* Main function */
void tick_boss_ai(boss_behaviour *beh, boss *bs,
        boss_projectile_buf proj_buf, explosion_buf expl_buf, term_state *ts);

#endif
