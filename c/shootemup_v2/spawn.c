/* shootemup_v2/spawn.c */
#include "spawn.h"
#include "geom.h"
#include "utils.h"

#include <stdlib.h>

void init_spawn_area(spawn_area *sa, int length)
{
    sa->area_length = length;
    sa->live_objects = calloc(sa->area_length, sizeof(int));
    sa->space_to_the_right_map = malloc(sa->area_length * sizeof(int));
}

void deinit_spawn_area(spawn_area *sa)
{
    free(sa->live_objects);
}

int try_spawn_object(spawn_area *sa, int object_width)
{
    int i;
    int num_eligible_spawn_points, chosen_idx;

    num_eligible_spawn_points = 0;
    for (i = sa->area_length-1; i >= 0; i--) {
        if (sa->live_objects[i] > 0)
            sa->space_to_the_right_map[i] = 0;
        else {
            sa->space_to_the_right_map[i] = i == sa->area_length-1 ? 1 : 
                sa->space_to_the_right_map[i+1] + 1;
            if (sa->space_to_the_right_map[i] >= object_width)
                num_eligible_spawn_points++;
        }
    }

    if (num_eligible_spawn_points == 0)
        return -1;

    chosen_idx = randint(0, num_eligible_spawn_points);
    for (i = 0; i < sa->area_length; i++) {
        if (sa->space_to_the_right_map[i] >= object_width) {
            if (chosen_idx > 0)
                chosen_idx--;
            else
                return i;
        }
    }

    return -1;
}
            
point position_of_area_point(spawn_area *sa, int area_idx)
{
    return point_literal(
            spawn_area_horizontal_offset + area_idx,
            spawn_area_vertical_offset
            );
}
