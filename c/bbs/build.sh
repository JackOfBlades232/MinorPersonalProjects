#!/bin/sh

#gcc -Wall -c client_src/*.c server_src/*.c
#gcc -Wall -g client.c client_src/*.o -o client 
#gcc -Wall -g server.c server_src/*.o -o server 

gcc -Wall -c protocol.c

gcc -Wall -g client.c protocol.o -o client 
gcc -Wall -g server.c protocol.o -o server 
