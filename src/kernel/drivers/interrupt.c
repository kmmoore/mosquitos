#include <common/mem_util.h>
#include <kernel/drivers/interrupt.h>
#include <kernel/drivers/gdt.h>
#include <kernel/drivers/apic.h>
#include <kernel/drivers/text_output.h>
#include <kernel/util.h>

enum IDTEntryType {
  INTERRUPT_GATE = 0b01110,
  TRAP_GATE = 0b01111
};

// Private structs
static struct IDTEntry {
  uint16_t base_low;
  uint16_t selector;
  uint8_t  zero_1;
  uint8_t  type:5;
  uint8_t  privilege_level:2;
  uint8_t  present:1;
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
static void set_idt_entry(int index, uint64_t base, enum IDTEntryType type) {
  memset(&IDT[index], 0, sizeof(IDT[index]));
  IDT[index].base_low    = (base & 0xFFFF);
  IDT[index].base_middle = (base >> 16) & 0xFFFF;
  IDT[index].base_high   = (base >> 32) & 0xFFFFFFFF;

  IDT[index].selector    = GDT_KERNEL_CS;
  IDT[index].type        = type;
  IDT[index].present     = 1;
}

void isr_common(uint64_t num, uint64_t error_code) {
  interrupts_handlers[num](error_code);
  apic_send_eoi_if_necessary(num);
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
// TODO: Figure out how to not special case this here
extern void scheduler_timer_isr(); // We have to handle this separately
extern void scheduler_yield_without_saving_isr(); // We have to handle this separately
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr39();

// Public functions
void interrupt_init() {
  // Setup GDT and APIC before we can do interrupts
  gdt_init();
  apic_init();

  REQUIRE_MODULE("gdt");
  REQUIRE_MODULE("apic");

  // Exceptions (trap gates)
  set_idt_entry(0, (uint64_t)isr0, TRAP_GATE);
  set_idt_entry(1, (uint64_t)isr1, TRAP_GATE);
  set_idt_entry(2, (uint64_t)isr2, TRAP_GATE);
  set_idt_entry(3, (uint64_t)isr3, TRAP_GATE);
  set_idt_entry(4, (uint64_t)isr4, TRAP_GATE);
  set_idt_entry(5, (uint64_t)isr5, TRAP_GATE);
  set_idt_entry(6, (uint64_t)isr6, TRAP_GATE);
  set_idt_entry(7, (uint64_t)isr7, TRAP_GATE);
  set_idt_entry(8, (uint64_t)isr8, TRAP_GATE);
  set_idt_entry(9, (uint64_t)isr9, TRAP_GATE);
  set_idt_entry(10, (uint64_t)isr10, TRAP_GATE);
  set_idt_entry(11, (uint64_t)isr11, TRAP_GATE);
  set_idt_entry(12, (uint64_t)isr12, TRAP_GATE);
  set_idt_entry(13, (uint64_t)isr13, TRAP_GATE);
  set_idt_entry(14, (uint64_t)isr14, TRAP_GATE);
  set_idt_entry(16, (uint64_t)isr16, TRAP_GATE);
  set_idt_entry(17, (uint64_t)isr17, TRAP_GATE);
  set_idt_entry(18, (uint64_t)isr18, TRAP_GATE);
  set_idt_entry(19, (uint64_t)isr19, TRAP_GATE);
  set_idt_entry(20, (uint64_t)isr20, TRAP_GATE);
  set_idt_entry(30, (uint64_t)isr30, INTERRUPT_GATE);

  // IRQs (interrupt gates)
  set_idt_entry(SCHEDULER_TIMER_IV, (uint64_t)scheduler_timer_isr, INTERRUPT_GATE); // Local APIC timer (scheduler)
  set_idt_entry(SCHEDULER_YEILD_WITHOUT_SAVING_IV, (uint64_t)scheduler_yield_without_saving_isr, INTERRUPT_GATE); // Local APIC timer (scheduler)

  set_idt_entry(KEYBOARD_IV, (uint64_t)isr35, INTERRUPT_GATE); // Keyboard
  set_idt_entry(PIC_TIMER_IV, (uint64_t)isr36, INTERRUPT_GATE); // PIC timer
  set_idt_entry(PCI_IV, (uint64_t)isr37, INTERRUPT_GATE); // PCI ISR

  set_idt_entry(LOCAL_APIC_CALIBRATION_IV, (uint64_t)isr39, INTERRUPT_GATE); // Local APIC timer (calibration)
  // Something is weird about IV 38...

  IDTR.size = sizeof(IDT) - 1;
  IDTR.address = (uint64_t)&IDT[0];

  __asm__ ("lidt %0" : : "m" (IDTR));

  REGISTER_MODULE("interrupt");
}

void interrupt_register_handler(int index, void (*handler)()) {
  if (index < 0 || index >= 256) return;

  interrupts_handlers[index] = handler;
}
