/* shootemup_v2/controls.c */
#include "controls.h"

#include <curses.h>

enum { key_escape = 27 };
enum { frame_duration = 10 };

void init_controls()
{
    cbreak();
    keypad(stdscr, 1);
}

void reset_controls_timeout()
{
    timeout(frame_duration);
}

input_action get_input_action()
{
    int key = getch();
    if (key == 'w' || key == KEY_UP)
        return up;
    else if (key == 's' || key == KEY_DOWN)
        return down;
    else if (key == 'a' || key == KEY_LEFT)
        return left;
    else if (key == 'd' || key == KEY_RIGHT)
        return right;
    else if (key == ' ' || key == '\r')
        return fire;
    else if (key == KEY_RESIZE)
        return resize;
    else if (key == key_escape)
        return quit;
    else
        return idle;
}
