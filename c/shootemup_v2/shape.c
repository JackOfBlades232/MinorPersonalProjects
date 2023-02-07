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

int move_shape(shape *sh, int dx, int dy)
{
    hide_shape(sh);
    sh->pos.x += dx;
    sh->pos.y += dy;
    show_shape(sh);

    return 1;
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
