#include "global.h"
RCSID("$id: $");

#include "config.h"

#if !defined(HAVE_LIBJPEG)
#undef HAVE_JPEGLIB_H
#endif

#ifdef HAVE_JPEGLIB_H



#endif /* HAVE_JPEGLIB_H */


void pike_module_exit(void)
{
}

void pike_module_init(void)
{
#ifdef HAVE_JPEGLIB_H
#endif /* HAVE_JPEGLIB_H */
}
