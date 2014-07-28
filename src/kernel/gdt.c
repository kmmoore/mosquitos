#include "gdt.h"
#include "text_output.h"

// Private structs 
static struct GDTEntry {
  uint16_t limit_low;         // The lower 16 bits of the limit.
  uint16_t base_low;          // The lower 16 bits of the base.
  uint8_t  base_middle;       // The next 8 bits of the base.
  uint8_t  access;            //type: 4, s: 1, dpl: 2, p: 1;
  uint8_t  limit_high:4, flags:4; //avl: 1, l: 1, d: 1, g: 1;
  uint8_t  base_high;         // The last 8 bits of the base.
} __attribute__((packed)) GDT[5];

struct GDTR {
  uint16_t size;
  uint64_t address;
} __attribute__((packed)) GDTR;

// Helper functions
extern void gdt_flush(); // gdt.s

static void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
   GDT[index].base_low    = (base & 0xFFFF);
   GDT[index].base_middle = (base >> 16) & 0xFF;
   GDT[index].base_high   = (base >> 24) & 0xFF;

   GDT[index].limit_low   = (limit & 0xFFFF);
   GDT[index].limit_high  = (limit >> 16) & 0x0F;

   GDT[index].flags       = flags;
   GDT[index].access      = access;
}

// Public functions
void gdt_init() {
  text_output_print("Loading GDT...");

  // Setup GDT
  set_gdt_entry(0, 0, 0, 0, 0);                // Null segment
  set_gdt_entry(1, 0, 0xFFFFFFFF, 0b10011010, 0b1010); // Code segment
  set_gdt_entry(2, 0, 0xFFFFFFFF, 0b10010010, 0b1010); // Data segment
  set_gdt_entry(3, 0, 0xFFFFFFFF, 0b11111010, 0b1010); // User mode code segment
  set_gdt_entry(4, 0, 0xFFFFFFFF, 0b11110010, 0b1010); // User mode data segment

  GDTR.size = sizeof(GDT) - 1;
  GDTR.address = (uint64_t)&GDT[0];

  gdt_flush();

  text_output_print("Done\n");
}