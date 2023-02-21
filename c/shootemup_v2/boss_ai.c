/* shootemup_v2/boss_ai.c */
#include "boss_ai.h"
#include "explosion.h"
#include "utils.h"

#include <stdlib.h>

enum {
    bullet_burst_first_move_frames = 150,
    bullet_burst_move_and_spray_frames = 225
};

#define BULLET_BURST_SCREEN_FRAC 0.6

void init_boss_ai(boss_behaviour *beh)
{
    beh->current_state.movement.type = not_moving;
    beh->current_state.attack.type = no_attack;
    beh->current_state.movement.completed = 1;
    beh->current_state.attack.completed = 1;
    beh->current_sequence = NULL;
}

static int target_is_in_bounds(boss *bs, 
        boss_movement_type type, int tg, term_state *ts)
{
    if (type == horizontal)
        return tg >= 0 && tg <= ts->col - boss_width && bs->pos.x != tg;
    else if (type == vertical)
        return tg >= 1 && tg <= ts->row - boss_height && bs->pos.y != tg;
    else
        return 0;
}

static int move_boss_to_xy(boss_behaviour *beh, boss *bs, 
        boss_movement_type type, int tg, int frames, term_state *ts)
{
    if (!target_is_in_bounds(bs, type, tg, ts))
        return 0;

    beh->current_state.movement.type = type;
    if (type == horizontal)
        beh->current_state.movement.goal.x = tg;
    else
        beh->current_state.movement.goal.y = tg;
    beh->current_state.movement.completed = 0;

    bs->state.cur_movement_frames = 
        frames / abs_int(tg - (type == horizontal ? bs->pos.x : bs->pos.y));

    return 1;
}

static int teleport_boss_to_pos(boss_behaviour *beh, 
        boss *bs, point pos, term_state *ts)
{
    if (!obj_is_in_bounds(pos, boss_width, boss_height, ts))
        return 0;

    beh->current_state.movement.type = teleport;
    beh->current_state.movement.goal.pos = pos;
    beh->current_state.movement.completed = 0;

    return 1;
}

int perform_boss_attack(boss_behaviour *beh, boss *bs,
                        boss_attack_type type, int frames)
{
    beh->current_state.attack.type = type;
    beh->current_state.attack.completed = 0;

    if (type != mine_field)
        beh->current_state.attack.frames = frames;

    return 1;
}

static int bullet_burst_first_movement(
        boss_behaviour *beh, boss *bs, term_state *ts)
{
    int x = randint(0, 2) ? 0 : ts->col - boss_width; /* choose left or right */

    return move_boss_to_xy(beh, bs, 
            horizontal, x, bullet_burst_first_move_frames, ts);
}

static int bullet_burst_second_movement(
        boss_behaviour *beh, boss *bs, term_state *ts)
{
    int x = bs->pos.x + (
            (int) ((double)ts->col * BULLET_BURST_SCREEN_FRAC - 1) *
            (bs->pos.x == 0 ? 1 : -1)
            );
    return move_boss_to_xy(beh, bs, 
            horizontal, x, bullet_burst_move_and_spray_frames, ts);
}

static int bullet_burst_second_attack(boss_behaviour *beh, boss *bs)
{
    return perform_boss_attack(beh, bs, 
            bullet_burst, bullet_burst_move_and_spray_frames);
}

static const boss_sequence_elem bullet_burst_seq[] =
{
    { bullet_burst_first_movement, NULL },
    { bullet_burst_second_movement, bullet_burst_second_attack },
    { NULL, NULL }
};

int perform_bullet_burst(boss_behaviour *beh)
{
    if (beh->current_state.movement.completed && 
            beh->current_state.attack.completed) {
        beh->current_sequence = bullet_burst_seq;
        return 1;
    }

    return 0;
}

static void halt_movement(boss_behaviour *beh)
{
    beh->current_state.movement.completed = 1;
    beh->current_state.movement.type = not_moving;
}

static void tick_horizontal_movement(boss_behaviour *beh, 
        boss *bs, term_state *ts)
{
    int dx = sgn_int(beh->current_state.movement.goal.x - bs->pos.x);
    move_boss(bs, dx, 0, ts);

    if (bs->pos.x == beh->current_state.movement.goal.x)
        halt_movement(beh);
}

static void tick_vertical_movement(boss_behaviour *beh, 
        boss *bs, term_state *ts)
{
    int dy = sgn_int(beh->current_state.movement.goal.y - bs->pos.y);
    move_boss(bs, 0, dy, ts);

    if (bs->pos.y == beh->current_state.movement.goal.y)
        halt_movement(beh);
}

static void perform_teleport(boss_behaviour *beh, boss *bs)
{
    hide_boss(bs);
    bs->pos = beh->current_state.movement.goal.pos;
    show_boss(bs);

    halt_movement(beh);
}

static void halt_attack(boss_behaviour *beh)
{
    beh->current_state.attack.completed = 1;
    beh->current_state.attack.type = no_attack;
}

static void tick_bullet_burst(boss_behaviour *beh, boss *bs,
        boss_projectile_buf proj_buf)
{
    boss_shoot_bullet(bs, proj_buf);

    beh->current_state.attack.frames--;

    if (beh->current_state.attack.frames <= 0)
        halt_attack(beh);
};

static void tick_gun_volley(boss_behaviour *beh, boss *bs,
        boss_projectile_buf proj_buf)
{
    boss_shoot_gun(bs, proj_buf);

    beh->current_state.attack.frames--;

    if (beh->current_state.attack.frames <= 0)
        halt_attack(beh);
}

static int all_mines_dead(boss_projectile_buf proj_buf)
{
    int i;
    for (i = 0; i < boss_projectile_bufsize; i++) {
        if (proj_buf[i].is_alive && proj_buf[i].type == mine)
            return 0;
    }

    return 1;
}

static void tick_mine_plant(boss_behaviour *beh, boss *bs,
        boss_projectile_buf proj_buf, term_state *ts)
{
    if (beh->current_state.attack.completed) {
        if (all_mines_dead(proj_buf))
            beh->current_state.attack.type = no_attack;
    } else { 
        boss_plant_mines(bs, proj_buf, ts);
        beh->current_state.attack.completed = 1;
    }
}

static void tick_force_blast(boss_behaviour *beh, 
        boss *bs, explosion_buf expl_buf, term_state *ts)
{
    if (beh->current_state.attack.completed) {
        beh->current_state.attack.frames--;

        if (beh->current_state.attack.frames <= 0)
            beh->current_state.attack.type = no_attack;
    } else { 
        boss_emit_force_field(bs, expl_buf, ts);
        beh->current_state.attack.completed = 1;
    }
}

static void tick_boss_movement(boss_behaviour *beh, boss *bs, term_state *ts)
{
    switch (beh->current_state.movement.type) {
        case not_moving:
            break;
        case horizontal:
            tick_horizontal_movement(beh, bs, ts);
            break;
        case vertical:
            tick_vertical_movement(beh, bs, ts);
            break;
        case teleport:
            perform_teleport(beh, bs);
            break;
    }     
}

static void tick_boss_attack(boss_behaviour *beh, boss *bs,
        boss_projectile_buf proj_buf, explosion_buf expl_buf, term_state *ts)
{
    switch (beh->current_state.attack.type) {
        case no_attack:
            break;
        case bullet_burst:
            tick_bullet_burst(beh, bs, proj_buf);
            break;
        case gun_volley:
            tick_gun_volley(beh, bs, proj_buf);
            break;
        case mine_field:
            tick_mine_plant(beh, bs, proj_buf, ts);
            break;
        case force_blast:
            tick_force_blast(beh, bs, expl_buf, ts);
            break;
    }     
}

static int next_sequence_elem_ready(boss_behaviour *beh)
{
    return beh->current_sequence && 
        beh->current_state.movement.completed &&
        beh->current_state.attack.completed;
}

static void tick_boss_sequence(boss_behaviour *beh, boss *bs, term_state *ts)
{
    if (next_sequence_elem_ready(beh)) {
        int res = beh->current_sequence->move || beh->current_sequence->atk;

        if (res) {
            if (beh->current_sequence->move) 
                (*(beh->current_sequence->move))(beh, bs, ts);
            else
                beh->current_state.movement.completed = 1;

            if (beh->current_sequence->atk) 
                (*(beh->current_sequence->atk))(beh, bs);
            else
                beh->current_state.attack.completed = 1;

            beh->current_sequence++;
        } else
            beh->current_sequence = NULL;
    }
}

void tick_boss_ai(boss_behaviour *beh, boss *bs,
        boss_projectile_buf proj_buf, explosion_buf expl_buf, term_state *ts)
{
    tick_boss_movement(beh, bs, ts);
    tick_boss_attack(beh, bs, proj_buf, expl_buf, ts);

    tick_boss_sequence(beh, bs, ts);
}
