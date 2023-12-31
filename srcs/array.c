#include "array.h"
#include "internal.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

array_allocator_t __array_allocator__ = {
    ._memory_alloc = malloc, ._memory_realloc = realloc, ._memory_free = free};

/* Aligns the size by the machine word.
 */
static inline size_t size_align(size_t n) {
  return (n + sizeof(PTR_T()) - 1) & ~(sizeof(PTR_T()) - 1);
}

static inline bool array_init(array_t **self, size_t size) {
  *self = __array_allocator__._memory_alloc(sizeof(array_t));

  if (unlikely(!*self)) {
    return (false);
  }

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

  SIZE_T(initial_cap) = size_align(elt_size * n);
  ARRAY_T(array) = NULL;

  if (likely(array_init(&array, initial_cap))) {
    array->_elt_size = elt_size;
    array->_free = free;
    array->_cap = initial_cap;
    array->settled = false;
  }

  IF_TRACING(array->_meta.n_allocs = 1);
  IF_TRACING(array->_meta.n_bytes_allocd = initial_cap);
  IF_TRACING(array->_meta.n_bytes_reachable = initial_cap);
  IF_TRACING(array->_meta.trace[array->_meta.tidx].alloc_size = initial_cap);
  IF_TRACING(array->_meta.trace[array->_meta.tidx].pointer = array->_ptr);
  IF_TRACING((array->_meta.tidx)++);
  IF_TRACING((array->_meta.tidx) %= META_TRACE_SIZE);
  
  return (array);
}

PTR_T(array_extract)(RDONLY_ARRAY_T(src), SIZE_T(sp), SIZE_T(ep)) {
  RETURN_VAL_IF_FAIL(src, NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_SUB(ep, sp), NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_MUL((ep - sp), src->_elt_size), NULL);
  RETURN_VAL_IF_FAIL(SIZE_T_SAFE_TO_SUB(src->_nmemb, sp), NULL);
  RETURN_VAL_IF_FAIL((src->_nmemb - sp) >= (ep - sp), NULL);

  PTR_T(ptr) = __array_allocator__._memory_alloc((ep - sp) * src->_elt_size);

  if (likely(ptr)) {
    (void)builtin_memcpy(ptr, GET_POINTER(src, sp), (ep - sp) * src->_elt_size);
  }

  return (ptr);
}

ARRAY_T(array_pull)(RDONLY_ARRAY_T(src), INT_T(sp), INT_T(ep)) {
  RETURN_VAL_IF_FAIL(src, NULL);
  RETURN_VAL_IF_FAIL(src->_nmemb > ABS(sp), NULL);
  RETURN_VAL_IF_FAIL(src->_nmemb > ABS(ep), NULL);

  ARRAY_T(arr) = NULL;

  if (sp < 0) {
    sp += src->_nmemb;
  }

  if (ep < 0) {
    ep += src->_nmemb;
  }

  sp < ep ? (ep++) : (ep--);

  SIZE_T(n_elems) = labs(sp - ep);
  SIZE_T(size) = n_elems * src->_elt_size;

  if (unlikely(!array_init(&arr, size))) {
    return (NULL);
  }

  arr->_elt_size = src->_elt_size;
  arr->_free = src->_free;
  arr->_cap = size;
  arr->_nmemb = n_elems;
  arr->settled = true;

  if (sp < ep) {
    (void)builtin_memcpy(arr->_ptr, GET_POINTER(src, sp), size);

  } else {
    PTR_T(ptr) = arr->_ptr;
    SIZE_T(step) = src->_elt_size;

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
    while (self->_nmemb--) {
      self->_free(GET_POINTER(self, self->_nmemb));
    }
  }

  self->_nmemb = 0;

  IF_TRACING(self->_meta.n_bytes_in_use = 0);
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

  SIZE_T(new_size) = 0;

  n += self->_nmemb;
  n *= self->_elt_size;

  if (likely(n < self->_cap)) {
    return (true);
  }

  if (unlikely(SETTLED(self))) {
    return (false);
  }

  SIZE_T(cap_2x) = self->_cap * 2;

  if (cap_2x < 16) {
    cap_2x = 16;
  } else {
    if (unlikely(cap_2x > SIZE_MAX))
      return (false);
  }

  if (n > cap_2x) {
    new_size = size_align(n);
  } else {
    new_size = cap_2x;
  }

  PTR_T(ptr) = __array_allocator__._memory_realloc(self->_ptr, new_size);

  if (unlikely(!ptr)) {
    return (false);
  }

  self->_cap = new_size;
  self->_ptr = ptr;

  IF_TRACING(self->_meta.n_frees++);
  IF_TRACING(self->_meta.n_allocs++);
  IF_TRACING(self->_meta.n_bytes_allocd += new_size);
  IF_TRACING(self->_meta.n_bytes_reachable = new_size);
  IF_TRACING(self->_meta.trace[self->_meta.tidx].alloc_size = new_size);
  IF_TRACING(self->_meta.trace[self->_meta.tidx].pointer = self->_ptr);
  IF_TRACING((self->_meta.tidx)++);
  IF_TRACING((self->_meta.tidx) %= META_TRACE_SIZE);

  return (true);
}

BOOL_T(array_push)(ARRAY_T(self), PTR_T(e)) {
  RETURN_VAL_IF_FAIL(self, false);

  if (unlikely(!array_adjust(self, 1))) {
    return (false);
  }

  (void)builtin_memmove(GET_POINTER(self, self->_nmemb), e, self->_elt_size);

  ++self->_nmemb;

  IF_TRACING(self->_meta.n_bytes_in_use += self->_elt_size);

  return (true);
}

NONE_T(array_pop)(ARRAY_T(self), PTR_T(into)) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(self->_nmemb);

  PTR_T(p) = GET_POINTER(self, --self->_nmemb);

  if (into) {
    (void)builtin_memcpy(into, p, self->_elt_size);
  }

  if (self->_free) {
    self->_free(p);
  }

  IF_TRACING(self->_meta.n_bytes_in_use -= self->_elt_size);
}

BOOL_T(array_pushf)(ARRAY_T(self), PTR_T(e)) {
  return (array_insert(self, 0, e));
}

NONE_T(array_popf)(ARRAY_T(self), PTR_T(into)) {
  RETURN_IF_FAIL(self);

  if (into) {
    (void)builtin_memcpy(into, self->_ptr, self->_elt_size);
  }

  array_evict(self, 0);
}

BOOL_T(array_insert)(ARRAY_T(self), SIZE_T(p), PTR_T(e)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(p <= self->_nmemb, false);

  if (unlikely(!array_adjust(self, 1))) {
    return (false);
  }

  if (!self->_nmemb || p == self->_nmemb) {
    goto skip;
  }

  (void)builtin_memmove(GET_POINTER(self, p + 1), GET_POINTER(self, p),
                        self->_nmemb * self->_elt_size - p * self->_elt_size);

skip:
  (void)builtin_memcpy(GET_POINTER(self, p), e, self->_elt_size);
  ++self->_nmemb;

  IF_TRACING(self->_meta.n_bytes_in_use += self->_elt_size);

  return (true);
}

NONE_T(array_tipex)(ARRAY_T(self), SIZE_T(off), RDONLY_PTR_T(src), SIZE_T(n)) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(src);

  (void)builtin_memmove((char *)self->_ptr + off, src, n);
}

BOOL_T(array_inject)(ARRAY_T(self), SIZE_T(p), RDONLY_PTR_T(src), SIZE_T(n)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(src, false);
  RETURN_VAL_IF_FAIL(p <= self->_nmemb, false);

  if (unlikely(!array_adjust(self, n))) {
    return (false);
  }

  if (unlikely(!n)) {
    return (true);
  }

  if (!self->_nmemb || p == self->_nmemb) {
    goto skip_moving;
  }

  (void)builtin_memmove(GET_POINTER(self, p + n), GET_POINTER(self, p),
                        self->_elt_size * (self->_nmemb - p));

skip_moving:
  (void)builtin_memmove(GET_POINTER(self, p), src, self->_elt_size * n);
  self->_nmemb += n;

  IF_TRACING(self->_meta.n_bytes_in_use += (n * self->_elt_size));

  return (true);
}

BOOL_T(array_append)(ARRAY_T(self), RDONLY_PTR_T(src), SIZE_T(n)) {
  RETURN_VAL_IF_FAIL(self, false);
  RETURN_VAL_IF_FAIL(src, false);

  if (unlikely(!array_adjust(self, n))) {
    return (false);
  }

  (void)builtin_memmove(GET_POINTER(self, self->_nmemb), src,
                        self->_elt_size * n);

  self->_nmemb += n;

  IF_TRACING(self->_meta.n_bytes_in_use += (n * self->_elt_size));

  return (true);
}

RDONLY_PTR_T(array_at)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  RETURN_VAL_IF_FAIL(self, NULL);
  RETURN_VAL_IF_FAIL(self->_nmemb > p, NULL);

  if (unlikely(p >= self->_nmemb)) {
    return (NULL);
  }

  return (GET_POINTER(self, p));
}

RDONLY_PTR_T(array_unsafe_at)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  return (GET_POINTER(self, p));
}

PTR_T(array_access)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  RETURN_VAL_IF_FAIL(self, NULL);
  RETURN_VAL_IF_FAIL(self->_nmemb > p, NULL);

  if (unlikely(p >= self->_nmemb)) {
    return (NULL);
  }

  return (GET_POINTER(self, p));
}

PTR_T(array_unsafe_access)(RDONLY_ARRAY_T(self), SIZE_T(p)) {
  return (GET_POINTER(self, p));
}

NONE_T(array_evict)(ARRAY_T(self), SIZE_T(p)) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(p < self->_nmemb);

  SIZE_T(__n) = (self->_nmemb - p) * self->_elt_size;

  if (self->_free) {
    self->_free(GET_POINTER(self, p));
  }

  if (p <= --self->_nmemb) {
    (void)builtin_memmove(GET_POINTER(self, p), GET_POINTER(self, p + 1),
                          __n - self->_elt_size);
  }

  IF_TRACING(self->_meta.n_bytes_in_use -= self->_elt_size);
}

NONE_T(array_wipe)(ARRAY_T(self), SIZE_T(sp), SIZE_T(ep)) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(sp < ep);
  RETURN_IF_FAIL(ep - sp <= self->_nmemb);

  SIZE_T(__n) = ep - sp;

  if (self->_free) {
    size_t i = 0;
  
    while (i < __n) {
      self->_free(GET_POINTER(self, i++));
    }
  }

  (void)builtin_memmove(GET_POINTER(self, sp), GET_POINTER(self, ep),
                        (self->_nmemb - sp - __n) * self->_elt_size);

  self->_nmemb -= __n;

  IF_TRACING(self->_meta.n_bytes_in_use -= (__n * self->_elt_size));
}

NONE_T(array_swap_elems)(ARRAY_T(self), SIZE_T(a), SIZE_T(b)) {
  RETURN_IF_FAIL(self);
  RETURN_IF_FAIL(a < self->_nmemb);
  RETURN_IF_FAIL(b < self->_nmemb);

  SIZE_T(__n) = self->_elt_size;

  char *p = GET_POINTER(self, a);
  char *q = GET_POINTER(self, b);

  for (; __n--; ++p, ++q) {
    *p ^= *q;
    *q ^= *p;
    *p ^= *q;
  }
}

PTR_T(array_head)(RDONLY_ARRAY_T(self)) { return (array_access(self, 0)); }

PTR_T(array_tail)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, NULL);

  if (!self->_nmemb) {
    return (NULL);
  }

  return (array_access(self, self->_nmemb - 1));
}

SIZE_T(array_size)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, 0);

  return (self->_nmemb);
}

PTR_T(array_data)(RDONLY_ARRAY_T(self)) { return array_head(self); }

PTR_T(array_uninitialized_data)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, NULL);
  RETURN_VAL_IF_FAIL(self->_ptr, NULL);

  return (array_unsafe_access(self, self->_nmemb));
}

SIZE_T(array_uninitialized_size)(RDONLY_ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, 0);

  SIZE_T(size_in_bytes) = self->_cap - self->_nmemb * self->_elt_size;

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

  if (n > array_uninitialized_size(self)) {
    return (false);
  }

  self->_nmemb += n;

  IF_TRACING(self->_meta.n_bytes_in_use += n);

  return (true);
}

BOOL_T(array_slimcheck)(ARRAY_T(self)) {
  RETURN_VAL_IF_FAIL(self, false);

  if (unlikely(SETTLED(self))) {
    return (false);
  }

  if (self->_cap) {
    SIZE_T(size) = self->_elt_size * self->_nmemb;

    if (size < self->_cap / 2) {
      PTR_T(ptr) = __array_allocator__._memory_realloc(self->_ptr, size);

      if (unlikely(!ptr)) {
        return (false);
      }

      self->_ptr = ptr;
      self->_cap = size;

      IF_TRACING(self->_meta.n_frees++);
      IF_TRACING(self->_meta.n_allocs++);
      IF_TRACING(self->_meta.n_bytes_allocd += size);
      IF_TRACING(self->_meta.n_bytes_reachable = size);
      IF_TRACING(self->_meta.trace[self->_meta.tidx].alloc_size = size);
      IF_TRACING(self->_meta.trace[self->_meta.tidx].pointer = self->_ptr);
      IF_TRACING((self->_meta.tidx)++);
      IF_TRACING((self->_meta.tidx) %= META_TRACE_SIZE);
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

NONE_T(array_trace)(RDONLY_ARRAY_T(self)) {
  RETURN_IF_FAIL(self);

#if defined (DISABLE_STATISTICS)
  (void)self;
#else

  size_t i = 0;
  size_t tidx = self->_meta.tidx;
  (void)fprintf(stderr, "ALLOC TRACE:\n");

  while (i < 10) {
    if (i + 1 == tidx)
      (void)fprintf(stderr, " @ ");
    else
      (void)fprintf(stderr, "   ");

    (void)fprintf(stderr, "(%16p): %8ld\n", self->_meta.trace[i].pointer,
                  self->_meta.trace[i].alloc_size);
    i++;
  }

  (void)fprintf(stderr, "\nARRAY SUMMARY:\n");
  (void)fprintf(stderr, " - %ld elements of %ld bytes:\n", self->_nmemb,
                self->_elt_size);
  (void)fprintf(
      stderr, "    - allocations: %ld bytes in %ld blocks (%ld freed)\n",
      self->_meta.n_bytes_allocd, self->_meta.n_allocs, self->_meta.n_frees);
  (void)fprintf(stderr, "    - in use:      %ld bytes out of %ld reserved\n",
                self->_meta.n_bytes_in_use, self->_meta.n_bytes_reachable);
#endif
}
