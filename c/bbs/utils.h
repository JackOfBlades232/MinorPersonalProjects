/* bbs/utils.h */

#define return_defer(code) do { result = code; goto defer; } while (0)
