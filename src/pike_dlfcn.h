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
DLHANDLE *dlopen(char *name, int flags);
int dlclose(DLHANDLE *h);

#endif
