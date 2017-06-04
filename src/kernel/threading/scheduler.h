#include <kernel/kernel_common.h>
#include <kernel/threading/thread.h>

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

void scheduler_init();
void scheduler_register_thread(KernelThread *thread);
void scheduler_unschedule_thread(KernelThread *thread);
// Removes the current thread from scheduling, *WITHOUT* saving it on the next
// context switch.
KernelThread *scheduler_remove_current_thread();
void scheduler_start_scheduling();

KernelThread *scheduler_current_thread();
void scheduler_yield();

#endif
