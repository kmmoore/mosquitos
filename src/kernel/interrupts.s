
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

.extern interrupts_handlers
.extern isr_common

# A macro with two parameters
#  implements the write system call
.macro isr_noerror num 
.globl isr\num
isr\num:
  save_context

    # TODO: Inline the isr_common call
    movq $0, %rsi
    movq $\num, %rdi
    call isr_common

    restore_context

    iretq
.endm

.macro isr_error num 
.globl isr\num
isr\num:
  save_context

    # TODO: Inline the isr_common call
    popq %rsi
    movq $\num, %rdi
    call isr_common

    restore_context

    iretq
.endm

# Actual isr definitions
isr_noerror 0
isr_noerror 1
isr_noerror 2
isr_noerror 3
isr_noerror 4
isr_noerror 5
isr_noerror 6
isr_noerror 7
isr_error 8
isr_noerror 9
isr_error 10
isr_error 11
isr_error 12
isr_error 13
isr_error 14
isr_noerror 16
isr_error 17
isr_noerror 18
isr_noerror 19
isr_noerror 20
isr_noerror 30
isr_noerror 33
