#include "array.h"
#include "internal.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

array_allocator_t allocator = {
    ._memory_alloc = malloc, ._memory_realloc = realloc, ._memory_free = free};

static inline bool array_init(array_t **self, size_t size) {
  *self = allocator._memory_alloc(sizeof(array_t));
  if (unlikely(!*self))
    return (false);

  (void)memset(*self, 0x00, sizeof(array_t));

  (*self)->_ptr = allocator._memory_alloc(size);
  if (unlikely(!(*self)->_ptr)) {
    allocator._memory_free(*self);
    return (false);
  }
  return (true);
}

array_t *array_create(size_t elt_size, size_t n, void (*free)(void *)) {
  __retval_if_fail__(SIZE_MAX / elt_size > n, 0);

  array_t *array = NULL;
  size_t initial_cap;

  if (!n)
    n = 1 << 8;

  initial_cap = elt_size * n;

  if (likely(array_init(&array, initial_cap))) {
    array->_elt_size = elt_size;
    array->_free = free;
    array->_cap = initial_cap;
    array->settled = false;
  }

#if defined(ENABLE_STATISTICS)
  array->_stats.n_allocs = 1;
  array->_stats.n_bytes_allocd = initial_cap;
  array->_stats.n_bytes_reachable = initial_cap;
#endif
  return (array);
}

void *array_extract(const array_t *src, size_t sp, size_t ep) {
  __retval_if_fail__(src, 0);
  __retval_if_fail__(sp < ep, 0);

  void *ptr = allocator._memory_alloc((ep - sp) * src->_elt_size);
  if (likely(ptr))
    (void)memcpy(ptr, INDEX_TO_PTR(src, sp), ep - sp);

  return (ptr);
}

array_t *array_pull(const array_t *src, st64_t sp, st64_t ep) {
  __retval_if_fail__(src, 0);

  if (sp < 0)
    sp += src->_nmemb;
  if (ep < 0)
    ep += src->_nmemb;

  sp < ep ? (ep++) : (ep--);

  array_t *arr = NULL;
  size_t n_elems = labs(sp - ep);
  size_t size = n_elems * src->_elt_size;

  if (unlikely(!array_init(&arr, size)))
    return (NULL);

  arr->_elt_size = src->_elt_size;
  arr->_free = src->_free;
  arr->_cap = size;
  arr->_nmemb = n_elems;
  arr->settled = true;

  if (sp < ep) {
    (void)memcpy(arr->_ptr, INDEX_TO_PTR(src, sp), size);

  } else {
    void *ptr = arr->_ptr;
    size_t step = src->_elt_size;

    while (sp != ep) {
      (void)memcpy(ptr, INDEX_TO_PTR(src, sp), step);
      ptr = (char *)ptr + step;
      sp--;
    }
  }

#if defined(ENABLE_STATISTICS)
  arr->_stats.n_allocs = 1;
  arr->_stats.n_bytes_allocd = size;
  arr->_stats.n_bytes_reachable = size;
  arr->_stats.n_bytes_in_use = size;
#endif
  return (arr);
}

void array_purge(array_t *self) {
  __ret_if_fail__(self);

  //(void)memset(self->_ptr, '\xff', self->_cap);
  if (self->_free) {
    while (self->_nmemb--)
      self->_free(INDEX_TO_PTR(self, self->_nmemb));

  } else {
    self->_nmemb = 0;
  }
#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use = 0;
#endif
}

void array_kill(array_t *self) {
  __ret_if_fail__(self);

  array_purge(self);
  if (self->_cap)
    allocator._memory_free(self->_ptr);

  allocator._memory_free(self);
}

bool array_adjust(array_t *self, size_t n) {
  __retval_if_fail__(self, 0);
  __retval_if_fail__(SIZE_MAX - self->_nmemb > n, 0);
  __retval_if_fail__(SIZE_MAX / self->_elt_size > n, 0);
  __retval_if_fail__(!SETTLED(self), 0);

  size_t size;
  void *ptr;

  if (unlikely(SETTLED(self)))
    return (false);

  n += self->_nmemb;
  n *= self->_elt_size;

  if (n < self->_cap)
    return (true);

  n < self->_cap << 1 ? (size = self->_cap << 1) : (size = n);

  ptr = allocator._memory_realloc(self->_ptr, size);
  if (unlikely(!ptr))
    return (false);

  self->_cap = size;
  self->_ptr = ptr;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_frees++;
  self->_stats.n_allocs++;
  self->_stats.n_bytes_allocd += size;
  self->_stats.n_bytes_reachable = size;
#endif
  return (true);
}

bool array_push(array_t *self, void *elem) {
  __retval_if_fail__(self, 0);

  if (unlikely(!array_adjust(self, 1)))
    return (false);

  (void)memmove(INDEX_TO_PTR(self, self->_nmemb), elem, self->_elt_size);

  ++self->_nmemb;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += self->_elt_size;
#endif
  return (true);
}

void array_pop(array_t *self, void *into) {
  __ret_if_fail__(self);
  __ret_if_fail__(self->_nmemb);

  void *p = INDEX_TO_PTR(self, --self->_nmemb);

  if (into)
    (void)memcpy(into, p, self->_elt_size);

  if (self->_free)
    self->_free(p);

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use -= self->_elt_size;
#endif
}

bool array_pushf(array_t *self, void *e) {
  return (array_insert(self, 0, e));
}

void array_popf(array_t *self, void *into) {
  __ret_if_fail__(self);
  
  if (into)
    (void)memcpy(into, self->_ptr, self->_elt_size);
  array_evict(self, 0);
}

bool array_insert(array_t *self, size_t p, void *node) {
  __retval_if_fail__(self, 0);
  __retval_if_fail__(p <= self->_nmemb, 0);

  if (unlikely(!array_adjust(self, 1)))
    return (false);

  if (!self->_nmemb || p == self->_nmemb)
    goto skip;

  (void)memmove(INDEX_TO_PTR(self, p + 1), INDEX_TO_PTR(self, p),
                self->_nmemb * self->_elt_size - p * self->_elt_size);

skip:
  (void)memcpy(INDEX_TO_PTR(self, p), node, self->_elt_size);
  ++self->_nmemb;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += self->_elt_size;
#endif
  return (true);
}

bool array_inject(array_t *self, size_t p, const void *src, size_t n) {
  __retval_if_fail__(self, 0);
  __retval_if_fail__(p <= self->_nmemb, 0);

  if (unlikely(!array_adjust(self, n)))
    return (false);

  if (!self->_nmemb || p == self->_nmemb)
    goto skip;

  (void)memmove(INDEX_TO_PTR(self, p + n), INDEX_TO_PTR(self, p),
                self->_elt_size * (self->_nmemb - p));

skip:
  (void)memmove(INDEX_TO_PTR(self, p), src, self->_elt_size * n);
  self->_nmemb += n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += (n * self->_elt_size);
#endif
  return (true);
}

bool array_append(array_t *self, const void *src, size_t n) {
  __retval_if_fail__(self, 0);
  __retval_if_fail__(src, 0);

  if (unlikely(!array_adjust(self, n)))
    return (false);

  (void)memmove(INDEX_TO_PTR(self, self->_nmemb), src, self->_elt_size * n);

  self->_nmemb += n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += (n * self->_elt_size);
#endif
  return (true);
}

const void *array_at(const array_t *self, size_t pos) {
  __retval_if_fail__(self, 0);
  __retval_if_fail__(self->_nmemb >= pos, 0);

  if (unlikely(pos >= self->_nmemb || 0 > pos))
    return (NULL);

  return (INDEX_TO_PTR(self, pos));
}

const void *array_unsafe_at(const array_t *self, size_t pos) {
  return (INDEX_TO_PTR(self, pos));
}

void *array_access(const array_t *self, size_t pos) {
  __retval_if_fail__(self, 0);
  __retval_if_fail__(self->_nmemb >= pos, 0);

  if (unlikely(pos >= self->_nmemb || 0 > pos))
    return (NULL);

  return (INDEX_TO_PTR(self, pos));
}

void *array_unsafe_access(const array_t *self, size_t pos) {
  return (INDEX_TO_PTR(self, pos));
}

void array_evict(array_t *self, size_t p) {
  __ret_if_fail__(self);
  __ret_if_fail__(p < self->_nmemb);

  size_t __n = (self->_nmemb - p) * self->_elt_size;

  if (self->_free)
    self->_free(INDEX_TO_PTR(self, p));

  if (p <= --self->_nmemb) {
    (void)memmove(INDEX_TO_PTR(self, p), INDEX_TO_PTR(self, p + 1),
                __n - self->_elt_size);
  }

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use -= self->_elt_size;
#endif
}

void array_wipe(array_t *self, size_t sp, size_t ep) {
  __ret_if_fail__(self);
  __ret_if_fail__(sp < ep);
  __ret_if_fail__(ep - sp <= self->_nmemb);

  size_t __n = ep - sp;

  if (self->_free) {
    size_t i = 0;
    while (i < __n)
      self->_free(INDEX_TO_PTR(self, i++));
  }

  (void)memmove(INDEX_TO_PTR(self, sp), INDEX_TO_PTR(self, ep),
                (self->_nmemb - sp - __n) * self->_elt_size);

  self->_nmemb -= __n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use -= (__n * self->_elt_size);
#endif
}

void array_clear(array_t *self) {
  __ret_if_fail__(self);

  self->_nmemb = 0;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use = 0;
#endif
}

void array_swap(array_t *self, size_t a, size_t b) {
  __ret_if_fail__(self);
  __ret_if_fail__(a < self->_nmemb);
  __ret_if_fail__(b < self->_nmemb);

  char *p = INDEX_TO_PTR(self, a);
  char *q = INDEX_TO_PTR(self, b);

  size_t __n = self->_elt_size;
  for (; __n--; ++p, ++q) {
    *p ^= *q;
    *q ^= *p;
    *p ^= *q;
  }
}

void *array_head(const array_t *self) {
  return (array_access(self, 0));
}

void *array_tail(const array_t *self) {
  __retval_if_fail__(self, NULL);

  return (array_access(self, self->_nmemb));
}

size_t array_size(const array_t *self) {
  __retval_if_fail__(self, -1);

  return (self->_nmemb);
}

size_t array_cap(const array_t *self) {
  __retval_if_fail__(self, -1);

  return (self->_cap);
}

void array_settle(array_t *self) {
  __ret_if_fail__(self);
  
  self->settled = true;
}

void array_unsettle(array_t *self) {
  __ret_if_fail__(self);
  
  self->settled = false;
}

bool array_is_settled(array_t *self) {
  __retval_if_fail__(self, false);
  
  return (self->settled);
}


void array_stats(const array_t *self) {
  __ret_if_fail__(self);

  (void)fprintf(stderr, "ARRAY MEMORY STATISTICS:\n");
  (void)fprintf(
      stderr, "  allocations: %ld bytes in %ld blocks (%ld blocks freed)\n",
      self->_stats.n_bytes_allocd, self->_stats.n_allocs, self->_stats.n_frees);
  (void)fprintf(stderr, "       in use: %ld bytes out of %ld reserved\n",
                self->_stats.n_bytes_in_use, self->_stats.n_bytes_reachable);
}
