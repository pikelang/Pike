/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: acconfig.h,v 1.4 2002/10/08 20:22:42 nilsson Exp $
\*/

/*
 * Config-file for the Pike sybase driver module
 *
 * by Francesco Chemolli
 */

#ifndef __PIKE_SYBASE_CONFIG_H
#define __PIKE_SYBASE_CONFIG_H

@TOP@
@BOTTOM@

/* End of automatic session. Doing stuff now */

#undef PIKE_HAVE_LIBCOMN
#undef PIKE_HAVE_LIBCS
#undef PIKE_HAVE_LIBCT
#undef PIKE_HAVE_LIBINTL
#undef PIKE_HAVE_LIBSYBTCL

#if defined(PIKE_HAVE_LIBCOMN) && defined(PIKE_HAVE_LIBCS) \
  && defined(PIKE_HAVE_LIBCT) && defined(PIKE_HAVE_LIBINTL) \
  && defined(PIKE_HAVE_LIBSYBTCL) && defined(HAVE_CTPUBLIC_H)
#define HAVE_SYBASE 
#endif

#endif /* __PIKE_SYBASE_CONFIG_H */
