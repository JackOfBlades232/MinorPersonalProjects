# Tetriller
This program is a reproduction of a game called tetriller to run in a terminal window.
The goal of the game is to build up tetris blocks so that a human can climb up to a certain height. The human moves independently, can crouch, fall down, walk laterally
and clib one-block-high edges. If the human is squashed by a block, the game is over.
Minimum window size to run the game is 32 by 32 symbols.

## State
The game has no menu. There are also a few minor bugs, which do not drastically change the experience. However, I intend to remake the game in C, correcting the
bugs and adding a menu

## Controls
WASD for block movement, space for rotation, escape for exit.

## Running
(for linux) In order to run this program, you have to compile it with the fpc compiler (`fpc tetriller.pas`), and run the generated executable file.
