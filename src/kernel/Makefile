# Makefile for MosquitOS kernel
# Note: Only works on x86_64

include ../../Makefile.inc

# Edit this line to add more source files (*.c or *.S)
C_SRCS          := ../common/build_info.c kernel.c text_output.c font.c keyboard_controller.c util.c apic.c ../common/mem_util.c gdt.c interrupts.c exceptions.c virtual_memory.c datastructures/list.c acpi.c timer.c scheduler.c
ASM_SRCS        := gdt.s interrupts.s
OBJS            := $(C_SRCS:%.c=%.o) $(ASM_SRCS:%.s=%_asm.o)
DEPS            := $(C_SRCS:.c=.d)

TARGET          := ../../build/kernel

LIBDIR          := /usr/lib
LIBS            := 

CFLAGS          := $(CFLAGS_COMMON) -nostdlib -fno-asynchronous-unwind-tables -ffreestanding -mcmodel=large -mno-red-zone
ifeq ($(ARCH),x86_64)
	CFLAGS        += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         := -nostdlib -static -z max-page-size=0x1000 -T kernel_link.lds

all: $(TARGET)

.PHONY : clean
clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

# Compute header dependencies
include $(DEPS)
%.d: %.c
	../../dependencies.sh $(CFLAGS) $*.c > $@

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%_asm.o: %.s 
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@