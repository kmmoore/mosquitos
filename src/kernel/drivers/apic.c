#include <kernel/drivers/apic.h>
#include <kernel/drivers/acpi.h>

#include <kernel/util.h>

#include <kernel/drivers/text_output.h>

#define PIC1    0x20    /* IO base address for master PIC */
#define PIC2    0xA0    /* IO base address for slave PIC */
#define PIC1_COMMAND  PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND  PIC2
#define PIC2_DATA (PIC2+1)
#define PIC_EOI   0x20    /* End-of-interrupt command code */

#define ICW1_ICW4 0x01    /* ICW4 (not) needed */
#define ICW1_INIT 0x10    /* Initialization - required! */
 
#define ICW4_8086 0x01    /* 8086/88 (MCS-80/85) mode */

#define APIC_TIMER_LVT_IDX 0x32
#define APIC_TIMER_DIV_IDX 0x3e
#define APIC_TIMER_ICR_IDX 0x38
#define APIC_TIMER_CCR_IDX 0x39

typedef struct {
  ACPISDTHeader header;
  uint32_t local_controller_address;
  uint32_t flags;
} __attribute__((packed)) MADT;

typedef struct {
  uint8_t device_type;
  uint8_t length;
} __attribute__((packed)) CommonMADTEntryHeader;

typedef struct {
  CommonMADTEntryHeader header;
  uint8_t processor_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((packed)) LocalAPICHeader;

typedef struct {
  CommonMADTEntryHeader header;
  uint8_t ioapic_id;
  uint8_t resertved;
  uint32_t address;
  uint32_t irq_base;
} __attribute__((packed)) IOAPICHeader;

typedef union {
  struct {
    uint8_t interrupt_vector;
    uint8_t reserved1:4;
    uint8_t event_pending:1;
    uint8_t reserved2:3;
    uint16_t masked:1;
    uint16_t mode:2;
    uint16_t reserved3:13;
  } svalue;

  uint32_t ivalue;
} LVT;

// This gets set from an MSR
static uint32_t *apic_base = NULL;

// These get set from the ACPI table
static uint32_t *ioapic_index = NULL;
static uint32_t *ioapic_data = NULL;

void ioapic_write(int index, uint32_t value) {
  *ioapic_index = index;
  *ioapic_data = value;
}

uint32_t ioapic_read(int index) {
  *ioapic_index = index;
  return *ioapic_data;
}

void apic_write(int index, uint32_t value) {
  apic_base[index * 4] = value; // Registers are 16 bytes wide
}

uint32_t apic_read(int index) {
  return *(apic_base + index * 4); // Registers are 16 bytes wide
}

void ioapic_map(uint8_t irq_index, uint8_t idt_index) {
  const uint32_t low_index = 0x10 + irq_index * 2;
  const uint32_t high_index = 0x10 + irq_index * 2 + 1;

  uint32_t high = ioapic_read(high_index);
  // Set APIC ID
  high &= ~0xff000000;
  high |= apic_read(0x02) << 24; // Local APIC id
  ioapic_write(high_index, high);

  uint32_t low = ioapic_read(low_index);

  // Unmask the IRQ
  low &= ~(1<<16);

  // Set to physical delivery mode
  low &= ~(1<<11);

  // Set to fixed delivery mode
  low &= ~0x700;

  // Set delivery vector
  low &= ~0xff;
  low |= idt_index;

  ioapic_write(low_index, low);
}

void apic_setup_local_timer(APICTimerDivider divider, uint8_t interrupt_vector, APICTimerMode mode, uint32_t initial_count) {
  LVT lvt;
  lvt.ivalue = 0;
  lvt.svalue.interrupt_vector = interrupt_vector;
  lvt.svalue.masked = 1;
  lvt.svalue.mode = mode;

  apic_write(APIC_TIMER_LVT_IDX, lvt.ivalue);

  uint32_t divider_value = apic_read(APIC_TIMER_DIV_IDX);
  divider_value &= ~0b1011;
  divider_value |= divider;
  apic_write(APIC_TIMER_DIV_IDX, divider_value);

  apic_write(APIC_TIMER_ICR_IDX, initial_count);
  apic_write(APIC_TIMER_CCR_IDX, 0);
}

void apic_set_local_timer_masked(bool masked) {
  LVT lvt;
  lvt.ivalue = apic_read(APIC_TIMER_LVT_IDX);

  lvt.svalue.masked = masked;

  apic_write(APIC_TIMER_LVT_IDX, lvt.ivalue);
}

// Locates the I/O APIC with IRQ Base == 0 and 
// loads it's address into the global variables
// `ioapic_index` and `ioapic_data`.
static bool load_ioapic_address() {
  MADT *madt = (MADT *)acpi_locate_table("APIC");

  if (!madt) return false;

  bool found = false;
  uint32_t position = sizeof(MADT);
  while (position < madt->header.Length) {
    uint8_t *madt_buffer = (uint8_t *)madt;
    CommonMADTEntryHeader *entry = (CommonMADTEntryHeader *)(madt_buffer + position);

    if (entry->device_type == 1) {
      IOAPICHeader *header = (IOAPICHeader *)entry;

      assert(header->irq_base == 0);

      if (header->irq_base == 0x0) {
        found = true;
        ioapic_index = (uint32_t *)(intptr_t)header->address;
        ioapic_data = (uint32_t *)(intptr_t)(header->address + 0x10);
      }
    }

    position += entry->length; // Second byte of entry is always 
  }

  return found;
}

void apic_init() {
  REQUIRE_MODULE("acpi_early");
  REQUIRE_MODULE("gdt");

  // Disable legacy PIC

  /* Set ICW1 */
  io_write_8(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);
  io_write_8(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);

  /* Set ICW2 (IRQ base offsets) */
  io_write_8(PIC1_DATA, 0xe0);
  io_write_8(PIC2_DATA, 0xe8);

  /* Set ICW3 */
  io_write_8(PIC1_DATA, 4);
  io_write_8(PIC2_DATA, 2);

  /* Set ICW4 */
  io_write_8(PIC1_DATA, ICW4_8086);
  io_write_8(PIC2_DATA, ICW4_8086);

  /* Disable all interrupts */
  io_write_8(PIC1_DATA, 0xff);
  io_write_8(PIC2_DATA, 0xff);

  if (!load_ioapic_address()) {
    panic("\nCould not find I/O APIC! This is currently required.\n");
  }

  // Enable APIC MSR
  uint64_t apic_msr = read_msr(0x1b);
  apic_base = (uint32_t *)(apic_msr & 0xffffff000); // Load Local APIC base address
  apic_msr |= (1 << 11);
  write_msr(0x1b, apic_msr);

  // Enable APIC flag and set SIVR IRQ to 0xFF
  uint32_t spurious_irq_num = 0xff;
  spurious_irq_num |= (1<<8);
  apic_write(0x0f, spurious_irq_num);

  REGISTER_MODULE("apic");
}

void apic_send_eoi() {
  apic_write(0x0b, 1);
}
