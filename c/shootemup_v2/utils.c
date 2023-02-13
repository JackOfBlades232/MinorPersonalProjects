/* shootemup_v2/utils.h */
#include "utils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

int min_int(int n1, int n2)
{
    return n1 < n2 ? n1 : n2;
}

void clamp_int(int *val, int min, int max)
{
    if (*val < min)
        *val = min;
    else if (*val > max)
        *val = max;
}

void init_random()
{
    srand(time(NULL));
}

int randint(int min, int max)
{
    return min + (int)((double)max * rand() / (RAND_MAX+1.0));
}

int char_is_symbol(int c)
{
    return c > 32 && c < 127;
}

int char_is_ascii(int c)
{
    return c >= 0 && c < 128;
}

void init_ctimer(countdown_timer *ct, int max_t_val)
{
    ct->max_ticks = max_t_val;
}

void reset_ctimer(countdown_timer *ct)
{
    ct->ticks_left = ct->max_ticks;
}

void update_ctimers(int d_ticks, ...)
{
    va_list vl;
    countdown_timer *ct;

    va_start(vl, d_ticks);
    while ((ct = va_arg(vl, countdown_timer *)) != NULL)
        ct->ticks_left -= d_ticks;

    va_end(vl);
}
