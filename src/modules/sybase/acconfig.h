/*
 * $Id: acconfig.h,v 1.2 2000/04/27 22:30:12 kinkie Exp $
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
