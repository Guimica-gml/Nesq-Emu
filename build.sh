#!/usr/bin/env sh
set -e

CFLAGS="-Wall -Wextra -pedantic -ggdb -std=c11"
CLIBS=""

gcc $CFLAGS -o nesq-emu main.c $CLIBS

if [ "$1" = "run" ]
then
    shift
    ./nesq-emu "$@"
fi
