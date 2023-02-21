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

typedef enum tag_game_stage { 
    asteroid_field = 0,
    boss_fight = 1,
    boss_expl = 2
} game_stage;

typedef enum tag_game_result { win = 0, lose = 1, shutdown = 2 } game_result;

/* settings */
enum {
    player_max_hp = 5,
    player_max_ammo = 100,
    player_bullet_damage = 1
};

enum {
    spawn_frame_delay = 25,
    crate_spawn_inv_chance = 19
};

enum { 
    score_for_stage_switch = 300,
    stage_switch_delay = 1 /* seconds */
};

enum {
    boss_max_hp = 250,
    boss_bullet_damage = 1,
    boss_gunshot_damage = 2,
    boss_mine_damage = 3,
    boss_ffield_damage = 4
};

enum {
    boss_expl_blink_frames = 300,
    boss_expl_final_frames = 150
};

#define BOSS_EXPL_RAD 20.0

/* one-time initialization */
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

/* game initialization (at the start of every try) */
static void reset_game_state(player *p, term_state *ts, 
        player_bullet_buf player_bullets, asteroid_buf asteroids, 
        crate_buf crates, spawn_area *spawn, countdown_timer *spawn_timer)
{
    reset_controls_timeout();

    init_player(p, player_max_hp, player_max_ammo, player_bullet_damage, ts);
    init_player_bullet_buf(player_bullets);
    init_asteroid_buf(asteroids);
    init_crate_buf(crates);
    init_spawn_area(spawn, ts->col - 2*spawn_area_horizontal_offset);
    init_ctimer(spawn_timer, spawn_frame_delay);

    reset_ctimer(spawn_timer);
}

/* =reached switch from ast field to boss */
static int reached_stage_switch(player *p, game_stage cur_stage)
{
    return cur_stage == asteroid_field && 
        p->state.score >= score_for_stage_switch;
}

/* asteroid field */
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

    if (randint(0, crate_spawn_inv_chance)) {
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

/* boss fight initialization */
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
    flush_input();

    init_boss(bs, boss_max_hp, boss_bullet_damage, boss_gunshot_damage,
            boss_mine_damage, boss_ffield_damage, ts);
    init_boss_ai(boss_beh);
    init_boss_projectile_buf(boss_projectiles);
    init_explosion_buf(explosions);
}

/* boss fight */
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

/* boss explosion initialization */
static void start_boss_expl(boss *bs, countdown_timer *boss_expl_timer,
        boss_projectile_buf boss_projectiles, explosion_buf explosions, 
        player_bullet_buf player_bullets, term_state *ts)
{
    kill_all_player_bullets(player_bullets);
    kill_all_boss_pojectiles(boss_projectiles);
    kill_all_explosions(explosions, ts);

    show_boss_with_blink(bs);
    bs->state.cur_hp = 1;

    refresh_scr();
    
    init_ctimer(boss_expl_timer, boss_expl_blink_frames);
    reset_ctimer(boss_expl_timer);
}

/* boss explosion */
static void process_boss_expl_frame(boss *bs, countdown_timer *boss_expl_timer,
        explosion_buf explosions, player *p, term_state *ts)
{
    update_ctimers(1, boss_expl_timer, NULL);
    update_live_explosions(explosions, ts);

    if (boss_expl_timer->ticks_left <= 0 && bs->state.cur_hp > 0) {
        init_ctimer(boss_expl_timer, boss_expl_final_frames);
        reset_ctimer(boss_expl_timer);

        bs->state.cur_hp = 0;
        hide_boss(bs);

        spawn_explosion(
                explosions, 
                point_literal(
                    bs->pos.x + boss_width/2,
                    bs->pos.y + boss_height/2
                    ), 
                BOSS_EXPL_RAD, 0, get_color_pair(expl_color_pair), ts
                );
    }

    show_player(p);
}

static int boss_expl_ended(boss *bs, countdown_timer *boss_expl_timer)
{
    return boss_expl_timer->ticks_left <= 0 && bs->state.cur_hp <= 0;
}

/* core loop */
static game_result game_loop(player *p, term_state *ts, 
        player_bullet_buf player_bullets, asteroid_buf asteroids, 
        crate_buf crates, spawn_area *spawn, countdown_timer *spawn_timer,
        boss *bs, boss_behaviour *boss_beh,
        boss_projectile_buf boss_projectiles, explosion_buf explosions,
        countdown_timer *boss_expl_timer)
{
    game_stage g_stage = asteroid_field;
    game_result g_res = shutdown;
    input_action action;
    
    while ((action = get_input_action()) != quit) {
        /* non-player updates, dependent on stage */
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
            case boss_expl:
                process_boss_expl_frame(bs, boss_expl_timer, explosions, p, ts);
                break;
        }

        /* stage switch logic */
        if (player_is_dead(p)) {
            g_res = lose;
            break;
        } else if (reached_stage_switch(p, g_stage)) {
            g_stage = boss_fight;
            switch_game_stage(bs, boss_beh, boss_projectiles, explosions,
                    player_bullets, asteroids, crates, spawn, p, ts);

            continue;
        } else if (player_has_won(bs, g_stage)) {
            g_stage = boss_expl;
            g_res = win;
            start_boss_expl(bs, boss_expl_timer, boss_projectiles, 
                    explosions, player_bullets, ts);
            continue;
        } else if (g_stage == boss_expl && boss_expl_ended(bs, boss_expl_timer))
            break;

        /* if last stage, not player actions */
        if (g_stage == boss_expl)
            goto draw_ui;

        /* player updates */
        update_player_frame_counters(p);

        /* input processing */
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
            case fire_up:
                move_player(p, 0, -1, ts);
                player_shoot(p, player_bullets);
                break;
            case fire_down:
                move_player(p, 0, 1, ts);
                player_shoot(p, player_bullets);
                break;
            case fire_left:
                move_player(p, -1, 0, ts);
                player_shoot(p, player_bullets);
                break;
            case fire_right:
                move_player(p, 1, 0, ts);
                player_shoot(p, player_bullets);
                break;
            case resize:
                goto end_loop;
            default:
                break;
        }

        /* some more player updates */
        handle_player_ammo_replenish(p);

draw_ui: /* hud plack drawing, stage dependent */
        switch (g_stage) {
            case asteroid_field:
                draw_player_hud_plack(&p->state, ts);
                break;
            case boss_fight:
                draw_boss_fight_hud_plack(&p->state, &bs->state, ts);
                break;
            case boss_expl:
                draw_boss_fight_hud_plack(&p->state, &bs->state, ts);
                break;
        }

        /* finally, flush all updates to screen */
        refresh_scr();
    }

end_loop:
    hide_player(p);

    return g_res;
}

/* final method before shutdown (light cause no heap allocations) */
static void deinit_game()
{
    deinit_graphics();
}

/* main game runner: init->loop->menu, restart?->loop or deinit */
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

    countdown_timer boss_expl_timer;

    game_result g_res;
    menu_res end_menu_res;

    FILE *log;

    /* global init */
    status = init_game(&t_state);
    if (status != 0)
        return status;

    log = fopen("debug.log", "w");

start_game: /* once-per-try init */
    fprintf(log, "Started game\n");

    reset_game_state(&p, &t_state, player_bullets, 
            asteroids, crates, &spawn, &spawn_timer);

    /* run core loop and get it's result */
    g_res = game_loop(&p, &t_state, player_bullets, asteroids, crates, 
                      &spawn, &spawn_timer, &bs, &boss_beh, boss_projectiles,
                      explosions, &boss_expl_timer);

    fprintf(log, "G-res: %d\n", g_res);

    /* show different ui depending on game result */
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

    /* global deinit */
    deinit_game();

    fclose(log);
    
    return 0;
}

int main()
{
    return run_game();
}
