/* shootemup_v2/utils.h */
#ifndef UTILS_SENTRY
#define UTILS_SENTRY

typedef struct tag_countdown_timer {
    int ticks_left;
    int max_ticks;
} countdown_timer;

void clamp_int(int *val, int min, int max);

void init_random();
int randint(int min, int max);

int char_is_symbol(int c);

void init_ctimer(countdown_timer *ct, int max_t_val);
void reset_ctimer(countdown_timer *ct);

/* takes countdown_timer*, last param must be NULL */
void update_ctimers(int d_ticks, ...);

#endif
