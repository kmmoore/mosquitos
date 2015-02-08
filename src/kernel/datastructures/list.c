#include <kernel/datastructures/list.h>

void list_init(List *l) {
  l->head = l->tail = NULL;
}

ListEntry *list_head(List *l) {
  return l->head;
}

ListEntry *list_tail(List *l) {
  return l->tail;
}

uint64_t list_entry_value(ListEntry *entry) {
  return entry->value;
}

void list_entry_set_value(ListEntry *entry, uint64_t value) {
  entry->value = value;
}

ListEntry *list_next(ListEntry *entry) {
  if (!entry) return NULL;
  return entry->next;
}

ListEntry *list_prev(ListEntry *entry) {
  if (!entry) return NULL;
  return entry->prev;
}

void list_push_front(List *l, ListEntry *new) {
  list_insert_before(l, list_head(l), new);
}

void list_push_back(List *l, ListEntry *new) {
  list_insert_after(l, list_tail(l), new);
}

void list_insert_before(List *l, ListEntry *before, ListEntry *new) {
  if (before == NULL) {
    new->prev = new->next = NULL;
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

void list_insert_after(List *l, ListEntry *after, ListEntry *new) {
  if (after == NULL) {
    new->prev = new->next = NULL;
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

void list_remove(List *l, ListEntry *to_remove) {
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
