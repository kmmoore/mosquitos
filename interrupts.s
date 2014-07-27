
.macro save_context
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %rbx
    push %rax

.endm

.macro restore_context
    pop %rax
    pop %rbx
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
.endm


.text

.globl gpe_isr
gpe_isr:
  pop %rax
  mov $123, %rdx

  iretq

.globl isr1
isr1:

  # save_context

  # inb  $0x60, %al
  # movb $0x20, %al
  # outb %al, $0x20
  # call print_something

  # restore_context

  iretq
