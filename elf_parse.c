#include <efi.h>
#include <efilib.h>

#include <elf.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "elf_parse.h"
#include "fileops.h"

static bool valid_elf64_header(Elf64_Ehdr *elf_hdr) {
  // Check header
  if (elf_hdr->e_ident[EI_MAG0] != ELFMAG0 ||
      elf_hdr->e_ident[EI_MAG1] != ELFMAG1 ||
      elf_hdr->e_ident[EI_MAG2] != ELFMAG2 ||
      elf_hdr->e_ident[EI_MAG3] != ELFMAG3) return false;

  // Check for 64-bit
  if (elf_hdr->e_ident[EI_CLASS] != ELFCLASS64) return false;

  // Check for System V (Unix) ABI
  if (elf_hdr->e_ident[EI_OSABI] != ELFOSABI_NONE) return false;

  // Check for executable file
  if (elf_hdr->e_type != ET_EXEC) return false;

  // Check for x86_64 architecture
  if (elf_hdr->e_machine != EM_X86_64) return false;

  // All valid
  return true;

}

EFI_STATUS load_kernel(CHAR16 *kernel_fname) {
  EFI_STATUS status;

  // Open filesystem
  EFI_FILE_IO_INTERFACE *fs = fops_get_filesystem();
  EFI_FILE *fs_root = fops_open_volume(fs);
  
  // Open kernel file
  EFI_FILE *kernel_file = fops_open_file(fs_root, kernel_fname, EFI_FILE_MODE_READ, 0);
  UINTN kernel_size = fops_file_size(kernel_file);

  Print(L"Kernel size: %lu\n", kernel_size);

  // Allocate memory for file
  uint8_t *buffer;
  status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, kernel_size, &buffer);

  if (status != EFI_SUCCESS) {
    Print(L"Error allocating kernel buffer!\n");
    return status;
  }

  // Read kernel
  UINTN bytes_read = fops_file_read(kernel_file, kernel_size, buffer);

  if (bytes_read != kernel_size) {
    Print(L"Error reading kernel file!\n");
    return EFI_ABORTED;
  }

  fops_file_close(kernel_file);

  Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *)buffer;

  if (!valid_elf64_header(elf_hdr)) {
    Print(L"Invalid ELF header!\n");
    return -1;
  }


  Print(L"Program header table size: %d\n", elf_hdr->e_phnum);
  Print(L"Section header table size: %d\n", elf_hdr->e_shnum);

  Print(L"\n");

  Elf64_Shdr *section_headers = (Elf64_Shdr *)(buffer + elf_hdr->e_shoff);

  // Elf64_Phdr *program_headers = (Elf64_Phdr *)(buffer + elf_hdr->e_phoff);

  Elf64_Shdr *strtab_header = (Elf64_Shdr *)(buffer + elf_hdr->e_shoff + (elf_hdr->e_shstrndx * elf_hdr->e_shentsize));
  char *string_table = (char *)(buffer + strtab_header->sh_offset);

  for (int i = 0; i < elf_hdr->e_shnum; ++i) {
    // assert (section_headers[i].sh_name < strtab_header->sh_size);

    Print(L"Section type: %d, name: %s\n", section_headers[i].sh_type, string_table + section_headers[i].sh_name);

    Print(L"Section virtual address: %p, size: %d bytes\n", (void *)section_headers[i].sh_addr, (int)section_headers[i].sh_size);
    if (section_headers[i].sh_addr != 0) {
      if (section_headers[i].sh_type == SHT_NOBITS) {
        // Create empty section for BSS
        memset((void *)section_headers[i].sh_addr, 0, section_headers[i].sh_size);
      } else {
        // Copy section from ELF file
        memcpy((void *)section_headers[i].sh_addr, buffer + section_headers[i].sh_offset, section_headers[i].sh_size);
        Print(L"Copied.\n");
      }
    }
    Print(L"\n");
  }

  // int (*kernel_main) () = (int (*) ())region;

  // printf("kernel_main(): %d\n", kernel_main());

  return 0;
}