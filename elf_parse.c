#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/mman.h>

#include "elf_parse.h"

bool valid_elf64_header(Elf64_Ehdr *elf_hdr) {
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

int main(int argc, char const *argv[]) {
  FILE *fp = fopen("kernel", "rb");

  fseek(fp, 0, SEEK_END); // seek to end of file
  int kernel_size = ftell(fp); // get current file pointer
  fseek(fp, 0, SEEK_SET); // seek back to beginning of file

  printf("Kernel size: %d\n", kernel_size);

  uint8_t *buffer = malloc(kernel_size);
  fread(buffer, 1, kernel_size, fp);
  fclose(fp);

  Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *)buffer;

  if (!valid_elf64_header(elf_hdr)) {
    fprintf(stderr, "Invalid ELF header!\n");
    return -1;
  }


  // Allocate memory to run executable from
  uint8_t *region = mmap((void *)elf_hdr->e_entry, kernel_size, PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  assert(region == (void *)0x100000);

  printf("Program header table size: %d\n", elf_hdr->e_phnum);
  printf("Section header table size: %d\n", elf_hdr->e_shnum);

  printf("\n");

  Elf64_Shdr *section_headers = (Elf64_Shdr *)(buffer + elf_hdr->e_shoff);

  Elf64_Phdr *program_headers = (Elf64_Phdr *)(buffer + elf_hdr->e_phoff);

  Elf64_Shdr *strtab_header = (Elf64_Shdr *)(buffer + elf_hdr->e_shoff + (elf_hdr->e_shstrndx * elf_hdr->e_shentsize));
  char *string_table = (char *)(buffer + strtab_header->sh_offset);

  for (int i = 0; i < elf_hdr->e_shnum; ++i) {
    assert (section_headers[i].sh_name < strtab_header->sh_size);

    printf("Section type: %d, name: %s\n", section_headers[i].sh_type, string_table + section_headers[i].sh_name);

    printf("Section virtual address: %p, size: %d bytes\n", (void *)section_headers[i].sh_addr, (int)section_headers[i].sh_size);
    if (section_headers[i].sh_addr != 0) {
      if (section_headers[i].sh_type == SHT_NOBITS) {
        // Create empty section for BSS
        memset((void *)section_headers[i].sh_addr, 0, section_headers[i].sh_size);
      } else {
        // Copy section from ELF file
        memcpy((void *)section_headers[i].sh_addr, buffer + section_headers[i].sh_offset, section_headers[i].sh_size);
        printf("Copied.\n");
      }
    }
    printf("\n");
  }

  int (*kernel_main) () = (int (*) ())region;

  printf("kernel_main(): %d\n", kernel_main());

  return 0;
}