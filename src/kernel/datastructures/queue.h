#include <kernel/kernel_common.h>

// Note: This datastructure is not thread safe

#ifndef _QUEUE_H
#define _QUEUE_H

typedef struct _Queue Queue;

typedef union {
  void *ptr;
  uint64_t u;
  int64_t i;
} QueueValue;

Queue * queue_alloc(int num_elements);
bool queue_enqueue(Queue *queue, QueueValue element, bool should_overwrite);
QueueValue queue_dequeue(Queue *queue); // Don't call unless queue_count(...) > 0
size_t queue_count(Queue *queue);

#endif
