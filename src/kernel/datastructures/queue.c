#include <kernel/datastructures/queue.h>

#include <kernel/memory/kmalloc.h>

struct _Queue {
  size_t max_size;
  size_t enqueue_idx, dequeue_idx;
  bool last_operation_was_write;
  QueueValue buffer[0];
};

static inline size_t circular_increment(size_t index, size_t wrap_at) {
  return (index + 1) % wrap_at;
}

static inline bool queue_is_full(Queue *queue) {
  return queue->last_operation_was_write && queue->enqueue_idx == queue->dequeue_idx;
}

static inline bool queue_is_empty(Queue *queue) {
  return !queue->last_operation_was_write && queue->enqueue_idx == queue->dequeue_idx;
}

Queue * queue_alloc(int num_elements) {
  Queue *queue = kmalloc(sizeof(Queue) + num_elements * sizeof(QueueValue));
  queue->max_size = num_elements;
  queue->enqueue_idx = queue->dequeue_idx = 0;
  queue->last_operation_was_write = false;

  return queue;
}

bool queue_enqueue(Queue *queue, QueueValue element, bool should_overwrite) {
  if (queue_is_full(queue)) {
    if (!should_overwrite) {
      return false; // Not enough space
    } else {
      queue->dequeue_idx = circular_increment(queue->dequeue_idx, queue->max_size);
    }
  }

  queue->buffer[queue->enqueue_idx] = element;
  queue->enqueue_idx = circular_increment(queue->enqueue_idx, queue->max_size);
  queue->last_operation_was_write = true;

  return true;
}

QueueValue queue_dequeue(Queue *queue) {
  assert(!queue_is_empty(queue));

  QueueValue to_return = queue->buffer[queue->dequeue_idx];

  queue->dequeue_idx = circular_increment(queue->dequeue_idx, queue->max_size);
  queue->last_operation_was_write = false;

  return to_return;
}

size_t queue_count(Queue *queue) {
  if (queue_is_empty(queue)) return 0;

  if (queue->enqueue_idx > queue->dequeue_idx) {
    return queue->enqueue_idx - queue->dequeue_idx;
  } else {
    return queue->max_size - queue->dequeue_idx + queue->enqueue_idx;
  }
}