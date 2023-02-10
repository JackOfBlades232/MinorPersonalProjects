/* shootemup_v2/utils.h */
#include "utils.h"

#include <stdlib.h>
#include <time.h>

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
