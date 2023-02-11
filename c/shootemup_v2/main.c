/* shootemup_v2/main.c */
#include "graphics.h"
#include "player.h"
#include "controls.h"
#include "asteroid.h"
#include "spawn.h"
#include "collisions.h"
#include "utils.h"

#include <curses.h>

static void init_game(player *p, term_state *ts, 
        player_bullet_buf player_bullets, 
        asteroid_buf asteroids, spawn_area *spawn)
{
    init_random();
    init_graphics(ts);
    init_controls();
    init_player(p, 5, ts);
    init_player_bullet_buf(player_bullets);
    init_asteroid_buf(asteroids);
    init_spawn_area(spawn, ts->col - 2*spawn_area_horizontal_offset);
}

static void update_moving_entities(player_bullet_buf player_bullets, 
        asteroid_buf asteroids)
{
    update_live_bullets(player_bullets);
    update_live_asteroids(asteroids);
}

static void process_collisions(player *p, 
        player_bullet_buf player_bullets, asteroid_buf asteroids)
{
    process_bullet_to_asteroid_collisions(player_bullets, asteroids);
    process_asteroid_to_player_collisions(p, asteroids);
}

/* test */
static void process_spawns(term_state *ts, asteroid_buf asteroids, 
        spawn_area *spawn)
{
    int idx;
    idx = try_spawn_object()
    

static void game_loop(player *p, term_state *ts,
        player_bullet_buf player_bullets, 
        asteroid_buf asteroids, spawn_area *spawn)
{
    input_action action;
    
    while ((action = get_input_action()) != quit) {
        update_moving_entities(player_bullets, asteroids);
        process_collisions(p, player_bullets, asteroids);

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
                spawn_asteroid(asteroids);
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
    asteroid_buf asteroids;
    spawn_area spawn;

    init_game(&p, &t_state, player_bullets, asteroids, &spawn);
    game_loop(&p, &t_state, player_bullets, asteroids, &spawn);
    deinit_game();

    return 0;
}

int main()
{
    return run_game();
}
