/* shootemup_v2/shape.h */
#ifndef SHAPE_SENTRY
#define SHAPE_SENTRY

#include "graphics.h"

typedef struct tag_shape {
    point pos;
    char fill_sym;
    point *loc_points;
    int num_points;
    int min_loc_x, max_loc_x, min_loc_y, max_loc_y;
} shape;

shape *create_shape(int num_points);
void destroy_shape(shape *sh);
void set_shape_bounds(shape *sh);
int truncate_shape_pos(shape *sh, term_state *ts);
void show_shape(shape *sh);
void hide_shape(shape *sh);
int redraw_shape(shape *sh, term_state *ts);
int move_shape(shape *sh, int dx, int dy, term_state *ts);
int point_is_in_shape(point p, shape *sh);
int shapes_overlap(shape *sh1, shape *sh2);

#endif
