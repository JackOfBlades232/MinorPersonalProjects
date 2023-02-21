/* shootemup_v2/main.c */
#include "geom.h"
#include "graphics.h"
#include "controls.h"
#include "player.h"
#include "boss.h"
#include "boss_ai.h"
#include "asteroid.h"
#include "buffs.h"
#include "spawn.h"
#include "explosion.h"
#include "collisions.h"
#include "hud.h"
#include "menus.h"
#include "colors.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum { 
    score_for_stage_switch = 1,
    stage_switch_delay = 1 /* seconds */
};

typedef enum tag_game_stage { asteroid_field = 0, boss_fight = 1 } game_stage;
typedef enum tag_game_result { win = 0, lose = 1, shutdown = 2 } game_result;

static int init_game(term_state *ts)
{
    int res;

    res = init_graphics(ts);
    if (!res)
        return 1;

    init_color_pairs();

    init_random();
    init_controls();

    init_asteroid_static_data();

    return 0;
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

static int reached_stage_switch(player *p, game_stage cur_stage)
{
    return cur_stage == asteroid_field && 
        p->state.score >= score_for_stage_switch;
}

static void update_ast_field_moving_entities(player_bullet_buf player_bullets, 
        asteroid_buf asteroids, crate_buf crates,
        spawn_area *spawn, player *p, term_state *ts)
{
    update_live_bullets(player_bullets);
    update_live_asteroids(asteroids, spawn, ts, p);
    update_live_crates(crates, spawn, ts);
}

static void process_ast_field_collisions(player *p, 
        player_bullet_buf player_bullets, asteroid_buf asteroids, 
        crate_buf crates, spawn_area *spawn)
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

static void process_asteroid_field_frame(player_bullet_buf player_bullets, 
        asteroid_buf asteroids, crate_buf crates, spawn_area *spawn, 
        countdown_timer *spawn_timer, player *p, term_state *ts)
{
    update_ast_field_moving_entities(player_bullets,
            asteroids, crates, spawn, p, ts);
    process_ast_field_collisions(p, player_bullets, asteroids, crates, spawn);

    update_ctimers(1, spawn_timer, NULL);
    process_spawns(asteroids, crates, spawn, spawn_timer);
}

static void switch_game_stage(boss *bs, boss_behaviour *boss_beh,
        boss_projectile_buf boss_projectiles,
        explosion_buf explosions, player_bullet_buf player_bullets, 
        asteroid_buf asteroids, crate_buf crates, spawn_area *spawn,
        player *p, term_state *ts)
{
    kill_all_player_bullets(player_bullets);
    kill_all_asteroids(asteroids, spawn);
    kill_all_cratres(crates, spawn);

    hide_player(p);
    reset_player_pos(p, ts);

    refresh_scr();

    sleep(stage_switch_delay);
    get_input_action(); /* clear buf */

    /* init_boss(bs, 250, 1, 2, 3, ts); */
    init_boss(bs, 250, 0, 0, 0, ts);
    init_boss_ai(boss_beh);
    init_boss_projectile_buf(boss_projectiles);
    init_explosion_buf(explosions);
}

static void update_boss_fight_moving_entities(
        boss_projectile_buf boss_projectiles, explosion_buf explosions, 
        player_bullet_buf player_bullets, term_state *ts)
{
    update_live_bullets(player_bullets);
    update_live_boss_projectiles(boss_projectiles, explosions, ts);
    update_live_explosions(explosions, ts);
}

static void process_boss_fight_collisions(boss *bs, 
        boss_projectile_buf boss_projectiles, explosion_buf explosions, 
        player_bullet_buf player_bullets, player *p, term_state *ts)
{
    process_bullet_to_boss_collisions(player_bullets, bs);
    process_player_to_boss_collisions(p, bs);
    process_projectile_to_player_collisions(
            boss_projectiles, p, explosions, ts);
    process_explosion_to_player_collisions(explosions, p, ts);
}

static void process_boss_fight_frame(boss *bs, 
        boss_projectile_buf boss_projectiles, explosion_buf explosions, 
        player_bullet_buf player_bullets, player *p, term_state *ts)
{
    update_boss_fight_moving_entities(boss_projectiles,
            explosions, player_bullets, ts);
    process_boss_fight_collisions(bs, boss_projectiles, 
            explosions, player_bullets, p, ts);

    update_boss_frame_counters(bs);
}

static int player_has_won(boss *bs, game_stage g_stage)
{
    return g_stage == boss_fight && boss_is_dead(bs);
}

static game_result game_loop(player *p, term_state *ts, 
        player_bullet_buf player_bullets, asteroid_buf asteroids, 
        crate_buf crates, spawn_area *spawn, countdown_timer *spawn_timer,
        boss *bs, boss_behaviour *boss_beh,
        boss_projectile_buf boss_projectiles, explosion_buf explosions)
{
    game_stage g_stage = asteroid_field;
    game_result g_res = shutdown;
    input_action action;
    
    while ((action = get_input_action()) != quit) {
        switch (g_stage) {
            case asteroid_field:
                process_asteroid_field_frame(player_bullets, asteroids, 
                        crates, spawn, spawn_timer, p, ts);
                break;
            case boss_fight:
                tick_boss_ai(boss_beh, bs, p, boss_projectiles, explosions, ts);
                process_boss_fight_frame(bs, boss_projectiles, explosions,
                        player_bullets, p, ts);
                break;
        }

        if (player_is_dead(p)) {
            g_res = lose;
            break;
        } else if (player_has_won(bs, g_stage)) {
            g_res = win;
            break;
        } else if (reached_stage_switch(p, g_stage)) {
            g_stage = boss_fight;
            switch_game_stage(bs, boss_beh, boss_projectiles, explosions,
                    player_bullets, asteroids, crates, spawn, p, ts);

            continue;
        }

        update_player_frame_counters(p);

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

            case fire1: /* boss debug */
                perform_bullet_burst(boss_beh);
                break;
            case fire2:
                perform_battering_ram(boss_beh);
                break;
            case fire3:
                perform_mine_plant(boss_beh);
                break;
            case fire4:
                perform_force_blast(boss_beh);
                break;
            case fire5:
                perform_sliding_volley(boss_beh);
                break; /* boss debug end */

            case resize:
                goto end_loop;
            default:
                break;
        }

        handle_player_ammo_replenish(p);

        switch (g_stage) {
            case asteroid_field:
                draw_player_hud_plack(&p->state, ts);
                break;
            case boss_fight:
                draw_boss_fight_hud_plack(&p->state, &bs->state, ts);
                break;
        }

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
    int status;
    term_state t_state;

    player p;
    player_bullet_buf player_bullets;

    asteroid_buf asteroids;
    crate_buf crates;
    spawn_area spawn;
    countdown_timer spawn_timer;

    boss bs;
    boss_behaviour boss_beh;
    boss_projectile_buf boss_projectiles;
    explosion_buf explosions;

    game_result g_res;
    menu_res end_menu_res;

    FILE *log;

    status = init_game(&t_state);
    if (status != 0)
        return status;

    log = fopen("debug.log", "w");

start_game:
    fprintf(log, "Started game\n");

    reset_game_state(&p, &t_state, player_bullets, 
            asteroids, crates, &spawn, &spawn_timer);

    g_res = game_loop(&p, &t_state, player_bullets, asteroids, crates, 
                      &spawn, &spawn_timer, &bs, &boss_beh, boss_projectiles,
                      explosions);

    fprintf(log, "G-res: %d\n", g_res);

    switch (g_res) {
        case win:
            end_menu_res = play_game_completed_menu(&t_state, &p.state);
            break;
        case lose:
            end_menu_res = play_game_over_menu(&t_state, &p.state);
            break;
        default:
            end_menu_res = exit_game;
            break;
    }

    if (end_menu_res == restart_game)
        goto start_game;

    deinit_game();

    fclose(log);
    
    return 0;
}

int main()
{
    return run_game();
}
