/* shootemup_v2/collisions.c */
#include "collisions.h"
#include "asteroid.h"
#include "player.h"

#include <curses.h>
#include <stdlib.h>

void process_bullet_to_asteroid_collisions(
        player_bullet_buf bullet_buf, asteroid_buf ast_buf)
{
    int i, j;
    
    for (i = 0; i < player_bullet_bufsize; i++) {
        player_bullet *b = bullet_buf + i;
        if (!b->is_alive)
            continue;

        for (j = 0; j < asteroid_bufsize; j++) {
            asteroid *as = ast_buf + j;
            if (as->is_alive && point_is_in_asteroid(as, b->pos)) {
                damage_asteroid(as, b->damage);
                kill_bullet(b);
            }
        }
    }
}

void process_asteroid_to_player_collisions(
        player *p, asteroid_buf ast_buf)
{
}
