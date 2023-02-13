player.o: player.c player.h geom.h graphics.h utils.h
asteroid.o: asteroid.c asteroid.h geom.h graphics.h player.h spawn.h \
 utils.h
collisions.o: collisions.c collisions.h player.h geom.h graphics.h \
 asteroid.h spawn.h buffs.h
buffs.o: buffs.c buffs.h geom.h spawn.h player.h graphics.h utils.h
spawn.o: spawn.c spawn.h geom.h utils.h
controls.o: controls.c controls.h
graphics.o: graphics.c graphics.h
geom.o: geom.c geom.h
hud.o: hud.c hud.h player.h geom.h graphics.h
menus.o: menus.c menus.h graphics.h player.h geom.h utils.h
utils.o: utils.c utils.h
