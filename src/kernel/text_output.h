#include "kernel_common.h"

#include <efi.h>
#include <efilib.h>

#include <stdarg.h>

#ifndef _TEXT_OUTPUT_H
#define _TEXT_OUTPUT_H


void text_output_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);
void text_output_clear_screen(uint32_t color);
void text_output_backspace();
void text_output_putchar(char c);
void text_output_print(char *str);
void text_output_printf(char *format, ...);
void text_output_vprintf(char *format, va_list arg_list);

#endif