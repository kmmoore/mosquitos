#include "acpi.h"
#include "../util.h"
#include "../drivers/text_output.h"
#include "../../common/mem_util.h"

#include <acpi.h>

#define ACPI_MAX_INIT_TABLES 16

static struct {
  uint64_t xdsp_address;
  ACPI_TABLE_DESC acpi_tables[ACPI_MAX_INIT_TABLES];
} acpi;

uint64_t acpi_xdsp_address() {
  return acpi.xdsp_address;
}

static bool acpica_enable_apic_mode() {
  ACPI_OBJECT_LIST        Params;
  ACPI_OBJECT             Obj;

  Params.Count = 1;
  Params.Pointer = &Obj;
  
  Obj.Type = ACPI_TYPE_INTEGER;
  Obj.Integer.Value = 1;     // 0 = PIC, 1 = APIC

  ACPI_STATUS status = AcpiEvaluateObject(NULL, "\\_PIC", &Params, NULL);
  if (ACPI_FAILURE(status)) {
    text_output_printf("Could not enable APIC mode %s\n", AcpiFormatException(status));
    return false;
  }

  return true;
}

static bool acpica_early_init() {
  ACPI_STATUS status;

  if (ACPI_FAILURE(status = AcpiInitializeTables(acpi.acpi_tables, ACPI_MAX_INIT_TABLES, FALSE))) {
    text_output_printf("ACPI INIT tables failure %s\n", AcpiFormatException(status));
    return false;
  }

  return true;
}

static bool acpica_enable() {
  REQUIRE_MODULE("acpi_early");
  REQUIRE_MODULE("virtual_memory");
  REQUIRE_MODULE("scheduler");

  ACPI_STATUS status ;

  if (ACPI_FAILURE(status = AcpiInitializeSubsystem())) {
    text_output_printf("ACPI INIT failure %s\n", AcpiFormatException(status));
    return false;
  }

  if (ACPI_FAILURE(status = AcpiReallocateRootTable())) {
    text_output_printf("ACPI REALLOC failure %s\n", AcpiFormatException(status));
    return false;
  }

  if (ACPI_FAILURE(status = AcpiLoadTables())) {
    text_output_printf("ACPI load tables failure %s\n", AcpiFormatException(status));
    return false;
  }

  if (ACPI_FAILURE(status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION))) {
    text_output_printf("ACPI enable failure %s\n", AcpiFormatException(status));
    return false;
  }

  if (!acpica_enable_apic_mode()) return false;

  if (ACPI_FAILURE(status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION))) {
    text_output_printf("ACPI init objects failure %s\n", AcpiFormatException(status));
    return false;
  }

  return true;
}

ACPISDTHeader * acpi_locate_table(char *name) {
  ACPI_TABLE_HEADER *header;
  ACPI_STATUS status = AcpiGetTable(name, 1, &header);
  if (status != AE_OK) text_output_printf("AcpiGetTable status: %d\n", status);
  assert(status == AE_OK);

  return (ACPISDTHeader *)header;
}

void acpi_init(void *xdsp_address) {
  REQUIRE_MODULE("text_output");

  acpi.xdsp_address = (uint64_t)xdsp_address;

  assert(acpica_early_init());

  REGISTER_MODULE("acpi_early");
}

void acpi_enable_acpica() {
  assert(acpica_enable());

  REGISTER_MODULE("acpi_full");
}
