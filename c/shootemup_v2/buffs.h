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
void init_crate_static_data();

buff_crate *get_queued_crate(crate_buf buf);
void spawn_crate(buff_crate *as, point pos, int spawn_area_idx);
void show_crate(buff_crate *as);
void update_live_crates(crate_buf buf, spawn_area *sa, 
        term_state *ts, player *p);
int collect_crate(buff_crate *as, int damage, spawn_area *sa, player *p);
int kill_crate(buff_crate *as, spawn_area *sa);

int point_is_in_crate(buff_crate *as, point p);

#endif
