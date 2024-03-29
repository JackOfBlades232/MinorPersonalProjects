/* shootemup_v2/graphics.h */
#ifndef GRAPHICS_SENTRY
#define GRAPHICS_SENTRY

#include "geom.h"

enum { min_term_width = 80, min_term_height = 24 };

typedef struct tag_term_state {
    int row, col;
} term_state;

int init_graphics(term_state *ts);
void deinit_graphics();

void refresh_scr();

int point_is_in_bounds(point p, term_state *ts);
void draw_point(point p, char symbol, term_state *ts);
void draw_local_point(int loc_y, int loc_x, point base, 
        char symbol, term_state *ts);
int obj_is_in_bounds(point pos, int width, int height, term_state *ts);

#endif
