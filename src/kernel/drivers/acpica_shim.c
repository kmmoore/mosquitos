#include "../kernel_common.h"
#include "../util.h"

#include "text_output.h"
#include "../hardware/acpi.h"
#include "../memory/kmalloc.h"
#include "../threading/mutex/semaphore.h"
#include "../threading/mutex/lock.h"

#include <acpi/acpi.h>

acpi_status acpi_os_initialize(void) {
  text_output_printf("ACPI OS Initialize!\n");

  return AE_OK;
}

acpi_status acpi_os_terminate(void) {
  text_output_printf("ACPI OS Terminate!\n");

  return AE_OK;
}

acpi_physical_address acpi_os_get_root_pointer(void) {
  return acpi_xdsp_address();
}

acpi_status acpi_os_predefined_override(const struct acpi_predefined_names *init_val UNUSED, acpi_string * new_val) {
  *new_val = 0;

  return AE_OK;
}

acpi_status acpi_os_table_override(struct acpi_table_header *existing_table UNUSED, struct acpi_table_header **new_table) {
  *new_table = 0;

  return AE_OK;
}

acpi_status acpi_os_physical_table_override(struct acpi_table_header *existing_table UNUSED, acpi_physical_address * new_address, u32 *new_table_length UNUSED) {

  *new_address = 0;

  return AE_OK;
}

acpi_status acpi_os_create_lock(acpi_spinlock * out_handle) {
  *out_handle = kmalloc(sizeof(SpinLock));
  if (*out_handle == NULL) return AE_NO_MEMORY;

  spinlock_init(*out_handle);

  return AE_OK;
}

void acpi_os_delete_lock(acpi_spinlock handle) {
  kfree(handle);
}

acpi_cpu_flags acpi_os_acquire_lock(acpi_spinlock handle) {
  spinlock_acquire(handle);

  return 0;
}

void acpi_os_release_lock(acpi_spinlock handle, acpi_cpu_flags flags) {
  (void)flags;
  spinlock_release(handle);
}

acpi_status acpi_os_create_semaphore(u32 max_units UNUSED, u32 initial_units, acpi_semaphore * out_handle) {
  *out_handle = kmalloc(sizeof(Semaphore));
  if (*out_handle == NULL) return AE_NO_MEMORY;

  semaphore_init(*out_handle, initial_units);

  return AE_OK;
}


