/* shootemup_v2/explosion.c */
#include "explosion.h"
#include "colors.h"
#include "geom.h"
#include "graphics.h"

#include <curses.h>
#include <stdlib.h>
#include <math.h>

#define EXPL_SYMBOL '*'
#define EXPL_D_RAD_PER_UPDATE 0.8

enum { expl_frames_per_update = 3 };

void init_explosion_buf(explosion_buf buf)
{
    int i;
    for (i = 0; i < explosion_bufsize; i++)
        buf[i].is_alive = 0;
}

static void draw_expl_circle(explosion *ex, char symbol, term_state *ts)
{
    int loc_x, loc_y;
    double sq_rad = ex->cur_rad * ex->cur_rad;
    for (loc_x = 0; loc_x < ceil(ex->cur_rad); loc_x++) {
        loc_y = ceil(X_TO_Y_RATIO * sqrt(sq_rad - (double) (loc_x*loc_x)));

        draw_local_point(loc_y, loc_x, ex->center, symbol, ts);
        draw_local_point(-loc_y, loc_x, ex->center, symbol, ts);
        draw_local_point(loc_y, -loc_x, ex->center, symbol, ts);
        draw_local_point(-loc_y, -loc_x, ex->center, symbol, ts);
    }
}

static void show_explosion(explosion *ex, term_state *ts)
{
    attrset(ex->color_pair);
    draw_expl_circle(ex, EXPL_SYMBOL, ts);
}

static void hide_explosion(explosion *ex, term_state *ts)
{
    attrset(get_color_pair(0));
    draw_expl_circle(ex, ' ', ts);
}

static explosion *get_queued_explosion(explosion_buf buf)
{
    int i;

    for (i = 0; i < explosion_bufsize; i++) {
        if (!buf[i].is_alive)
            return buf+i;
    }

    return NULL;
}

int spawn_explosion(explosion_buf buf, point pos,
        double max_rad, int damage, int color_pair, term_state *ts)
{
    explosion *ex = get_queued_explosion(buf);
    if (!ex)
        return 0;

    ex->is_alive = 1;
    ex->center = pos;
    ex->cur_rad = 0;
    ex->updates_left = (int) (max_rad/EXPL_D_RAD_PER_UPDATE);
    ex->frames_to_update = expl_frames_per_update;
    ex->damage = damage;
    ex->color_pair = color_pair;

    show_explosion(ex, ts);
    return 1;
}

static void update_explosion(explosion *ex, term_state *ts)
{
    if (!ex->is_alive)
        return;

    if (ex->frames_to_update > 0) {
        ex->frames_to_update--;
        return;
    }

    ex->updates_left--;

    if (ex->updates_left <= 0)
        kill_explosion(ex, ts);
    else {
        hide_explosion(ex, ts);
        ex->cur_rad += EXPL_D_RAD_PER_UPDATE;
        show_explosion(ex, ts);

        ex->frames_to_update = expl_frames_per_update;
    }
}

void update_live_explosions(explosion_buf buf, term_state *ts)
{
    int i;
    for (i = 0; i < explosion_bufsize; i++)
        update_explosion(buf+i, ts);
}

int kill_explosion(explosion *ex, term_state *ts)
{
    if (ex->is_alive) {
        hide_explosion(ex, ts);
        ex->is_alive = 0;
        return 1;
    }

    return 0;
}

void kill_all_explosions(explosion_buf buf, term_state *ts)
{
    int i;
    for (i = 0; i < explosion_bufsize; i++)
        kill_explosion(buf+i, ts);
}

void deactivate_explosion(explosion *ex)
{
    ex->damage = 0;
}
