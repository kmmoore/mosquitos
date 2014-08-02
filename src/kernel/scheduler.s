.text

# NOTE: Should be called from the end of an interrupt handler
# instead of iretq
.globl scheduler_load_thread
scheduler_load_thread:
  # rdi contains the address of ss in the KernelThread struct
  push  0x00(%rdi) # ss
  push  0x08(%rdi) # rsp
  push  0x10(%rdi) # rflags
  push  0x18(%rdi) # cs
  push  0x20(%rdi) # rip

  #uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
  #uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  # uint64_t ds, es, fs, gs;

  mov   0x28(%rdi), %rax
  mov   0x30(%rdi), %rbx
  mov   0x38(%rdi), %rcx
  mov   0x40(%rdi), %rdx
  mov   0x48(%rdi), %rsi
  # Skip rdi for the moment, becuase we're using it
  mov   0x58(%rdi), %rbp

  mov   0x60(%rdi), %r8
  mov   0x68(%rdi), %r9
  mov   0x70(%rdi), %r10
  mov   0x78(%rdi), %r11
  mov   0x80(%rdi), %r12
  mov   0x88(%rdi), %r13
  mov   0x90(%rdi), %r14
  mov   0x98(%rdi), %r15

  mov   0xA0(%rdi), %ds
  mov   0xA8(%rdi), %es
  mov   0xB0(%rdi), %fs
  mov   0xB8(%rdi), %gs

  # Set rdi now that we're done with it
  mov   0x50(%rdi), %rdi

  iretq

# NOTE: Should be called from inside an interrupt handler
# Assumes ss, rsp, rflags, cs, and rip are pushed on the
# stack. WILL remove these values (must be restored with
# scheduler_load_thread())
# NOTE: The caller MUST push rdi to the stack beforehand
.globl scheduler_save_thread
scheduler_save_thread:
  # rdi contains the address of ss in the KernelThread struct
  # The previous thread's rdi is pushed to the stack

  pop   %rax
  mov   %rax, 0x50(%rdi) # rdi

  pop   %rax
  mov   %rax, 0x20(%rdi) # rip
  pop   %rax
  mov   %rax, 0x18(%rdi) # cs
  pop   %rax
  mov   %rax, 0x10(%rdi) # rflags
  pop   %rax
  mov   %rax, 0x08(%rdi) # rsp
  pop   %rax
  mov   %rax, 0x00(%rdi) # ss

  #uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
  #uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  # uint64_t ds, es, fs, gs;

  mov   %rax, 0x28(%rdi)
  mov   %rbx, 0x30(%rdi)
  mov   %rcx, 0x38(%rdi)
  mov   %rdx, 0x40(%rdi)
  mov   %rsi, 0x48(%rdi)
  # Skip rdi for the moment, becuase we're using it
  mov   %rbp, 0x58(%rdi)

  mov   %r8, 0x60(%rdi)
  mov   %r9, 0x68(%rdi)
  mov   %r10, 0x70(%rdi)
  mov   %r11, 0x78(%rdi)
  mov   %r12, 0x80(%rdi)
  mov   %r13, 0x88(%rdi)
  mov   %r14, 0x90(%rdi)
  mov   %r15, 0x98(%rdi)

  mov   %ds, 0xA0(%rdi)
  mov   %es, 0xA8(%rdi)
  mov   %fs, 0xB0(%rdi)
  mov   %gs, 0xB8(%rdi)

  # Set rdi now that we're done with it
  mov   0x50(%rdi), %rdi

  iretq
