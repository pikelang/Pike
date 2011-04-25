/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * config file for Pike DVB module
 *
 * Honza Petrous, hop@unibase.cz
 */

#ifndef DVB_CONFIG_H
#define DVB_CONFIG_H

@TOP@
@BOTTOM@

/* end of automatic section */

#undef HAVE_DVB

#ifdef HAVE_LINUX_DVB_VERSION_H
  #ifdef HAVE_LINUX_DVB_SEC_H
   #define HAVE_DVB	20
  #endif
#else
  #ifdef HAVE_OST_FRONTEND_H
    #define HAVE_DVB	9
  #endif
#endif

#endif /* DVB_CONFIG_H */
