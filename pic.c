#include <stdint.h>

#include "pic.h"
#include "util.h"
#include "text_output.h"

#define PIC1    0x20    /* IO base address for master PIC */
#define PIC2    0xA0    /* IO base address for slave PIC */
#define PIC1_COMMAND  PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND  PIC2
#define PIC2_DATA (PIC2+1)
#define PIC_EOI   0x20    /* End-of-interrupt command code */

#define ICW1_ICW4 0x01    /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL  0x08    /* Level triggered (edge) mode */
#define ICW1_INIT 0x10    /* Initialization - required! */
 
#define ICW4_8086 0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM 0x10    /* Special fully nested (not) */

void pic_init() {
  unsigned char a1, a2;
   
    a1 = inb(PIC1_DATA);                        // save masks
    a2 = inb(PIC2_DATA);
   
    outb(PIC1_COMMAND, ICW1_INIT+ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    outb(PIC2_COMMAND, ICW1_INIT+ICW1_ICW4);
    outb(PIC1_DATA, 0x20);                 // ICW2: Master PIC vector offset
    outb(PIC2_DATA, 0x28);                 // ICW2: Slave PIC vector offset
    outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
   
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
   
    outb(PIC1_DATA, a1);   // restore saved masks.
    outb(PIC2_DATA, a2);
}

void pic_unmask_line(uint8_t line) {
    uint16_t port;
    uint8_t value;
 
    if(line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }
    value = inb(port) | (1 << line);
    outb(port, value);        
}
 
void pic_mask_line(uint8_t line) {
    uint16_t port;
    uint8_t value;
 
    if(line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        line -= 8;
    }
    value = inb(port) & ~(1 << line);
    outb(port, value);        
}