/* shootemup_v2/menus.c */
#include "menus.h"

#include <curses.h>
#include <string.h>
#include <unistd.h>

enum { key_escape_c = 27 };
enum { go_menu_init_delay = 1500 }; /* ms */
enum { nanosec_in_ms = 1000 };

static const char game_over_text_l1[] = "GAME OVER";
static const char game_over_text_l2[] = "SCORE: ";
static const char game_over_text_l3[] = "press <ESC> to exit or "
                                        "any other key to resart";

game_over_menu_res play_game_over_menu(term_state *ts, player_state *ps)
{
    int x, y;
    int key;

    timeout(-1);

    erase();

    y = (ts->row - 1)/2;
    x = (ts->col - strlen(game_over_text_l1))/2;
    move(y, x);
    addstr(game_over_text_l1);

    y++;
    x = (ts->col - strlen(game_over_text_l2) - 2)/2;
    move(y, x);
    printw("%s%d", game_over_text_l2, ps->score);

    y++;
    x = (ts->col - strlen(game_over_text_l3))/2;
    move(y, x);
    addstr(game_over_text_l3);

    refresh();

    usleep(go_menu_init_delay*nanosec_in_ms);

    key = getch();
    erase();

    return key == key_escape_c || key == KEY_RESIZE ?
        exit_game :
        restart_game;
}
