/* shootemup_v2/utils.h */
#ifndef UTILS_SENTRY
#define UTILS_SENTRY

void clamp_int(int *val, int min, int max);

void init_random();
int randint(int min, int max);

int char_is_symbol(int c);

#endif
