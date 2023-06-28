#!/bin/sh

#DEFINES=""
DEFINES="-D DEBUG"

gcc -Wall -g $DEFINES server.c -o server 
