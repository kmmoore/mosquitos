#include "../kernel_common.h"

#ifndef _LIST_H
#define _LIST_H

typedef struct _list_entry {
  struct _list_entry *next, *prev;
  uint64_t value;
} list_entry;

list_entry * list_insert_before(list_entry *before, list_entry *new);
list_entry * list_insert_after(list_entry *after, list_entry *new);
list_entry * list_remove(list_entry *to_remove);

#endif
