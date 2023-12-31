#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define ARRAY_INITIAL_SIZE 128
#define META_TRACE_SIZE 10

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

#if defined(DISABLE_TRACING)
#else
  struct {
    size_t n_allocs;          /* allocation counter */
    size_t n_frees;           /* deallocation counter */
    size_t n_bytes_allocd;    /* total number of bytes allocated */
    size_t n_bytes_reachable; /* total number of bytes not deallocated yet */
    size_t n_bytes_in_use;    /* total number of actively used bytes */
    struct {
      void *pointer;     /* the pointer */
      size_t alloc_size; /* the size of the allocation */
    } trace[META_TRACE_SIZE];
    size_t tidx;
  } _meta;
#endif /* ENABLE_STATISTICS */
} array_t;

/* DEFINED TYPES */
#define ARRAY_T(__x) array_t *__x
#define BOOL_T(__b) bool __b
#define RDONLY_ARRAY_T(__a) const array_t *__a
#define SIZE_T(__n) size_t __n
#define INT_T(__n) int64_t __n
#define INDEX_T(__i) size_t __i
#define PTR_T(__p) void *__p
#define RDONLY_PTR_T(__p) const void *__p
#define NONE_T(__x) void __x

#define SETTLED(array) array->settled

/* Creates an array and adjusts its starting capacity to be at least
 * enough to hold 'n' elements.
 */
ARRAY_T(array_create)(SIZE_T(elt_size), SIZE_T(n), void (*_free)(void *));

/* Reallocates the array if more than half of the current capacity is unused.
 * If the reallocation fails the array is untouched and the function
 * returns false.
 */
BOOL_T(array_slimcheck)(ARRAY_T(self));

/* Once settled, the array is unable to reserve additional memory.
 * This should be called once the array is known to have gotten to
 * it's final size.
 */
NONE_T(array_settle)(ARRAY_T(self));

/* Marks the array as unsettled.
 */
NONE_T(array_unsettle)(ARRAY_T(self));

/* Returns true if 'self' is marked as settled.
 */
BOOL_T(array_is_settled)(RDONLY_ARRAY_T(self));

/* Returns the number of elements contained in the array.
 */
SIZE_T(array_size)(RDONLY_ARRAY_T(self));

/* Returns the number of bytes currently reserved by the array.
 */
SIZE_T(array_cap)(RDONLY_ARRAY_T(self));

/* Pointer to the underlying element storage. For non-empty containers,
 * the returned pointer compares equal to the address of the first element.
 */
PTR_T(array_data)(RDONLY_ARRAY_T(self));

/* Returns the number of elements that can fit in the buffer without
 * having to reallocate.
 */
SIZE_T(array_uninitialized_size)(RDONLY_ARRAY_T(self));

/* Pointer to the first uninitialized element in the buffer.
 */
PTR_T(array_uninitialized_data)(RDONLY_ARRAY_T(self));

/* Returns the data contained in 'self' in between sp -> ep into a newly
 * allocated buffer.
 */
PTR_T(array_extract)(RDONLY_ARRAY_T(src), SIZE_T(sp), SIZE_T(ep));

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
ARRAY_T(array_pull)(RDONLY_ARRAY_T(src), INT_T(sp), INT_T(ep));

/* Frees the array, clearing the content beforhand.
 */
NONE_T(array_kill)(ARRAY_T(self));

/* Adds a new element at the end of the array, after its current
 * last element. The data pointed to by 'e' is copied (or moved) to the
 * new element.
 */
BOOL_T(array_push)(ARRAY_T(self), PTR_T(e));

/* Removes the last element of the array, effectively reducing
 * the container size by one.
 */
NONE_T(array_pop)(ARRAY_T(self), PTR_T(into));

/* Adds a new element to the front of the array, before the
 * first element. The content of 'e' is copied (or moved) to the
 * new element.
 */
BOOL_T(array_pushf)(ARRAY_T(self), PTR_T(e));

/* Removes the first element from the array, reducing
 * the container size by one.
 */
NONE_T(array_popf)(ARRAY_T(self), PTR_T(into));

/* Copies 'n' bytes of data pointed to by 'src' directly into the array's buffer
 * at the specified offset (in bytes).
 */
NONE_T(array_copy)(ARRAY_T(self), SIZE_T(off), RDONLY_PTR_T(src), SIZE_T(n));

/* The array is extended by injecting a new element before the
 * element at the specified position, effectively increasing
 * the array's size by one.
 */
BOOL_T(array_insert)(ARRAY_T(self), SIZE_T(p), PTR_T(e));

/* Injects 'n' elements pointed to by 'src' into the array, at
 * potitions 'p'.
 */
BOOL_T(array_inject)(ARRAY_T(self), SIZE_T(p), RDONLY_PTR_T(src), SIZE_T(n));

/* Appends 'n' elements pointed to by 'src' into the array.
 */
BOOL_T(array_append)(ARRAY_T(self), RDONLY_PTR_T(src), SIZE_T(n));

/* Returns a pointer to the element at the specified position.
 */
PTR_T(array_access)(RDONLY_ARRAY_T(self), SIZE_T(p));
PTR_T(array_unsafe_access)(RDONLY_ARRAY_T(self), SIZE_T(p));

/* Identical to 'access', only this return a const pointer
 */
RDONLY_PTR_T(array_at)(RDONLY_ARRAY_T(self), SIZE_T(p));
RDONLY_PTR_T(array_unsafe_at)(RDONLY_ARRAY_T(self), SIZE_T(pos));

/* Appends n elements from capacity. The application must have initialized
 * the storage backing these elements otherwise the behavior is undefined.
 */
BOOL_T(array_append_from_capacity)(ARRAY_T(self), SIZE_T(n));

/* The element at position 'a' and the element at position 'b'
 * are swapped.
 */
NONE_T(array_swap_elems)(ARRAY_T(self), SIZE_T(a), SIZE_T(b));

/* Removes all elements from the array within start - end
 * (which are ran through v->free), leaving the container with
 * a size of v->_nmemb - abs(start - end).
 */
NONE_T(array_wipe)(ARRAY_T(self), SIZE_T(sp), SIZE_T(ep));

/* Removes all the elements from the array.
 */
NONE_T(array_clear)(ARRAY_T(self));

/* Removes the element at 'pos' from the array,
 * decreasing the size by one.
 */
NONE_T(array_evict)(ARRAY_T(self), SIZE_T(p));

/* Adjusts the array capacity to be at least enough to
 * contain the current + n elements.
 */
BOOL_T(array_adjust)(ARRAY_T(self), SIZE_T(n));

/* Returns a pointer to the first element in the array.
 */
PTR_T(array_head)(RDONLY_ARRAY_T(self));

/* Returns a pointer to the last element in the array.
 */
PTR_T(array_tail)(RDONLY_ARRAY_T(self));

/* Dumps a trace of the array. If DISABLE_TRACING is defined
 * no data is collected at runtime.
 */
NONE_T(array_trace)(RDONLY_ARRAY_T(self));

#endif /* __ARRAY_H__*/
