#!/bin/sh

gcc src/main.c -o out/app -Wall -Wextra -g -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
