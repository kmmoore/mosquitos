#include "exception.h"
#include "interrupt.h"
#include "../util.h"

#include "../drivers/text_output.h"

static void div_by_zero() {
  text_output_print("\nDivision by Zero!\n");
}

static void debug() {
  text_output_print("\nDebug!\n");
}

static void nmi() {
  text_output_print("\nNon-Maskable Interrupt!\n");
}

static void breakpoint() {
  text_output_print("\nBreakpoint!\n");
}

static void overflow() {
  text_output_print("\nOverflow!\n");
}

static void bound_range_exceeded() {
  text_output_print("\nBound Range Exceeded!\n");
}

static void invalid_opcode(int error_code) {
  panic("\nInvalid Opcode! RIP: 0x%x\n", error_code);
}

static void device_not_available() {
  text_output_print("\nDevice Not Available!\n");
}

static void double_fault(int error_code) {
  panic("\nDouble Fault -- Halting! Error Code: %d\n", error_code);
  __asm__ ("cli; hlt");
}

static void invalid_tss(int error_code) {
  text_output_printf("\nInvalid TSS!\n Error Code: %d\n", error_code);
}

static void segment_not_present(int error_code) {
  text_output_printf("\nSegment Not Present! Error Code: %d\n", error_code);
}

static void stack_segment_fault(int error_code) {
  text_output_printf("\nStack Segment Fault! Error Code: %d\n", error_code);
}

static void general_protection_fault(int error_code) {
  panic("\nGeneral Protection Fault! Error Code: 0x%x\n", error_code);
}

static void page_fault(int error_code) {
  uint64_t cr2;
  __asm__ volatile("movq %%cr2, %0" : "=r" (cr2));
  panic("\nPage Fault! Error Code: 0x%x, cr2 0x%x\n", error_code, cr2);
}

static void x87_fp_exeption() {
  text_output_print("\nx87 FPU Exception!\n");
}

static void alignment_check(int error_code) {
  text_output_printf("\nAlignment Check! Error Code: %d\n", error_code);
}

static void machine_check() {
  text_output_print("\nMachine Check Error -- Halting!\n");
  __asm__ ("cli; hlt");
}

static void simd_fp_exception() {
  text_output_print("\nSIMD FPU Exception!\n");
}

static void virtualization_exception() {
  text_output_print("\nVirtualization Exception!\n");
}

static void security_exception() {
  text_output_print("\nSecurity Exception!\n");
}


void exception_init() {
  REQUIRE_MODULE("interrupt");

  // Setup interrupt handlers for all exceptions
  interrupt_register_handler(0, div_by_zero);
  interrupt_register_handler(1, debug);
  interrupt_register_handler(2, nmi);
  interrupt_register_handler(3, breakpoint);
  interrupt_register_handler(4, overflow);
  interrupt_register_handler(5, bound_range_exceeded);
  interrupt_register_handler(6, invalid_opcode);
  interrupt_register_handler(7, device_not_available);
  interrupt_register_handler(8, double_fault);
  interrupt_register_handler(10, invalid_tss);
  interrupt_register_handler(11, segment_not_present);
  interrupt_register_handler(12, stack_segment_fault);
  interrupt_register_handler(13, general_protection_fault);
  interrupt_register_handler(14, page_fault);
  interrupt_register_handler(16, x87_fp_exeption);
  interrupt_register_handler(17, alignment_check);
  interrupt_register_handler(18, machine_check);
  interrupt_register_handler(19, simd_fp_exception);
  interrupt_register_handler(20, virtualization_exception);
  interrupt_register_handler(30, security_exception);
}
