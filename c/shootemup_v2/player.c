/* shootemup_v2/player.c */
#include "player.h"
#include "utils.h"

#include <curses.h>

enum { player_width = 8, player_height = 4 };
enum { player_init_screen_edge_offset = 3 };

static const char player_shape[player_height][player_width] =
{ 
    "   **   ", 
    "   **   ", 
    " * ** * ", 
    "** ** **"
};

void init_player(player *p, int max_hp, term_state *ts)
{
    p->pos.x = (ts->col - player_width)/2 + 1;
    p->pos.y = ts->row - player_init_screen_edge_offset - 1;
    p->cur_hp = p->max_hp = max_hp;
    show_player(p);
}

static void truncate_player_pos(player *p, term_state *ts)
{
    clamp_int(&p->pos.x, 0, ts->col-1);
    clamp_int(&p->pos.y, 0, ts->row-1);
}

void show_player(player *p)
{
    int i;
    for (i = 0; i < player_height; i++) {
        move(p->pos.x, p->pos.y + i);
        addstr(player_shape[i]);
    }
}

void hide_player(player *p)
{
    int i, j;
    for (i = 0; i < player_height; i++) {
        move(p->pos.x, p->pos.y + i);
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
    return pt.x < 0 || pt.x >= player_width || 
        pt.y < 0 || pt.y >= player_height;
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
