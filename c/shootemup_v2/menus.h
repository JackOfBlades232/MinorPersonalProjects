/* shootemup_v2/menus.h */
#ifndef MENUS_SENTRY
#define MENUS_SENTRY

#include "graphics.h"
#include "player.h"

typedef enum tag_menu_res { 
    restart_game, exit_game 
} menu_res;

menu_res play_game_completed_menu(term_state *ts, player_state *ps);
menu_res play_game_over_menu(term_state *ts, player_state *ps);

#endif
