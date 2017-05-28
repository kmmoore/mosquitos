#include <kernel/kernel_common.h>

#ifndef _LIST_H
#define _LIST_H

typedef struct _List List;
typedef struct _ListEntry ListEntry;

struct _List {
  struct _ListEntry *head;
  struct _ListEntry *tail;
};

struct _ListEntry {
  struct _ListEntry *next, *prev;
  uint64_t value;
};

void list_init(List *l);

ListEntry *list_head(List *l);
ListEntry *list_tail(List *l);

#define ListEntry_cast(entry, type) ((type)(ListEntry_value(entry)))

uint64_t list_entry_value(ListEntry *entry);
void list_entry_set_value(ListEntry *entry, uint64_t value);
ListEntry *list_next(ListEntry *entry);
ListEntry *list_prev(ListEntry *entry);

/* In the following functions, `before`, `after`, and
   `to_remove` must be in `l`. */
void list_push_front(List *l, ListEntry *new);
void list_push_back(List *l, ListEntry *new);
void list_insert_before(List *l, ListEntry *before, ListEntry *new);
void list_insert_after(List *l, ListEntry *after, ListEntry *new);
void list_remove(List *l, ListEntry *to_remove);

#endif
