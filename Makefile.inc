ARCH            := $(shell uname -m | sed s,i[3456789]86,ia32,)

CC              := gcc
LD              := ld
OBJCOPY         := objcopy

EFIINC          := /usr/include/efi
EFIINCS         := -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol

CFLAGS_COMMON   := $(EFIINCS) -std=gnu99 -Wall -Wextra -Werror -Os
