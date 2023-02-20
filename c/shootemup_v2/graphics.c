/* shootemup_v2/graphics.c */
#include "graphics.h"

#include <curses.h>
#include <stdio.h>

int init_graphics(term_state *ts)
{
    initscr();

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Can't show colors on a BW screen!\n");
        return 0;
    }

    noecho();
    curs_set(0);
    start_color();
    getmaxyx(stdscr, ts->row, ts->col);

    return ts->col >= min_term_width && ts->row >= min_term_height;
}

void deinit_graphics()
{
    endwin();
}

void refresh_scr()
{
    refresh();
}

void draw_local_point(int loc_y, int loc_x, point base, char symbol)
{
    move(base.y + loc_y, base.x + loc_x);
    addch(symbol);
}

int obj_is_in_bounds(point pos, int width, int height, term_state *ts)
{
    return pos.x >= 0 && pos.y >= 1 &&
        pos.x <= ts->col - width &&
        pos.y <= ts->row - height;
}
