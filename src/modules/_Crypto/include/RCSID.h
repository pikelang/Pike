#ifndef RCSID_H_INCLUDED
#define RCSID_H_INCLUDED

/* Taken from pike/src/global.h */

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
#define RCSID2(name, X) \
 static char *name __attribute__ ((unused)) =X
#elif __GNUC__ == 2
#define RCSID2(name, X) \
 static char *name = X; \
 static void *use_##name=(&use_##name, (void *)&name)
#else
#define RCSID2(name, X) \
 static char *name = X
#endif

#endif /* RCSID_H_INCLUDED */
