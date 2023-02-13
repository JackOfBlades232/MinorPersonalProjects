/* shootemup_v2/buffs.h */
#ifndef BUFFS_SENTRY
#define BUFFS_SENTRY

#include "geom.h"
#include "spawn.h"
#include "player.h"
#include "graphics.h"

enum { crate_bufsize = 16 };

typedef struct tag_buff_data {
    int width, height;
    int hp_restore;
    int movement_frames;
} buff_data;

typedef struct tag_buff_crate {
    buff_data *data;
    point pos;
    int dx, dy;
    int spawn_area_idx;
    int is_alive;
    int frames_until_move;
} buff_crate;

typedef buff_crate crate_buf[crate_bufsize];

void init_crate_buf(crate_buf buf);

buff_crate *get_queued_crate(crate_buf buf);
void spawn_crate(buff_crate *crate, point pos, int spawn_area_idx);
void show_crate(buff_crate *crate);
void update_live_crates(crate_buf buf, spawn_area *sa, term_state *ts);
int collect_crate(buff_crate *crate, spawn_area *sa, player *p);
int kill_crate(buff_crate *crate, spawn_area *sa);

int point_is_in_crate(buff_crate *crate, point p);

#endif
