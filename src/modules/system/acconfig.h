/*
 * $Id: acconfig.h,v 1.3 1998/05/31 14:07:51 grubba Exp $
 *
 * System dependant definitions for the system module for Pike
 *
 * Henrik Grubbström 1997-01-20
 */
 
#ifndef SYSTEM_MACHINE_H
#define SYSTEM_MACHINE_H
 
@TOP@
@BOTTOM@ 

/* Define if you have solaris stype gethostbyname_r.  */
#undef HAVE_SOLARIS_GETHOSTBYNAME_R
 
/* Define if you have OSF1 stype gethostbyname_r.  */
#undef HAVE_OSF1_GETHOSTBYNAME_R
 
/* Define if you have h_addr_list in the hostent struct */
#undef HAVE_H_ADDR_LIST

/* Define if you have pw_gecos in the passwd struct */
#undef HAVE_PW_GECOS

/* Define if <netinet/in.h> defines the in_addr_t type. */
#undef HAVE_IN_ADDR_T

#endif /* SYSTEM_MACHINE_H */
