.text

.extern GDTR
.globl gdt_flush
gdt_flush:
  lgdt (GDTR)         # Load GDT

  mov $0x18, %ax
  ltr %ax             # Load TSS

  movq %rsp, %rax
  pushq $0x10         # New SS at 16-bytes in to GDT
  pushq %rax          # New RSP
  pushfq              # New flags
  pushq $0x08         # New CS at 8-bytes in to GDT
  movabs $1f, %rax    # Load address of label 1:
  pushq %rax          # New RIP
  iretq               # Loads registers (the CS register cannot be set via mov) and jumps to 1:
1:
  ret
