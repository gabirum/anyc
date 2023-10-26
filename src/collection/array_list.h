#ifndef _COLLECTION_ARRAYLIST_H_
#define _COLLECTION_ARRAYLIST_H_

#include <stddef.h>

#include "../function/defs.h"

#define ARRAY_LIST_DEFAULT_SIZE 10

typedef struct alist
{
  void **data;
  dispose_f dispose;
  bool is_fixed;
  size_t capacity, position, size;
} alist_t;

alist_t *alist_create(dispose_f);
alist_t *alist_create_init_cap(size_t, dispose_f);
alist_t *alist_create_fixed(size_t, dispose_f);
void alist_dispose(alist_t **);

bool alist_increase_capacity(alist_t *, size_t);

void *alist_get(alist_t *, size_t);
bool alist_set(alist_t *, size_t, void *);
bool alist_add(alist_t *, void *);
void alist_remove(alist_t *, size_t);

void alist_remove_item(alist_t *, void *);

size_t alist_find(alist_t *, predicate_f, void *);
void *alist_find_item(alist_t *, predicate_f, void *);

bool alist_every(alist_t *, predicate_f, void *);
bool alist_some(alist_t *, predicate_f, void *);
void alist_for_each(alist_t *, consumer_f, void *);

void alist_clear(alist_t **);

#endif // _COLLECTION_ARRAYLIST_H_
