.text

.globl isr1
isr1:
  ret

.data
msg:
  .ascii  "ISR!\n"
