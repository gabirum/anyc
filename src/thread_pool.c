#include "thread_pool.h"

#include <stdlib.h>

static int worker(void *_pool)
{
  tpool_t *pool = _pool;

  atomic_fetch_add(&pool->thread_count, 1);

  while (!atomic_load(&pool->stop))
  {
    mtx_lock(&pool->mutex);

    if (pool->queue_size == 0)
    {
      cnd_wait(&pool->queue_item_cnd, &pool->mutex);
      mtx_unlock(&pool->mutex);
      continue;
    }

    task_node_t *task_node = pool->head;
    pool->head = task_node->next;
    pool->queue_size--;

    mtx_unlock(&pool->mutex);

    switch (task_node->task_type)
    {
    case TASK:
    {
      task_t t = task_node->task.task;
      t.execute(t.arg, t.arg_cleanup);
      free(task_node);
      break;
    }

    case FUTURE:
    {
      future_t *pf = &task_node->task.future;
      pf->result = pf->call(pf->arg, pf->arg_cleanup);
      pf->completed = true;
      break;
    }

    default:
      free(task_node);
      break;
    }

    cnd_signal(&pool->task_end_cnd);
  }

  atomic_fetch_sub(&pool->thread_count, 1);
  cnd_signal(&pool->worker_die_cnd);

  return 0;
}

tpool_t *tpool_create(size_t size)
{
  tpool_t *pool = malloc(sizeof(tpool_t));

  mtx_init(&pool->mutex, mtx_plain);
  cnd_init(&pool->worker_die_cnd);
  cnd_init(&pool->task_end_cnd);
  cnd_init(&pool->queue_item_cnd);

  atomic_init(&pool->stop, false);
  atomic_init(&pool->thread_count, 0);

  pool->max_threads = size;
  pool->head = NULL;
  pool->tail = NULL;
  pool->queue_size = 0;

  pool->workers = calloc(size, sizeof(thrd_t));

  for (size_t i = 0; i < size; i++)
    thrd_create(&pool->workers[i], worker, pool);

  return pool;
}

static void cleanup_task_node_arg(task_node_t *task_node)
{
  switch (task_node->task_type)
  {
  case TASK:
    if (task_node->task.task.arg_cleanup != NULL)
      task_node->task.task.arg_cleanup(task_node->task.task.arg);
    break;
  case FUTURE:
    if (task_node->task.future.arg_cleanup != NULL)
      task_node->task.future.arg_cleanup(task_node->task.future.arg);
    break;

  default:
    break;
  }
}

void tpool_cleanup(tpool_t **p_pool)
{
  tpool_t *pool = *p_pool;

  atomic_store(&pool->stop, true);

  mtx_lock(&pool->mutex);
  cnd_broadcast(&pool->queue_item_cnd);

  while (atomic_load(&pool->thread_count) > 0)
    cnd_wait(&pool->worker_die_cnd, &pool->mutex);

  cnd_destroy(&pool->queue_item_cnd);
  cnd_destroy(&pool->worker_die_cnd);
  cnd_destroy(&pool->task_end_cnd);
  mtx_destroy(&pool->mutex);

  task_node_t *node = pool->head;

  if (node != NULL)
    while (node->next != NULL)
    {
      task_node_t *aux = node->next;

      cleanup_task_node_arg(node);
      free(node);

      node = aux;
    }

  free(pool->workers);
  free(*p_pool);
  *p_pool = NULL;
}

static void enqueue_task(tpool_t *pool, task_node_t *task_node)
{
  mtx_lock(&pool->mutex);

  if (pool->queue_size == 0)
  {
    pool->head = pool->tail = task_node;
    goto end;
  }

  pool->tail->next = task_node;
  pool->tail = pool->tail->next;

end:
  pool->queue_size++;
  cnd_signal(&pool->queue_item_cnd);
  mtx_unlock(&pool->mutex);
}

void tpool_run(tpool_t *pool, execute_f exec, void *arg, cleanup_f arg_cleanup)
{
  task_node_t *task_node = malloc(sizeof(task_node_t));

  task_node->next = NULL;
  task_node->task.task.arg = arg;
  task_node->task.task.execute = exec;
  task_node->task.task.arg_cleanup = arg_cleanup;
  task_node->task_type = TASK;

  enqueue_task(pool, task_node);
}

future_t *tpool_submit(tpool_t *pool, call_f call, void *arg, cleanup_f arg_cleanup)
{
  task_node_t *task_node = malloc(sizeof(task_node_t));

  task_node->next = NULL;
  task_node->task.future.arg = arg;
  task_node->task.future.call = call;
  task_node->task.future.completed = false;
  task_node->task.future.result = NULL;
  task_node->task.future.related_task_node = task_node;
  task_node->task.future.arg_cleanup = arg_cleanup;
  task_node->task_type = FUTURE;

  enqueue_task(pool, task_node);

  return &task_node->task.future;
}

void tpool_wait(tpool_t *pool)
{
  mtx_lock(&pool->mutex);

  while (pool->queue_size != 0)
    cnd_wait(&pool->task_end_cnd, &pool->mutex);

  mtx_unlock(&pool->mutex);
}

void tpool_wait_future(tpool_t *pool, future_t *future)
{
  mtx_lock(&pool->mutex);

  while (!future->completed)
    cnd_wait(&pool->task_end_cnd, &pool->mutex);

  mtx_unlock(&pool->mutex);
}

void tpool_wait_futures(tpool_t *pool, future_t **futures, size_t n)
{
  mtx_lock(&pool->mutex);

  for (size_t i = 0; i < n; i++)
    while (!futures[i]->completed)
      cnd_wait(&pool->task_end_cnd, &pool->mutex);

  mtx_unlock(&pool->mutex);
}

void *tpool_future_get(tpool_t *pool, future_t *future)
{
  tpool_wait_future(pool, future);

  void *result = NULL;

  if (future->completed)
  {
    result = future->result;
    goto cleanup;
  }

cleanup:
  free(future->related_task_node);
  return result;
}
