/* shootemup_v2/main.c */
#include "graphics.h"
#include "controls.h"
#include "player.h"
#include "asteroid.h"
#include "buffs.h"
#include "spawn.h"
#include "collisions.h"
#include "hud.h"
#include "menus.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum tag_game_result { win, lose, shutdown } game_result;

static void init_game(term_state *ts)
{
    init_random();
    init_graphics(ts);
    init_controls();

    init_asteroid_static_data();
}

static void reset_game_state(player *p, term_state *ts, 
        player_bullet_buf player_bullets, asteroid_buf asteroids, 
        crate_buf crates, spawn_area *spawn, countdown_timer *spawn_timer)
{
    reset_controls_timeout();

    init_player(p, 5, 100, 1, ts);
    init_player_bullet_buf(player_bullets);
    init_asteroid_buf(asteroids);
    init_crate_buf(crates);
    init_spawn_area(spawn, ts->col - 2*spawn_area_horizontal_offset);
    init_ctimer(spawn_timer, 25);

    reset_ctimer(spawn_timer);
}

static void update_moving_entities(player_bullet_buf player_bullets, 
        asteroid_buf asteroids, crate_buf crates,
        spawn_area *spawn, player *p, term_state *ts)
{
    update_live_bullets(player_bullets);
    update_live_asteroids(asteroids, spawn, ts, p);
    update_live_crates(crates, spawn, ts);
}

static void process_collisions(player *p, player_bullet_buf player_bullets,
        asteroid_buf asteroids, crate_buf crates, spawn_area *spawn)
{
    process_bullet_to_asteroid_collisions(player_bullets, asteroids, spawn, p);
    process_bullet_to_crate_collisions(player_bullets, crates, spawn);
    process_asteroid_to_player_collisions(p, asteroids, spawn);
    process_crate_to_player_collisions(p, crates, spawn);
}

/* TODO : refactor */
static void process_spawns(asteroid_buf asteroids, crate_buf crates,
        spawn_area *spawn, countdown_timer *spawn_timer)
{
    int idx;
    asteroid *as;
    buff_crate *crate;

    if (spawn_timer->ticks_left > 0)
        return;

    if (randint(0, 18)) {
        as = get_queued_asteroid(asteroids);
        idx = try_spawn_object(spawn, as->data->width);

        if (idx >= 0) {
            lock_area_for_object(spawn, idx, as->data->width);
            spawn_asteroid(as, position_of_area_point(spawn, idx), idx);
        }
    } else {
        crate = get_queued_crate(crates);
        idx = try_spawn_object(spawn, crate->data->width);

        if (idx >= 0) {
            lock_area_for_object(spawn, idx, crate->data->width);
            spawn_crate(crate, position_of_area_point(spawn, idx), idx);
        }
    }

    if (idx >= 0)
        reset_ctimer(spawn_timer);
}

static game_result game_loop(player *p, term_state *ts, 
        player_bullet_buf player_bullets, asteroid_buf asteroids, 
        crate_buf crates, spawn_area *spawn, countdown_timer *spawn_timer)
{
    game_result g_res = shutdown;
    input_action action;
    
    while ((action = get_input_action()) != quit) {
        update_ctimers(1, spawn_timer, NULL);
        update_player_frame_counters(p);

        update_moving_entities(player_bullets, asteroids, crates, spawn, p, ts);
        process_collisions(p, player_bullets, asteroids, crates, spawn);

        if (player_is_dead(p)) {
            g_res = lose;
            break;
        }

        process_spawns(asteroids, crates, spawn, spawn_timer);

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

        handle_player_ammo_replenish(p);

        draw_hud_plack(&p->state);
        refresh_scr();
    }

end_loop:
    hide_player(p);
    return g_res;
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
    crate_buf crates;
    countdown_timer spawn_timer;
    game_result g_res;
    game_over_menu_res go_menu_res;

    FILE *log;
    log = fopen("debug.log", "w");

    init_game(&t_state);

start_game:
    fprintf(log, "Started game\n");

    reset_game_state(&p, &t_state, player_bullets, 
            asteroids, crates, &spawn, &spawn_timer);

    g_res = game_loop(&p, &t_state, player_bullets, 
                asteroids, crates, &spawn, &spawn_timer);

    switch (g_res) {
        case win:
            break;
        case lose:
            go_menu_res = play_game_over_menu(&t_state, &p.state);
            if (go_menu_res == restart_game)
                goto start_game;
            break;
        default:
            break;
    }

    deinit_game();

    fclose(log);
    
    return 0;
}

int main()
{
    return run_game();
}
