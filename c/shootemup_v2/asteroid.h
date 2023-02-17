/* shootemup_v2/asteroid.h */
#ifndef ASTEROID_SENTRY
#define ASTEROID_SENTRY

#include "geom.h"
#include "graphics.h"
#include "player.h"
#include "spawn.h"

enum { asteroid_bufsize = 32 };
enum { asteroid_type_cnt = 3 };

typedef enum tag_asteroid_type { small, medium, big } asteroid_type;

typedef struct tag_asteroid_data {
    asteroid_type type;
    int width, height;
    int max_hp;
    int damage;
    int movement_frames;
    int score_for_kill, score_for_skip;
} asteroid_data;

typedef struct tag_asteroid {
    asteroid_data *data;
    point pos;
    int dx, dy;
    int spawn_area_idx;
    int cur_hp;
    int is_alive;
    int frames_since_moved;
} asteroid;

typedef asteroid asteroid_buf[asteroid_bufsize];

void init_asteroid_buf(asteroid_buf buf);
void init_asteroid_static_data();

asteroid *get_queued_asteroid(asteroid_buf buf);
void spawn_asteroid(asteroid *as, point pos, int spawn_area_idx);
void show_asteroid(asteroid *as);
void update_live_asteroids(asteroid_buf buf, spawn_area *sa, 
        term_state *ts, player *p);
int damage_asteroid(asteroid *as, int damage, spawn_area *sa, player *p);
int kill_asteroid(asteroid *as, spawn_area *sa);

int buffer_has_live_asteroids(asteroid_buf buf);
void kill_all_asteroids(asteroid_buf buf, spawn_area *sa);

int point_is_in_asteroid(asteroid *as, point p);

#endif
