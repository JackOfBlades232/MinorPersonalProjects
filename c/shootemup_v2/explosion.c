/* shootemup_v2/explosion.c */
#include "explosion.h"
#include "geom.h"
#include "graphics.h"

#include <curses.h>
#include <stdlib.h>
#include <math.h>

#define EXPL_SYMBOL '*'
#define EXPL_D_RAD_PER_UPDATE 0.8
#define X_TO_Y_RATIO 0.666666

#define DIST_TOLERANCE 0.51

enum { expl_frames_per_update = 2 };

void init_explosion_buf(explosion_buf buf)
{
    int i;
    for (i = 0; i < explosion_bufsize; i++)
        buf[i].is_alive = 0;
}

static void draw_expl_circle(explosion *ex, char symbol)
{
    int loc_x, loc_y;
    double sq_rad = ex->cur_rad * ex->cur_rad;
    for (loc_x = 0; loc_x <= (int) ex->cur_rad; loc_x++) {
        loc_y = X_TO_Y_RATIO * sqrt(sq_rad - (double) (loc_x*loc_x));

        draw_local_point(loc_y, loc_x, ex->center, symbol);
        draw_local_point(-loc_y, loc_x, ex->center, symbol);
        draw_local_point(loc_y, -loc_x, ex->center, symbol);
        draw_local_point(-loc_y, -loc_x, ex->center, symbol);
    }
}

static void show_explosion(explosion *ex)
{
    draw_expl_circle(ex, EXPL_SYMBOL);
}

static void hide_explosion(explosion *ex)
{
    draw_expl_circle(ex, ' ');
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

int spawn_explosion(explosion_buf buf, point pos, double max_rad)
{
    explosion *ex = get_queued_explosion(buf);
    if (!ex)
        return 0;

    ex->is_alive = 1;
    ex->center = pos;
    ex->cur_rad = 0;
    ex->updates_left = (int) (max_rad/EXPL_D_RAD_PER_UPDATE);
    ex->frames_to_update = expl_frames_per_update;

    show_explosion(ex);
    return 1;
}

static void update_explosion(explosion *ex)
{
    if (!ex->is_alive)
        return;

    if (ex->frames_to_update > 0) {
        ex->frames_to_update--;
        return;
    }

    ex->updates_left--;

    if (ex->updates_left <= 0)
        kill_explosion(ex);
    else {
        hide_explosion(ex);
        ex->cur_rad += EXPL_D_RAD_PER_UPDATE;
        show_explosion(ex);

        ex->frames_to_update = expl_frames_per_update;
    }
}

void update_live_explosions(explosion_buf buf)
{
    int i;
    for (i = 0; i < explosion_bufsize; i++)
        update_explosion(buf+i);
}

int kill_explosion(explosion *ex)
{
    if (ex->is_alive) {
        hide_explosion(ex);
        ex->is_alive = 0;
        return 1;
    }

    return 0;
}

int point_is_in_explosion(explosion *ex, point p)
{
    return sqrt(sq_dist(ex->center, p)) - ex->cur_rad < DIST_TOLERANCE;
}
