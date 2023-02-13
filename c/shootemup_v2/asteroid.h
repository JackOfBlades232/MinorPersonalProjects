/* shootemup_v2/asteroid.h */
#ifndef ASTEROID_SENTRY
#define ASTEROID_SENTRY

#include "geom.h"
#include "graphics.h"
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
} asteroid_data;

typedef struct tag_asteroid {
  asteroid_data *data;
  point pos;
  int dx, dy;
  int spawn_area_idx;
  int cur_hp;
  int is_alive;
  int frames_until_move;
} asteroid;

typedef asteroid asteroid_buf[asteroid_bufsize];

void init_asteroid_buf(asteroid_buf buf);

asteroid *get_queued_asteroid(asteroid_buf buf);
void spawn_asteroid(asteroid *as, point pos, int spawn_area_idx);
void show_asteroid(asteroid *as);
void update_live_asteroids(asteroid_buf buf, spawn_area *sa, term_state *ts);
int damage_asteroid(asteroid *as, int damage, spawn_area *sa);
int kill_asteroid(asteroid *as, spawn_area *sa);

int point_is_in_asteroid(asteroid *as, point p);

#endif
