/* shootemup_v2/controls.h */
#ifndef CONTROLS_SENTRY
#define CONTROLS_SENTRY

typedef enum tag_input_action {
    idle, up, down, left, right, fire, resize, quit,

    fire1, fire2, fire3, fire4, fire5 /* boss debug */
} input_action;

void init_controls();
void reset_controls_timeout();
input_action get_input_action();

#endif
