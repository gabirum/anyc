#include "hash_set.h"

#include <limits.h>
#include <stdlib.h>

#include "../collection/array_list.h"
#include "../collection/linked_list.h"

static void dispose_bucket(void *list)
{
  llist_dispose(list);
}

static hset_t *_new_hashset(size_t size, float load_factor, compare_f compare, hash_f hash, dispose_f dispose)
{
  hset_t *hset = malloc(sizeof(hset_t));

  hset->buckets = alist_create_fixed(size, dispose_bucket);
  hset->compare = compare;
  hset->hash = hash;
  hset->dispose = dispose;
  hset->load_factor = load_factor;
  hset->size = 0;

  return hset;
}

hset_t *hset_create(float load_factor, compare_f compare, hash_f hash, dispose_f dispose)
{
  return _new_hashset(HASH_SET_DEFAULT_SIZE, load_factor, compare, hash, dispose);
}

static size_t get_pos(hset_t *set, void *item)
{
  return set->hash(item) % set->buckets->capacity;
}

bool default_compare(void *a, void *b)
{
  return a == b;
}

uint64_t default_hash(void *item)
{
  return (size_t)item ^ ULONG_MAX;
}

hset_t *hset_create_with_defaults(float load_factor, dispose_f dispose)
{
  return hset_create(load_factor, default_compare, default_hash, dispose);
}

void hset_dispose(hset_t **_set)
{
  hset_t set = **_set;
  alist_dispose(&set.buckets);
  free(*_set);
  *_set = NULL;
}

bool hset_has(hset_t *set, void *item)
{
  if (set->size == 0)
    return false;

  size_t pos = get_pos(set, item);

  llist_t *list = alist_get(set->buckets, pos);
  if (list == NULL)
    return false;

  return llist_some(list, set->compare, item);
}

static void unsafe_set(hset_t *set, void *item)
{
  size_t pos = get_pos(set, item);

  llist_t *list = alist_get(set->buckets, pos);
  if (list == NULL)
    alist_set(set->buckets, pos, llist_create(set->dispose));

  list = alist_get(set->buckets, pos);
  llist_add_last(list, item);
  set->size++;
}

static void increment_and_rebuild_set(hset_t **_set)
{
  hset_t set = **_set;
  hset_t *new_set = _new_hashset(2 * set.buckets->capacity, set.load_factor, set.compare, set.hash, set.dispose);

  for (size_t i = 0; i < set.buckets->capacity; i++)
  {
    llist_t *bucket = alist_get(set.buckets, i);

    if (bucket == NULL)
      continue;

    while (bucket->size > 0)
    {
      void *item = llist_rm_first(bucket);
      unsafe_set(new_set, item);
    }
  }

  hset_dispose(_set);
  *_set = new_set;
}

bool hset_set(hset_t **_set, void *item)
{
  if (!hset_has(*_set, item))
    return false;

  if ((*_set)->load_factor < (((float)((*_set)->size + 1)) / ((float)(*_set)->buckets->capacity)))
    increment_and_rebuild_set(_set);

  unsafe_set(*_set, item);

  return true;
}

void *hset_find(hset_t *set, predicate_f predicate, void *f_arg)
{
  for (size_t i = 0; i < set->buckets->capacity; i++)
  {
    llist_t *entry = alist_get(set->buckets, i);

    if (entry != NULL)
    {
      void *result = llist_find(entry, predicate, f_arg);

      if (result != NULL)
        return result;
    }
  }

  return NULL;
}

void hset_remove(hset_t *set, void *item)
{
  if (hset_has(set, item))
  {
    size_t hash = set->hash(item);
    llist_t *list = alist_get(set->buckets, hash);

    if (list == NULL)
      return;

    llist_rm(list, item);
    set->size--;
  }
}

void hset_for_each(hset_t *set, consumer_f consumer, void *f_data)
{
  for (size_t i = 0; i < set->buckets->capacity; i++)
  {
    llist_t *entry = alist_get(set->buckets, i);

    if (entry != NULL)
      llist_for_each(entry, consumer, f_data);
  }
}

void hset_clear(hset_t *set)
{
  alist_clear(&set->buckets);
  set->size = 0;
}
