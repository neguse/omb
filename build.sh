#!/bin/sh -ex

clang-format -i src/main.cc

export REDIS_HOME=$HOME/src/github.com/antirez/redis

gcc $REDIS_HOME/src/anet.c -c -o anet.o
gcc $REDIS_HOME/src/ae.c -c -o ae.o
gcc $REDIS_HOME/src/zmalloc.c -c -o zmalloc.o

g++ -o main.out src/main.cc `sdl2-config --cflags --static-libs` anet.o ae.o zmalloc.o -I $REDIS_HOME/src -DUSE_SDL=1
g++ -o main_headless.out src/main.cc anet.o ae.o zmalloc.o -I $REDIS_HOME/src 

