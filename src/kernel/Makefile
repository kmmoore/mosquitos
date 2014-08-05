# Makefile for MosquitOS kernel
# Note: Only works on x86_64

include ../../Makefile.inc

# Edit this line to add more source files (*.c or *.S)
C_SRCS          :=  kernel.c
C_SRCS          += ../common/build_info.c util.c ../common/mem_util.c
C_SRCS          += datastructures/list.c
C_SRCS          += hardware/gdt.c hardware/interrupt.c hardware/exception.c hardware/timer.c hardware/acpi.c hardware/apic.c
C_SRCS          += drivers/text_output.c drivers/font.c drivers/keyboard_controller.c drivers/pci.c
C_SRCS          += memory/virtual_memory.c memory/kmalloc.c
C_SRCS          += threading/scheduler.c threading/thread.c threading/mutex/semaphore.c threading/mutex/lock.c

ASM_SRCS        := hardware/gdt.s hardware/interrupt.s threading/scheduler.s

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