#include <efi.h>
#include <efilib.h>

#include <elf.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>

#include "elf_parse.h"
#include "fileops.h"
#include "../common/mem_util.h"

static bool valid_elf64_header(Elf64_Ehdr *elf_hdr) {
  // Check header
  if (elf_hdr->e_ident[EI_MAG0] != ELFMAG0 ||
      elf_hdr->e_ident[EI_MAG1] != ELFMAG1 ||
      elf_hdr->e_ident[EI_MAG2] != ELFMAG2 ||
      elf_hdr->e_ident[EI_MAG3] != ELFMAG3) return false;

  // Check for 64-bit
  if (elf_hdr->e_ident[EI_CLASS] != ELFCLASS64) return false;

  // Check for System V (Unix) ABI
  if (elf_hdr->e_ident[EI_OSABI] != ELFOSABI_NONE &&
      elf_hdr->e_ident[EI_OSABI] != ELFOSABI_GNU) return false;

  // Check for executable file
  if (elf_hdr->e_type != ET_EXEC) return false;

  // Check for x86_64 architecture
  if (elf_hdr->e_machine != EM_X86_64) return false;

  // All valid
  return true;

}

void print_c_str(char *str) {
  while (*str != '\0') {
    Print(L"%c", (CHAR16)(*str));
    str++;
  }
}

EFI_STATUS load_kernel(CHAR16 *kernel_fname, OUT void **entry_address) {
  EFI_STATUS status;

  // Open filesystem
  EFI_FILE_IO_INTERFACE *fs = fops_get_filesystem();
  EFI_FILE *fs_root = fops_open_volume(fs);
  if (fs_root == NULL) return EFI_ABORTED;
  
  // Open kernel file
  EFI_FILE *kernel_file = fops_open_file(fs_root, kernel_fname, EFI_FILE_MODE_READ, 0);
  if (kernel_file == NULL) return EFI_ABORTED;
  
  UINTN kernel_size = fops_file_size(kernel_file);

  Print(L"Kernel size: %d\n", kernel_size);

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
    return EFI_ABORTED;
  }

  Elf64_Phdr *program_headers = (Elf64_Phdr *)(buffer + elf_hdr->e_phoff);

  // Determine how much memory we need
  Elf64_Addr highest_addr_found = 0;
  for (int i = 0; i < elf_hdr->e_phnum; ++i) {
    Elf64_Addr chunk_end = program_headers[i].p_vaddr + program_headers[i].p_memsz;
    if (chunk_end > highest_addr_found) highest_addr_found = chunk_end;
  }

  int bytes_needed = (int)(highest_addr_found - elf_hdr->e_entry);

  Print(L"Need %d bytes for kernel.\n", bytes_needed);

  // Allocate pages from which to execute kernel
  int num_pages_needed = (bytes_needed / EFI_PAGE_SIZE) + 1;
  EFI_PHYSICAL_ADDRESS region = elf_hdr->e_entry;

  Print(L"Trying to allocate %d pages at 0x%x\n", num_pages_needed, region);
  status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, num_pages_needed, &region);

  if (status != EFI_SUCCESS || region != elf_hdr->e_entry) {
    Print(L"Error allocating pages for kernel.\n");
    Print(L"status: %d, region: 0x%x, e_entry: 0x%x\n", status, region, elf_hdr->e_entry);
    return EFI_ABORTED;
  }

  // Copy the loadable program sections into the VM region we allocated
  for (int i = 0; i < elf_hdr->e_phnum; ++i) {
    Elf64_Phdr ph = program_headers[i];

    if (ph.p_type == PT_LOAD) {
      // Copy as much data as we have to beginning
      memcpy((void *)ph.p_vaddr, buffer + ph.p_offset, ph.p_filesz);
      // Zero out the rest of the section
      memset((void *)(ph.p_vaddr + ph.p_filesz), 0, ph.p_memsz - ph.p_filesz);
    }
  }

  *entry_address = (void *)elf_hdr->e_entry;

  // Free the kernel file buffer
  uefi_call_wrapper(BS->FreePool, 1, buffer);

  return EFI_SUCCESS;
}