/* shootemup_v2/graphics.c */
#include "graphics.h"

#include <curses.h>

int init_graphics(term_state *ts)
{
    initscr();
    noecho();
    curs_set(0);
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
