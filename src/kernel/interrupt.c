#include "interrupt.h"
#include "gdt.h"
#include "apic.h"
#include "text_output.h"
#include "util.h"

// Private structs
static struct IDTEntry {
  uint16_t base_low;
  uint16_t selector;
  uint8_t  zero_1;
  uint8_t  attributes;
  uint16_t base_middle;
  uint32_t base_high;
  uint32_t zero_2;
} __attribute__((packed)) IDT[256];

static struct IDTR {
  uint16_t size;
  uint64_t address;
} __attribute__((packed)) IDTR;

static void (*interrupts_handlers[256])(int);

// Helper functions
static void set_idt_entry(int index, uint64_t base, uint16_t selector, uint8_t attributes) {
   IDT[index].base_low    = (base & 0xFFFF);
   IDT[index].base_middle = (base >> 16) & 0xFFFF;
   IDT[index].base_high   = (base >> 32) & 0xFFFFFFFF;

   IDT[index].selector    = selector;
   IDT[index].attributes  = attributes;

   IDT[index].zero_1      = 0;
   IDT[index].zero_2      = 0;
}

void isr_common(uint64_t num, uint64_t error_code) {
  interrupts_handlers[num](error_code);
}

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr30();
extern void isr33();
extern void isr34();
extern void isr35();
// TODO: Figure out how to not special case this here
extern void scheduler_timer_isr(); // We have to handle this separately
extern void scheduler_yield_without_saving_isr(); // We have to handle this separately

// Public functions
void interrupts_init() {
  // Setup GDT and APIC before we can do interrupts
  gdt_init();
  apic_init();

  text_output_print("Loading IDT...");

  // TODO: Explain what the attributes mean

  // Exceptions (trap gates)
  set_idt_entry(0, (uint64_t)isr0, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(1, (uint64_t)isr1, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(2, (uint64_t)isr2, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(3, (uint64_t)isr3, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(4, (uint64_t)isr4, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(5, (uint64_t)isr5, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(6, (uint64_t)isr6, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(7, (uint64_t)isr7, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(8, (uint64_t)isr8, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(9, (uint64_t)isr9, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(10, (uint64_t)isr10, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(11, (uint64_t)isr11, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(12, (uint64_t)isr12, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(13, (uint64_t)isr13, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(14, (uint64_t)isr14, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(16, (uint64_t)isr16, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(17, (uint64_t)isr17, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(18, (uint64_t)isr18, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(19, (uint64_t)isr19, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(20, (uint64_t)isr20, GDT_KERNEL_CS, 0b10001111);
  set_idt_entry(30, (uint64_t)isr30, GDT_KERNEL_CS, 0b10001110);

  // IRQs (interrupt gates)
  set_idt_entry(33, (uint64_t)isr33, GDT_KERNEL_CS, 0b10001110); // Keyboard
  set_idt_entry(34, (uint64_t)isr34, GDT_KERNEL_CS, 0b10001110); // PIC timer
  set_idt_entry(35, (uint64_t)isr35, GDT_KERNEL_CS, 0b10001110); // Local APIC timer (calibration)
  set_idt_entry(36, (uint64_t)scheduler_timer_isr, GDT_KERNEL_CS, 0b10001110); // Local APIC timer (scheduler)
  set_idt_entry(37, (uint64_t)scheduler_yield_without_saving_isr, GDT_KERNEL_CS, 0b10001110); // Local APIC timer (scheduler)

  IDTR.size = sizeof(IDT) - 1;
  IDTR.address = (uint64_t)&IDT[0];

  __asm__ ("lidt %0" : : "m" (IDTR));

  text_output_print("Done\n");
}

void interrupts_register_handler(int index, void (*handler)()) {
  if (index < 0 || index >= 256) return;

  interrupts_handlers[index] = handler;
}