#include "queue.h"

#include <stdlib.h>

static queue_t *_new_queue(size_t size, dispose_f dispose)
{
  queue_t *queue = malloc(sizeof(queue_t));

  queue->linked_list = llist_create(dispose);
  queue->max_size = size;

  return queue;
}

queue_t *queue_create(dispose_f dispose)
{
  return _new_queue(-1, dispose);
}

queue_t *queue_create_fixed(size_t max_size, dispose_f dispose)
{
  return _new_queue(max_size, dispose);
}

void queue_dispose(queue_t **queue)
{
  llist_dispose(&(*queue)->linked_list);
  free(*queue);
  *queue = NULL;
}

bool queue_enqueue(queue_t *queue, void *value)
{
  if (queue->linked_list->size >= queue->max_size)
    return false;

  llist_add_last(queue->linked_list, value);
  return true;
}

void *queue_dequeue(queue_t *queue)
{
  return llist_rm_first(queue->linked_list);
}

bool queue_is_empty(queue_t *queue)
{
  return queue->linked_list->size == 0;
}

void queue_clear(queue_t *queue)
{
  llist_clear(&queue->linked_list);
}
