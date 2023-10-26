#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdbool.h>

typedef void (*consumer_f)(void *, void *);

typedef bool (*predicate_f)(void *, void *);
typedef bool (*compare_f)(void *, void *);

typedef void (*dispose_f)(void *);

#endif // _DEFS_H_
