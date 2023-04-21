#!/bin/sh

#gcc -Wall -c client_src/*.c server_src/*.c
#gcc -Wall -g client.c client_src/*.o -o client 
#gcc -Wall -g server.c server_src/*.o -o server 

gcc -Wall -g client.c -o client 
gcc -Wall -g server.c -o server 
