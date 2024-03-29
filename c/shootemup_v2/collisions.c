/* shootemup_v2/collisions.c */
#include "collisions.h"
#include "geom.h"
#include "player.h"

#define Y_TO_X_RATIO 1/X_TO_Y_RATIO
#define EXPL_COLLISION_TOLERANCE 1.51

void process_bullet_to_asteroid_collisions(player_bullet_buf bullet_buf, 
        asteroid_buf ast_buf, spawn_area *sa, player *p)
{
    int i, j;
    
    for (i = 0; i < player_bullet_bufsize; i++) {
        player_bullet *b = bullet_buf + i;
        if (!b->is_alive)
            continue;

        for (j = 0; j < asteroid_bufsize; j++) {
            asteroid *as = ast_buf + j;

            if (as->is_alive && point_is_in_asteroid(as, b->pos)) {
                damage_asteroid(as, b->damage, sa, p);
                kill_bullet(b);

                break;
            }
        }
    }
}

void process_bullet_to_crate_collisions(player_bullet_buf bullet_buf, 
        crate_buf cr_buf, spawn_area *sa) 
{
    int i, j;
    
    for (i = 0; i < player_bullet_bufsize; i++) {
        player_bullet *b = bullet_buf + i;
        if (!b->is_alive)
            continue;

        for (j = 0; j < crate_bufsize; j++) {
            buff_crate *crate = cr_buf + j;

            if (crate->is_alive && point_is_in_crate(crate, b->pos)) {
                kill_crate(crate, sa);
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

                    add_score(p, as->data->score_for_skip);
                    kill_asteroid(as, sa);

                    goto nest_break_ap;
                }
            }

nest_break_ap:
        if (0) {}
    }

    show_player(p);
}

void process_crate_to_player_collisions(player *p, 
        crate_buf cr_buf, spawn_area *sa)
{
    buff_crate *crate;
    int x, y;

    for (crate = cr_buf; crate - cr_buf < crate_bufsize; crate++) {
        if (!crate->is_alive)
            continue;

        for (y = p->pos.y; y < p->pos.y + player_height; y++)
            for (x = p->pos.x; x < p->pos.x + player_width; x++) {
                point pt = point_literal(x, y);
                if (point_is_in_player(p, pt) && point_is_in_crate(crate, pt)) {
                    collect_crate(crate, sa, p);
                    kill_crate(crate, sa);

                    goto nest_break_cp;
                }
            }

nest_break_cp:
        if (0) {}
    }

    show_player(p);
}

void process_bullet_to_boss_collisions(player_bullet_buf bullet_buf, boss *bs)
{
    player_bullet *blt;

    for (blt = bullet_buf; blt - bullet_buf < player_bullet_bufsize; blt++) {
        if (!blt->is_alive)
            continue;

        if (point_is_in_boss(bs, blt->pos)) {
            damage_boss(bs, blt->damage);
            kill_bullet(blt);
        }
    }

    show_boss(bs);
}

void process_player_to_boss_collisions(player *p, boss *bs)
{
    int x, y;

    for (y = p->pos.y; y < p->pos.y + player_height; y++)
        for (x = p->pos.x; x < p->pos.x + player_width; x++) {
            point pt = point_literal(x, y);
            if (point_is_in_player(p, pt) && point_is_in_boss(bs, pt)) {
                damage_player(p, p->state.cur_hp);
                break;
            }
        }

    show_boss(bs);
}

static void hit_player_with_projectile(boss_projectile *proj,
        player *p, explosion_buf expl_buf, term_state *ts)
{
    switch (proj->type) {
        case bullet:
            damage_player(p, proj->damage);
            kill_boss_projectile(proj);
            break;
        case gunshot:
            set_gunshot_off(proj, expl_buf, ts);
            break;
        case mine:
            set_mine_off(proj, expl_buf, ts);
            break;
    }
}

static void try_explode_gunshot_near_player(boss_projectile *proj,
        player *p, explosion_buf expl_buf, term_state *ts)
{
    int dist = (int) distance_to_player(p, proj->pos, Y_TO_X_RATIO);

    if (distance_is_in_gunshot_expl_reach(dist, proj))
        set_gunshot_off(proj, expl_buf, ts);
}

void process_projectile_to_player_collisions(boss_projectile_buf proj_buf,
        player *p, explosion_buf expl_buf, term_state *ts)
{
    boss_projectile *proj;

    for (proj = proj_buf; proj - proj_buf < boss_projectile_bufsize; proj++) {
        if (!proj->is_alive)
            continue;

        if (point_is_in_player(p, proj->pos))
            hit_player_with_projectile(proj, p, expl_buf, ts);
        else if (proj->type == gunshot)
            try_explode_gunshot_near_player(proj, p, expl_buf, ts);
    }

    show_player(p);
}

void process_explosion_to_player_collisions(
        explosion_buf expl_buf, player *p, term_state *ts)
{
    explosion *expl;
    double dist;

    for (expl = expl_buf; expl - expl_buf < explosion_bufsize; expl++) {
        if (!expl->is_alive)
            continue;

        dist = distance_to_player(p, expl->center, Y_TO_X_RATIO);
        if (dist <= expl->cur_rad + EXPL_COLLISION_TOLERANCE) {
            damage_player(p, expl->damage);
            deactivate_explosion(expl);
        }
    }
}
