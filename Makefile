# Makefile for MosquitOS bootloader
# Note: Only works on x86_64

ARCH            = $(shell uname -m | sed s,i[3456789]86,ia32,)

# Edit this line to add more source files (*.c or *.S)
SRCS            = loader.c elf_parse.c fileops.c mem_util.c
TMP_SRCS        = $(SRCS:%.c=%.o)
OBJS            = $(TMP_SRCS:%.s=%.o)

TARGET          = loader.efi
SO_FILE         = $(TARGET:%.efi=%.so)

CC              = gcc
LD              = ld
OBJCOPY         = objcopy

EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
LIBDIR          = /usr/lib
EFI_CRT_OBJS    = $(LIBDIR)/crt0-efi-$(ARCH).o
EFI_LDS         = $(LIBDIR)/elf_$(ARCH)_efi.lds
LIBS            = -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name)

CFLAGS          = $(EFIINCS) -std=gnu99 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -Werror -ggdb
ifeq ($(ARCH),x86_64)
  CFLAGS += -DEFI_FUNCTION_WRAPPER
endif

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared -Bsymbolic -L $(LIBDIR) $(EFI_CRT_OBJS) 

all: $(TARGET)

.PHONY : clean
clean:
	rm -f $(TARGET) $(OBJS) $(SO_FILE)

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