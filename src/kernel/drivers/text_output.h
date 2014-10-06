#include "../kernel_common.h"

#include <efi.h>
#include <efilib.h>

#include <stdarg.h>

#ifndef _TEXT_OUTPUT_H
#define _TEXT_OUTPUT_H

void text_output_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);
void text_output_clear_screen(uint32_t color);
void text_output_backspace();
void text_output_putchar(const char c);
void text_output_print(const char *str);
int text_output_printf(const char *format, ...);
int text_output_vprintf(const char *format, va_list arg_list);

void text_output_safe_printf(const char *format, ...);
void text_output_safe_vprintf(const char *format, va_list arg_list);

#endif