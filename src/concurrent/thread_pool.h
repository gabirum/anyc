#ifndef _CONCURRENT_THREAD_POOL_H_
#define _CONCURRENT_THREAD_POOL_H_

#include <threads.h>
#include <stdatomic.h>

#include "../function/defs.h"
#include "../collection/array_list.h"
#include "queue.h"

typedef void *(*execute_f)(void *);
typedef void (*dispose_future_f)(void *, void *);

typedef struct future
{
  execute_f execute;
  dispose_future_f dispose;
  void *arg, *result;
  bool completed;
} future_t;

typedef struct tpool
{
  alist_t *worker_list;
  sync_queue_t *task_queue;
  mtx_t pool_mtx;
  cnd_t worker_die_cnd, task_end_cnd;
  atomic_size_t working_threads_count, thread_count;
  atomic_bool stop;
  size_t max_threads;
} tpool_t;

tpool_t *tpool_create(size_t);
void tpool_dispose(tpool_t **);
void tpool_dispose_future(future_t *);

future_t *tpool_submit(tpool_t *, execute_f, void *, dispose_future_f);
void tpool_wait(tpool_t *);

void tpool_wait_future(tpool_t *, future_t *);
void tpool_wait_futures(tpool_t *, future_t **, size_t);
void *tpool_future_get(tpool_t *, future_t *);

#endif // _CONCURRENT_THREAD_POOL_H_
