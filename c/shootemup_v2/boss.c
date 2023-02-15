/* shootemup_v2/boss.c */
#include "boss.h"
#include "colors.h"
#include "utils.h"

#include <curses.h>

enum { 
    boss_movement_frames = 4,
    boss_bullet_shooting_frames = 4,
    boss_gunshot_shooting_frames = 32
};

enum { 
    bullet_movement_frames = 1,
    gunshot_movement_frames = 3
};

enum { 
    bullet_emitter_cnt = 4,
    gunshot_emitter_cnt = 2
};

enum {
    mine_field_width = 5,
    mine_field_height = 3,
    mine_expl_frames = 300
};

#define MINE_EXPL_RAD 10.0

static const char boss_shape[boss_height][boss_width+1] =
{
    "*******************",
    " ** *********** ** ",
    "  * *********** *  ",
    "    *** *** ***    ",
    "     ** *** **     ",
    "        ***        ",
    "        ***        ",
    "         *         "
};

static const int bullet_emitter_positions[bullet_emitter_cnt][2] =
{
    { 3, 1 }, { 7, 3 }, { 11, 3 }, { 15, 1 }
};

static const int gunshot_emitter_positions[gunshot_emitter_cnt][2] =
{
    { 7, 3 }, { 11, 3 } 
};

void init_boss(boss *bs, 
        int max_hp, int bullet_dmg, int gunshot_dmg, int mine_dmg,
        term_state *ts)
{
    bs->pos.x = (ts->col - boss_width)/2;
    bs->pos.y = 1;

    bs->state.cur_hp = bs->state.max_hp = max_hp;

    bs->state.bullet_dmg = bullet_dmg;
    bs->state.gunshot_dmg = gunshot_dmg;
    bs->state.mine_dmg = mine_dmg;

    bs->state.frames_since_moved = boss_movement_frames;
    bs->state.frames_since_shot = boss_bullet_shooting_frames;

    show_boss(bs);
}

static void truncate_boss_pos(boss *bs, term_state *ts)
{
    clamp_int(&bs->pos.x, 1, ts->col-boss_width);
    clamp_int(&bs->pos.y, 1, ts->row-boss_height);
}

void show_boss(boss *bs)
{
    int i, j;
    attrset(get_color_pair(boss_color_pair));
    for (i = 0; i < boss_height; i++) {
        move(bs->pos.y + i, bs->pos.x);
        for (j = 0; j < boss_width; j++) {
            char shape_symbol = boss_shape[i][j];
            if (char_is_symbol(shape_symbol))
                addch(shape_symbol);
            else
                move(bs->pos.y + i, bs->pos.x + j + 1);
        }
    }
}

void hide_boss(boss *bs)
{
    int i, j;
    attrset(get_color_pair(0));
    for (i = 0; i < boss_height; i++) {
        move(bs->pos.y + i, bs->pos.x);
        for (j = 0; j < boss_width; j++)
            addch(' ');
    }
}

void move_boss(boss *bs, int dx, int dy, term_state *ts)
{
    if (bs->state.frames_since_moved >= boss_movement_frames) {
        hide_boss(bs);
        bs->pos.x += dx;
        bs->pos.y += dy;
        truncate_boss_pos(bs, ts);
        show_boss(bs);

        bs->state.frames_since_moved = 0;
    }
}

static int local_point_is_in_boss_square(point pt)
{
    return pt.x >= 0 && pt.x < boss_width && 
        pt.y >= 0 && pt.y < boss_height;
}

int point_is_in_boss(boss *bs, point pt)
{
    int i;
    int not_leftmost, not_rightmost;
    const char *boss_line;

    pt.x -= bs->pos.x;
    pt.y -= bs->pos.y;
    if (!local_point_is_in_boss_square(pt))
        return 0;

    boss_line = boss_shape[pt.y];
    not_leftmost = not_rightmost = 0;

    for (i = pt.x; i >= 0; i++) {
        if (char_is_symbol(boss_line[i])) {
            not_leftmost = 1;
            break;
        }
    }
    for (i = pt.x; i < boss_width; i++) {
        if (char_is_symbol(boss_line[i])) {
            not_rightmost = 1;
            break;
        }
    }

    return not_leftmost && not_rightmost;
}

int damage_boss(boss *bs, int damage)
{
    bs->state.cur_hp -= damage;
    return boss_is_dead(bs);
}

int boss_is_dead(boss *bs)
{
    return bs->state.cur_hp <= 0;
}

void init_boss_projectile_buf(boss_projectile_buf projectile_buf)
{
    int i;
    for (i = 0; i < boss_projectile_bufsize; i++)
        projectile_buf[i].is_alive = 0;
}

static int projectile_color_pair(boss_projectile *pr)
{
    switch (pr->type) {
        case bullet:
            return get_color_pair(bbullet_color_pair);
        case gunshot:
            return get_color_pair(gunshot_color_pair);
        case mine:
            return get_color_pair(mine_color_pair) | A_BLINK;
        default:
            return 0;
    }
}

static char projectile_symbol(boss_projectile *pr)
{
    switch (pr->type) {
        case bullet:
            return 'v';
        case gunshot:
            return '|';
        case mine:
            return '$';
        default:
            return ' ';
    }
}

static void show_projectile(boss_projectile *pr)
{
    attrset(projectile_color_pair(pr));
    move(pr->pos.y, pr->pos.x);
    addch(projectile_symbol(pr));
}

static void hide_projectile(boss_projectile *pr)
{
    attrset(get_color_pair(0));
    move(pr->pos.y, pr->pos.x);
    addch(' ');
}

static void shoot_bullet_or_gunshot(boss_projectile *pr, int is_bullet,
        boss *bs, int emitter_x, int emitter_y)
{
    pr->type = is_bullet ? bullet : gunshot;
    pr->pos = point_literal(emitter_x, emitter_y);
    pr->is_alive = 1;
    pr->damage = is_bullet ? bs->state.bullet_dmg : bs->state.gunshot_dmg;
    pr->frames_since_moved = 0;
    pr->dx = 0;
    pr->dy = 1;
    show_projectile(pr);
}

static int boss_shoot_bullet_or_gunshot(boss *bs, int is_bullet,
        boss_projectile_buf projectile_buf)
{
    int i;
    int emitters_left = is_bullet ? bullet_emitter_cnt : gunshot_emitter_cnt;
    int shooting_frames = is_bullet ? 
        boss_bullet_shooting_frames : 
        boss_gunshot_shooting_frames;
    
    /* temp (emitters) */
    if (bs->state.frames_since_shot >= shooting_frames) {
        for (i = 0; i < boss_projectile_bufsize && emitters_left > 0; i++) {
            if (!projectile_buf[i].is_alive) {
                const int *emitter_pos;
                
                emitters_left--;
                emitter_pos = is_bullet ?
                    bullet_emitter_positions[emitters_left] :
                    gunshot_emitter_positions[emitters_left];

                shoot_bullet_or_gunshot(
                    &projectile_buf[i], is_bullet, bs,
                    bs->pos.x + emitter_pos[0],
                    bs->pos.y + emitter_pos[1]
                );
            }
        }

        bs->state.frames_since_shot = 0;
        
        return bullet_emitter_cnt - emitters_left;
    } else
        return 0;
}

int boss_shoot_bullet(boss *bs, boss_projectile_buf projectile_buf)
{
    return boss_shoot_bullet_or_gunshot(bs, 1, projectile_buf);
}

int boss_shoot_gun(boss *bs, boss_projectile_buf projectile_buf)
{
    return boss_shoot_bullet_or_gunshot(bs, 0, projectile_buf);
}

static int plant_mine(boss_projectile *pr, boss *bs, int x, int y)
{
    pr->type = mine;
    pr->pos = point_literal(x, y);

    if (point_is_in_boss(bs, pr->pos))
        return 0;

    pr->is_alive = 1;
    pr->damage = bs->state.mine_dmg;
    pr->mine_radius = MINE_EXPL_RAD;
    pr->mine_frames_to_expl = mine_expl_frames;

    show_projectile(pr);

    return 1;
}

int boss_plant_mines(boss *bs, boss_projectile_buf projectile_buf,
        term_state *ts)
{
    int w_step, h_step;
    int x, y, i;
    int mines_cnt = 0;

    w_step = ts->col / mine_field_width;
    h_step = ts->row / mine_field_height;
    x = w_step/2;
    y = h_step/2;

    for (i = 0; i < boss_projectile_bufsize; i++) {
        if (!projectile_buf[i].is_alive) {
            mines_cnt += plant_mine(projectile_buf+i, bs, x, y);
            x += w_step;

            if (x >= ts->col) {
                x = w_step/2;
                y += h_step;
            }

            if (y >= ts->row)
                break;
        }
    }

    return mines_cnt;
}

static void update_bullet_or_gunshot(boss_projectile *pr, int is_bullet,
        term_state *ts)
{
    int movement_frames = is_bullet ? 
        bullet_movement_frames : 
        gunshot_movement_frames;

    if (pr->frames_since_moved < movement_frames)
        pr->frames_since_moved++;
    else {
        hide_projectile(pr);
        pr->pos.x += pr->dx;
        pr->pos.y += pr->dy;

        /* temp */
        if (pr->pos.y >= ts->row) {
            kill_boss_projectile(pr);
            return;
        }

        show_projectile(pr);

        pr->frames_since_moved = 0;
    }
}

static void update_mine(boss_projectile *pr, explosion_buf expl_buf)
{
    if (pr->mine_frames_to_expl <= 0) {
        kill_boss_projectile(pr);
        spawn_explosion(expl_buf, pr->pos, pr->mine_radius);
    } else
        pr->mine_frames_to_expl--;
}

static void update_projectile(boss_projectile *pr, 
        explosion_buf expl_buf, term_state *ts)
{
    if (!pr->is_alive)
        return;

    switch (pr->type) {
        case bullet:
            update_bullet_or_gunshot(pr, 1, ts);
            break;
        case gunshot:
            update_bullet_or_gunshot(pr, 0, ts);
            break;
        case mine:
            update_mine(pr, expl_buf);
            break;
    }
}

void update_live_boss_projectiles(boss_projectile_buf projectile_buf,
        explosion_buf expl_buf, term_state *ts)
{
    int i;
    for (i = 0; i < boss_projectile_bufsize; i++)
        update_projectile(projectile_buf+i, expl_buf, ts);
}

int kill_boss_projectile(boss_projectile *pr)
{
    pr->is_alive = 0;
    hide_projectile(pr);

    return 1;
}

void update_boss_frame_counters(boss *bs)
{
    bs->state.frames_since_moved++;
    bs->state.frames_since_shot++;
}
