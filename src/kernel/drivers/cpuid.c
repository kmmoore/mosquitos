#include <kernel/drivers/cpuid.h>

#include <common/mem_util.h>
#include <kernel/util.h>

static struct {
  char vendor_id[13];
  uint32_t max_calling_param;
  uint32_t signature;
  uint64_t capabilities, extended_capabilities;
} cpuid_data;

void read_vendor_id() {
  uint32_t id[3] = {0};
  __asm__("cpuid"
          : "=eax"(cpuid_data.max_calling_param), "=ebx"(id[0]), "=edx"(id[1]),
            "=ecx"(id[2])
          : "eax"(0x0));
  memcpy(&cpuid_data.vendor_id[0], &id[0], 12);
}

void read_capabilities() {
  uint64_t capabilities1, capabilities2;
  __asm__("cpuid"
          : "=eax"(cpuid_data.signature), "=edx"(capabilities1),
            "=ecx"(capabilities2)
          : "eax"(0x1)
          : "%rbx");
  cpuid_data.capabilities = (capabilities2 << 32) | capabilities1;
}

void read_extended_capabilities() {
  uint32_t capabilities1, capabilities2;
  __asm__("cpuid"
          : "=ebx"(capabilities1), "=ecx"(capabilities2)
          : "eax"(0x7), "ecx"(0x0)
          : "%rax", "%rdx");
  cpuid_data.extended_capabilities =
      ((uint64_t)capabilities2 << 32) | capabilities1;
}

bool cpuid_has_capability(const enum CPUCapability capability) {
  return (cpuid_data.capabilities & (1ULL << capability)) != 0;
}

bool cpuid_has_extended_capability(
    const enum CPUExtendedCapability capability) {
  return (cpuid_data.extended_capabilities & (1ULL << capability)) != 0;
}

void cpuid_init() {
  read_vendor_id();
  read_capabilities();
  read_extended_capabilities();

  REGISTER_MODULE("cpuid");
}
