#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define ARRAY_INITIAL_SIZE 256
#define STATS_HISTORY_SIZE 10

#define GET_POINTER(v, pos) ((char *)(v)->_ptr + (v)->_elt_size * (pos))

typedef struct {
  void *(*_memory_alloc)(size_t);
  void *(*_memory_realloc)(void *, size_t);
  void (*_memory_free)(void *);
} array_allocator_t;

extern array_allocator_t __array_allocator__;

typedef struct {
  void *_ptr;            /* data pointer */
  size_t _nmemb;         /* size in use */
  size_t _cap;           /* allocated memory (in bytes) */
  size_t _elt_size;      /* element size */
  void (*_free)(void *); /* element free function */
  bool settled;

#define ENABLE_STATISTICS // currently defined by default
#if defined(ENABLE_STATISTICS)
  struct {
    size_t n_allocs;          /* allocation counter */
    size_t n_frees;           /* deallocation counter */
    size_t n_bytes_allocd;    /* total number of bytes allocated */
    size_t n_bytes_reachable; /* total number of bytes not deallocated yet */
    size_t n_bytes_in_use;    /* total number of actively used bytes */
    struct {
      void *pointer;     /* the pointer */
      size_t alloc_size; /* the size of the allocation */
    } history[STATS_HISTORY_SIZE];
    size_t hindex;
  } _stats;
#else
#endif /* ENABLE_STATISTICS */
} array_t;

#define SELF array_t *
#define SELF_RDONLY const array_t *

#define SETTLED(array) array->settled

/* Creates an array and adjusts its starting capacity to be at least
 * enough to hold 'n' elements.
 */
array_t *array_create(size_t elt_size, size_t n, void (*_free)(void *));

/* Reallocates the array if more than half of the current capacity is unused.
 * If the reallocation fails the array remains untouched and the function
 * returns false.
 */
bool array_slimcheck(SELF);

/* Marks the array as settled which makes it unable to reserve additional
 * memory. Should be called once the array is known to have gotten to
 * it's final size.
 */
void array_settle(SELF);

/* Cancels 'array_settle()'.
 */
void array_unsettle(SELF);

/* Returns true if 'self' is marked as settled.
 */
bool array_is_settled(SELF_RDONLY);

/* Returns the number of elements contained in the array.
 */
size_t array_size(SELF_RDONLY);

/* Returns the number of bytes currently reserved by the array.
 */
size_t array_cap(SELF_RDONLY);

/* Returns the data contained in 'self' in between sp -> ep into a newly
 * allocated buffer.
 */
void *array_extract(SELF_RDONLY, size_t sp, size_t ep);

/* Extract and returns the data from 'self' in between sp -> ep (included) into
 * a newly allocated array. If a position is negative, it is iterpreted as an
 * offset from the end. If 'sp' comes after 'ep', the copy is made backwards.
 *
 *   Example:
 *     let [a, b, c] be the array.
 * 	   array_pull(v, 0, 2)   = [a, b, c].
 *     array_pull(v, 0, -1)  = [a, b, c].
 *     array_pull(v, 0, -2)  = [a, b].
 *     array_pull(v, -1, 0)  = [c, b, a].
 *     array_pull(v, -1, -1) = [c].
 *     array_pull(v, -2, -1) = [b, c].
 */
array_t *array_pull(SELF_RDONLY, st64_t sp, st64_t ep);

/* Frees the the array, purging the content beforhand.
 */
void array_kill(SELF);

/* Adds a new element at the end of the array, after its current
 * last element. The data pointed to by 'e' is copied (or moved) to the
 * new element.
 */
bool array_push(SELF, void *e);

/* Removes the last element of the array, effectively reducing
 * the container size by one.
 */
void array_pop(SELF, void *into);

/* Adds a new element to the front of the array, before the
 * first element. The content of 'e' is copied (or moved) to the
 * new element.
 */
bool array_pushf(SELF, void *e);

/* Removes the first element from the array, reducing
 * the container size by one.
 */
void array_popf(SELF, void *into);

/* Copies 'n' bytes of data pointed to by 'src' directly into the array's buffer
 * at the specified offset (in bytes).
 */
bool array_copy(SELF, off_t off, const void *src, size_t n); //--------- TODO

/* The array is extended by injecting a new element before the
 * element at the specified position, effectively increasing
 * the array's size by one.
 */
bool array_insert(SELF, size_t p, void *e);

/* Injects 'n' elements pointed to by 'src' into the array, at
 * potitions 'p'.
 */
bool array_inject(SELF, size_t p, const void *src, size_t n);

/* Appends 'n' elements pointed to by 'src' into the array.
 */
bool array_append(SELF, const void *src, size_t n);

/* Returns a pointer to the element at the specified position.
 */
void *array_access(SELF_RDONLY, size_t p);
void *array_unsafe_access(SELF_RDONLY, size_t p);

/* Identical to 'access', only this return a const pointer
 */
const void *array_at(SELF_RDONLY, size_t p);
const void *array_unsafe_at(SELF_RDONLY, size_t pos);

/* The element at position 'a' and the element at position 'b'
 * are swapped.
 */
void array_swap(SELF, size_t a, size_t b);

/* Removes all elements from the array within start - end
 * (which are ran through v->free), leaving the container with
 * a size of v->_nmemb - abs(start - end).
 */
void array_wipe(SELF, size_t sp, size_t ep);

/* Removes all the elements from the array.
 */
void array_clear(SELF);

/* Removes the element at 'pos' from the array,
 * decreasing the size by one.
 */
void array_evict(SELF, size_t p);

/* Adjusts the array capacity to be at least enough to
 * contain the current + n elements.
 */
bool array_adjust(SELF, size_t n);

/* Returns a pointer to the first element in the array.
 */
void *array_head(SELF_RDONLY);

/* Returns a pointer to the last element in the array.
 */
void *array_tail(SELF_RDONLY);

#if defined(ENABLE_STATISTICS)
/* Dump the stats of the array. The stats are traced only when
 * ENABLE_STATISTICS is defined
 */
void array_stats(SELF_RDONLY);
#endif /* ENABLE_STATISTICS */

#endif /* __array_H__*/
