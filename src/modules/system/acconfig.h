/*
 * $Id: acconfig.h,v 1.1 1997/05/18 21:59:42 grubba Exp $
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

#endif /* SYSTEM_MACHINE_H */
