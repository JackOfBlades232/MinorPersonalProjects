/* shootemup_v2/graphics.h */
#ifndef GRAPHICS_SENTRY
#define GRAPHICS_SENTRY

typedef struct tag_point {
    int x, y;
} point;

typedef struct tag_term_state {
    int row, col;
} term_state;

point point_literal(int x, int y);
point add_points(point p1, point p2);
int points_are_equal(point p1, point p2);

int init_graphics(term_state *ts);
void deinit_graphics();
void draw_pix(point p, char sym);
void refresh_scr();

#endif
