player.o: player.c player.h geom.h graphics.h utils.h
asteroid.o: asteroid.c asteroid.h geom.h utils.h
collisions.o: collisions.c collisions.h player.h geom.h graphics.h \
 asteroid.h
controls.o: controls.c controls.h
graphics.o: graphics.c graphics.h
geom.o: geom.c geom.h
utils.o: utils.c utils.h
