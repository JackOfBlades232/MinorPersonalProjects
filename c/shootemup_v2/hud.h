/* shootemup_v2/hud.h */
#ifndef HUD_SENTRY
#define HUD_SENTRY

#include "player.h"
#include "boss.h"
#include "graphics.h"

void draw_player_hud_plack(player_state *ps, term_state *ts);
void draw_boss_fight_hud_plack(player_state *ps, boss_state *bss,
        term_state *ts);

#endif
