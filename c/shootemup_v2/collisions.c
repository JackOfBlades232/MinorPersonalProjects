/* shootemup_v2/collisions.c */
#include "collisions.h"
#include "asteroid.h"
#include "geom.h"
#include "player.h"

void process_bullet_to_asteroid_collisions(player_bullet_buf bullet_buf, 
        asteroid_buf ast_buf, spawn_area *sa)
{
    int i, j;
    
    for (i = 0; i < player_bullet_bufsize; i++) {
        player_bullet *b = bullet_buf + i;
        if (!b->is_alive)
            continue;

        for (j = 0; j < asteroid_bufsize; j++) {
            asteroid *as = ast_buf + j;

            if (as->is_alive && point_is_in_asteroid(as, b->pos)) {
                damage_asteroid(as, b->damage, sa);
                kill_bullet(b);

                break;
            }
        }
    }
}

void process_asteroid_to_player_collisions(player *p,
        asteroid_buf ast_buf, spawn_area *sa)
{
    asteroid *as;
    int x, y;

    for (as = ast_buf; as - ast_buf < asteroid_bufsize; as++) {
        if (!as->is_alive)
            continue;

        for (y = p->pos.y; y < p->pos.y + player_height; y++)
            for (x = p->pos.x; x < p->pos.x + player_width; x++) {
                point pt = point_literal(x, y);
                if (point_is_in_player(p, pt) && point_is_in_asteroid(as, pt)) {
                    damage_player(p, as->data->damage);
                    kill_asteroid(as, sa);
                }
            }
    }

    show_player(p);
}
