/* shootemup_v2/collisions.h */
#ifndef COLLISIONS_SENTRY
#define COLLISIONS_SENTRY

#include "player.h"
#include "boss.h"
#include "asteroid.h"
#include "buffs.h"
#include "spawn.h"

/* Asteroid field phase */
void process_bullet_to_asteroid_collisions(player_bullet_buf bullet_buf, 
        asteroid_buf ast_buf, spawn_area *sa, player *p);
void process_bullet_to_crate_collisions(player_bullet_buf bullet_buf, 
        crate_buf cr_buf, spawn_area *sa);
void process_asteroid_to_player_collisions(player *p,
        asteroid_buf ast_buf, spawn_area *sa);
void process_crate_to_player_collisions(player *p, 
        crate_buf cr_buf, spawn_area *sa);

/* Boss fight phase */
void process_bullet_to_boss_collisions(player_bullet_buf bullet_buf, boss *bs);
void process_player_to_boss_collisions(player *p, boss *bs);

#endif
