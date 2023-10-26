#include "linked_list.h"

#include <stdlib.h>

llist_t *llist_create(dispose_f dispose)
{
  llist_t *list = malloc(sizeof(llist_t));

  list->head = NULL;
  list->tail = NULL;
  list->dispose = dispose;
  list->size = 0;

  return list;
}

void llist_dispose(llist_t **_list)
{
  llist_t list = **_list;
  ll_node_t *node = list.head;

  while (node != NULL)
  {
    ll_node_t *aux = node->next;

    list.dispose(node->value);
    free(node);

    node = aux;
  }

  free(*_list);
  *_list = NULL;
}

void *llist_get_first(llist_t *list)
{
  if (list->size == 0)
    return NULL;

  return list->head->value;
}

void *llist_get_last(llist_t *list)
{
  if (list->size == 0)
    return NULL;

  return list->tail->value;
}

static ll_node_t *alloc_node(void *value, ll_node_t *next)
{
  ll_node_t *node = malloc(sizeof(ll_node_t));
  node->value = value;
  node->next = next;

  return node;
}

static bool alloc_first(llist_t *list, void *value)
{
  if (list->size > 0)
    return false;

  list->tail = list->head = alloc_node(value, NULL);
  list->size++;
  return true;
}

void llist_add_first(llist_t *list, void *value)
{
  if (alloc_first(list, value))
    return;

  list->head = alloc_node(value, list->head);
  list->size++;

  if (list->size == 2)
    list->tail = list->head->next;
}

void llist_add_last(llist_t *list, void *value)
{
  if (alloc_first(list, value))
    return;

  list->tail->next = alloc_node(value, NULL);
  list->tail = list->tail->next;
  list->size++;
}

void llist_rm(llist_t *list, void *item)
{
  for (ll_node_t **_node = &list->head; _node != NULL; _node = &(*_node)->next)
  {
    ll_node_t *node = *_node;
    if (node->value == item)
    {
      ll_node_t *next = node->next;
      list->dispose(node->value);
      free(node);
      *_node = next;
      return;
    }
  }
}

void *llist_rm_first(llist_t *list)
{
  if (list->size == 0)
    return NULL;

  ll_node_t *head_ref = list->head;
  void *value_ref = head_ref->value;

  list->head = head_ref->next;
  list->size--;

  free(head_ref);

  return value_ref;
}

void *llist_rm_last(llist_t *list)
{
  if (list->size == 0)
    return NULL;

  void *value = list->tail->value;

  if (list->head == list->tail)
  {
    free(list->head);
    list->head = NULL;
    list->tail = NULL;
    list->size--;

    return value;
  }

  ll_node_t *node = list->head;
  while (node->next != list->tail)
    node = node->next;

  free(list->tail);
  list->tail = node;
  list->tail->next = NULL;
  list->size--;

  return value;
}

void llist_drm_first(llist_t *list)
{
  list->dispose(llist_rm_first(list));
}

void llist_drm_last(llist_t *list)
{
  list->dispose(llist_rm_last(list));
}

void *llist_find(llist_t *list, predicate_f predicate, void *f_arg)
{
  for (ll_node_t *node = list->head; node != NULL; node = node->next)
    if (predicate(node->value, f_arg))
      return node->value;

  return NULL;
}

bool llist_every(llist_t *list, predicate_f predicate, void *f_arg)
{
  for (ll_node_t *node = list->head; node != NULL; node = node->next)
    if (!predicate(node->value, f_arg))
      return false;

  return true;
}

bool llist_some(llist_t *list, predicate_f predicate, void *f_arg)
{
  for (ll_node_t *node = list->head; node != NULL; node = node->next)
    if (predicate(node->value, f_arg))
      return true;

  return false;
}

void llist_for_each(llist_t *list, consumer_f consumer, void *f_arg)
{
  for (ll_node_t *node = list->head; node != NULL; node = node->next)
    consumer(node->value, f_arg);
}

void llist_clear(llist_t **_list)
{
  llist_t list = **_list;
  llist_dispose(_list);
  *_list = llist_create(list.dispose);
}
