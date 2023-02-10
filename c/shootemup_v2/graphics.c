/* shootemup_v2/graphics.c */
#include "graphics.h"

#include <curses.h>

point point_literal(int x, int y)
{
    point p;
    p.x = x;
    p.y = y;
    return p;
}

point add_points(point p1, point p2)
{
    return point_literal(p1.x+p2.x, p1.y+p2.y);
}

int points_are_equal(point p1, point p2)
{
    return p1.x == p2.x && p1.y == p2.y;
}

int init_graphics(term_state *ts)
{
    initscr();
    noecho();
    curs_set(0);

    getmaxyx(stdscr, ts->row, ts->col);
    return 1;
}

void refresh_term_state(term_state *ts)
{
    getmaxyx(stdscr, ts->row, ts->col);
}

void deinit_graphics()
{
    endwin();
}

void draw_pix(point p, char sym)
{
    move(p.y, p.x);
    addch(sym);
}

void refresh_scr()
{
    refresh();
}
