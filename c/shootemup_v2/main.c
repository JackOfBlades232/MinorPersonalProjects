/* shootemup_v2/main.c */
#include "graphics.h"
#include "player.h"
#include "controls.h"

static void init_game(player *p, term_state *ts)
{
    init_graphics(ts);
    init_controls();
    init_player(p, 5, ts);
}

static void game_loop(player *p, term_state *ts)
{
    input_action action = idle;
    
    while (action != exit) {
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
                move_player(p, 0, -10, ts);
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

    init_game(&p, &t_state);
    game_loop(&p, &t_state);
    deinit_game();

    return 0;
}

int main()
{
    return run_game();
}
