#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define ENABLE_STATISTICS
#define INDEX_TO_PTR(v, pos) ((char *)(v)->_ptr + (v)->_elt_size * (pos))

typedef struct {
  void *(*_memory_alloc)(size_t);
  void *(*_memory_realloc)(void *, size_t);
  void (*_memory_free)(void *);
} array_allocator_t;

extern array_allocator_t allocator;

typedef struct {
  void *_ptr;            /* data pointer */
  size_t _nmemb;         /* size in use */
  size_t _elt_size;      /* element size */
  size_t _cap;           /* allocated memory (in bytes) */
  void (*_free)(void *); /* element free function */
#if defined(ENABLE_STATISTICS)
  struct {
    size_t n_allocs;          /* allocation counter */
    size_t n_frees;           /* deallocation counter */
    size_t n_bytes_allocd;    /* total number of bytes allocated */
    size_t n_bytes_reachable; /* total number of bytes not deallocated yet */
    size_t n_bytes_in_use;    /* total number of actively used bytes */
  } _stats;
#else
#endif
} array_t;

/* Creates a array and adjusts its starting capacity to be at least
 * enough to hold 'n' elements.
 */
array_t *array_create(size_t elt_size, size_t n, void (*_free)(void *));

/* Returns the number of elements currently in the array
 */
size_t array_size(const array_t *v);

/* Returns the capacity of the allocated buffer
 */
size_t array_cap(const array_t *v);

/* Extracts all the content of src in between sp -> ep into
 * a new allocated void *.
 */
void *array_extract(const array_t *src, size_t sp, size_t ep);

/* Extract the content from 'src' in between sp -> ep (included) into a new
 * array. if one of the positions is negative, it is iterpreted as an offset
 * from the end.
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
array_t *array_pull(const array_t *src, st64_t sp, st64_t ep);

/* Frees all the elements from vec, leaving the array empty but valid.
 * The content is se to '\xff' before being freed.
 */
void array_purge(array_t *self);

/* Frees the allocated array, purging it beforhands if needed.
 */
void array_kill(array_t *self);

/* Assigns new contents to the container,
 * replacing its current contents, and modifying
 * its size accordingly.
 */
bool array_assign(
    array_t *self,
    const array_t *src); //---------------------------------- MISSING

/* Adds a new element at the end of the array, after its current
 * last element. The content of v is copied (or moved) to the
 * new element.
 */
bool array_push(array_t *self, void *e);

/* Removes the last element of the array, effectively reducing
 * the container size by one.
 */
void array_pop(array_t *self, void *into);

/* Adds a new element to the front of the array, before the
 * first element. The content of v is copied (or moved) to the
 * new element.
 */
bool array_pushf(array_t *self, void *e);

/* Removes the first element from the array, reducing
 * the container size by one.
 */
void array_popf(array_t *self, void *into);

/* copies src into v->_ptr like memcpy while
 * making sure to allocate more space if
 * necessary
 */
bool array_copy(array_t *self, void *dst, const void *src,
                size_t n); //-------------------------------------

/* The array is extended by injecting a new element before the
 * element at the specified position, effectively increasing
 * the container size by one.
 */
bool array_insert(array_t *self, size_t p, void *e);

/* Injects the content pointed to by 'src' into the array 'v', at
 * potitions 'p'. The data type size is expected to be the same as the array.
 */
bool array_inject(array_t *self, size_t p, const void *src, size_t n);

/* Appends the content pointed to by 'src' into the array 'v' (max n elements).
 * The data type size is expected to be the same as the array.
 */
bool array_append(array_t *self, const void *src, size_t n);

/* Returns a pointer to the element at position pos in
 * the array.
 */
// make faster version of this
void *array_access(const array_t *self, size_t p);
void *array_unsafe_access(const array_t *self, size_t p);

/* Identical to 'access', only this return a const pointer
 */
const void *array_at(const array_t *self, size_t p);
const void *array_unsafe_at(const array_t *self, size_t pos);

/* The element at position 'a' and the element at position 'b'
 * are swapped.
 */
void array_swap(array_t *self, size_t a, size_t b);

/* Removes all elements from the array within start - end
 * (which are ran through v->free), leaving the container with
 * a size of v->_nmemb - abs(start - end).
 */
void array_wipe(array_t *self, size_t sp, size_t ep);

/* Sets the array size to 0
 */
void array_clear(array_t *self);

/* Removes the element at 'pos' from the array,
 * decreasing the size by one.
 */
void array_evict(array_t *self, size_t p);

/* Adjusts the array capacity to be at least enough to
 * contain the current + n elements.
 */
bool array_adjust(array_t *self, size_t n);

/* Returns a pointer to the first element in the array.
 */
void *array_head(const array_t *self);

/* Returns a pointer to the last element in the array.
 */
void *array_tail(const array_t *self);

// bool         array_slimcheck    (const array_t *v);
// bool         array_swoop        (void);

/* if ENABLE_STATISTICS is define, this function dumps the stats
 * of the array
 */
void array_stats(const array_t *self);

#endif /* __array_H__*/
