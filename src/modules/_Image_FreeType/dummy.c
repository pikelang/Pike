/*
 * $Id: dummy.c,v 1.1 2002/10/29 10:08:07 grubba Exp $
 *
 * Strap needed libc symbols into the main pike binary.
 *
 * 2002-10-29 Henrik Grubbström
 */

#include "config.h"

#ifdef DYNAMIC_MODULE
void *ft_dummy_strap(void)
{
#ifdef HAVE__ALLDIV
  /* NT needs _alldiv. */
  extern void *_alldiv[];

  return _alldiv[0];
#else /* !HAVE__ALLDIV */
  return (void *)0;
#endif /* HAVE__ALLDIV */
}
#else /* !DYNAMIC_MODULE */

static int placeholder;		/* Keep the compiler happy */

#endif /* DYNAMIC_MODULE */
