/* shootemup_v2/player.c */
#include "player.h"
#include "geom.h"
#include "utils.h"

#include <curses.h>

enum { player_init_screen_edge_offset = 3 };

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

void init_player(player *p, int max_hp, int bullet_dmg, term_state *ts)
{
    p->pos.x = (ts->col - player_width)/2 + 1;
    p->pos.y = ts->row - player_init_screen_edge_offset - player_height;
    p->cur_hp = p->max_hp = max_hp;
    p->bullet_dmg = bullet_dmg;

    show_player(p);
}

static void truncate_player_pos(player *p, term_state *ts)
{
    clamp_int(&p->pos.x, 1, ts->col-player_width);
    clamp_int(&p->pos.y, 1, ts->row-player_height);
}

void show_player(player *p)
{
    int i;
    for (i = 0; i < player_height; i++) {
        move(p->pos.y + i, p->pos.x);
        addstr(player_shape[i]);
    }
}

void hide_player(player *p)
{
    int i, j;
    for (i = 0; i < player_height; i++) {
        move(p->pos.y + i, p->pos.x);
        for (j = 0; j < player_width; j++)
            addch(' ');
    }
}

void move_player(player *p, int dx, int dy, term_state *ts)
{
    hide_player(p);
    p->pos.x += dx;
    p->pos.y += dy;
    truncate_player_pos(p, ts);
    show_player(p);
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
    p->cur_hp -= damage;

    move(0, 0);
    printw("%d   ", p->cur_hp);

    return player_is_dead(p);
}

int player_is_dead(player *p)
{
    return p->cur_hp <= 0;
}

void init_player_bullet_buf(player_bullet_buf bullet_buf)
{
    int i;
    for (i = 0; i < player_bullet_bufsize; i++)
        bullet_buf[i].is_alive = 0;
}

static void show_bullet(player_bullet *b)
{
    move(b->pos.y, b->pos.x);
    addch('^');
}

static void hide_bullet(player_bullet *b)
{
    move(b->pos.y, b->pos.x);
    addch(' ');
}

static void shoot_bullet(player_bullet *b, player *p,
        int emitter_x, int emitter_y)
{
    b->pos = point_literal(emitter_x, emitter_y);
    b->is_alive = 1;
    b->damage = p->bullet_dmg;
    b->frames_until_move = bullet_movement_frames;
    b->dx = 0;
    b->dy = -1;
    show_bullet(b);
}

int player_shoot(player *p, player_bullet_buf bullet_buf)
{
    int i;
    int emitters_left = bullet_emitter_cnt;

    /* temp */
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
    
    return bullet_emitter_cnt - emitters_left;
}

static void update_bullet(player_bullet *b)
{
    if (b->is_alive) {
        if (b->frames_until_move > 0)
            b->frames_until_move--;
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

            b->frames_until_move = bullet_movement_frames;
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
