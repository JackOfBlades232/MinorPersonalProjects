#!/bin/sh

#DEFINES=""
DEFINES="-D DEBUG"

gcc -Wall -c $DEFINES database.c
gcc -Wall -c $DEFINES protocol.c
gcc -Wall -c $DEFINES utils.c
gcc -Wall -c $DEFINES debug.c

gcc -Wall -g $DEFINES client.c protocol.o utils.o debug.o -o client 
gcc -Wall -g $DEFINES server.c database.o protocol.o utils.o debug.o -o server 
gcc -Wall -g $DEFINES test.c database.o protocol.o utils.o debug.o -o test 
