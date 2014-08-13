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
  text_output_printf("\nDouble Fault -- Halting! Error Code: %d\n", error_code);
  __asm__ ("hlt");
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
  text_output_printf("\nGeneral Protection Fault! Error Code: %d\n", error_code);
}

static void page_fault(int error_code) {
  text_output_printf("\nPage Fault! Error Code: %d\n", error_code);
}

static void x87_fp_exeption() {
  text_output_print("\nx87 FPU Exception!\n");
}

static void alignment_check(int error_code) {
  text_output_printf("\nAlignment Check! Error Code: %d\n", error_code);
}

static void machine_check() {
  text_output_print("\nMachine Check Error -- Halting!\n");
  __asm__ ("hlt");
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


void exceptions_init() {
  // Setup interrupt handlers for all exceptions
  interrupts_register_handler(0, div_by_zero);
  interrupts_register_handler(1, debug);
  interrupts_register_handler(2, nmi);
  interrupts_register_handler(3, breakpoint);
  interrupts_register_handler(4, overflow);
  interrupts_register_handler(5, bound_range_exceeded);
  interrupts_register_handler(6, invalid_opcode);
  interrupts_register_handler(7, device_not_available);
  interrupts_register_handler(8, double_fault);
  interrupts_register_handler(10, invalid_tss);
  interrupts_register_handler(11, segment_not_present);
  interrupts_register_handler(12, stack_segment_fault);
  interrupts_register_handler(13, general_protection_fault);
  interrupts_register_handler(14, page_fault);
  interrupts_register_handler(16, x87_fp_exeption);
  interrupts_register_handler(17, alignment_check);
  interrupts_register_handler(18, machine_check);
  interrupts_register_handler(19, simd_fp_exception);
  interrupts_register_handler(20, virtualization_exception);
  interrupts_register_handler(30, security_exception);
}
