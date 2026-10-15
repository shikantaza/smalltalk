#ifndef MOCK_GC_H
#define MOCK_GC_H

#include <stdlib.h>
#include <string.h>

// 1. Map allocation functions. 
// Note: GC_malloc zeros memory, so calloc(1, size) is the correct match.
#define GC_MALLOC(size)               calloc(1, (size))
#define GC_malloc(size)               calloc(1, (size))

// Atomic allocation doesn't guarantee zeroed memory, so plain malloc works fine.
#define GC_MALLOC_ATOMIC(size)        malloc(size)
#define GC_malloc_atomic(size)        malloc(size)

#define GC_REALLOC(ptr, size)         realloc((ptr), (size))
#define GC_realloc(ptr, size)         realloc((ptr), (size))

#define GC_FREE(ptr)                  free(ptr)
#define GC_free(ptr)                  free(ptr)

// 2. Stub out administrative and lifecycle functions
#define GC_INIT()                     ((void)0)
#define GC_gcollect()                 ((void)0)
#define GC_enable_incremental()       ((void)0)

#define GC_strdup(ptr)                strdup(ptr)
#define GC_strndup(ptr, size)         strndup((ptr), (size))

/* static inline int mock_posix_memalign(void **p, size_t align, size_t size) */
/* { */
/*     int rc = posix_memalign(p, align, size); */
/*     if (rc == 0) memset(*p, 0, size); */
/*     return rc; */
/* } */

#define GC_posix_memalign(ptr, alignment, size) posix_memalign((ptr), (alignment), (size))

#endif // MOCK_GC_H
