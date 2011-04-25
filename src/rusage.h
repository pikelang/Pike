/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef RUSAGE_H
#define RUSAGE_H

/* Prototypes begin here */
typedef INT32 pike_rusage_t[30];
int pike_get_rusage(pike_rusage_t rusage_values);
INT32 *low_rusage(void);
INT32 internal_rusage(void);
#if defined(PIKE_DEBUG) || defined(INTERNAL_PROFILING)
void debug_print_rusage(FILE *out);
#endif
/* Prototypes end here */

#endif
