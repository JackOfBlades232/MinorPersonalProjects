/* shootemup_v2/geom.c */
#include "geom.h"

point point_literal(int x, int y)
{
    point p;
    p.x = x;
    p.y = y;
    return p;
}

int sq_dist(point p1, point p2)
{
    int dx = p1.x-p2.x;
    int dy = p1.y-p2.y;
    return dx*dx + dy*dy;
}
