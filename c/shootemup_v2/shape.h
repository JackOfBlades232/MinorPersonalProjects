#ifndef SHAPE_SENTRY
#define SHAPE_SENTRY

typedef struct tag_point {
    int x, y;
} point;

typedef struct tag_shape {
    point pos;
    point *loc_points;
    int num_points;
} shape;

shape *create_shape();
void destroy_shape();
void show_shape(shape *sh);
void hide_shape(shape *sh);
int move_shape(shape *sh, int dx, int dy);
int point_is_in_shape(point p, shape *sh);
int shapes_overlap(shape *sh1, shape *sh2);

#endif
