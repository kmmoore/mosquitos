#include "keyboard_controller.h"
#include "text_output.h"
#include "util.h"

struct GDTEntry {
  uint16_t limit_low;         // The lower 16 bits of the limit.
  uint16_t base_low;          // The lower 16 bits of the base.
  uint8_t  base_middle;       // The next 8 bits of the base.
  uint8_t  access;            //type: 4, s: 1, dpl: 2, p: 1;
  uint8_t  limit_high: 4, flags:4; //avl: 1, l: 1, d: 1, g: 1;
  uint8_t  base_high;         // The last 8 bits of the base.
} __attribute__((packed)) GDT[5];

struct GDTR {
  uint16_t size;
  uint64_t address;
} __attribute__((packed)) GDTR;

struct IDTEntry {
  uint16_t base_low;
  uint16_t selector;
  uint8_t  zero_1;
  uint8_t  attributes;
  uint16_t base_middle;
  uint32_t base_high;
  uint32_t zero_2;
} __attribute__((packed)) IDT[256];

struct IDTR {
  uint16_t size;
  uint64_t address;
} __attribute__((packed)) IDTR;

static void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
   GDT[index].base_low    = (base & 0xFFFF);
   GDT[index].base_middle = (base >> 16) & 0xFF;
   GDT[index].base_high   = (base >> 24) & 0xFF;

   GDT[index].limit_low   = (limit & 0xFFFF);
   GDT[index].limit_high  = (limit >> 16) & 0x0F;

   GDT[index].flags       = flags;
   GDT[index].access      = access;
}

static void set_idt_entry(int index, uint64_t base, uint16_t selector, uint8_t attributes) {
   IDT[index].base_low    = (base & 0xFFFF);
   IDT[index].base_middle = (base >> 16) & 0xFFFF;
   IDT[index].base_high   = (base >> 32) & 0xFFFFFFFF;

   IDT[index].selector    = selector;
   IDT[index].attributes  = attributes;

   IDT[index].zero_1      = 0;
   IDT[index].zero_2      = 0;
}

void isr() {
  text_output_print("ISR\n");

  __asm__ ("iretq");
}

void keyboard_controller_init() {
  text_output_print("Attempting to setup GDT and interrupts.\n");

  // Setup page table



  // Setup GDT

  set_gdt_entry(0, 0, 0, 0, 0);                // Null segment
  set_gdt_entry(1, 0, 0xFFFFFFFF, 0b10011010, 0b00001010); // Code segment
  set_gdt_entry(2, 0, 0xFFFFFFFF, 0b10010010, 0b00001010); // Data segment
  set_gdt_entry(3, 0, 0xFFFFFFFF, 0b11111010, 0b00001010); // User mode code segment
  set_gdt_entry(4, 0, 0xFFFFFFFF, 0b11110010, 0b00001010); // User mode data segment

  GDTR.size = sizeof(GDT) - 1;
  GDTR.address = (uint64_t)&GDT[0];

  __asm__ ("lgdt %0" : : "m" (GDTR));

  text_output_print("Loaded GDT.\n");
  
  for (int i = 0; i < 256; ++i) {
    set_idt_entry(i, (uint64_t)isr, 0x08, 0b10001110);
  }

  IDTR.size = sizeof(IDT) - 1;
  IDTR.address = (uint64_t)&IDT[0];

  __asm__ ("lidt %0" : : "m" (IDTR));

  text_output_print("Loaded IDT.\n");

  char buf[20];
  int2str((uint64_t)isr, buf, sizeof(buf));
  text_output_print(buf);
  text_output_print("\n");

  __asm__ ("int $0x1");

  text_output_print("Fired interrupt 0x1.");
}
