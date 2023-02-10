/* shootemup_v2/main.c */
#include "graphics.h"
#include "player.h"
#include "controls.h"

#include <curses.h>

static void init_game(player *p, term_state *ts, 
        player_bullet_buf player_bullets)
{
    init_graphics(ts);
    init_controls();
    init_player(p, 5, ts);
    init_player_bullet_buf(player_bullets);
}

static void game_loop(player *p, term_state *ts,
        player_bullet_buf player_bullets)
{
    input_action action;
    
    while ((action = get_input_action()) != quit) {
        update_live_bullets(player_bullets);

        switch (action) {
            case up:
                move_player(p, 0, -1, ts);
                break;
            case down:
                move_player(p, 0, 1, ts);
                break;
            case left:
                move_player(p, -1, 0, ts);
                break;
            case right:
                move_player(p, 1, 0, ts);
                break;
            case fire:
                player_shoot(p, player_bullets);
                break;
            case resize:
                goto end_loop;
            default:
                break;
        }

        refresh_scr();
    }

end_loop:
    hide_player(p);
}

static void deinit_game()
{
    deinit_graphics();
}

static int run_game()
{
    term_state t_state;
    player p;
    player_bullet_buf player_bullets;

    init_game(&p, &t_state, player_bullets);
    game_loop(&p, &t_state, player_bullets);
    deinit_game();

    return 0;
}

int main()
{
    return run_game();
}
