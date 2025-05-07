#!/usr/bin/sh
set -e

CFLAGS="-Wall -Wextra -pedantic -ggdb -std=c11"
CLIBS="-lraylib -lm"

gcc $CFLAGS -o nesq-emu main.c $CLIBS
