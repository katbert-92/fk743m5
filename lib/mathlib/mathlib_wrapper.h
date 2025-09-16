#ifndef __MATHLIB_WRAPPER_H
#define __MATHLIB_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MATHLIB_USE_CUSTOM

#ifndef MATHLIB_USE_CUSTOM
#ifndef MATHLIB_MALLOC
#define MATHLIB_MALLOC(sz) (malloc((sz)))
#endif /* MATHLIB_MALLOC */

#ifndef MATHLIB_FREE
#define MATHLIB_FREE(pt) (free((pt)))
#endif /* MATHLIB_FREE */

#else /* MATHLIB_USE_CUSTOM */
#include "mem_wrapper.h"

#ifndef MATHLIB_MALLOC
// Maybe should use MEM_ALLOC_UNLIM_TMO
#define MATHLIB_MALLOC(sz) (MemWrap_Malloc((sz), __FILE__, __LINE__, MEM_ALLOC_DEF_TMO))
#endif /* MATHLIB_MALLOC */

#ifndef MATHLIB_FREE
#define MATHLIB_FREE(pt) (MemWrap_Free((pt)))
#endif /* MATHLIB_FREE */

#endif /* MATHLIB_USE_CUSTOM */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MATHLIB_WRAPPER_H */
