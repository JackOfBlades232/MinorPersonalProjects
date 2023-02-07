/* shootemup_v2/shape.h */
#ifndef SHAPE_SENTRY
#define SHAPE_SENTRY

#include "graphics.h"

typedef struct tag_shape {
    point pos;
    char fill_sym;
    point *loc_points;
    int num_points;
} shape;

shape *create_shape(int num_points);
void destroy_shape(shape *sh);
void show_shape(shape *sh);
void hide_shape(shape *sh);
int move_shape(shape *sh, int dx, int dy);
int point_is_in_shape(point p, shape *sh);
int shapes_overlap(shape *sh1, shape *sh2);

#endif
