#include <kernel/kernel_common.h>

#include <efi.h>
#include <efilib.h>

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

void graphics_init(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop);
void graphics_clear_screen(uint32_t color);
void graphics_fill_rect(int x, int y, int w, int h, uint32_t color);
void graphics_draw_pixel(int x, int y, uint32_t color);

#endif