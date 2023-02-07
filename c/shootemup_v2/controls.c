/* shootemup_v2/controls.c */
#include "controls.h"

#include <curses.h>

enum { input_timeout = 25000 };
enum { key_escape = 27 };

void init_controls()
{
    cbreak();
    keypad(stdscr, 1);
    timeout(input_timeout);
}

input_action get_player_action()
{
    int key = getch();
    switch (key) {
        case 'w':
            return up;
        case 's':
            return down;
        case 'a':
            return left;
        case 'd':
            return right;
        case ' ':
            return fire;
        case key_escape:
            return exit;
        case KEY_RESIZE:
            return resize;
        default:
            return idle;
    }
}
