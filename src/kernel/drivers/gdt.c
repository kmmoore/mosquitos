#include <common/mem_util.h>
#include <kernel/drivers/gdt.h>
#include <kernel/drivers/text_output.h>

// Private structs 
static struct GDTEntry {
  uint16_t limit_low;         // The lower 16 bits of the limit.
  uint16_t base_low;          // The lower 16 bits of the base.
  uint8_t  base_middle;       // The next 8 bits of the base.
  uint8_t  accessed:1;            //type: 4, s: 1, dpl: 2, p: 1;
  uint8_t  read_write:1;
  uint8_t  direction_conforming:1;
  uint8_t  executable:1;
  uint8_t  type:1; // 0 for system, 1 for code/data segments
  uint8_t  ring:2; // 0 for kernel, 3 for user
  uint8_t  present:1;
  uint8_t  limit_high:4;
  uint8_t  available:1;
  uint8_t  is_64_bit:1;
  uint8_t  is_32_bit:1;
  uint8_t  granularity:1; // If 1, limit is in pages, else in bytes
  uint8_t  base_high;         // The last 8 bits of the base.
} __attribute__((packed)) GDT[5];

struct GDTR {
  uint16_t size;
  uint64_t address;
} __attribute__((packed)) GDTR;

struct TSSDescriptor {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t  base_middle;

  uint8_t  type:4;
  uint8_t  reserved0:1;
  uint8_t  ring:2;
  uint8_t  present:1;

  uint8_t  limit_high:4;
  uint8_t  available:1;
  uint8_t  reserved1:2;
  uint8_t  granularity:1;

  uint8_t  base_middle2;
  uint32_t base_high;
  uint32_t reserved2;
} __attribute__((packed));

struct TSS {
  uint32_t reserved0;
  uint32_t rsp0_low;
  uint32_t rsp0_high;
  uint32_t rsp1_low;
  uint32_t rsp1_high;
  uint32_t rsp2_low;
  uint32_t rsp2_high;

  uint64_t reserved1;
  uint32_t ist1_low;
  uint32_t ist1_high;
  uint32_t ist2_low;
  uint32_t ist2_high;
  uint32_t ist3_low;
  uint32_t ist3_high;
  uint32_t ist4_low;
  uint32_t ist4_high;
  uint32_t ist5_low;
  uint32_t ist5_high;
  uint32_t ist6_low;
  uint32_t ist6_high;
  uint32_t ist7_low;
  uint32_t ist7_high;

  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base_address;
} __attribute__((packed)) TSS;

// TODO: Deterimine how large stacks should be
// TODO: Don't put these in the bss section
struct Stack {
  uint8_t data[4096];
};
static struct Stack ring_stacks[3];
static struct Stack ist_stacks[7];

// Helper functions
extern void gdt_flush(); // gdt.s

static void set_kernel_gdt_entry(int index, bool is_code) {
  memset(&GDT[index], 0, sizeof(GDT[index]));
  GDT[index].type = 1;
  GDT[index].present = 1;
  GDT[index].read_write = 1;
  GDT[index].executable = is_code;
  GDT[index].is_64_bit = is_code;
}

static void setup_tss(int index) {
  // Fill TSS
  uint64_t ring_stack_addresses[3];
  for (int i = 0; i < 3; ++i) {
    ring_stack_addresses[i] = UNION_CAST(&ring_stacks[0].data, uint64_t);
  }
  TSS.rsp0_low = ring_stack_addresses[0] & 0xFFFFFFFF;
  TSS.rsp0_high = (ring_stack_addresses[0] >> 32) & 0xFFFFFFFF;
  TSS.rsp1_low = ring_stack_addresses[1] & 0xFFFFFFFF;
  TSS.rsp1_high = (ring_stack_addresses[1] >> 32) & 0xFFFFFFFF;
  TSS.rsp2_low = ring_stack_addresses[2] & 0xFFFFFFFF;
  TSS.rsp2_high = (ring_stack_addresses[2] >> 32) & 0xFFFFFFFF;

  uint64_t ist_stack_addresses[7];
  for (int i = 0; i < 7; ++i) {
    ist_stack_addresses[i] = UNION_CAST(&ist_stacks[0].data, uint64_t);
  }
  TSS.ist1_low = ist_stack_addresses[0] & 0xFFFFFFFF;
  TSS.ist1_high = (ist_stack_addresses[0] >> 32) & 0xFFFFFFFF;
  TSS.ist2_low = ist_stack_addresses[1] & 0xFFFFFFFF;
  TSS.ist2_high = (ist_stack_addresses[1] >> 32) & 0xFFFFFFFF;
  TSS.ist3_low = ist_stack_addresses[2] & 0xFFFFFFFF;
  TSS.ist3_high = (ist_stack_addresses[2] >> 32) & 0xFFFFFFFF;
  TSS.ist4_low = ist_stack_addresses[3] & 0xFFFFFFFF;
  TSS.ist4_high = (ist_stack_addresses[3] >> 32) & 0xFFFFFFFF;
  TSS.ist5_low = ist_stack_addresses[4] & 0xFFFFFFFF;
  TSS.ist5_high = (ist_stack_addresses[4] >> 32) & 0xFFFFFFFF;
  TSS.ist6_low = ist_stack_addresses[5] & 0xFFFFFFFF;
  TSS.ist6_high = (ist_stack_addresses[5] >> 32) & 0xFFFFFFFF;
  TSS.ist7_low = ist_stack_addresses[6] & 0xFFFFFFFF;
  TSS.ist7_high = (ist_stack_addresses[6] >> 32) & 0xFFFFFFFF;

  TSS.io_map_base_address = sizeof(TSS);

  // Set TSS GDT entry
  memset(&GDT[index], 0, sizeof(GDT[index]));
  struct TSSDescriptor *descriptor = UNION_CAST(&GDT[index], struct TSSDescriptor*);
  descriptor->type = 0b1001; // From Intel Manual Vol. 3A section 3.5
  descriptor->present = 1;
  descriptor->available = 1;

  assert(sizeof(TSS) < (1 << NUM_BITS(sizeof(descriptor->limit_low))));
  descriptor->limit_low = sizeof(TSS);
  uint64_t address = UNION_CAST(&TSS, uint64_t);
  descriptor->base_low = address & 0xFFFF;
  descriptor->base_middle = (address >> 16) & 0xFF;
  descriptor->base_middle2 = (address >> 24) & 0xFF;
  descriptor->base_high = (address >> 32) & 0xFFFFFFFF;
}

// Public functions
void gdt_init() {
  // Setup GDT
  memset(&GDT[0], 0, sizeof(GDT[0])); // Null segment
  set_kernel_gdt_entry(1, true); // Code segment
  set_kernel_gdt_entry(2, false); // Data segment
  setup_tss(3); // 16 bytes (two gdt_entries)

  GDTR.size = sizeof(GDT) - 1;
  GDTR.address = (uint64_t)&GDT[0];

  gdt_flush();

  REGISTER_MODULE("gdt");
}