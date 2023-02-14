/* shootemup_v2/player.c */
#include "player.h"
#include "geom.h"
#include "colors.h"
#include "utils.h"

#include <curses.h>

enum { player_init_screen_edge_offset = 3 };

enum { 
    player_movement_frames = 2,
    player_shooting_frames = 16,
    player_ammo_replenish_frames = 9,
    frames_from_shot_to_bullet_replenish = 300 
};

enum { bullet_emitter_cnt = 2 };
enum { bullet_movement_frames = 1 };

static const char player_shape[player_height][player_width+1] =
{ 
    "   **   ", 
    "   **   ", 
    " * ** * ", 
    "** ** **"
};

static const int bullet_emitter_positions[bullet_emitter_cnt][2] = 
{
    { 2, 2 }, { 5, 2 }
};

void init_player(player *p, 
        int max_hp, int max_ammo, int bullet_dmg,
        term_state *ts)
{
    p->pos.x = (ts->col - player_width)/2 + 1;
    p->pos.y = ts->row - player_init_screen_edge_offset - player_height;

    p->state.cur_hp = p->state.max_hp = max_hp;
    p->state.cur_ammo = p->state.max_ammo = max_ammo;
    p->state.bullet_dmg = bullet_dmg;
    p->state.score = 0;
    p->state.frames_since_moved = player_movement_frames;
    p->state.frames_since_shot = frames_from_shot_to_bullet_replenish;
    p->state.frames_since_recovered_ammo = player_ammo_replenish_frames;

    show_player(p);
}

static void truncate_player_pos(player *p, term_state *ts)
{
    clamp_int(&p->pos.x, 1, ts->col-player_width);
    clamp_int(&p->pos.y, 1, ts->row-player_height);
}

void show_player(player *p)
{
    int i, j;
    attrset(get_color_pair(player_color_pair));
    for (i = 0; i < player_height; i++) {
        move(p->pos.y + i, p->pos.x);
        for (j = 0; j < player_width; j++) {
            char shape_symbol = player_shape[i][j];
            if (char_is_symbol(shape_symbol))
                addch(shape_symbol);
            else
                move(p->pos.y + i, p->pos.x + j + 1);
        }
    }
}

void hide_player(player *p)
{
    int i, j;
    attrset(get_color_pair(0));
    for (i = 0; i < player_height; i++) {
        move(p->pos.y + i, p->pos.x);
        for (j = 0; j < player_width; j++)
            addch(' ');
    }
}

void move_player(player *p, int dx, int dy, term_state *ts)
{
    if (p->state.frames_since_moved >= player_movement_frames) {
        hide_player(p);
        p->pos.x += dx;
        p->pos.y += dy;
        truncate_player_pos(p, ts);
        show_player(p);

        p->state.frames_since_moved = 0;
    }
}

static int local_point_is_in_player_square(point pt)
{
    return pt.x >= 0 && pt.x < player_width && 
        pt.y >= 0 && pt.y < player_height;
}

int point_is_in_player(player *pl, point pt)
{
    int i;
    int not_leftmost, not_rightmost;
    const char *player_line;

    pt.x -= pl->pos.x;
    pt.y -= pl->pos.y;
    if (!local_point_is_in_player_square(pt))
        return 0;

    player_line = player_shape[pt.y];
    not_leftmost = not_rightmost = 0;

    for (i = pt.x; i >= 0; i++) {
        if (char_is_symbol(player_line[i])) {
            not_leftmost = 1;
            break;
        }
    }
    for (i = pt.x; i < player_width; i++) {
        if (char_is_symbol(player_line[i])) {
            not_rightmost = 1;
            break;
        }
    }

    return not_leftmost && not_rightmost;
}

int damage_player(player *p, int damage)
{
    p->state.cur_hp -= damage;
    return player_is_dead(p);
}

int player_is_dead(player *p)
{
    return p->state.cur_hp <= 0;
}

void add_score(player *p, int d_score)
{
    p->state.score += d_score;
}

void init_player_bullet_buf(player_bullet_buf bullet_buf)
{
    int i;
    for (i = 0; i < player_bullet_bufsize; i++)
        bullet_buf[i].is_alive = 0;
}

static int player_is_eligible_for_ammo_repl(player *p)
{
    return p->state.frames_since_shot >= frames_from_shot_to_bullet_replenish &&
        p->state.frames_since_recovered_ammo >= player_ammo_replenish_frames &&
        p->state.cur_ammo < p->state.max_ammo;
}

void handle_player_ammo_replenish(player *p)
{
    if (player_is_eligible_for_ammo_repl(p)) {
        p->state.cur_ammo++;
        p->state.frames_since_recovered_ammo = 0;
    }
}

static void show_bullet(player_bullet *b)
{
    attrset(switch_and_get_pbullet_color_pair());
    move(b->pos.y, b->pos.x);
    addch('^');
}

static void hide_bullet(player_bullet *b)
{
    attrset(get_color_pair(0));
    move(b->pos.y, b->pos.x);
    addch(' ');
}

static void shoot_bullet(player_bullet *b, player *p,
        int emitter_x, int emitter_y)
{
    b->pos = point_literal(emitter_x, emitter_y);
    b->is_alive = 1;
    b->damage = p->state.bullet_dmg;
    b->frames_since_moved = 0;
    b->dx = 0;
    b->dy = -1;
    show_bullet(b);
}

static int player_is_eligible_for_shooting(player *p)
{
    return p->state.cur_ammo > 0 && 
        p->state.frames_since_shot >= player_shooting_frames;
}

int player_shoot(player *p, player_bullet_buf bullet_buf)
{
    int i;
    int emitters_left = bullet_emitter_cnt;

    
    /* temp */
    if (player_is_eligible_for_shooting(p)) {
        for (i = 0; i < player_bullet_bufsize && emitters_left > 0; i++) {
            if (!bullet_buf[i].is_alive) {
                emitters_left--;
                shoot_bullet(
                    &bullet_buf[i], p,
                    p->pos.x + bullet_emitter_positions[emitters_left][0],
                    p->pos.y + bullet_emitter_positions[emitters_left][1]
                );
            }
        }

        p->state.cur_ammo--;
        p->state.frames_since_shot = 0;
        
        return bullet_emitter_cnt - emitters_left;
    } else
        return 0;
}

static void update_bullet(player_bullet *b)
{
    if (b->is_alive) {
        if (b->frames_since_moved < bullet_movement_frames)
            b->frames_since_moved++;
        else {
            hide_bullet(b);
            b->pos.x += b->dx;
            b->pos.y += b->dy;

            /* temp */
            if (b->pos.y < 0) {
                kill_bullet(b);
                return;
            }

            show_bullet(b);

            b->frames_since_moved = 0;
        }
    }
}

void update_live_bullets(player_bullet_buf bullet_buf)
{
    int i;
    for (i = 0; i < player_bullet_bufsize; i++)
        update_bullet(bullet_buf+i);
}

int kill_bullet(player_bullet *b)
{
    b->is_alive = 0;
    hide_bullet(b);

    return 1;
}

void update_player_frame_counters(player *p)
{
    p->state.frames_since_moved++;
    p->state.frames_since_shot++;
    p->state.frames_since_recovered_ammo++;
}
