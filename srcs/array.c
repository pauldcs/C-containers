#include "array.h"
#include "internal.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
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

ARRAY_T(array_create)(SIZE_T(elt_size), SIZE_T(n), void (*free)(void *)) {
  RETURN_VAL_IF_FAIL(elt_size, NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_MUL(elt_size, n), NULL);

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
  array->_stats.history[array->_stats.hindex].alloc_size = initial_cap;
  array->_stats.history[array->_stats.hindex].pointer = array->_ptr;
  (array->_stats.hindex)++;
  (array->_stats.hindex) %= STATS_HISTORY_SIZE;
#endif
  return (array);
}

PTR_T(array_extract)(RDONLY_ARRAY_T(src), SIZE_T(sp), SIZE_T(ep)) {
  RETURN_VAL_IF_FAIL(src, NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_SUB(ep, sp), NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_MUL((ep - sp), src->_elt_size), NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_SUB(src->_nmemb, sp), NULL);
  RETURN_VAL_IF_FAIL((src->_nmemb - sp) >= (ep - sp), NULL);

  void *ptr = __array_allocator__._memory_alloc((ep - sp) * src->_elt_size);
  if (likely(ptr))
    (void)builtin_memcpy(ptr, GET_POINTER(src, sp), (ep - sp) * src->_elt_size);

  return (ptr);
}

ARRAY_T(array_pull)(RDONLY_ARRAY_T(src), INT_T(sp), INT_T(ep)) {
  RETURN_VAL_IF_FAIL(src, NULL);
  RETURN_VAL_IF_FAIL(src->_nmemb > ABS(sp), NULL);
  RETURN_VAL_IF_FAIL(src->_nmemb > ABS(ep), NULL);

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

  return (arr);
}

NONE_T(array_clear)(ARRAY_T(self)) {
  RETURN_IF_FAIL(self);

  if (self->_free) {
    while (self->_nmemb--)
      self->_free(GET_POINTER(self, self->_nmemb));
  }
  self->_nmemb = 0;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use = 0;
#endif
}

NONE_T(array_kill)(ARRAY_T(self)) {
  RETURN_IF_FAIL(self);

  array_clear(self);
  __array_allocator__._memory_free(self);
}

BOOL_T(array_adjust)(ARRAY_T(self), SIZE_T(n)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_ADD(self->_nmemb, n), false);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_MUL(self->_cap, 2), false);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_MUL(self->_nmemb + n, self->_elt_size),
                     false);

  size_t size = 0;

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
  self->_stats.history[self->_stats.hindex].alloc_size = size;
  self->_stats.history[self->_stats.hindex].pointer = self->_ptr;
  (self->_stats.hindex)++;
  (self->_stats.hindex) %= STATS_HISTORY_SIZE;
#endif
  return (true);
}

BOOL_T(array_push)(ARRAY_T(self), PTR_T(e)) {
  RETURN_VAL_IF_FAIL(self, false);

  if (unlikely(!array_adjust(self, 1)))
    return (false);

  (void)builtin_memmove(GET_POINTER(self, self->_nmemb), e, self->_elt_size);

  ++self->_nmemb;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += self->_elt_size;
#endif
  return (true);
}

NONE_T(array_pop)(ARRAY_T(self), PTR_T(into)) {
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

BOOL_T(array_pushf)(ARRAY_T(self), PTR_T(e)) {
  return (array_insert(self, 0, e));
}

NONE_T(array_popf)(ARRAY_T(self), PTR_T(into)) {
  RETURN_IF_FAIL(self);

  if (into)
    (void)builtin_memcpy(into, self->_ptr, self->_elt_size);
  array_evict(self, 0);
}

BOOL_T(array_insert)(ARRAY_T(self), SIZE_T(p), PTR_T(e)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(p <= self->_nmemb, false);

  if (unlikely(!array_adjust(self, 1)))
    return (false);

  if (!self->_nmemb || p == self->_nmemb)
    goto skip;

  (void)builtin_memmove(GET_POINTER(self, p + 1), GET_POINTER(self, p),
                        self->_nmemb * self->_elt_size - p * self->_elt_size);

skip:
  (void)builtin_memcpy(GET_POINTER(self, p), e, self->_elt_size);
  ++self->_nmemb;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += self->_elt_size;
#endif
  return (true);
}

NONE_T(array_copy)(ARRAY_T(self), SIZE_T(off), RDONLY_PTR_T(src), SIZE_T(n)) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(src);

  (void)builtin_memmove((char *)self->_ptr + off, src, n);
}

BOOL_T(array_inject)(ARRAY_T(self), SIZE_T(p), RDONLY_PTR_T(src), SIZE_T(n)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(src, false);
  RETURN_VAL_IF_FAIL(p <= self->_nmemb, false);

  if (unlikely(!array_adjust(self, n)))
    return (false);

  if (unlikely(!n))
    return (true);
  
  if (!self->_nmemb || p == self->_nmemb)
    goto skip_moving;

  (void)builtin_memmove(GET_POINTER(self, p + n), GET_POINTER(self, p),
                        self->_elt_size * (self->_nmemb - p));

skip_moving:
  (void)builtin_memmove(GET_POINTER(self, p), src, self->_elt_size * n);
  self->_nmemb += n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += (n * self->_elt_size);
#endif
  return (true);
}

BOOL_T(array_append)(ARRAY_T(self), RDONLY_PTR_T(src), SIZE_T(n)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(src, false);

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

RDONLY_PTR_T(array_at)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  RETURN_VAL_IF_FAIL(self, NULL);
  RETURN_VAL_IF_FAIL(self->_nmemb >= p, NULL);

  if (unlikely(p >= self->_nmemb))
    return (NULL);

  return (GET_POINTER(self, p));
}

RDONLY_PTR_T(array_unsafe_at)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  return (GET_POINTER(self, p));
}

PTR_T(array_access)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  RETURN_VAL_IF_FAIL(self, NULL);
  RETURN_VAL_IF_FAIL(self->_nmemb >= p, NULL);

  if (unlikely(p >= self->_nmemb))
    return (NULL);

  return (GET_POINTER(self, p));
}

PTR_T(array_unsafe_access)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  return (GET_POINTER(self, p));
}

NONE_T(array_evict)(ARRAY_T(self), SIZE_T(p)) {
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

NONE_T(array_wipe)(ARRAY_T(self), SIZE_T(sp), SIZE_T(ep)) {
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

NONE_T(array_swap_elems)(ARRAY_T(self), SIZE_T(a), SIZE_T(b)) {
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

PTR_T(array_head)(RDONLY_ARRAY_T(self)) { return (array_access(self, 0)); }

PTR_T(array_tail)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, NULL);

  if (!self->_nmemb)
    return (NULL);

  return (array_access(self, self->_nmemb - 1));
}

SIZE_T(array_size)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, 0);

  return (self->_nmemb);
}
PTR_T(array_data)(RDONLY_ARRAY_T(self)) { return array_head(self); }

PTR_T(array_uninitialized_data)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, NULL);

  return (array_unsafe_access(self, self->_nmemb));
}

SIZE_T(array_uninitialized_size)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, 0);

  size_t size_in_bytes = self->_cap - self->_nmemb * self->_elt_size;
  if (size_in_bytes) {
    size_in_bytes /= self->_elt_size;
  }
  return (size_in_bytes);
}

SIZE_T(array_cap)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, 0);

  return (self->_cap);
}

BOOL_T(array_append_from_capacity)(ARRAY_T(self), SIZE_T(n)) {
  RETURN_VAL_IF_FAIL(self, false);

  if (n > array_uninitialized_size(self))
    return (false);

  self->_nmemb += n;

#if defined(ENABLE_STATISTICS)
  self->_stats.n_bytes_in_use += n;
#endif
  return (true);
}

BOOL_T(array_slimcheck)(ARRAY_T(self)) {
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
      self->_stats.history[self->_stats.hindex].alloc_size = required;
      self->_stats.history[self->_stats.hindex].pointer = self->_ptr;
      (self->_stats.hindex)++;
      (self->_stats.hindex) %= STATS_HISTORY_SIZE;
#endif
    }
  }
  return (true);
}

NONE_T(array_settle)(ARRAY_T(self)) {
  RETURN_IF_FAIL(self);

  self->settled = true;
}

NONE_T(array_unsettle)(ARRAY_T(self)) {
  RETURN_IF_FAIL(self);

  self->settled = false;
}

BOOL_T(array_is_settled)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, false);

  return (self->settled);
}

#if defined(ENABLE_STATISTICS)
NONE_T(array_stats)(RDONLY_ARRAY_T(self)) {
  RETURN_IF_FAIL(self);

  size_t i = 0;
  size_t hindex = self->_stats.hindex;
  (void)fprintf(stderr, "ALLOC HISTORY:\n");
  while (i < 10) {
    if (i + 1 == hindex)
      (void)fprintf(stderr, " @ ");
    else
      (void)fprintf(stderr, "   ");

    (void)fprintf(stderr, "(%16p): %8ld\n", self->_stats.history[i].pointer,
                  self->_stats.history[i].alloc_size);
    i++;
  }
  (void)fprintf(stderr, "\nSTATISTICS:\n");
  (void)fprintf(stderr, " - %ld elements of %ld bytes:\n", self->_nmemb,
                self->_elt_size);
  (void)fprintf(
      stderr, "    - allocations: %ld bytes in %ld blocks (%ld freed)\n",
      self->_stats.n_bytes_allocd, self->_stats.n_allocs, self->_stats.n_frees);
  (void)fprintf(stderr, "    - in use:      %ld bytes out of %ld reserved\n",
                self->_stats.n_bytes_in_use, self->_stats.n_bytes_reachable);
}
#endif /* ENABLE_STATISTICS */
