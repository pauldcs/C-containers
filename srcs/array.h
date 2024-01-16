#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
  void *(*_memory_alloc)(size_t);
  void *(*_memory_realloc)(void *, size_t);
  void (*_memory_free)(void *);
} array_allocator_t;

extern array_allocator_t __array_allocator__;

typedef struct {
  void *_ptr;       /* A pointer to the start of the buffer */
  size_t _nmemb;    /* The number of elements in the buffer */
  size_t _cap;      /* The size of the reserved memory block (in bytes) */
  size_t _elt_size; /* The size of one element (in bytes) */

  bool _settled; /* Once settled, any attempts to reallocate the buffer are
                  * blocked so new pointers to the data are guaranteed to
                  * stay valid until the array is freed, or unsettled. */

  void (*_free)(void *); /* the element destructor function */

#if defined(DISABLE_ARRAY_TRACING)
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
#endif /* DISABLE_ARRAY_TRACING */
} array_t;

#define _data(array) array->_ptr
#define _size(array) array->_nmemb
#define _capacity(array) array->_cap
#define _typesize(array) array->_elt_size
#define _settled(array) array->_settled
#define _freefunc(array) array->_free

#define _relative_data(array, pos)                                             \
  ((char *)(array)->_ptr + (array)->_elt_size * (pos))
/* Creates an array and adjusts its starting capacity to be at least
 * enough to hold 'n' elements.
 */
ARRAY_TYPE(array_create)
(SIZE_TYPE(elt_size), SIZE_TYPE(n), void (*_free)(void *));

/* Creates an array with 'buffer' as the data, if the buffer was not allocated
 * through the same allocator as for the array, the behavior is undefined. */
ARRAY_TYPE(array_use_buffer)
(PTR_TYPE(*buffer), SIZE_TYPE(bufsize), SIZE_TYPE(elt_size), SIZE_TYPE(n),
 void (*_free)(void *));

/* Works the same as 'use_buffer' but directly sets the array as settled. This
 * blocks potential reallocations and enables the safe use of static memory. */
ARRAY_TYPE(array_use_settled_buffer)
(PTR_TYPE(*buffer), SIZE_TYPE(bufsize), SIZE_TYPE(elt_size), SIZE_TYPE(n),
 void (*_free)(void *));

/* Reallocates the array if more than half of the current capacity is unused.
 * If the reallocation fails the array is untouched and the function
 * returns false.
 */
BOOL_TYPE(array_slimcheck)(ARRAY_TYPE(self));

/* Once settled, the array is unable to reserve additional memory.
 * This should be called once the array is known to have gotten to
 * it's final size.
 */
NONE_TYPE(array_settle)(ARRAY_TYPE(self));

/* Marks the array as unsettled.
 */
NONE_TYPE(array_unsettle)(ARRAY_TYPE(self));

/* Returns true if 'self' is marked as settled.
 */
__attr_pure BOOL_TYPE(array_is_settled)(RDONLY_ARRAY_TYPE(self));

/* Returns the number of elements contained in the array.
 */
__attr_pure SIZE_TYPE(array_size)(RDONLY_ARRAY_TYPE(self));

/* Returns the number of bytes in use in the array.
 */
__attr_pure SIZE_TYPE(array_sizeof)(RDONLY_ARRAY_TYPE(self));

/* Returns the number of bytes currently reserved by the array.
 */
__attr_pure SIZE_TYPE(array_cap)(RDONLY_ARRAY_TYPE(self));

/* Pointer to the underlying element storage. For non-empty containers,
 * the returned pointer compares equal to the address of the first element.
 */
__attr_pure PTR_TYPE(array_data)(RDONLY_ARRAY_TYPE(self));

/* Returns the number of elements that can fit in the buffer without
 * having to reallocate.
 */
__attr_pure SIZE_TYPE(array_uninitialized_size)(RDONLY_ARRAY_TYPE(self));

/* Pointer to the first uninitialized element in the buffer.
 */
__attr_pure PTR_TYPE(array_uninitialized_data)(RDONLY_ARRAY_TYPE(self));

/* Returns the data contained in 'self' in between start -> end into a newly
 * allocated buffer.
 */
PTR_TYPE(array_extract)
(RDONLY_ARRAY_TYPE(src), SIZE_TYPE(start), SIZE_TYPE(end));

/* Extract and returns the data from 'self' in between start -> end (included)
 * into a newly allocated array. If a position is negative, it is iterpreted as
 * an offset from the end. If 'start' comes after 'end', the copy is made
 * backwards.
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
ARRAY_TYPE(array_pull)
(RDONLY_ARRAY_TYPE(src), SSIZE_TYPE(start), SSIZE_TYPE(end));

/* Frees the array, clearing the content beforhand.
 */
NONE_TYPE(array_kill)(ARRAY_TYPE(self));

/* Adds a new element at the end of the array, after its current
 * last element. The data pointed to by 'e' is copied (or moved) to the
 * new element.
 */
BOOL_TYPE(array_push)(ARRAY_TYPE(self), RDONLY_PTR_TYPE(e));

/* Removes the last element of the array, effectively reducing
 * the container size by one.
 */
NONE_TYPE(array_pop)(ARRAY_TYPE(self), PTR_TYPE(into));

/* Adds a new element to the front of the array, before the
 * first element. The content of 'e' is copied (or moved) to the
 * new element.
 */
BOOL_TYPE(array_pushf)(ARRAY_TYPE(self), PTR_TYPE(e));

/* Removes the first element from the array, reducing
 * the container size by one.
 */
NONE_TYPE(array_popf)(ARRAY_TYPE(self), PTR_TYPE(into));

/* Copies 'n' bytes of data pointed to by 'src' directly into the array's
 * buffer, overwriting the data at the specified offset (in bytes).
 */
NONE_TYPE(array_tipex)
(ARRAY_TYPE(self), SIZE_TYPE(off), RDONLY_PTR_TYPE(src), SIZE_TYPE(n));

/* The array is extended by injecting a new element before the
 * element at the specified position, effectively increasing
 * the array's size by one.
 */
BOOL_TYPE(array_insert)(ARRAY_TYPE(self), SIZE_TYPE(p), PTR_TYPE(e));

/* Injects 'n' elements pointed to by 'src' into the array, at
 * potitions 'p'.
 */
BOOL_TYPE(array_inject)
(ARRAY_TYPE(self), SIZE_TYPE(p), RDONLY_PTR_TYPE(src), SIZE_TYPE(n));

/* Appends 'n' elements pointed to by 'src' into the array.
 */
BOOL_TYPE(array_append)(ARRAY_TYPE(self), RDONLY_PTR_TYPE(src), SIZE_TYPE(n));

/* Creates a new array, filtered down to just the elements from 'self' that
 * pass the test implemented by the callback.
 */
ARRAY_TYPE(array_filter)
(RDONLY_ARRAY_TYPE(self), bool (*callback)(RDONLY_PTR_TYPE(elem)));

/* executes a provided function once for each array element.
 */
BOOL_TYPE(array_foreach)(ARRAY_TYPE(self), bool (*callback)(PTR_TYPE(elem)));

/* Returns the first element in 'self' that satisfies the callback.
 * If no values satisfy the testing function, NULL is returned. */
__attr_pure PTR_TYPE(array_find)(ARRAY_TYPE(self),
                                 bool (*callback)(RDONLY_PTR_TYPE(elem)));

/* Behaves the same as 'find' except it returns an index (that can be used with
 * 'at'), or -1 if no element was found.
 */
__attr_pure
    SSIZE_TYPE(array_find_index)(ARRAY_TYPE(self),
                                 bool (*callback)(RDONLY_PTR_TYPE(elem)));

/* Behaves the same as 'find' expect it starts the search from the end.
 */
__attr_pure PTR_TYPE(array_rfind)(ARRAY_TYPE(self),
                                  bool (*callback)(RDONLY_PTR_TYPE(elem)));

/* Behaves the same as 'find' except it starts the search from the end and
 * returns an index (that can be used with 'at'), or -1 if no element was found.
 */
__attr_pure
    PTR_TYPE(array_rfind_index)(ARRAY_TYPE(self),
                                bool (*callback)(RDONLY_PTR_TYPE(elem)));

/* Returns a pointer to the element at the specified position.
 */
__attr_pure PTR_TYPE(array_access)(RDONLY_ARRAY_TYPE(self), SIZE_TYPE(p));
__attr_pure PTR_TYPE(array_unsafe_access)(RDONLY_ARRAY_TYPE(self),
                                          SIZE_TYPE(p));

/* Identical to 'access', only this returns a const pointer.
 */
__attr_pure RDONLY_PTR_TYPE(array_at)(RDONLY_ARRAY_TYPE(self), SIZE_TYPE(p));
__attr_pure RDONLY_PTR_TYPE(array_unsafe_at)(RDONLY_ARRAY_TYPE(self),
                                             SIZE_TYPE(pos));

/* Appends 'n' elements from capacity. The application must have initialized
 * the storage backing these elements otherwise the behavior is undefined.
 */
BOOL_TYPE(array_append_from_capacity)(ARRAY_TYPE(self), SIZE_TYPE(n));

/* The element at position 'a' and the element at position 'b'
 * are swapped.
 */
NONE_TYPE(array_swap_elems)(ARRAY_TYPE(self), SIZE_TYPE(a), SIZE_TYPE(b));

/* Removes all elements from the array within start -> end
 * (which are ran through v->free), leaving the container with
 * a size of v->_nmemb - abs(start - end).
 */
NONE_TYPE(array_wipe)(ARRAY_TYPE(self), SIZE_TYPE(start), SIZE_TYPE(end));

/* Removes all the elements from the array and the capacity remains unchanged.
 */
NONE_TYPE(array_clear)(ARRAY_TYPE(self));

/* Removes the element at 'pos' from the array,
 * decreasing the size by one.
 */
NONE_TYPE(array_evict)(ARRAY_TYPE(self), SIZE_TYPE(p));

/* Adjusts the array capacity to be at least enough to
 * contain the current + 'n' elements.
 */
BOOL_TYPE(array_adjust)(ARRAY_TYPE(self), SIZE_TYPE(n));

/* Returns a pointer to the first element in the array.
 */
__attr_pure PTR_TYPE(array_head)(RDONLY_ARRAY_TYPE(self));

/* Returns a pointer to the last element in the array.
 */
__attr_pure PTR_TYPE(array_tail)(RDONLY_ARRAY_TYPE(self));

/* Dumps a trace of the array. If DISABLE_TRACING is defined
 * no data is collected at runtime.
 */
NONE_TYPE(array_trace)(RDONLY_ARRAY_TYPE(self));

#endif /* __ARRAY_H__*/
