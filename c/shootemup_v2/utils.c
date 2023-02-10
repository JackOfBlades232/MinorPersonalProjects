/* shootemup_v2/utils.h */
#include "utils.h"

void clamp_int(int *val, int min, int max)
{
    if (*val < min)
        *val = min;
    else if (*val > max)
        *val = max;
}

int char_is_symbol(int c)
{
    return c > 32 && c < 127;
}
