ARCH            := $(shell uname -m | sed s,i[3456789]86,ia32,)

CC              := gcc
LD              := ld
OBJCOPY         := objcopy

# TODO: Add cflags