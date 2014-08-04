#include "kernel_common.h"
#include "thread.h"

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

void scheduler_init();
void scheduler_register_thread(KernelThread *thread);
void scheduler_remove_thread(KernelThread *thread);
void scheduler_start_scheduling();

KernelThread * scheduler_current_thread();
void scheduler_yield();
void scheduler_yield_no_save();

#endif
