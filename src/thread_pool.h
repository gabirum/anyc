#ifndef _CONCURRENT_THREAD_POOL_H_
#define _CONCURRENT_THREAD_POOL_H_

#include <stdbool.h>
#include <stdatomic.h>
#include <threads.h>

typedef void (*cleanup_f)(void *);
typedef void (*execute_f)(void *, cleanup_f);
typedef void *(*call_f)(void *, cleanup_f);

typedef struct future
{
  call_f call;
  cleanup_f arg_cleanup;
  void *arg, *result;
  bool completed;
  void *related_task_node;
} future_t;

typedef struct task
{
  execute_f execute;
  cleanup_f arg_cleanup;
  void *arg;
} task_t;

enum task_type_e
{
  TASK,
  FUTURE
};

typedef struct task_node
{
  union
  {
    task_t task;
    future_t future;
  } task;
  struct task_node *next;
  enum task_type_e task_type;
} task_node_t;

typedef struct tpool
{
  mtx_t mutex;
  cnd_t queue_item_cnd, worker_die_cnd, task_end_cnd;
  thrd_t *workers;
  task_node_t *head, *tail;
  size_t queue_size, max_threads;
  atomic_bool stop;
  atomic_size_t thread_count;
} tpool_t;

tpool_t *tpool_create(size_t);
void tpool_cleanup(tpool_t **);

void tpool_run(tpool_t *, execute_f, void *, cleanup_f);
future_t *tpool_submit(tpool_t *, call_f, void *, cleanup_f);
void tpool_wait(tpool_t *);

void tpool_wait_future(tpool_t *, future_t *);
void tpool_wait_futures(tpool_t *, future_t **, size_t);
void *tpool_future_get(tpool_t *, future_t *);

#endif // _CONCURRENT_THREAD_POOL_H_
