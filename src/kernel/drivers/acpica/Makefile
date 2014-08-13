#
# Makefile for ACPICA Core interpreter
#

include ../../../../Makefile.inc

CFLAGS			:= -Os -D_MOSQUITOS -nostdlib -fno-asynchronous-unwind-tables -ffreestanding -mcmodel=large -mno-red-zone -isystem include
LDFLAGS     := -nostdlib -static -r

# use acpi.o to put all files here into acpi.o modparam namespace
TARGET	+= acpica.o

C_SRCS      := $(wildcard *.c)
OBJS        := $(C_SRCS:%.c=%.o)

all: $(TARGET)

.PHONY: clean
clean:
	rm $(OBJS) $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.s 
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@