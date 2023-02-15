/* shootemup_v2/explosion.h */
#ifndef EXPLOSION_SENTRY
#define EXPLOSION_SENTRY

#include "geom.h"

enum { explosion_bufsize = 16 };

typedef struct tag_explosion {
    point center;
    double cur_rad;
    int updates_left;
    int frames_to_update;
    int is_alive;
} explosion;

typedef explosion explosion_buf[explosion_bufsize];

void init_explosion_buf(explosion_buf buf);

int spawn_explosion(explosion_buf buf, point pos, double max_rad);

void update_live_explosions(explosion_buf buf);
int kill_explosion(explosion *ex);

int point_is_in_explosion(explosion *ex, point p);

#endif
