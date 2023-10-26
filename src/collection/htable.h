#ifndef _COLLECTION_HASHTABLE_H_
#define _COLLECTION_HASHTABLE_H_

#include <stdbool.h>

#include "../function/defs.h"
#include "hash_set.h"

typedef struct entry
{
  void *key;
  void *value;
} entry_t;

typedef struct htable
{
  hset_t *entry_set;
  compare_f compare;
  hash_f hash;
  dispose_f dispose;
} htable_t;

htable_t *htable_create(compare_f, hash_f, dispose_f);
void htable_dispose(htable_t **);

bool htable_has(htable_t *, void *);
entry_t *htable_get(htable_t *, void *);

bool htable_set(htable_t *, entry_t *);
void htable_remove(htable_t *, void *);

void htable_for_each(htable_t *, consumer_f);

void htable_clear(htable_t **);

#endif // _COLLECTION_HASHTABLE_H_
