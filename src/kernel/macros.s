# Save/restore caller-saved registers
.macro save_context
    push %r11
    push %r10
    push %r9
    push %r8
    push %rdx
    push %rcx
    push %rax

.endm

.macro restore_context
    pop %rax
    pop %rcx
    pop %rdx
    pop %r8
    pop %r9
    pop %r10
    pop %r11
.endm
