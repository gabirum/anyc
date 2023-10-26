#ifndef _COLLECTION_QUEUE_H_
#define _COLLECTION_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>

#include "../function/defs.h"
#include "linked_list.h"

typedef struct queue
{
  llist_t *linked_list;
  size_t max_size;
} queue_t;

queue_t *queue_create(dispose_f);
queue_t *queue_create_fixed(size_t, dispose_f);

void queue_dispose(queue_t **);

bool queue_enqueue(queue_t *, void *);
void *queue_dequeue(queue_t *);

bool queue_is_empty(queue_t *);

void queue_clear(queue_t *);

#endif // _COLLECTION_QUEUE_H_
