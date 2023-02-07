/* shootemup_v2/controls.h */
#ifndef CONTROLS_SENTRY
#define CONTROLS_SENTRY

typedef enum tag_input_action { 
    idle, up, down, left, right, fire, exit, resize
} input_action;

void init_controls();
input_action get_player_action();

#endif
