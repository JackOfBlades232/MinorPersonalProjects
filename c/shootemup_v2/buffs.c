/* shootemup_v2/buffs.c */
#include "buffs.h"
#include "colors.h"
#include "utils.h"

#include <curses.h>
#include <stdlib.h>

enum { heal_crate_height = 2, heal_crate_width = 3 };
enum { heal_crate_hp_restore_amt = 2 };

enum { crate_movement_frames = 8 };

static const char heal_crate_shape[heal_crate_height][heal_crate_width+1] = 
{
    "+++",
    "+++"
};

static buff_data heal_crate_data =
{
    heal_crate_width, heal_crate_height, 
    heal_crate_hp_restore_amt, crate_movement_frames
};

void init_crate_buf(crate_buf buf)
{
    int i;
    for (i = 0; i < crate_bufsize; i++) {
        buf[i].is_alive = 0;
        /* temp, until there are more types */
        buf[i].data = &heal_crate_data;
    }
}

void show_crate(buff_crate *crate)
{
    int i, j;
    /* temp : remake to macro if multiple types */
    attrset(get_color_pair(hcrate_color_pair));
    for (i = 0; i < crate->data->height; i++) {
        move(crate->pos.y + i, crate->pos.x); 

        for (j = 0; j < crate->data->width; j++) {
            char shape_symbol = heal_crate_shape[i][j];
            if (char_is_symbol(shape_symbol))
                addch(shape_symbol);
            else
                move(crate->pos.y + i, crate->pos.x + j + 1); 
        }
    }
}

static void hide_crate(buff_crate *crate)
{
    int y, i;
    attrset(get_color_pair(0));
    for (y = crate->pos.y; y < crate->pos.y + crate->data->height; y++) {
        move(y, crate->pos.x);
        for (i = 0; i < crate->data->width; i++)
            addch(' ');
    }
}

buff_crate *get_queued_crate(crate_buf buf)
{
    buff_crate *crate = NULL;

    for (crate = buf; crate - buf < crate_bufsize; crate++) {
        if (!crate->is_alive)
            break;
    }

    return crate - buf < crate_bufsize ? crate : NULL;
}

void spawn_crate(buff_crate *crate, point pos, int spawn_area_idx)
{
    crate->is_alive = 1;
    crate->pos = pos;
    crate->spawn_area_idx = spawn_area_idx;

    crate->frames_since_moved = 0;

    crate->dx = 0;
    crate->dy = 1;

    show_crate(crate);
}

static void update_crate(buff_crate *crate, spawn_area *sa, term_state *ts)
{
    if (crate->is_alive) {
        if (crate->frames_since_moved < crate_movement_frames)
            crate->frames_since_moved++;
        else {
            hide_crate(crate);
            crate->pos.x += crate->dx;
            crate->pos.y += crate->dy;

            if (crate->pos.y + crate->data->height >= ts->row) {
                kill_crate(crate, sa);
                return;
            }

            show_crate(crate);

            crate->frames_since_moved = 0;
        }
    }
}    

void update_live_crates(crate_buf buf, spawn_area *sa, term_state *ts)
{
    int i;
    for (i = 0; i < crate_bufsize; i++)
        update_crate(buf+i, sa, ts);
}

int collect_crate(buff_crate *crate, spawn_area *sa, player *p)
{
    p->state.cur_hp = min_int(
            p->state.cur_hp + crate->data->hp_restore,
            p->state.max_hp
            );
    
    return kill_crate(crate, sa);
}

int kill_crate(buff_crate *crate, spawn_area *sa)
{
    if (!crate->is_alive)
        return 0;

    crate->is_alive = 0;
    hide_crate(crate);

    free_area_of_object(sa, crate->spawn_area_idx, crate->data->width);
    
    return 1;
}

int buffer_has_live_crates(crate_buf buf)
{
    int i;
    for (i = 0; i < crate_bufsize; i++) {
        if (buf[i].is_alive)
            return 1;
    }

    return 0;
}

void kill_all_cratres(crate_buf buf, spawn_area *sa)
{
    int i;
    for (i = 0; i < crate_bufsize; i++) {
        if (buf[i].is_alive)
            kill_crate(buf+i, sa);
    }
}

int point_is_in_crate(buff_crate *crate, point p)
{
    int loc_x, loc_y;
    /* temp : remake to macro if multiple types */
    loc_x = p.x - crate->pos.x;
    loc_y = p.y - crate->pos.y;
    if (loc_x < 0 || loc_x >= crate->data->width ||
            loc_y < 0 || loc_y >= crate->data->height)
        return 0;
    else
        return heal_crate_shape[loc_y][loc_x] != ' ';
}
