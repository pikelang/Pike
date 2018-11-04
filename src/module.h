/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef MODULE_H
#define MODULE_H

/* global.h includes machine.h, pike_int_type.h, port.h, dmalloc.h,
   <stdio.h>, <stdarg.h>, <stdlib.h>, <stddef.h>, <string.h>,
   <limits.h>, <float.h>, <assert.h> and <alloca.h> */
#include "global.h"

#if defined(DYNAMIC_MODULE) && defined(__NT__)
#define PIKE_MODULE_INIT __declspec(dllexport) void pike_module_init(void)
#define PIKE_MODULE_EXIT __declspec(dllexport) void pike_module_exit(void)
#else
#define PIKE_MODULE_INIT PMOD_EXPORT void pike_module_init(void)
#define PIKE_MODULE_EXIT PMOD_EXPORT void pike_module_exit(void)
#endif /* DYNAMIC_MODULE && __NT__ */

PIKE_MODULE_INIT;
PIKE_MODULE_EXIT;

/*
   pike_memory.h -> block_alloc_h.h
*/
#include "pike_memory.h"

/*
   svalue.h  -> buffer.h -> bitvector.h
*/
#include "svalue.h"

/*
    array.h -> dmalloc.h
 */
#include "array.h"

/*
    mapping.h -> dmalloc.h
              -> svalue.h
 */
#include "mapping.h"

/*
   multiset.h -> dmalloc.h
              -> svalue.h
              -> rbtree.h -> array.h
*/
#include "multiset.h"

/*
    stralloc.h -> pike_macros.h -> pike_memory.h
 */
#include "stralloc.h"

#include "object.h"

/*
    program.h -> pike_error.h
              -> svalue.h
              -> time_stuff.h
 */
#include "program.h"

#endif
