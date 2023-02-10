/* shootemup_v2/asteroid.c */
#include "asteroid.h"
#include "geom.h"
#include "utils.h"

#include <curses.h>
#include <stdlib.h>

#include <unistd.h>

enum { 
    small_asteroid_width = 4, small_asteroid_height = 3,
    medium_asteroid_width = 6, medium_asteroid_height = 4,
    big_asteroid_width = 10, big_asteroid_height = 7
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

static void show_asteroid(asteroid *as)
{
    int x, y, i;
    switch (as->type) {
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
    int w, h;
    int y, i;

    switch (as->type) {
        case small:
            w = small_asteroid_width;
            h = small_asteroid_height;
            break;
        case medium:
            w = medium_asteroid_width;
            h = medium_asteroid_height;
            break;
        case big:
            w = big_asteroid_width;
            h = big_asteroid_height;
            break;
    }

    for (y = as->pos.y; y < as->pos.y + h; y++) {
        move(y, as->pos.x);
        for (i = 0; i < w; i++)
            addch(' ');
    }
} 

int spawn_asteroid(asteroid_buf buf)
{
    int i;
    asteroid *as = NULL;

    for (i = 0; i < asteroid_bufsize; i++) {
        if (!buf[i].is_alive) {
            as = buf + i;
            break;
        }
    }

    if (!as)
        return 0;

    /* temp */
    as->pos = point_literal(0, 0);
    as->is_alive = 1;
    as->type = all_asteroid_types[randint(0, asteroid_type_cnt)];
    as->damage = 2;
    as->cur_hp = 1;
    as->dx = 0;
    as->dy = 1;

    show_asteroid(as);

    return 1;
}

static void update_asteroid(asteroid *as)
{
    if (as->is_alive) {
        hide_asteroid(as);
        as->pos.x += as->dx;
        as->pos.y += as->dy;

        /* temp */
        if (as->pos.y > 20) {
            kill_asteroid(as);
            return;
        }

        show_asteroid(as);
    }
}    

void update_live_asteroids(asteroid_buf buf)
{
    int i;
    for (i = 0; i < asteroid_bufsize; i++)
        update_asteroid(buf+i);
}

int damage_asteroid(asteroid *as, int damage)
{
    as->cur_hp -= damage;
    if (as->cur_hp <= 0)
        return kill_asteroid(as);

    return 0;
}

int kill_asteroid(asteroid *as)
{
    as->is_alive = 0;
    hide_asteroid(as);
    
    return 1;
}

#define CHECK_POINT_IN_ASTEROID(ARR, W, H, AST, P) \
    loc_x = AST->pos.x + P.x; \
    loc_y = AST->pos.y + P.y; \
    if (loc_x < 0 || loc_x >= W || loc_y < 0 || loc_y >= H) \
        return false; \
    else \
        return ARR[loc_y][loc_x] != ' ';

int point_is_in_asteroid(asteroid *as, point p)
{
    int loc_x, loc_y;
    switch (as->type) {
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
