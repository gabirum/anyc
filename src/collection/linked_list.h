#ifndef _COLLECTION_LINKEDLIST_H_
#define _COLLECTION_LINKEDLIST_H_

#include <stddef.h>

#include "../function/defs.h"

typedef struct ll_node
{
  void *value;
  struct ll_node *next;
} ll_node_t;

typedef struct llist
{
  ll_node_t *head, *tail;
  dispose_f dispose;
  size_t size;
} llist_t;

llist_t *llist_create(dispose_f);
void llist_dispose(llist_t **);

void *llist_get_first(llist_t *);
void *llist_get_last(llist_t *);

void llist_add_first(llist_t *, void *);
void llist_add_last(llist_t *, void *);

void llist_rm(llist_t *, void *);
void *llist_rm_first(llist_t *);
void *llist_rm_last(llist_t *);
void llist_drm_first(llist_t *);
void llist_drm_last(llist_t *);

void *llist_find(llist_t *, predicate_f, void *);

bool llist_every(llist_t *, predicate_f, void *);
bool llist_some(llist_t *, predicate_f, void *);
void llist_for_each(llist_t *, consumer_f, void *);

void llist_clear(llist_t **);

#endif // _COLLECTION_LINKEDLIST_H_
