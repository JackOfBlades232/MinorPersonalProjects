/* shootemup_v2/boss_ai.c */
#include "boss_ai.h"
#include "explosion.h"
#include "geom.h"
#include "utils.h"

#include <stdlib.h>

enum {
    left_movement_aim = 3,
    right_movement_aim = -3
};

enum {
    in_move_delay_frames = 50
};

enum {
    bullet_burst_first_move_frames = 150,
    bullet_burst_move_and_spray_frames = 225
};

enum {
    battering_ram_thrust_frames = 100
};

enum {
    force_blast_frames = 100
};

enum {
    sliding_volley_movement_frames = 350,
    sliding_volley_attack_frames = 275
};

enum {
    num_regular_sequences = 5,
    inter_sequence_delay_frames_min = 100,
    inter_sequence_delay_frames_max = 200
};

#define BULLET_BURST_SCREEN_FRAC 0.6

void init_boss_ai(boss_behaviour *beh)
{
    beh->current_state.movement.type = not_moving;
    beh->current_state.attack.type = no_attack;
    beh->current_state.movement.completed = 1;
    beh->current_state.attack.completed = 1;
    beh->current_sequence = NULL;
    beh->has_reached_half_hp = 0;
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

static int perform_boss_attack(boss_behaviour *beh, boss *bs,
                        boss_attack_type type, int frames)
{
    beh->current_state.attack.type = type;
    beh->current_state.attack.completed = 0;

    if (type != mine_field)
        beh->current_state.attack.frames = frames;

    return 1;
}

static int perform_delay(boss_behaviour *beh, boss *bs, int frames)
{
    beh->current_state.movement.type = not_moving;
    beh->current_state.attack.type = no_attack;
    beh->current_state.movement.completed = 0;
    beh->current_state.attack.completed = 0;

    beh->current_state.attack.frames = frames;

    return 1;
}

static int perform_in_move_delay(boss_behaviour *beh, boss *bs, player *p)
{
    return perform_delay(beh, bs, in_move_delay_frames);
}

static int ready_for_sequence(boss_behaviour *beh)
{
    return beh->current_state.movement.completed &&
        beh->current_state.movement.type == not_moving &&
        beh->current_state.attack.completed &&
        beh->current_state.attack.type == no_attack;
}

static int teleport_to_init_pos(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int x = (ts->col-boss_width)/2;
    return teleport_boss_to_pos(beh, bs, 
            point_literal(x, boss_init_pos_y), ts);
}

static int return_to_top_movement(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    return move_boss_to_xy(beh, bs, 
            vertical, 1, bullet_burst_first_move_frames, ts);
}

static int bullet_burst_first_movement(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int x = randint(0, 2) ? 
        left_movement_aim : 
        ts->col - boss_width + right_movement_aim; /* choose left or right */

    return move_boss_to_xy(beh, bs, 
            horizontal, x, bullet_burst_first_move_frames, ts);
}

static int bullet_burst_second_movement(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int x = bs->pos.x + (
            (int) ((double)ts->col * BULLET_BURST_SCREEN_FRAC - 1) *
            (bs->pos.x <= left_movement_aim ? 1 : -1)
            );
    return move_boss_to_xy(beh, bs, 
            horizontal, x, bullet_burst_move_and_spray_frames, ts);
}

static int bullet_burst_second_attack(boss_behaviour *beh, boss *bs, player *p)
{
    return perform_boss_attack(beh, bs, 
            bullet_burst, bullet_burst_move_and_spray_frames);
}

static const boss_sequence_elem bullet_burst_seq[] =
{
    { return_to_top_movement, NULL },
    { bullet_burst_first_movement, NULL },
    { bullet_burst_second_movement, bullet_burst_second_attack },
    { NULL, NULL }
};

static int battering_ram_teleport(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int x = min_int(
            p->pos.x + (player_width-boss_width)/2,
            ts->col - boss_width
            );
    return teleport_boss_to_pos(beh, bs, point_literal(x, 1), ts);
}

static int battering_ram_forward_thrust(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int y = min_int(p->pos.y, ts->row - boss_height);
    return move_boss_to_xy(
            beh, bs, vertical, y, battering_ram_thrust_frames, ts
            );
}

static const boss_sequence_elem battering_ram_seq[] =
{
    { battering_ram_teleport, NULL },
    { NULL, perform_in_move_delay },
    { battering_ram_forward_thrust, NULL },
    { battering_ram_teleport, NULL },
    { battering_ram_forward_thrust, NULL },
    { battering_ram_teleport, NULL },
    { battering_ram_forward_thrust, NULL },
    { teleport_to_init_pos, NULL },
    { NULL, NULL }
};

static int mine_plant_attack(boss_behaviour *beh, boss *bs, player *p)
{
    return perform_boss_attack(beh, bs, mine_field, 0);
}

static const boss_sequence_elem mine_plant_seq[] = 
{
    { teleport_to_init_pos, NULL },
    { NULL, perform_in_move_delay },
    { NULL, mine_plant_attack },
    { NULL, NULL }
};

static int teleport_to_center(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int x = (ts->col-boss_width)/2;
    int y = (ts->row-boss_height)/2;
    return teleport_boss_to_pos(beh, bs, point_literal(x, y), ts);
}

static int force_blast_attack(boss_behaviour *beh, boss *bs, player *p)
{
    return perform_boss_attack(beh, bs, force_blast, force_blast_frames);
}

static const boss_sequence_elem force_blast_seq[] =
{
    { teleport_to_center, NULL },
    { NULL, perform_in_move_delay },
    { NULL, force_blast_attack },
    { NULL, force_blast_attack },
    { NULL, force_blast_attack },
    { NULL, NULL }
};

static int sliding_volley_movement(
        boss_behaviour *beh, boss *bs, player *p, term_state *ts)
{
    int x;

    if (bs->pos.x <= left_movement_aim || 
            bs->pos.x >= ts->col - boss_width + right_movement_aim) {
        x = (ts->col-boss_width)/2;
    } else if (p->pos.x <= (ts->col - player_width)/2)
        x = left_movement_aim;
    else 
        x = ts->col - boss_width + right_movement_aim;

    return move_boss_to_xy(beh, bs, horizontal, 
            x, sliding_volley_movement_frames, ts);
}

static int sliding_volley_random_attack(
        boss_behaviour *beh, boss *bs, player *p)
{
    boss_attack_type type = randint(0, 2) ? gun_volley : bullet_burst;
    return perform_boss_attack(beh, bs, type, sliding_volley_attack_frames);
}

static const boss_sequence_elem sliding_volley_seq[] =
{
    { return_to_top_movement, NULL },
    { sliding_volley_movement, sliding_volley_random_attack },
    { sliding_volley_movement, sliding_volley_random_attack },
    { sliding_volley_movement, sliding_volley_random_attack },
    { sliding_volley_movement, sliding_volley_random_attack },
    { NULL, NULL }
};

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

static int boss_is_in_delay(boss_behaviour *beh)
{
    return beh->current_state.movement.type == not_moving &&
        beh->current_state.attack.type == no_attack &&
        !beh->current_state.movement.completed &&
        !beh->current_state.attack.completed;
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

static void tick_boss_delay(boss_behaviour *beh)
{
    if (boss_is_in_delay(beh)) {
        beh->current_state.attack.frames--;

        if (beh->current_state.attack.frames <= 0) {
            beh->current_state.movement.completed = 1;
            beh->current_state.attack.completed = 1;
        }
    }
}

static int next_sequence_elem_ready(boss_behaviour *beh)
{
    return beh->current_sequence && ready_for_sequence(beh);
}

static void tick_boss_sequence(boss_behaviour *beh, boss *bs, 
        player *p, term_state *ts)
{
    if (next_sequence_elem_ready(beh)) {
        int res = beh->current_sequence->move || beh->current_sequence->atk;

        if (res) {
            if (beh->current_sequence->move) 
                (*(beh->current_sequence->move))(beh, bs, p, ts);
            else
                beh->current_state.movement.completed = 1;

            if (beh->current_sequence->atk) 
                (*(beh->current_sequence->atk))(beh, bs, p);
            else
                beh->current_state.attack.completed = 1;

            beh->current_sequence++;
        } else
            beh->current_sequence = NULL;
    }
}

static const boss_sequence_elem *all_sequences[num_regular_sequences] = 
{
    bullet_burst_seq, 
    battering_ram_seq, 
    mine_plant_seq,
    sliding_volley_seq
};

static void try_decide_next_sequence(boss_behaviour *beh, boss *bs)
{
    if (beh->current_sequence || !ready_for_sequence(beh))
        return;

    if (!beh->has_reached_half_hp && bs->state.cur_hp <= bs->state.max_hp / 2) {
        beh->current_sequence = force_blast_seq;
        beh->has_reached_half_hp = 1;
    } else {
        beh->current_sequence =
            all_sequences[randint(0, num_regular_sequences)];
    }

    perform_delay(
            beh, bs,
            randint(
                inter_sequence_delay_frames_min, 
                inter_sequence_delay_frames_max
                )
            );
}

void tick_boss_ai(boss_behaviour *beh, boss *bs, player *p,
        boss_projectile_buf proj_buf, explosion_buf expl_buf, term_state *ts)
{
    tick_boss_movement(beh, bs, ts);
    tick_boss_attack(beh, bs, proj_buf, expl_buf, ts);
    tick_boss_delay(beh);

    tick_boss_sequence(beh, bs, p, ts);

    try_decide_next_sequence(beh, bs);
}
