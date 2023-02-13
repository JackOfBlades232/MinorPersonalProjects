/* shootemup_v2/menus.h */
#ifndef MENUS_SENTRY
#define MENUS_SENTRY

#include "graphics.h"
#include "player.h"

typedef enum tag_game_over_menu_res { 
    restart_game, exit_game 
} game_over_menu_res;

game_over_menu_res play_game_over_menu(term_state *ts, player_state *ps);

#endif
