/*
 * $Id: acconfig.h,v 1.1 2002/09/09 20:10:15 hop Exp $
 *
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
  #define HAVE_DVB	20
#else
  #ifdef HAVE_OST_FRONTEND_H
    #define HAVE_DVB	9
  #endif
#endif

#endif /* DVB_CONFIG_H */
