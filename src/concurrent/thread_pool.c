#include "thread_pool.h"

#include <stdlib.h>

static void dispose_worker(void *_thread)
{
  thrd_t thread = *(thrd_t *)_thread;

  thrd_detach(thread);
  free(_thread);
}

static int worker(void *_pool)
{
  tpool_t *pool = _pool;
  atomic_fetch_add(&pool->thread_count, 1);

  while (!atomic_load(&pool->stop))
  {
    future_t *future = squeue_wait_dequeue(pool->task_queue);

    if (future == NULL)
      continue;

    atomic_fetch_add(&pool->working_threads_count, 1);

    future->result = future->execute(future->arg);
    future->completed = true;

    atomic_fetch_sub(&pool->working_threads_count, 1);

    cnd_signal(&pool->task_end_cnd);
  }

  atomic_fetch_sub(&pool->thread_count, 1);

  cnd_signal(&pool->worker_die_cnd);

  return 0;
}

tpool_t *tpool_create(size_t size)
{
  tpool_t *pool = malloc(sizeof(tpool_t));

  pool->task_queue = squeue_create((dispose_f)tpool_dispose_future);
  pool->worker_list = alist_create_fixed(size, dispose_worker);
  mtx_init(&pool->pool_mtx, mtx_plain);
  cnd_init(&pool->worker_die_cnd);
  cnd_init(&pool->task_end_cnd);

  pool->max_threads = size;
  atomic_init(&pool->thread_count, 0);
  atomic_init(&pool->working_threads_count, 0);
  atomic_init(&pool->stop, false);

  for (size_t i = 0; i < size; i++)
  {
    thrd_t *thread = malloc(sizeof(thrd_t));

    thrd_create(thread, worker, pool);
    alist_add(pool->worker_list, thread);
  }

  return pool;
}

void tpool_dispose(tpool_t **_pool)
{
  tpool_t pool = **_pool;
  atomic_store(&pool.stop, true);

  squeue_dispose(&pool.task_queue);

  mtx_lock(&pool.pool_mtx);

  while (atomic_load(&pool.thread_count) > 0)
    cnd_wait(&pool.worker_die_cnd, &pool.pool_mtx);

  mtx_unlock(&pool.pool_mtx);

  alist_dispose(&pool.worker_list);
  cnd_destroy(&pool.worker_die_cnd);
  cnd_destroy(&pool.task_end_cnd);
  mtx_destroy(&pool.pool_mtx);

  free(*_pool);
  *_pool = NULL;
}

void tpool_dispose_future(future_t *future)
{
  if (future->dispose != NULL)
    future->dispose(future->arg, future->result);

  free(future);
}

future_t *tpool_submit(tpool_t *pool, execute_f executor, void *arg, dispose_future_f dispose)
{
  future_t *future = malloc(sizeof(future_t));

  future->execute = executor;
  future->arg = arg;
  future->result = NULL;
  future->completed = false;
  future->dispose = dispose;

  bool enqueued = squeue_enqueue(pool->task_queue, future);

  if (!enqueued)
  {
    free(future);
    return NULL;
  }

  return future;
}

void tpool_wait(tpool_t *pool)
{
  mtx_lock(&pool->pool_mtx);

  while (atomic_load(&pool->working_threads_count) > 0)
    cnd_wait(&pool->task_end_cnd, &pool->pool_mtx);

  mtx_unlock(&pool->pool_mtx);
}

void tpool_wait_future(tpool_t *pool, future_t *future)
{
  mtx_lock(&pool->pool_mtx);

  while (!future->completed)
    cnd_wait(&pool->task_end_cnd, &pool->pool_mtx);

  mtx_unlock(&pool->pool_mtx);
}

void tpool_wait_futures(tpool_t *pool, future_t **futures, size_t n)
{
  mtx_lock(&pool->pool_mtx);

  for (size_t i = 0; i < n; i++)
    while (!futures[i]->completed)
      cnd_wait(&pool->task_end_cnd, &pool->pool_mtx);

  mtx_unlock(&pool->pool_mtx);
}

void *tpool_future_get(tpool_t *pool, future_t *future)
{
  tpool_wait_future(pool, future);

  if (future->completed)
    return future->result;

  return NULL;
}
