/*
 * $Id: acconfig.h,v 1.1 2000/03/26 20:43:08 kinkie Exp $
 * 
 * Config-file for the Pike sybase driver module
 *
 * by Francesco Chemolli
 * (C) Roxen IS
 */

#ifndef __PIKE_SYBASE_CONFIG_H
#define __PIKE_SYBASE_CONFIG_H

@TOP@
@BOTTOM@

/* End of automatic session. Doing stuff now */

#if defined(HAVE_LIBCOMN) || defined(HAVE_LIBCOMN64) || \
  defined(HAVE_LIBCOMN_DCE) || defined(HAVE_LIBCOMN_DCE64) || \
  defined(HAVE_LIBCOMN_R)
#define PIKE_HAVE_LIBCOMN
#endif

#if defined(HAVE_LIBCS) || defined(HAVE_LIBCS64) || \
  defined(HAVE_LIBCS_R) || defined(HAVE_LIBCS_R64)
#define PIKE_HAVE_LIBCS
#endif

#if defined(HAVE_LIBCT) || defined(HAVE_LIBCT64) || \
  defined(HAVE_LIBCT_R) || defined(HAVE_LIBCT_R64)
#define PIKE_HAVE_LIBCT
#endif

#if defined(HAVE_LIBINTL) || defined(HAVE_LIBINTL64) || \
  defined(HAVE_LIBINTL_R) || defined(HAVE_LIBINTL_R64)
#define PIKE_HAVE_LIBINTL
#endif

#if defined(HAVE_LIBSYBTCL) || defined(HAVE_LIBTCL) || \
  defined(HAVE_LIBTCL64) || defined(HAVE_LIBTCL_DCE) || \
  defined(HAVE_LIBTCL_DCE64) || defined(HAVE_LIBTCL_R)
#define PIKE_HAVE_LIBSYBTCL
#endif

#if defined(PIKE_HAVE_LIBCOMN) && defined(PIKE_HAVE_LIBCS) \
  && defined(PIKE_HAVE_LIBCT) && defined(PIKE_HAVE_LIBINTL) \
  && defined(PIKE_HAVE_LIBSYBTCL) && defined(HAVE_CTPUBLIC_H)
#define HAVE_SYBASE
#endif

#endif /* __PIKE_SYBASE_CONFIG_H */
