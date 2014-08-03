#include "../kernel_common.h"

#ifndef _LIST_H
#define _LIST_H

typedef struct _list list;
typedef struct _list_entry list_entry;

struct _list {
  struct _list_entry *head;
  struct _list_entry *tail;
};

struct _list_entry {
  struct _list_entry *next, *prev;
  uint64_t value;
};

// TODO: We need a way to tell the client how large a list entry is

void list_init(list *l);

list_entry *list_head(list *l);
list_entry *list_tail(list *l);

uint64_t list_entry_value(list_entry *entry);
void list_entry_set_value(list_entry *entry, uint64_t value);
list_entry *list_next(list_entry *entry);
list_entry *list_prev(list_entry *entry);

/* In the following functions, `before`, `after`, and
   `to_remove` must be in `l`. */
void list_insert_before(list *l, list_entry *before, list_entry *new);
void list_insert_after(list *l, list_entry *after, list_entry *new);
void list_remove(list *l, list_entry *to_remove);

#endif
