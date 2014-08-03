#include "kernel_common.h"

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

typedef struct KernelThread KernelThread;
typedef void *(*KernelThreadMain) (void *);

void scheduler_init();
KernelThread * scheduler_create_thread(KernelThreadMain main_func, void * parameter, uint8_t priority);
void scheduler_register_thread(KernelThread *thread);
void scheduler_start_scheduling();

#endif
