#
# Makefile for ACPICA Core interpreter
#

include ../../../../Makefile.inc

CFLAGS			:= -O0 -D_MOSQUITOS -nostdlib -fno-asynchronous-unwind-tables -ffreestanding -mcmodel=large -mno-red-zone -ggdb -isystem include
LDFLAGS     := -nostdlib -static -r -z max-page-size=0x1000

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