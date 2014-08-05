#include "../kernel_common.h"
#include "../datastructures/list.h"

#ifndef _THREAD_H
#define _THREAD_H

typedef struct KernelThread KernelThread;
typedef void *(*KernelThreadMain) (void *);

KernelThread * thread_create(KernelThreadMain main_func, void * parameter, uint8_t priority, uint64_t stack_num_pages);

uint32_t thread_id(KernelThread *thread);
uint8_t thread_priority(KernelThread *thread);
bool thread_can_run(KernelThread *thread);

list_entry * thread_list_entry (KernelThread *thread);
KernelThread * thread_from_list_entry (list_entry *entry);
uint64_t * thread_register_list_pointer (KernelThread *thread);

// Functions that can be called by threads
void thread_exit();

// Functions that should not be called by threads
void thread_sleep(KernelThread *thread);
void thread_wake(KernelThread *thread);

#endif