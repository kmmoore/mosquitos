#include <kernel/kernel_common.h>

#ifndef _GDT_H
#define _GDT_H

#define GDT_KERNEL_CS 0x08
#define GDT_KERNEL_DS 0x10
#define GDT_TSS 0x18

void gdt_init();

#endif