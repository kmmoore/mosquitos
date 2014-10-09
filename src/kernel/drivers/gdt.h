#include "../kernel_common.h"

#ifndef _GDT_H
#define _GDT_H

#define GDT_KERNEL_CS 0x08
#define GDT_KERNEL_DS 0x10
#define GDT_USER_CS 0x18
#define GDT_USER_DS 0x20

void gdt_init();

#endif