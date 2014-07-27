# Makefile for MosquitOS bootloader
# Note: Only works on x86_64

include ../../Makefile.inc

# Edit this line to add more source files (*.c or *.S)
C_SRCS          := loader.c elf_parse.c fileops.c ../common/mem_util.c
ASM_SRCS        := 
OBJS            := $(C_SRCS:%.c=%.o) $(ASM_SRCS:%.s=%_asm.o)
DEPS            := $(C_SRCS:.c=.d)

TARGET          := ../../build/loader.efi
SO_FILE         := $(TARGET:%.efi=%.so)

EFIINC          := /usr/include/efi
EFIINCS         := -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIBDIR          := /usr/lib
EFI_CRT_OBJS    := $(LIBDIR)/crt0-efi-$(ARCH).o
EFI_LDS         := $(LIBDIR)/elf_$(ARCH)_efi.lds
LIBS            := -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name)

CFLAGS          := $(EFIINCS) -std=gnu99 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -Werror
ifeq ($(ARCH),x86_64)
  CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         := -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(LIBDIR) $(EFI_CRT_OBJS) 

all: $(TARGET)

.PHONY : clean
clean:
	rm -f $(TARGET) $(OBJS) $(SO_FILE) $(DEPS)

# Compute header dependencies
include $(DEPS)
%.d: %.c
	../../dependencies.sh $(CFLAGS) $*.c > $@

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s 
	$(CC) $(CFLAGS) -c $< -o $@

# Link
%.so: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

# Copy
%.efi: %.so 
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		   -j .rela -j .reloc --target=efi-app-$(ARCH) $*.so $@