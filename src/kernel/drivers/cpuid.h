#include <kernel/kernel_common.h>

#ifndef _KERNEL_DRIVERS_CPUID_H
#define _KERNEL_DRIVERS_CPUID_H

enum CPUCapability {
  CPUID_CAP_TSC = 1ULL << 4,
  CPUID_CAP_APIC = 1ULL << 9,
  CPUID_CAP_SYSENTER = 1ULL << 11,
  CPUID_CAP_XSAVE = 1ULL << (32 + 26),
  CPUID_CAP_RDRAND = 1ULL << (32 + 30),
};

enum CPUExtendedCapability {
  CPUID_EXCAP_RDSEED = 1ULL << 18,
};

void cpuid_init();
bool cpuid_has_capability(enum CPUCapability capability);
bool cpuid_has_extended_capability(enum CPUExtendedCapability capability);

#endif
