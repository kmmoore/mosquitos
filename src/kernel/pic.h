#include "kernel_common.h"

#ifndef _PIC_H
#define _PIC_H

void pic_init();
void pic_unmask_line(uint8_t line);
void pic_mask_line(uint8_t line);

#endif