/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef PIKE_DLFCN_H
#define PIKE_DLFCN_H

#define RTLD_GLOBAL 1
#define RTLD_LAZY 0 /* never */
#define RTLD_NOW 0 /* always */

#ifdef DL_INTERNAL
#define DLHANDLE struct DLHandle
#else
#define DLHANDLE void
#endif

void *dlsym(DLHANDLE *handle, char *name);
const char *dlerror(void);
DLHANDLE *dlopen(const char *name, int flags);
int dlclose(DLHANDLE *h);

#endif
