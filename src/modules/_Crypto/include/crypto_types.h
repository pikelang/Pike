/* $Id: crypto_types.h,v 1.6 2002/08/13 17:27:14 grubba Exp $
 *
 * Defines the types INT32 and INT8 */

#ifndef CRYPTO_TYPES_H_INCLUDED
#define CRYPTO_TYPES_H_INCLUDED

#ifdef PIKE
#include "global.h"
#include "pike_types.h"
#else /* !PIKE */
#define INT32 long
#define INT16 short
#define INT8 char
#endif

#endif /* CRYPTO_TYPES_H_INCLUDED */
