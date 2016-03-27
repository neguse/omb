#!/bin/sh

export REDIS_HOME=$HOME/src/github.com/antirez/redis

gcc $REDIS_HOME/src/anet.c -c -o anet.o
gcc $REDIS_HOME/src/ae.c -c -o ae.o
gcc $REDIS_HOME/src/zmalloc.c -c -o zmalloc.o

g++ src/server.cc anet.o ae.o zmalloc.o -I $REDIS_HOME/src
