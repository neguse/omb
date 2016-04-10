#!/bin/sh -ex

clang-format -i src/main.cc
clang-format -i src/server.cc

g++ -o main.out src/main.cc `sdl2-config --cflags --static-libs`

export REDIS_HOME=$HOME/src/github.com/antirez/redis

gcc $REDIS_HOME/src/anet.c -c -o anet.o
gcc $REDIS_HOME/src/ae.c -c -o ae.o
gcc $REDIS_HOME/src/zmalloc.c -c -o zmalloc.o

g++ -o server.out src/server.cc anet.o ae.o zmalloc.o -I $REDIS_HOME/src
