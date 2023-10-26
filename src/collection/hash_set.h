#ifndef _COLLECTION_HASH_SET_H_
#define _COLLECTION_HASH_SET_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "../function/defs.h"
#include "array_list.h"

#define HASH_SET_DEFAULT_SIZE 10

typedef uint64_t (*hash_f)(void *);

typedef struct hset
{
  alist_t *buckets; // each bucket is a linkedlist for linked colision
  compare_f compare;
  hash_f hash;
  dispose_f dispose;
  float load_factor;
  size_t size;
} hset_t;

hset_t *hset_create(float, compare_f, hash_f, dispose_f);
hset_t *hset_create_with_defaults(float, dispose_f);
void hset_dispose(hset_t **);

bool hset_has(hset_t *, void *);

bool hset_set(hset_t **, void *);
void hset_remove(hset_t *, void *);

void *hset_find(hset_t *, predicate_f, void *);
void hset_for_each(hset_t *, consumer_f, void *);

void hset_clear(hset_t *);

#endif // _COLLECTION_HASH_SET_H_
