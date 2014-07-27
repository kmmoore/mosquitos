#include <stdint.h>

#ifndef _PIC_H
#define _PIC_H

void pic_remap(int offset1, int offset2);
void pic_unmask_line(uint8_t line);
void pic_mask_line(uint8_t line);

#endif