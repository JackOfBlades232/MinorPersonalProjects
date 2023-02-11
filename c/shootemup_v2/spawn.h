/* shootemup_v2/spawn.h */
#ifndef SPAWN_SENTRY
#define SPAWN_SENTRY

#include "geom.h"

typedef struct tag_spawn_area {
    int area_length;
    int *live_objects, *space_to_the_right_map;
} spawn_area;

void init_spawn_area(spawn_area *sa, int length);
void deinit_spawn_area(spawn_area *sa);
int try_spawn_object(spawn_area *sa, int object_width);
point position_of_area_point(spawn_area *sa, int area_idx);

#endif
