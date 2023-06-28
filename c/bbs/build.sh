#!/bin/sh

#DEFINES=""
DEFINES="-D DEBUG"

gcc -Wall -g -c $DEFINES types.c
gcc -Wall -g -c $DEFINES database.c
gcc -Wall -g -c $DEFINES protocol.c
gcc -Wall -g -c $DEFINES utils.c
gcc -Wall -g -c $DEFINES debug.c

gcc -Wall -g $DEFINES client.c ./*.o -o client 
gcc -Wall -g $DEFINES server.c ./*.o -o server 
#gcc -Wall -g $DEFINES test.c ./*.o -o test 
