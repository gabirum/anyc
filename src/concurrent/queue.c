#include "queue.h"

#include <stdlib.h>

static sync_queue_t *_new_queue(size_t size, dispose_f dispose)
{
  sync_queue_t *queue = malloc(sizeof(sync_queue_t));

  queue->queue = size == -1 ? queue_create(dispose) : queue_create_fixed(size, dispose);

  queue->stop_wait = false;
  queue->waiting = 0;

  mtx_init(&queue->mtx, mtx_plain);
  cnd_init(&queue->item_cnd);
  cnd_init(&queue->waiting_cnd);

  return queue;
}

sync_queue_t *squeue_create(dispose_f dispose)
{
  return _new_queue(-1, dispose);
}

sync_queue_t *squeue_create_fixed(size_t size, dispose_f dispose)
{
  return _new_queue(size, dispose);
}

void squeue_dispose(sync_queue_t **_queue)
{
  sync_queue_t queue = **_queue;
  mtx_lock(&queue.mtx);

  queue.stop_wait = true;
  cnd_broadcast(&queue.item_cnd);

  while (queue.waiting > 0)
    cnd_wait(&queue.waiting_cnd, &queue.mtx);

  queue_dispose(&queue.queue);

  cnd_destroy(&queue.item_cnd);
  cnd_destroy(&queue.waiting_cnd);
  mtx_destroy(&queue.mtx);

  free(*_queue);
  *_queue = NULL;
}

bool squeue_enqueue(sync_queue_t *queue, void *value)
{
  mtx_lock(&queue->mtx);

  bool result = queue_enqueue(queue->queue, value);

  if (result)
    cnd_signal(&queue->item_cnd);

  mtx_unlock(&queue->mtx);

  return result;
}

void *squeue_dequeue(sync_queue_t *queue)
{
  mtx_lock(&queue->mtx);

  void *dequeued = queue_dequeue(queue->queue);

  mtx_unlock(&queue->mtx);

  return dequeued;
}

void *squeue_wait_dequeue(sync_queue_t *queue)
{
  mtx_lock(&queue->mtx);

  queue->waiting++;
  while (queue_is_empty(queue->queue))
  {
    cnd_wait(&queue->item_cnd, &queue->mtx);

    if (queue->stop_wait)
    {
      queue->waiting--;
      cnd_broadcast(&queue->waiting_cnd);

      mtx_unlock(&queue->mtx);

      return NULL;
    }
  }
  queue->waiting--;

  void *dequeued = queue_dequeue(queue->queue);

  mtx_unlock(&queue->mtx);

  return dequeued;
}

bool squeue_is_empty(sync_queue_t *queue)
{
  mtx_lock(&queue->mtx);

  bool result = queue_is_empty(queue->queue);

  mtx_unlock(&queue->mtx);

  return result;
}

void squeue_clear(sync_queue_t *queue)
{
  mtx_lock(&queue->mtx);
  queue_clear(queue->queue);
  mtx_unlock(&queue->mtx);
}
