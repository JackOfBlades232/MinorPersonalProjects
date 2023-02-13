/* shootemup_v2/menus.c */
#include "menus.h"
#include "utils.h"

#include <curses.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum { key_escape_code = 27 };

enum { go_menu_init_delay = 1500 }; /* ms */
enum { nanosec_in_ms = 1000 };

enum { out_text_bufsize = 64 };

static char out_text_buf[out_text_bufsize];
static const char plain_text_fstr[] = "%.63s";
static const char text_with_int_fstr[] = "%.52s%d";

static const char game_over_txt[] = "GAME OVER";
static const char score_txt[] = "SCORE: ";
static const char instructions_txt[] = "press <ESC> to exit or "
                                       "R to resart";

static void draw_text_in_center(int y, 
        const char *prefix, int *content,
        term_state *ts)
{
    int x;

    if (content)
        sprintf(out_text_buf, text_with_int_fstr, prefix, *content);
    else
        sprintf(out_text_buf, plain_text_fstr, prefix);

    x = (ts->col - strlen(out_text_buf))/2;
    move(y, x);
    addstr(out_text_buf);
}

static void draw_game_over_menu(term_state *ts, player_state *ps)
{
    int y;

    erase();

    y = (ts->row - 1)/2;
    draw_text_in_center(y, game_over_txt, NULL, ts);

    y++;
    draw_text_in_center(y, score_txt, &ps->score, ts);

    y++;
    draw_text_in_center(y, instructions_txt, NULL, ts);

    refresh_scr();
}

game_over_menu_res play_game_over_menu(term_state *ts, player_state *ps)
{
    int key;

    timeout(-1);

    draw_game_over_menu(ts, ps);
    usleep(go_menu_init_delay*nanosec_in_ms);

    while ((key = getch()) != key_escape_code) {
        switch (key) {
            case 'r':
                clear();
                return restart_game;
            case KEY_RESIZE:
                return exit_game;
            default:
                break;
        }
    }

    return exit_game;
}
