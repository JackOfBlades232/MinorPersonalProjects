/* shootemup_v2/controls.h */
#ifndef CONTROLS_SENTRY
#define CONTROLS_SENTRY

typedef enum tag_input_action {
    idle, up, down, left, right, fire, 
    fire_up, fire_down, fire_left, fire_right,
    resize, quit
} input_action;

void init_controls();
void reset_controls_timeout();
input_action get_input_action();
void flush_input();

#endif
