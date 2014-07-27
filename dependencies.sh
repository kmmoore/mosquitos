#!/bin/sh
gcc -MM -MG "$@" | sed 's/^\(.*\).o:/\1.o \1.d:/' 