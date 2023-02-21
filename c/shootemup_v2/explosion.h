/* shootemup_v2/explosion.h */
#ifndef EXPLOSION_SENTRY
#define EXPLOSION_SENTRY

#include "geom.h"
#include "graphics.h"

#define X_TO_Y_RATIO 0.666666

enum { explosion_bufsize = 16 };

typedef struct tag_explosion {
    point center;
    double cur_rad;
    int updates_left;
    int frames_to_update;
    int damage;
    int color_pair;
    int is_alive;
} explosion;

typedef explosion explosion_buf[explosion_bufsize];

void init_explosion_buf(explosion_buf buf);

int spawn_explosion(explosion_buf buf, point pos, double max_rad, 
        int damage, int color_pair, term_state *ts);

void update_live_explosions(explosion_buf buf, term_state *ts);
int kill_explosion(explosion *ex, term_state *ts);
void kill_all_explosions(explosion_buf buf, term_state *ts);

void deactivate_explosion(explosion *ex);

#endif
