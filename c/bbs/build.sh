#!/bin/sh

#gcc -Wall -c client_src/*.c server_src/*.c
#gcc -Wall -g client.c client_src/*.o -o client 
#gcc -Wall -g server.c server_src/*.o -o server 

gcc -Wall -c database.c
gcc -Wall -c protocol.c
gcc -Wall -c utils.c

#gcc -Wall -g client.c protocol.o utils.o -o client 
#gcc -Wall -g server.c database.o protocol.o utils.o -o server 
gcc -Wall -g test.c database.o protocol.o utils.o -o test 
