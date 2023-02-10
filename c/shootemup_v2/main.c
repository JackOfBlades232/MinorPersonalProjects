/* shootemup_v2/main.c */
#include "shape.h"
#include "graphics.h"
#include "controls.h"

#include <stdio.h>
#include <unistd.h>

int controls_test(term_state *ts)
{
    shape *sh;
    input_action action;

    sh = create_shape(1);
    sh->pos = point_literal(20, 20);
    sh->fill_sym = '*';

    truncate_shape_pos(sh, ts);
    show_shape(sh);

    while ((action = get_player_action()) != exit) {
        switch (action) {
            case up:
                move_shape(sh, 0, -1, ts);
                break;
            case down:
                move_shape(sh, 0, 1, ts);
                break;
            case left:
                move_shape(sh, -1, 0, ts);
                break;
            case right:
                move_shape(sh, 1, 0, ts);
                break;
            case fire:
                hide_shape(sh);
                sh->fill_sym++;
                show_shape(sh);
                break;
            case resize:
                refresh_term_state(ts);
                redraw_shape(sh, ts);
                break;
            default:
                break;
        }

        refresh_scr();
    }

    destroy_shape(sh);

    return 0;
}

int run_game()
{
    int status;
    term_state t_state;

    status = init_graphics(&t_state);
    if (!status)
        return 1;

    status = controls_test(&t_state);

    deinit_graphics();

    return status;
}

int main()
{
    int status;

    status = run_game();
    printf("Ran game with status %d\n", status);

    return status;
}
