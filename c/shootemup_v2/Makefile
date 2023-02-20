SRCMODULES = player.c asteroid.c buffs.c boss.c boss_ai.c spawn.c \
			 collisions.c colors.c explosion.c controls.c geom.c \
			 graphics.c hud.c menus.c utils.c
OBJMODULES = $(SRCMODULES:.c=.o)
CC = gcc
CFLAGS = -g -Wall
LFLAGS = -lm -lncurses

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

prog: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRCMODULES)
	$(CC) -MM $^ > $@

clean:
	rm -f *.o prog
