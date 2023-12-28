#include "array.h"
#include "internal.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

array_allocator_t __array_allocator__ = {
    ._memory_alloc = malloc, ._memory_realloc = realloc, ._memory_free = free};

static inline bool array_init(array_t **self, size_t size) {
  *self = __array_allocator__._memory_alloc(sizeof(array_t));
  if (unlikely(!*self))
    return (false);

  (void)builtin_memset(*self, 0x00, sizeof(array_t));

  (*self)->_ptr = __array_allocator__._memory_alloc(size);
  if (unlikely(!(*self)->_ptr)) {
    __array_allocator__._memory_free(*self);
    return (false);
  }
  return (true);
}

array_t *array_create(size_t elt_size, size_t n, void (*free)(void *)) {
  RETURN_VAL_IF_FAIL(SIZE_MAX / elt_size > n, 0);

  if (!n) {
    n = ARRAY_INITIAL_SIZE;
  }

  size_t initial_cap = elt_size * n;
  array_t *array = NULL;

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
  RETURN_VAL_IF_FAIL(src, 0);
  RETURN_VAL_IF_FAIL(sp < ep, 0);
  RETURN_VAL_IF_FAIL(sp <= LONG_MAX, 0);
  RETURN_VAL_IF_FAIL(ep <= LONG_MAX, 0);
  RETURN_VAL_IF_FAIL(SIZE_MAX > (ep - sp) * src->_elt_size - 1, 0);
  RETURN_VAL_IF_FAIL(
      src->_elt_size * src->_nmemb >= (ep - sp) * src->_elt_size - 1, 0);

  void *ptr = __array_allocator__._memory_alloc((ep - sp) * src->_elt_size);
  if (likely(ptr))
    (void)builtin_memcpy(ptr, GET_POINTER(src, sp), ep - sp);

  return (ptr);
}

array_t *array_pull(const array_t *src, st64_t sp, st64_t ep) {
  RETURN_VAL_IF_FAIL(src, 0);
  RETURN_VAL_IF_FAIL((sp >= LONG_MIN && sp <= LONG_MAX), 0);
  RETURN_VAL_IF_FAIL((ep >= LONG_MIN && ep <= LONG_MAX), 0);

  if (sp < 0)
    sp += src->_nmemb;
  if (ep < 0)
    ep += src->_nmemb;

  sp < ep ? (ep++) : (ep--);

  array_t *arr = NULL;
  size_t n_elems = labs(sp - ep);
  size_t size = n_elems * src->_elt_size;

  RETURN_VAL_IF_FAIL(sp + size < src->_elt_size * src->_nmemb, 0);

  if (unlikely(!array_init(&arr, size)))
    return (NULL);

  arr->_elt_size = src->_elt_size;
  arr->_free = src->_free;
  arr->_cap = size;
  arr->_nmemb = n_elems;
  arr->settled = true;

  if (sp < ep) {
    (void)builtin_memcpy(arr->_ptr, GET_POINTER(src, sp), size);

  } else {
    void *ptr = arr->_ptr;
    size_t step = src->_elt_size;

    while (sp != ep) {
      (void)builtin_memcpy(ptr, GET_POINTER(src, sp), step);
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
  RETURN_IF_FAIL(self);

  if (self->_free) {
    while (self->_nmemb--)
      self->_free(GET_POINTER(self, self->_nmemb));

  } else {
    self->_nmemb = 0;
  }
#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use = 0;
#endif
}

void array_kill(array_t *self) {
  RETURN_IF_FAIL(self);

  array_purge(self);
  if (self->_cap)
    __array_allocator__._memory_free(self->_ptr);

  __array_allocator__._memory_free(self);
}

bool array_adjust(array_t *self, size_t n) {
  RETURN_VAL_IF_FAIL(self, 0);
  RETURN_VAL_IF_FAIL(SIZE_MAX - self->_nmemb > n, 0);
  RETURN_VAL_IF_FAIL(SIZE_MAX / self->_elt_size > n, 0);

  size_t size;

  n += self->_nmemb;
  n *= self->_elt_size;

  if (n < self->_cap)
    return (true);

  if (unlikely(SETTLED(self)))
    return (false);

  size_t cap_2x = self->_cap * 2;
  if (cap_2x < 16) {
    cap_2x = 16;
  } else {
    if (cap_2x > SIZE_MAX)
      return (false);
  }

  if (n > cap_2x) {
    size = n;
  } else {
    size = cap_2x;
  }

  void *ptr = __array_allocator__._memory_realloc(self->_ptr, size);
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
  RETURN_VAL_IF_FAIL(self, 0);

  if (unlikely(!array_adjust(self, 1)))
    return (false);

  (void)builtin_memmove(GET_POINTER(self, self->_nmemb), elem, self->_elt_size);

  ++self->_nmemb;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += self->_elt_size;
#endif
  return (true);
}

void array_pop(array_t *self, void *into) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(self->_nmemb);

  void *p = GET_POINTER(self, --self->_nmemb);

  if (into)
    (void)builtin_memcpy(into, p, self->_elt_size);

  if (self->_free)
    self->_free(p);

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use -= self->_elt_size;
#endif
}

bool array_pushf(array_t *self, void *e) { return (array_insert(self, 0, e)); }

void array_popf(array_t *self, void *into) {
  RETURN_IF_FAIL(self);

  if (into)
    (void)builtin_memcpy(into, self->_ptr, self->_elt_size);
  array_evict(self, 0);
}

bool array_insert(array_t *self, size_t p, void *node) {
  RETURN_VAL_IF_FAIL(self, 0);
  RETURN_VAL_IF_FAIL(p <= self->_nmemb, 0);

  if (unlikely(!array_adjust(self, 1)))
    return (false);

  if (!self->_nmemb || p == self->_nmemb)
    goto skip;

  (void)builtin_memmove(GET_POINTER(self, p + 1), GET_POINTER(self, p),
                        self->_nmemb * self->_elt_size - p * self->_elt_size);

skip:
  (void)builtin_memcpy(GET_POINTER(self, p), node, self->_elt_size);
  ++self->_nmemb;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += self->_elt_size;
#endif
  return (true);
}

bool array_inject(array_t *self, size_t p, const void *src, size_t n) {
  RETURN_VAL_IF_FAIL(self, 0);
  RETURN_VAL_IF_FAIL(p <= self->_nmemb, 0);

  if (unlikely(!array_adjust(self, n)))
    return (false);

  if (!self->_nmemb || p == self->_nmemb)
    goto skip;

  (void)builtin_memmove(GET_POINTER(self, p + n), GET_POINTER(self, p),
                        self->_elt_size * (self->_nmemb - p));

skip:
  (void)builtin_memmove(GET_POINTER(self, p), src, self->_elt_size * n);
  self->_nmemb += n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += (n * self->_elt_size);
#endif
  return (true);
}

bool array_append(array_t *self, const void *src, size_t n) {
  RETURN_VAL_IF_FAIL(self, 0);
  RETURN_VAL_IF_FAIL(src, 0);

  if (unlikely(!array_adjust(self, n)))
    return (false);

  (void)builtin_memmove(GET_POINTER(self, self->_nmemb), src,
                        self->_elt_size * n);

  self->_nmemb += n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += (n * self->_elt_size);
#endif
  return (true);
}

const void *array_at(const array_t *self, size_t pos) {
  RETURN_VAL_IF_FAIL(self, 0);
  RETURN_VAL_IF_FAIL(self->_nmemb >= pos, 0);

  if (unlikely(pos >= self->_nmemb || 0 > pos))
    return (NULL);

  return (GET_POINTER(self, pos));
}

const void *array_unsafe_at(const array_t *self, size_t pos) {
  return (GET_POINTER(self, pos));
}

void *array_access(const array_t *self, size_t pos) {
  RETURN_VAL_IF_FAIL(self, 0);
  RETURN_VAL_IF_FAIL(self->_nmemb >= pos, 0);

  if (unlikely(pos >= self->_nmemb || 0 > pos))
    return (NULL);

  return (GET_POINTER(self, pos));
}

void *array_unsafe_access(const array_t *self, size_t pos) {
  return (GET_POINTER(self, pos));
}

void array_evict(array_t *self, size_t p) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(p < self->_nmemb);

  size_t __n = (self->_nmemb - p) * self->_elt_size;

  if (self->_free)
    self->_free(GET_POINTER(self, p));

  if (p <= --self->_nmemb) {
    (void)builtin_memmove(GET_POINTER(self, p), GET_POINTER(self, p + 1),
                          __n - self->_elt_size);
  }

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use -= self->_elt_size;
#endif
}

void array_wipe(array_t *self, size_t sp, size_t ep) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(sp < ep);
  RETURN_IF_FAIL(ep - sp <= self->_nmemb);

  size_t __n = ep - sp;

  if (self->_free) {
    size_t i = 0;
    while (i < __n)
      self->_free(GET_POINTER(self, i++));
  }

  (void)builtin_memmove(GET_POINTER(self, sp), GET_POINTER(self, ep),
                        (self->_nmemb - sp - __n) * self->_elt_size);

  self->_nmemb -= __n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use -= (__n * self->_elt_size);
#endif
}

void array_clear(array_t *self) {
  RETURN_IF_FAIL(self);

  self->_nmemb = 0;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use = 0;
#endif
}

void array_swap(array_t *self, size_t a, size_t b) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(a < self->_nmemb);
  RETURN_IF_FAIL(b < self->_nmemb);

  char *p = GET_POINTER(self, a);
  char *q = GET_POINTER(self, b);

  size_t __n = self->_elt_size;
  for (; __n--; ++p, ++q) {
    *p ^= *q;
    *q ^= *p;
    *p ^= *q;
  }
}

void *array_head(const array_t *self) { return (array_access(self, 0)); }

void *array_tail(const array_t *self) {
  RETURN_VAL_IF_FAIL(self, NULL);

  return (array_access(self, self->_nmemb));
}

size_t array_size(const array_t *self) {
  RETURN_VAL_IF_FAIL(self, -1);

  return (self->_nmemb);
}

size_t array_cap(const array_t *self) {
  RETURN_VAL_IF_FAIL(self, -1);

  return (self->_cap);
}

bool array_slimcheck(array_t *self) {
  RETURN_VAL_IF_FAIL(self, false);

  if (unlikely(SETTLED(self)))
    return (false);

  if (self->_cap) {
    size_t required = self->_elt_size * self->_nmemb;
    if (required < self->_cap / 2) {
      void *ptr = __array_allocator__._memory_realloc(self->_ptr, required);
      if (unlikely(!ptr))
        return (false);

      self->_ptr = ptr;
      self->_cap = required;
#if defined(ENABLE_STATISTICS)
      self->_stats.n_frees++;
      self->_stats.n_allocs++;
      self->_stats.n_bytes_allocd += required;
      self->_stats.n_bytes_reachable = required;
#endif
    }
  }
  return (true);
}

void array_settle(array_t *self) {
  RETURN_IF_FAIL(self);

  self->settled = true;
}

void array_unsettle(array_t *self) {
  RETURN_IF_FAIL(self);

  self->settled = false;
}

bool array_is_settled(array_t *self) {
  RETURN_VAL_IF_FAIL(self, false);

  return (self->settled);
}

void array_stats(const array_t *self) {
  RETURN_IF_FAIL(self);

  (void)fprintf(stderr, "ARRAY MEMORY STATISTICS:\n");
  (void)fprintf(stderr, "  allocations: %ld bytes in %ld blocks (%ld freed)\n",
                self->_stats.n_bytes_allocd, self->_stats.n_allocs,
                self->_stats.n_frees);
  (void)fprintf(stderr, "       in use: %ld bytes out of %ld reserved\n",
                self->_stats.n_bytes_in_use, self->_stats.n_bytes_reachable);
}
