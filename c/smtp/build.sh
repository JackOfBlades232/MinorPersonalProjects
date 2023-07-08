#!/bin/sh

DEFINES=""
#DEFINES="-D DEBUG"
#DEFINES="-D DEBUG -D TEST"

gcc -Wall -g $DEFINES server.c -o server 
