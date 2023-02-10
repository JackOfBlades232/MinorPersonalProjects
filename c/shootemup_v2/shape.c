/* shootemup_v2/shape.c */
#include "shape.h"
#include "graphics.h"

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>

#define ERRCODE_BASE 10

int is_atomic(shape *sh)
{
    return sh->num_points == 1;
}

shape *create_shape(int num_points)
{
    shape *s;

    if (num_points < 1) {
        fprintf(stderr, "Trying to create shape with < 1 points\n");
        exit(ERRCODE_BASE+1);
    }

    s = malloc(sizeof(shape));
    s->num_points = num_points;

    if (num_points > 1)
        s->loc_points = malloc(num_points * sizeof(point));

    return s;
}

void destroy_shape(shape *sh)
{
    if (!is_atomic(sh))
        free(sh->loc_points);

    free(sh);
}

void set_shape_bounds(shape *sh)
{
    int i;
    if (is_atomic(sh)) {
        sh->min_loc_x = sh->max_loc_x = sh->pos.x;
        sh->min_loc_y = sh->max_loc_y = sh->pos.y;
    } else {
        sh->min_loc_x = sh->min_loc_y = 10000;
        sh->max_loc_x = sh->max_loc_y = -1;
        for (i = 0; i < sh->num_points; i++) {
            point loc_p = sh->loc_points[i];
            if (loc_p.x < sh->min_loc_x)
                sh->min_loc_x = loc_p.x;
            if (loc_p.x > sh->max_loc_x)
                sh->max_loc_x = loc_p.x;
            if (loc_p.y < sh->min_loc_y)
                sh->min_loc_y = loc_p.y;
            if (loc_p.y > sh->max_loc_y)
                sh->max_loc_y = loc_p.y;
        }
    }
}

static int shape_is_larger_than_screen(shape *sh, term_state *ts)
{
    return sh->max_loc_x - sh->min_loc_x >= ts->col ||
           sh->max_loc_y - sh->min_loc_y >= ts->row; 
}

int truncate_shape_pos(shape *sh, term_state *ts)
{
    int min_x, max_x, min_y, max_y;
    min_x = sh->pos.x + sh->min_loc_x;
    max_x = sh->pos.x + sh->max_loc_x;
    min_y = sh->pos.y + sh->min_loc_y;
    max_y = sh->pos.y + sh->max_loc_y;

    if (shape_is_larger_than_screen(sh, ts))
        return 0;

    if (min_x < 0)
        sh->pos.x -= min_x;
    else if (max_x >= ts->col)
        sh->pos.x -= max_x - ts->col + 1;
    if (min_y < 0)
        sh->pos.y -= min_y;
    else if (max_y >= ts->row)
        sh->pos.y -= max_y - ts->row + 1;

    return 1;
}

static point global_shape_point(shape *sh, int index)
{
    return add_points(sh->pos, sh->loc_points[index]);
}

static void fill_shape(shape *sh, char sym)
{
    if (is_atomic(sh))
        draw_pix(sh->pos, sym);
    else {
        int i;

        for (i = 0; i < sh->num_points; i++)
            draw_pix(global_shape_point(sh, i), sym);
    }
}

void show_shape(shape *sh)
{
    fill_shape(sh, sh->fill_sym);
}

void hide_shape(shape *sh)
{
    fill_shape(sh, ' ');
}

int redraw_shape(shape *sh, term_state *ts)
{
    int success;
    hide_shape(sh);
    success = truncate_shape_pos(sh, ts);
    if (success)
        show_shape(sh);
    return success;
}

int move_shape(shape *sh, int dx, int dy, term_state *ts)
{
    int success;
    hide_shape(sh);
    sh->pos.x += dx;
    sh->pos.y += dy;
    success = truncate_shape_pos(sh, ts);
    if (success)
        show_shape(sh);
    return success;
}

int point_is_in_shape(point p, shape *sh)
{
    if (is_atomic(sh))
        return points_are_equal(p, sh->pos);
    else {
        int i;

        for (i = 0; i < sh->num_points; i++) {
            if (points_are_equal(p, global_shape_point(sh, i)))
                return 1;
        }

        return 0;
    }
}

int shapes_overlap(shape *sh1, shape *sh2)
{
    if (is_atomic(sh1))
        return point_is_in_shape(sh1->pos, sh2);
    else if (is_atomic(sh2))
        return point_is_in_shape(sh2->pos, sh1);
    else {
        int i;

        for (i = 0; i < sh1->num_points; i++) {
            if (point_is_in_shape(global_shape_point(sh1, i), sh2))
                return 1;
        }

        return 0;
    }
}
