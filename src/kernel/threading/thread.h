#include <kernel/datastructures/list.h>
#include <kernel/kernel_common.h>

#ifndef _THREAD_H
#define _THREAD_H

typedef struct KernelThread KernelThread;
typedef void *(*KernelThreadMain)(void *);

typedef enum {
  THREAD_RUNNING,
  THREAD_SLEEPING,
  THREAD_EXITED
} KernelThreadStatus;

// Priority is in the range [0, 31]. Higher priority threads run before lower
// priority threads. The idle thread runs at priority (0), so any thread that
// does work should be priority > 0.
KernelThread *thread_create(KernelThreadMain main_func, void *parameter,
                            uint8_t priority, uint64_t stack_num_pages);

uint32_t thread_id(KernelThread *thread);
uint8_t thread_priority(KernelThread *thread);
uint8_t thread_status(KernelThread *thread);
bool thread_can_run(KernelThread *thread);

ListEntry *thread_list_entry(KernelThread *thread);
KernelThread *thread_from_list_entry(ListEntry *entry);
uint64_t *thread_register_list_pointer(KernelThread *thread);

// Functions that can be called by threads
void thread_exit();

// Functions that should not be called by threads
void thread_start(KernelThread *thread);
void thread_sleep(KernelThread *thread);
void thread_wake(KernelThread *thread);

#endif
