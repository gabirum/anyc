#include "htable.h"

#include <stdlib.h>

htable_t *htable_create(compare_f compare, hash_f hash, dispose_f dispose)
{
  htable_t *ht = malloc(sizeof(htable_t));

  ht->entry_set = hset_create(.75f, compare, hash, dispose);
  ht->compare = compare;
  ht->hash = hash;
  ht->dispose = dispose;

  return ht;
}

void htable_dispose(htable_t **_table)
{
  htable_t table = **_table;
  hset_dispose(&table.entry_set);
  free(*_table);
  *_table = NULL;
}

bool htable_has(htable_t *table, void *key)
{
  return hset_has(table->entry_set, key);
}

entry_t *htable_get(htable_t *table, void *key)
{
  return hset_find(table->entry_set, table->compare, key);
}

bool htable_set(htable_t *table, entry_t *entry)
{
  return hset_set(&table->entry_set, entry);
}

void htable_remove(htable_t *table, void *key)
{
  entry_t entry = {.key = key, .value = NULL};

  hset_remove(table->entry_set, &entry);
}

void htable_for_each(htable_t *table, consumer_f consumer)
{
  hset_for_each(table->entry_set, consumer, NULL);
}

void htable_clear(htable_t **_table)
{
  htable_t table = **_table;

  htable_dispose(_table);
  *_table = htable_create(table.compare, table.hash, table.dispose);
}
