/* shootemup_v2/boss_ai.c */
#include "boss_ai.h"
#include "utils.h"

int move_boss_to_x(boss_behaviour_state *bh, 
        boss *bs, int x, int frames, term_state *ts)
{
    if (x < 0 || x > ts->col - boss_width || bs->pos.x == x)
        return 0;

    bh->current_movement.type = horizontal;
    bh->current_movement.goal.x = x;
    bh->current_movement.completed = 0;

    bs->state.cur_movement_frames = frames / abs_int(x - bs->pos.x);

    return 1;
}

int move_boss_to_y(boss_behaviour_state *bh, 
        boss *bs, int y, int frames, term_state *ts)
{
    if (y < 1 || y > ts->row - boss_height || bs->pos.y == y)
        return 0;

    bh->current_movement.type = vertical;
    bh->current_movement.goal.y = y;
    bh->current_movement.completed = 0;

    bs->state.cur_movement_frames = frames / abs_int(y - bs->pos.y);

    return 1;
}

int teleport_boss_to_pos(boss_behaviour_state *bh, 
        boss *bs, point pos, term_state *ts)
{
    if (!obj_is_in_bounds(pos, boss_width, boss_height, ts))
        return 0;

    bh->current_movement.type = teleport;
    bh->current_movement.goal.pos = pos;
    bh->current_movement.completed = 0;

    return 1;
}

static void tick_horizontal_movement(boss_behaviour_state *bh, 
        boss *bs, term_state *ts)
{
    int dx = sgn_int(bh->current_movement.goal.x - bs->pos.x);
    move_boss(bs, dx, 0, ts);

    if (bs->pos.x == bh->current_movement.goal.x) {
        bh->current_movement.completed = 1;
        bh->current_movement.type = not_moving;
    }
}

static void tick_vertical_movement(boss_behaviour_state *bh, 
        boss *bs, term_state *ts)
{
    int dy = sgn_int(bh->current_movement.goal.y - bs->pos.y);
    move_boss(bs, 0, dy, ts);

    if (bs->pos.y == bh->current_movement.goal.y) {
        bh->current_movement.completed = 1;
        bh->current_movement.type = not_moving;
    }
}

static void perform_teleport(boss_behaviour_state *bh, boss *bs)
{
    hide_boss(bs);
    bs->pos = bh->current_movement.goal.pos;
    show_boss(bs);

    bh->current_movement.completed = 1;
    bh->current_movement.type = not_moving;
}

void tick_boss_ai(boss_behaviour_state *bh, boss *bs, term_state *ts)
{
    switch (bh->current_movement.type) {
        case not_moving:
            break;
        case horizontal:
            tick_horizontal_movement(bh, bs, ts);
            break;
        case vertical:
            tick_vertical_movement(bh, bs, ts);
            break;
        case teleport:
            perform_teleport(bh, bs);
            break;
    }     

    switch (bh->current_attack.type) {
        case no_attack:
            break;
        case bullet_burst:
            break;
        case gun_volley:
            break;
        case mine_field:
            break;
        case force_blast:
            break;
    }     
}
