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
