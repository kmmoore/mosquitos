#include <kernel/kernel_common.h>

#include <stdarg.h>

#ifndef _TEXT_OUTPUT_H
#define _TEXT_OUTPUT_H

void text_output_init();
void text_output_set_background_color(uint32_t color);
void text_output_set_foreground_color(uint32_t color);
uint32_t text_output_get_background_color();
uint32_t text_output_get_foreground_color();
void text_output_clear_screen();
void text_output_backspace();
void text_output_putchar(const char c);
void text_output_print(const char *str);
int text_output_printf(const char *format, ...);
int text_output_vprintf(const char *format, va_list arg_list);

void text_output_safe_printf(const char *format, ...);
void text_output_safe_vprintf(const char *format, va_list arg_list);

#endif