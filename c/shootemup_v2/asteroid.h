/* shootemup_v2/asteroid.h */
#ifndef ASTEROID_SENTRY
#define ASTEROID_SENTRY

#include "geom.h"

enum { asteroid_bufsize = 32 };
enum { asteroid_type_cnt = 3 };

typedef enum tag_asteroid_type { small, medium, big } asteroid_type;

typedef struct tag_asteroid {
    asteroid_type type;
    point pos;
    int dx, dy;
    int cur_hp;
    int damage;
    int is_alive;
} asteroid;

typedef asteroid asteroid_buf[asteroid_bufsize];

void init_asteroid_buf(asteroid_buf buf);

/* TODO : add spawner "array" */
int spawn_asteroid(asteroid_buf buf);
void update_live_asteroids(asteroid_buf buf);
int damage_asteroid(asteroid *as, int damage);
int kill_asteroid(asteroid *as);

int point_is_in_asteroid(asteroid *as, point p);

#endif 
