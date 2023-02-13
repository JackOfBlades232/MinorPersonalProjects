/* shootemup_v2/collisions.h */
#ifndef COLLISIONS_SENTRY
#define COLLISIONS_SENTRY

#include "player.h"
#include "asteroid.h"
#include "spawn.h"

void process_bullet_to_asteroid_collisions(player_bullet_buf bullet_buf, 
        asteroid_buf ast_buf, spawn_area *sa);
void process_asteroid_to_player_collisions(player *p,
        asteroid_buf ast_buf, spawn_area *sa);

#endif
