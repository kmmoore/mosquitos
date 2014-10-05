# Makefile for MosquitOS kernel
# Note: Only works on x86_64

include ../../Makefile.inc

# Edit this line to add more source files (*.c or *.S)
C_SRCS          :=  kernel.c
C_SRCS          += ../common/build_info.c util.c ../common/mem_util.c
C_SRCS          += datastructures/list.c
C_SRCS          += hardware/gdt.c hardware/interrupt.c hardware/exception.c hardware/timer.c hardware/acpi.c hardware/apic.c
C_SRCS          += format/format.c
C_SRCS          += drivers/text_output.c drivers/font.c drivers/keyboard_controller.c drivers/pci.c drivers/sata.c drivers/acpica_shim.c
C_SRCS          += memory/virtual_memory.c memory/kmalloc.c
C_SRCS          += threading/scheduler.c threading/thread.c threading/mutex/semaphore.c threading/mutex/lock.c

ASM_SRCS        := hardware/gdt.s hardware/interrupt.s threading/scheduler.s

OBJS            := $(C_SRCS:%.c=%.o) $(ASM_SRCS:%.s=%_asm.o)
DEPS            := $(C_SRCS:.c=.d)

ACPI_INC        := drivers/acpica/include
ACPICA_DIR      := drivers/acpica
ACPICA_LIB      := $(ACPICA_DIR)/acpica.o

TARGET          := ../../build/kernel

LIBDIR          := /usr/lib
LIBS            := 

CFLAGS          := -isystem $(ACPI_INC) $(CFLAGS_COMMON) -nostdlib -fno-asynchronous-unwind-tables -ffreestanding -mcmodel=large -mno-red-zone  -fno-omit-frame-pointer
ifeq ($(ARCH),x86_64)
	CFLAGS        += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         := -nostdlib -static -z max-page-size=0x1000 -T kernel_link.lds

all: $(ACPICA_LIB) $(TARGET)

.PHONY : clean
clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
	$(MAKE) -j -C $(ACPICA_DIR) clean

$(ACPICA_LIB):
	$(MAKE) -j -C $(ACPICA_DIR)

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
	$(LD) $(LDFLAGS) $^ $(ACPICA_LIB) -o $@