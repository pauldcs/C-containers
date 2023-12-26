#ifndef __INTERNAL_H__
# define __INTERNAL_H__

# include <stddef.h>
# include <stdint.h>
# include <stdbool.h>
# include <stdio.h>
# include <string.h>

# define cut8_t  const uint8_t
# define ct8_t      const char
# define ut64_t       uint64_t
# define st64_t        int64_t
# define ut32_t       uint32_t
# define st32_t        int32_t
# define ut16_t       uint16_t
# define st16_t        int16_t
# define ut8_t         uint8_t
# define st8_t          int8_t
# define ptr_t          void *
# define diff_t         size_t
# define cstr_t         const char *
# define custr_t        const unsigned char *
# define str_t          char *

typedef struct { st8_t  _v; } x_st8_t;
typedef struct { st16_t _v; } x_st16_t;
typedef struct { st32_t _v; } x_st32_t;
typedef struct { st64_t _v; } x_st64_t;
typedef struct { ut8_t  _v; } x_ut8_t;
typedef struct { ut16_t _v; } x_ut16_t;
typedef struct { ut32_t _v; } x_ut32_t;
typedef struct { ut64_t _v; } x_ut64_t;
typedef struct { cstr_t _ptr; size_t _size; } x_str_t;
typedef struct { st32_t _val; st32_t _point; } x_fixed_t;

# define __PTRIZE_ST8__(x)  &(x_st8_t)  { ._v = x }
# define __PTRIZE_ST16__(x) &(x_st16_t) { ._v = x }
# define __PTRIZE_ST32__(x) &(x_st32_t) { ._v = x }
# define __PTRIZE_ST64__(x) &(x_st64_t) { ._v = x }
# define __PTRIZE_UT8__(x)  &(x_ut8_t)  { ._v = x }
# define __PTRIZE_UT16__(x) &(x_ut16_t) { ._v = x }
# define __PTRIZE_UT32__(x) &(x_ut32_t) { ._v = x }
# define __PTRIZE_UT64__(x) &(x_ut64_t) { ._v = x }
# define __PTRIZE_STR__(s)  &(x_str_t)  { \
	._ptr  = s,            \
	._size = strlen(s) \
}

# define likely(x)	 (__builtin_expect(!!(x), 1))
# define unlikely(x) (__builtin_expect(!!(x), 0))

# if defined (DISABLE_ASSERTIONS)
# define __ret_if_fail__(expr) 
# define __retval_if_fail__(expr,val)
# else
# define __ret_if_fail__(expr) \
	do { \
		if (!(expr)) { \
			(void)fprintf(stderr, "file '%s', line: %d\n/!\\ alert: (%s)\n", \
				__FILE__, __LINE__, \
				#expr);	\
			return;	\
		}; \
	} while (0)

# define __retval_if_fail__(expr,val) \
	do { \
    	if (!(expr)) { \
	 		(void)fprintf(stderr, "file '%s', line: %d\n/!\\ alert: (%s)\n", \
				__FILE__, __LINE__,  \
				#expr);	\
	 		return val; \
       	}; \
	} while (0)
# endif /* defined (ENABLE_ASSERTIONS) */

#endif /* __INTERNAL_H__ */
