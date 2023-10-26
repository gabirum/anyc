#ifndef _CONCURRENT_QUEUE_H_
#define _CONCURRENT_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>
#include <threads.h>

#include "../function/defs.h"
#include "../collection/queue.h"

typedef struct sync_queue
{
  queue_t *queue;
  mtx_t mtx;
  cnd_t item_cnd, waiting_cnd;
  bool stop_wait;
  size_t waiting;
} sync_queue_t;

sync_queue_t *squeue_create(dispose_f);
sync_queue_t *squeue_create_fixed(size_t, dispose_f);

void squeue_dispose(sync_queue_t **);

bool squeue_enqueue(sync_queue_t *, void *);
void *squeue_dequeue(sync_queue_t *);
void *squeue_wait_dequeue(sync_queue_t *);

bool squeue_is_empty(sync_queue_t *);

void squeue_clear(sync_queue_t *);

#endif // _CONCURRENT_QUEUE_H_
