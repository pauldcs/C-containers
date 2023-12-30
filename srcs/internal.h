#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define cut8_t const uint8_t
#define ct8_t const char
#define ut64_t uint64_t
#define st64_t int64_t
#define ut32_t uint32_t
#define st32_t int32_t
#define ut16_t uint16_t
#define st16_t int16_t
#define ut8_t uint8_t
#define st8_t int8_t
#define ptr_t void *
#define diff_t size_t
#define cstr_t const char *
#define custr_t const unsigned char *
#define str_t char *

typedef struct {
  st8_t _v;
} x_st8_t;
typedef struct {
  st16_t _v;
} x_st16_t;
typedef struct {
  st32_t _v;
} x_st32_t;
typedef struct {
  st64_t _v;
} x_st64_t;
typedef struct {
  ut8_t _v;
} x_ut8_t;
typedef struct {
  ut16_t _v;
} x_ut16_t;
typedef struct {
  ut32_t _v;
} x_ut32_t;
typedef struct {
  ut64_t _v;
} x_ut64_t;
typedef struct {
  cstr_t _ptr;
  size_t _size;
} x_str_t;
typedef struct {
  st32_t _val;
  st32_t _point;
} x_fixed_t;

#define __PTRIZE_ST8__(x)                                                      \
  &(x_st8_t) { ._v = x }
#define __PTRIZE_ST16__(x)                                                     \
  &(x_st16_t) { ._v = x }
#define __PTRIZE_ST32__(x)                                                     \
  &(x_st32_t) { ._v = x }
#define __PTRIZE_ST64__(x)                                                     \
  &(x_st64_t) { ._v = x }
#define __PTRIZE_UT8__(x)                                                      \
  &(x_ut8_t) { ._v = x }
#define __PTRIZE_UT16__(x)                                                     \
  &(x_ut16_t) { ._v = x }
#define __PTRIZE_UT32__(x)                                                     \
  &(x_ut32_t) { ._v = x }
#define __PTRIZE_UT64__(x)                                                     \
  &(x_ut64_t) { ._v = x }
#define __PTRIZE_STR__(s)                                                      \
  &(x_str_t) { ._ptr = s, ._size = strlen(s) }

#ifdef __has_builtin
#if __has_builtin(__builtin_memset) && __has_builtin(__builtin_memcpy) &&      \
    __has_builtin(__builtin_memmove)
#define BUILTIN_MEM_FUNCTIONS_AVAILABLE
#endif
#if __has_builtin(__builtin_expect)
#define BUILTIN_EXPECT_AVAILABLE
#endif
#endif

#ifndef BUILTIN_EXPECT_AVAILABLE
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) x
#define unlikely(x) x
#endif

#ifdef BUILTIN_MEM_FUNCTIONS_AVAILABLE
#define builtin_memset(dest, value, size) __builtin_memset(dest, value, size)
#define builtin_memcpy(dest, src, size) __builtin_memcpy(dest, src, size)
#define builtin_memmove(dest, src, size) __builtin_memmove(dest, src, size)
#else
#include <string.h>
#define builtin_memset(dest, value, size) memset(dest, value, size)
#define builtin_memcpy(dest, src, size) memcpy(dest, src, size)
#define builtin_memmove(dest, src, size) memmove(dest, src, size)
#endif

#if defined(DISABLE_HARDENED_RUNTIME)
#define RETURN_IF_FAIL(expr)
#define RETURN_VAL_IF_FAIL(expr, val)
#else
#define RETURN_IF_FAIL(expr)                                                   \
  do {                                                                         \
    if (!(expr)) {                                                             \
      (void)fprintf(stderr, "file '%s', line: %d\nHARDENED_RUNTIME: (%s)\n",   \
                    __FILE__, __LINE__, #expr);                                \
      return;                                                                  \
    };                                                                         \
  } while (0)

#define RETURN_VAL_IF_FAIL(expr, val)                                          \
  do {                                                                         \
    if (!(expr)) {                                                             \
      (void)fprintf(stderr, "file '%s', line: %d\nHARDENED_RUNTIME: (%s)\n",   \
                    __FILE__, __LINE__, #expr);                                \
      return val;                                                              \
    };                                                                         \
  } while (0)
#endif /* defined (DISABLE_ASSERTIONS) */

#define SAFE_TO_ADD(a, b, max) (a <= max - b)
#define SAFE_TO_MUL(a, b, max) (b == 0 || a <= max / b)
#define SAFE_TO_SUB(a, b, min) (a >= min + b)

#define SIZE_T_SAFE_TO_MUL(a, b) SAFE_TO_MUL(a, b, SIZE_MAX)
#define SIZE_T_SAFE_TO_ADD(a, b) SAFE_TO_ADD(a, b, SIZE_MAX)
#define SIZE_T_SAFE_TO_SUB(a, b) SAFE_TO_SUB(a, b, 0)

#endif /* __INTERNAL_H__ */
