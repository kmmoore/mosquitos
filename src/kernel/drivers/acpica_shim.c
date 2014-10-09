#include "../kernel_common.h"
#include "../util.h"

#include "text_output.h"
#include "pci.h"

#include "../drivers/acpi.h"
#include "../drivers/timer.h"
#include "../drivers/interrupt.h"
#include "../drivers/timer.h"
#include "../memory/virtual_memory.h"
#include "../memory/kmalloc.h"
#include "../threading/thread.h"
#include "../threading/scheduler.h"
#include "../threading/mutex/semaphore.h"
#include "../threading/mutex/lock.h"

#include <acpi.h>

ACPI_STATUS AcpiOsInitialize(void) {
  // text_output_printf("ACPI OS Initialize!\n");

  return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void) {
  text_output_printf("ACPI OS Terminate!\n");

  return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {
  return acpi_xdsp_address();
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *init_val UNUSED, ACPI_STRING * new_val) {
  *new_val = 0;

  return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *existing_table UNUSED, ACPI_TABLE_HEADER **new_table) {
  *new_table = 0;

  return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *existing_table UNUSED, ACPI_PHYSICAL_ADDRESS * new_address, UINT32 *new_table_length UNUSED) {

  *new_address = 0;

  return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK * out_handle) {
  *out_handle = kmalloc(sizeof(SpinLock));
  if (*out_handle == NULL) return AE_NO_MEMORY;

  spinlock_init(*out_handle);

  return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK handle) {
  kfree(handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle) {
  spinlock_acquire(handle);

  return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flags) {
  (void)flags;
  spinlock_release(handle);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 max_units UNUSED, UINT32 initial_units, ACPI_SEMAPHORE * out_handle) {
  // TODO: Implement max_units

  *out_handle = kmalloc(sizeof(Semaphore));
  if (*out_handle == NULL) return AE_NO_MEMORY;

  semaphore_init(*out_handle, initial_units);

  return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle) {
  kfree(handle);

  return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, UINT32 units, UINT16 timeout) {
  if (semaphore_down(handle, units, timeout)) {
    return AE_OK;
  } else {
    return AE_TIME;
  }
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, UINT32 units) {
  semaphore_up(handle, units);

  return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE size) {
  return kmalloc(size);
}

void AcpiOsFree(void * memory) {
  kfree(memory);
}

ACPI_STATUS AcpiOsSignal (UINT32 function, void *info UNUSED) {
  // TODO: Do something with this?
  text_output_printf("Got ACPICA signal: 0x%x\n", function);
  return AE_OK;
}

ACPI_STATUS AcpiOsExecute (ACPI_EXECUTE_TYPE type UNUSED, ACPI_OSD_EXEC_CALLBACK function, void *context) {
  KernelThread *thread = thread_create((KernelThreadMain)function, context, 16, 2);
  if (!thread) return AE_BAD_PARAMETER;
  thread_start(thread);

  text_output_printf("Started thread for function: 0x%x\n", function);

  return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
  return thread_id(scheduler_current_thread());
}

void AcpiOsSleep(UINT64 milliseconds) {
  timer_thread_sleep(milliseconds);
}

void AcpiOsStall(UINT32 microseconds) {
  timer_thread_stall(microseconds);
}

void * AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS where, ACPI_SIZE length UNUSED) {
  return (void *)where;
}

void AcpiOsUnmapMemory (void *address UNUSED, ACPI_SIZE size UNUSED) {
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 *value, UINT32 width) {
  uint64_t full_value = *(uint64_t *)address;
  *value = (full_value & BOTTOM_N_BITS_ON(width));

  return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 value, UINT32 width) {
  switch (width) {
    case 64:
    *(uint64_t *)address = value;
    break;

    case 32:
    *(uint32_t *)address = value;
    break;

    case 16:
    *(uint16_t *)address = value;
    break;

    case 8:
    *(uint8_t *)address = value;
    break;
  }

  return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS address, UINT32 *value, UINT32 width) {
  switch (width) {
    case 32:
    *value = io_read_32(address);
    break;

    case 16:
    *value = io_read_16(address) & BOTTOM_N_BITS_ON(width);
    break;

    case 8:
    *value = io_read_8(address) & BOTTOM_N_BITS_ON(width);
    break;
  }

  return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS address, UINT32 value, UINT32 width) {
  switch (width) {
    case 32:
    io_write_32(address, value);
    break;

    case 16:
    io_write_16(address, value);
    break;

    case 8:
    io_write_8(address, value);
    break;
  }

  return AE_OK;
}

void AcpiOsWaitEventsComplete(void) {
  panic("AcpiOsWaitEventsComplete");
}

UINT64 AcpiOsGetTimer(void) {
  // Return system time in 100-nanosecond units
  return timer_ticks() * 10000000 / TIMER_FREQUENCY;
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *pci_id, UINT32 reg, UINT64 *value, UINT32 width) {
  uint32_t word = pci_config_read_word(pci_id->Bus, pci_id->Device, pci_id->Function, reg);
  uint32_t byte_offset_in_word = reg & ((1 << 2) - 1);
  *value = field_in_word(word, byte_offset_in_word, width >> 3);

  return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *pci_id, UINT32 reg, UINT64 value, UINT32 width) {
  uint32_t word = pci_config_read_word(pci_id->Bus, pci_id->Device, pci_id->Function, reg);
  uint32_t byte_offset_in_word = reg & ((1 << 2) - 1);

  uint32_t mask = FIELD_MASK(width, NUM_BITS(byte_offset_in_word));
  word = (word & ~mask) | (value << NUM_BITS(byte_offset_in_word));

  pci_config_write_word(pci_id->Bus, pci_id->Device, pci_id->Function, reg, word);

  return AE_OK;
}

// ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 interrupt_number, ACPI_OSD_HANDLER isr, void *context) {
//   if (!interrupt_slot_empty(interrupt_number)) return AE_BAD_PARAMETER;

//   interrupt_register_handler(interrupt_number, isr, context);

//   return AE_OK;
// }

// ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 interrupt_number, ACPI_OSD_HANDLER isr UNUSED) {
//   interrupt_remove_handler(interrupt_number);

//   return AE_OK;
// }

void AcpiOsPrintf(const char *format, ...) {
  va_list arg_list;
  va_start(arg_list, format);

  text_output_vprintf(format, arg_list);

  va_end(arg_list);
}

void AcpiOsVprintf(const char *format, va_list args) {
  text_output_vprintf(format, args);
}