/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* $Id$ */
#ifndef RCSID_H_INCLUDED
#define RCSID_H_INCLUDED

/* Taken from pike/src/global.h */

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define RCSID2(name, X) \
 static char *name __attribute__ ((unused)) =X
#elif __GNUC__ == 2
 /* No need for PIKE_CONCAT() here since gcc supports ## */
#define RCSID2(name, X) \
 static char *name = X; \
 static void *use_##name=(&use_##name, (void *)&name)
#else
#define RCSID2(name, X) \
 static char *name = X
#endif

#endif /* RCSID_H_INCLUDED */
