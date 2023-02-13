/* shootemup_v2/asteroid.c */
#include "asteroid.h"
#include "geom.h"
#include "spawn.h"
#include "utils.h"

#include <curses.h>
#include <stdlib.h>

#include <unistd.h>

enum { 
    small_asteroid_width = 4, small_asteroid_height = 3,
    medium_asteroid_width = 6, medium_asteroid_height = 4,
    big_asteroid_width = 10, big_asteroid_height = 7
};

enum { 
    small_asteroid_movement_frames = 2,
    medium_asteroid_movement_frames = 3,
    big_asteroid_movement_frames = 4 };

enum { 
    small_asteroid_hp = 1,
    medium_asteroid_hp = 2,
    big_asteroid_hp = 4
};

enum { 
    small_asteroid_damage = 1,
    medium_asteroid_damage = 2,
    big_asteroid_damage = 3
};

static const char 
    small_asteroid_shape[small_asteroid_height][small_asteroid_width+1] =
{
    " ** ",
    "****",
    " ** "
};

static const char 
    medium_asteroid_shape[medium_asteroid_height][medium_asteroid_width+1] =
{
    " **** ",
    "******",
    "******",
    " **** "
};

static const char 
    big_asteroid_shape[big_asteroid_height][big_asteroid_width+1] =
{
    "   ****   ",
    " ******** ",
    " ******** ",
    "**********",
    " ******** ",
    " ******** ",
    "   ****   "
};

static asteroid_data small_asteroid_data =
{
    small, small_asteroid_width, small_asteroid_height, small_asteroid_hp, 
    small_asteroid_damage, small_asteroid_movement_frames
};

static asteroid_data medium_asteroid_data =
{
    medium, medium_asteroid_width, medium_asteroid_height, medium_asteroid_hp, 
    medium_asteroid_damage, medium_asteroid_movement_frames
};

static asteroid_data big_asteroid_data =
{
    big, big_asteroid_width, big_asteroid_height, big_asteroid_hp, 
    big_asteroid_damage, big_asteroid_movement_frames
};

static const asteroid_type all_asteroid_types[asteroid_type_cnt] = 
{
    small, medium, big
};

void init_asteroid_buf(asteroid_buf buf)
{
    int i;
    for (i = 0; i < asteroid_bufsize; i++)
        buf[i].is_alive = 0;
}

#define DRAW_ASTEROID_SHAPE(ARR, H, AST) \
    x = AST->pos.x; \
    y = AST->pos.y; \
    for (i = 0; i < H; i++) { \
        move(y, x); \
        addstr(ARR[i]); \
        y++; \
    }

void show_asteroid(asteroid *as)
{
    int x, y, i;
    switch (as->data->type) {
        case small:
            DRAW_ASTEROID_SHAPE(small_asteroid_shape, 
                    small_asteroid_height, as);
            break;
        case medium:
            DRAW_ASTEROID_SHAPE(medium_asteroid_shape, 
                    medium_asteroid_height, as);
            break;
        case big:
            DRAW_ASTEROID_SHAPE(big_asteroid_shape, 
                    big_asteroid_height, as);
            break;
    }
}

static void hide_asteroid(asteroid *as)
{
    int y, i;

    for (y = as->pos.y; y < as->pos.y + as->data->height; y++) {
        move(y, as->pos.x);
        for (i = 0; i < as->data->width; i++)
            addch(' ');
    }
} 

asteroid *get_queued_asteroid(asteroid_buf buf)
{
    asteroid *as = NULL;
    asteroid_type chosen_type;

    for (as = buf; as - buf < asteroid_bufsize; as++) {
        if (!as->is_alive)
            break;
    }

    if (as - buf >= asteroid_bufsize)
        return NULL;

    chosen_type = all_asteroid_types[randint(0, asteroid_type_cnt)];

    switch (chosen_type) {
        case small:
            as->data = &small_asteroid_data;
            break;
        case medium:
            as->data = &medium_asteroid_data;
            break;
        case big:
            as->data = &big_asteroid_data;
            break;
    }

    return as;
}

void spawn_asteroid(asteroid *as, point pos, int spawn_area_idx)
{
    as->is_alive = 1;
    as->pos = pos;
    as->spawn_area_idx = spawn_area_idx;

    as->cur_hp = as->data->max_hp;
    as->frames_until_move = as->data->movement_frames;

    as->dx = 0;
    as->dy = 1;

    show_asteroid(as);
}

static void update_asteroid(asteroid *as, spawn_area *sa, term_state *ts)
{
    if (as->is_alive) {
        if (as->frames_until_move > 0)
            as->frames_until_move--;
        else {
            hide_asteroid(as);
            as->pos.x += as->dx;
            as->pos.y += as->dy;

            if (as->pos.y + as->data->height >= ts->row) {
                kill_asteroid(as, sa);
                return;
            }

            show_asteroid(as);

            as->frames_until_move = as->data->movement_frames;
        }
    }
}    

void update_live_asteroids(asteroid_buf buf, spawn_area *sa, term_state *ts)
{
    int i;
    for (i = 0; i < asteroid_bufsize; i++)
        update_asteroid(buf+i, sa, ts);
}

int damage_asteroid(asteroid *as, int damage, spawn_area *sa)
{
    as->cur_hp -= damage;
    if (as->cur_hp <= 0)
        return kill_asteroid(as, sa);

    return 0;
}

int kill_asteroid(asteroid *as, spawn_area *sa)
{
    if (!as->is_alive)
        return 0;

    as->is_alive = 0;
    hide_asteroid(as);

    free_area_of_object(sa, as->spawn_area_idx, as->data->width);
    
    return 1;
}

#define CHECK_POINT_IN_ASTEROID(ARR, W, H, AST, P) \
    loc_x = P.x - AST->pos.x; \
    loc_y = P.y - AST->pos.y; \
    if (loc_x < 0 || loc_x >= W || loc_y < 0 || loc_y >= H) \
        return false; \
    else \
        return ARR[loc_y][loc_x] != ' ';

int point_is_in_asteroid(asteroid *as, point p)
{
    int loc_x, loc_y;
    switch (as->data->type) {
        case small:
            CHECK_POINT_IN_ASTEROID(small_asteroid_shape, 
                    small_asteroid_width, small_asteroid_height,
                    as, p);
        case medium:
            CHECK_POINT_IN_ASTEROID(medium_asteroid_shape, 
                    medium_asteroid_width, medium_asteroid_height,
                    as, p);
        case big:
            CHECK_POINT_IN_ASTEROID(big_asteroid_shape, 
                    big_asteroid_width, big_asteroid_height,
                    as, p);
    }

    return 0;
}
