/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: acconfig.h,v 1.6 2005/03/21 15:16:09 grubba Exp $
*/

/*
 * Config-file for the Pike sybase driver module
 *
 * by Francesco Chemolli
 */

#ifndef __PIKE_SYBASE_CONFIG_H
#define __PIKE_SYBASE_CONFIG_H

@TOP@

/* Define if you have libintl */
#undef PIKE_HAVE_LIBINTL

/* Define if you have libcomn */
#undef PIKE_HAVE_LIBCOMN

/* Define if you have libcs */
#undef PIKE_HAVE_LIBCS

/* Define if you have libsystcl */
#undef PIKE_HAVE_LIBSYBTCL

/* Define if you have libct */
#undef PIKE_HAVE_LIBCT

/* Define if you have -framework SybaseOpenClient */
#undef HAVE_FRAMEWORK_SYBASEOPENCLIENT

@BOTTOM@

/* End of automatic session. Doing stuff now */

#if defined(HAVE_SYBASEOPENCLIENT_SYBASEOPENCLIENT_H) && \
  defined(HAVE_FRAMEWORK_SYBASEOPENCLIENT)
#define HAVE_SYBASE
#elif defined(PIKE_HAVE_LIBCOMN) && defined(PIKE_HAVE_LIBCS) \
  && defined(PIKE_HAVE_LIBCT) && defined(PIKE_HAVE_LIBINTL) \
  && defined(PIKE_HAVE_LIBSYBTCL) && defined(HAVE_CTPUBLIC_H)
#define HAVE_SYBASE 
#endif

#endif /* __PIKE_SYBASE_CONFIG_H */
