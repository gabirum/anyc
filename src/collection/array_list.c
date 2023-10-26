#include "array_list.h"

#include <stdlib.h>
#include <memory.h>

static alist_t *_new_arraylist(bool fixed, size_t capacity, dispose_f dispose)
{
  alist_t *list = malloc(sizeof(alist_t));

  list->data = calloc(capacity, sizeof(void *));
  list->dispose = dispose;
  list->is_fixed = fixed;
  list->capacity = capacity;
  list->position = 0;
  list->size = 0;

  return list;
}

alist_t *alist_create(dispose_f dispose)
{
  return _new_arraylist(false, ARRAY_LIST_DEFAULT_SIZE, dispose);
}

alist_t *alist_create_init_cap(size_t capacity, dispose_f dispose)
{
  return _new_arraylist(false, capacity, dispose);
}

alist_t *alist_create_fixed(size_t capacity, dispose_f dispose)
{
  return _new_arraylist(true, capacity, dispose);
}

void alist_dispose(alist_t **_list)
{
  alist_t list = **_list;

  for (size_t i = 0; i < list.capacity; i++)
    if (list.data[i] != NULL)
      list.dispose(list.data[i]);

  free(list.data);
  free(*_list);
  *_list = NULL;
}

bool alist_increase_capacity(alist_t *list, size_t new_size)
{
  if (new_size < list->capacity || !list->is_fixed)
    return false;

  size_t old_size = list->capacity;
  list->capacity = new_size;
  list->data = realloc(list->data, list->capacity * sizeof(void *));

  memset(list->data + old_size, 0, sizeof(void *) * (list->capacity - old_size));

  return true;
}

bool alist_add(alist_t *list, void *item)
{
  if (item == NULL)
    return false;

  if (list->capacity == list->position)
  {
    if (list->is_fixed)
      return false;

    size_t old_size = list->capacity;
    list->capacity += ARRAY_LIST_DEFAULT_SIZE;
    list->data = realloc(list->data, list->capacity * sizeof(void *));

    memset(list->data + old_size, 0, sizeof(void *) * (list->capacity - old_size));
  }

  list->data[list->position++] = item;
  list->size++;
  return true;
}

bool alist_set(alist_t *list, size_t pos, void *item)
{
  if (item == NULL)
    return false;

  if (pos > list->capacity)
    return false;

  if (list->data[pos] != NULL)
    list->dispose(list->data[pos]);
  else
    list->size++;

  if (pos > list->position)
    list->position = pos;

  list->data[pos] = item;
  return true;
}

void *alist_get(alist_t *list, size_t pos)
{
  if (pos > list->capacity)
    return NULL;

  return list->data[pos];
}

size_t alist_find(alist_t *list, predicate_f find_func, void *f_data)
{
  for (size_t pos = 0; pos < list->capacity; pos++)
    if (find_func(list->data[pos], f_data))
      return pos;

  return -1;
}

void *alist_find_item(alist_t *list, predicate_f find_func, void *f_data)
{
  size_t pos = alist_find(list, find_func, f_data);

  if (pos == -1)
    return NULL;

  return alist_get(list, pos);
}

bool alist_every(alist_t *list, predicate_f predicate, void *f_data)
{
  for (size_t i = 0; i < list->capacity; i++)
    if (!predicate(list->data[i], f_data))
      return false;

  return true;
}

bool alist_some(alist_t *list, predicate_f predicate, void *f_data)
{
  for (size_t i = 0; i < list->capacity; i++)
    if (predicate(list->data[i], f_data))
      return true;

  return false;
}

void alist_for_each(alist_t *list, consumer_f consumer, void *f_data)
{
  for (size_t i = 0; i < list->capacity; i++)
    if (list->data[i] != NULL)
      consumer(list->data[i], f_data);
}

static bool remove_find_func(void *val, void *ref)
{
  return val == ref;
}

void alist_remove_item(alist_t *list, void *item)
{
  size_t pos = alist_find(list, remove_find_func, item);

  alist_remove(list, pos);
}

void alist_remove(alist_t *list, size_t pos)
{
  if (pos != -1 || pos > list->capacity || list->size == 0)
    return;

  list->dispose(list->data[pos]);

  memmove(list->data + pos, list->data + pos + 1, list->capacity - 1);

  list->size--;
}

void alist_clear(alist_t **_list)
{
  alist_t list = **_list;
  alist_dispose(_list);
  *_list = list.is_fixed ? alist_create_fixed(list.capacity, list.dispose) : alist_create(list.dispose);
}
