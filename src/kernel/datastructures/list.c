#include "list.h"

list_entry * list_insert_before(list_entry *before, list_entry *new) {
  new->next = before;
  if (new->next) {
    new->next->prev = new;
    new->prev = before->prev;
  } else {
    new->prev = NULL;
  }

  if (new->prev) {
    new->prev->next = new;
  }

  return new;
}

list_entry * list_insert_after(list_entry *after, list_entry *new) {
  new->prev = after;
  if (new->prev) {
    new->prev->next = new;
    new->next = after->next;
  } else {
    new->next = NULL;
  }

  if (new->next) {
    new->next->prev = new;
  }
  return new;
}

list_entry * list_remove(list_entry *to_remove) {
  if (to_remove->prev) {
    to_remove->prev->next = to_remove->next;
  }

  if (to_remove->next) {
    to_remove->next->prev = to_remove->prev;
  }

  return to_remove;
}
