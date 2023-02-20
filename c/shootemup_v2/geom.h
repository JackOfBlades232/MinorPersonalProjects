/* shootemup_v2/geom.h */
#ifndef GEOM_SENTRY
#define GEOM_SENTRY

typedef struct tag_point {
    int x, y;
} point;

point point_literal(int x, int y);
point point_sum(point p1, point p2);
int sq_dist(point p1, point p2);

#endif
