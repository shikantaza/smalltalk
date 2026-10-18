/**
  Copyright 2026 Rajesh Jayaprakash <rajesh.jayaprakash@protonmail.com>

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  It is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this file.  If not, see <http://www.gnu.org/licenses/>.
**/

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

#define GC_posix_memalign(ptr, alignment, size) posix_memalign((ptr), (alignment), (size))

#endif // MOCK_GC_H
