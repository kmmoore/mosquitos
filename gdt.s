.text

.extern GDTR
.globl gdt_flush
gdt_flush:
  lgdt (GDTR)         # Load GDT

  movq %rsp, %rax
  pushq $0x10         # New SS
  pushq %rax          # New RSP
  pushfq              # New flags
  pushq $0x08         # New CS
  movabs $1f, %rax    # Load address of label 1:
  pushq %rax          # New RIP
  iretq               # Loads registers and jumps to 1:
1:
  ret
