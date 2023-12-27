#ifndef __DYNSTR_H__
#define __DYNSTR_H__

#include "array.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

typedef struct {
  char *_ptr;    /* data pointer */
  size_t _nmemb; /* size in use */
  size_t _cap;   /* allocated memory (in bytes) */
} dynstr_t;

/* Creates a new empty dynamic string and adjusts its starting capacity
 * to be at least enough to hold 'n' + 1 characters.
 */
dynstr_t *dynstr_create(size_t n);

/* Marks the dynamic string as 'settled', which means that the pointer
 * '_ptr' will no longer change due to a reallocation. The buffer cannot be
 * reallocated.
 */
void dynstr_done(dynstr_t *self);

/* Creates a new dynamic string from the data pointed to by 'src',
 * while copying a maximum of 'n' bytes.
 */
dynstr_t *dynstr_assign(const char *src, st64_t n);

/* Frees the dynamic string.
 */
void dynstr_kill(dynstr_t *self);

/* Appends the string pointed to by 'src' into the dynamic string `self`.
 * It copies the string until '\0' if 'n' == -1, or until 'n'.
 */
bool dynstr_append(dynstr_t *self, const char *str, st64_t n);

/* Extract and returns the data from 'src' in between sp -> ep (included) into
 * a newly allocated array. If a position is negative, it is iterpreted as an
 * offset from the end.
 */
array_t *dynstr_pull(const dynstr_t *self, st64_t sp, st64_t ep);

/* Injects the string pointed to by 'src' into the dynamic string 'self', at
 * potitions 'p'.
 */
bool dynstr_inject(dynstr_t *self, size_t pos, const char *src, st64_t n);

/* Sets the dynamic string `self` size to 0
 */
void dynstr_clear(dynstr_t *self);

/* Adjusts the dynamic string `self` capacity to be at least enough to
 * contain the current size + n characters.
 */
bool dynstr_adjust(dynstr_t *self, size_t n);

/* Removes all characters from the dynamic string within index
 * start -> end
 */
void dynstr_wipe(dynstr_t *self, size_t start, size_t end);

/* Returns the size in bytes of the currently reverved buffer.
 */
size_t dynstr_cap(dynstr_t *self) __readonly;

#endif /* __DYNSTR_H__ */
