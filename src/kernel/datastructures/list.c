#include "list.h"

// List entry type is opaque

struct _list_entry {
  struct _list_entry *next, *prev;
  uint64_t value;
};

void list_init(list *l) {
  l->head = l->tail = NULL;
}

list_entry *list_head(list *l) {
  return l->head;
}

list_entry *list_tail(list *l) {
  return l->tail;
}

uint64_t list_entry_value(list_entry *entry) {
  return entry->value;
}

void list_entry_set_value(list_entry *entry, uint64_t value) {
  entry->value = value;
}

list_entry *list_next(list_entry *entry) {
  return entry->next;
}

list_entry *list_prev(list_entry *entry) {
  return entry->prev;
}

void list_insert_before(list *l, list_entry *before, list_entry *new) {
  if (before == NULL) {
    l->head = new;
    l->tail = new;
    return;
  }

  new->next = before;
  new->prev = before->prev;
  before->prev = new;

  if (new->prev) {
    new->prev->next = new;
  }

  if (before == l->head) {
    l->head = new;
  }
}

void list_insert_after(list *l, list_entry *after, list_entry *new) {
  if (after == NULL) {
    l->head = new;
    l->tail = new;
    return;
  }

  new->prev = after;
  new->next = after->next;
  after->next = new;

  if (new->next) {
    new->next->prev = new;
  }

  if (after == l->tail) {
    l->tail = new;
  }
}

void list_remove(list *l, list_entry *to_remove) {
  if (to_remove->prev) {
    to_remove->prev->next = to_remove->next;
  }

  if (to_remove->next) {
    to_remove->next->prev = to_remove->prev;
  }

  if (to_remove == l->head) {
    l->head = to_remove->next;
  }
  if (to_remove == l->tail) {
    l->tail = to_remove->prev;
  }
}
