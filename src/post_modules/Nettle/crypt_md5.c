/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: crypt_md5.c,v 1.8 2005/12/12 20:52:28 nilsson Exp $
*/

/*
 *  crypt_md5.c :  Implementation of the MD5 password hash function
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * Adapted to Pike by Andreas Lange and Martin Nilsson
 *
 */

#include "nettle_config.h"

#ifdef HAVE_LIBNETTLE

#include <string.h>
#include <nettle/md5.h>

#include "nettle.h"

static const unsigned char itoa64[] =	/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

#define TO64(X,Y,Z) l = (final[X]<<16)|(final[Y]<<8)|final[Z]; \
 *p++ = itoa64[ l & 0x3f ]; \
 *p++ = itoa64[ (l>>=6) & 0x3f ]; \
 *p++ = itoa64[ (l>>=6) & 0x3f ]; \
 *p++ = itoa64[ (l>>=6) & 0x3f ];


char *pike_crypt_md5(int pl, const char *const pw,
		     int sl, const char *const salt)
{
  static const char *const magic = "$1$"; /*
			       * This string is magic for
			       * this algorithm.  Having
			       * it this way, we can get
			       * better later on
			       */
  static char passwd[22], *p;
  unsigned char final[MD5_DIGEST_SIZE];
  int i;
  struct md5_ctx ctx;
  unsigned long l;

  /* Max 8 characters of salt */
  if(sl>8) sl=8;

  /* Generate first hash */
  md5_init(&ctx);
  md5_update(&ctx, pl, (uint8_t *) pw);
  md5_update(&ctx, sl, (uint8_t *) salt);
  md5_update(&ctx, pl, (uint8_t *) pw);
  md5_digest(&ctx, MD5_DIGEST_SIZE, final);

  /* Generate second hash */
  /* ctx is now reset by md5_digest */
  md5_update(&ctx, pl, (uint8_t *) pw);
  md5_update(&ctx, strlen(magic), (uint8_t *) magic);
  md5_update(&ctx, sl, (uint8_t *) salt);

  /* Take as many charaters from first hash as there are
     in the password. */
  for(i = pl; i > 0; i -= MD5_DIGEST_SIZE)
    md5_update(&ctx, (i>MD5_DIGEST_SIZE ? MD5_DIGEST_SIZE : i),
	       (uint8_t *) final);

  /* Then something really weird... */
  for (i = pl; i ; i >>= 1)
    if(i&1)
      md5_update(&ctx, 1, (const uint8_t *)"\0");
    else
      md5_update(&ctx, 1, (const uint8_t *) pw);

  md5_digest(&ctx, MD5_DIGEST_SIZE, final);

  /* Obfuscate the MD5 hash further by doing 1000 silly passes
     of MD5 over and over. */
  /* ctx is now reset by md5_digest above and will be reset by
     md5_digest at the bottom of the loop. */
  for(i=0; i<1000; i++) {
    if(i & 1)
      md5_update(&ctx, pl, (const uint8_t *) pw);
    else
      md5_update(&ctx, MD5_DIGEST_SIZE, (const uint8_t *) final);

    if(i % 3) md5_update(&ctx, sl, (uint8_t *) salt);
    if(i % 7) md5_update(&ctx, pl, (uint8_t *) pw);

    if(i & 1)
      md5_update(&ctx, MD5_DIGEST_SIZE, (const uint8_t *) final);
    else
      md5_update(&ctx, pl, (const uint8_t *) pw);

    md5_digest(&ctx, MD5_DIGEST_SIZE, final);
  }

  p = passwd;

  TO64(0, 6, 12);
  TO64(1, 7, 13);
  TO64(2, 8, 14);
  TO64(3, 9, 15);
  TO64(4, 10, 5);
  l = final[11];
  *p++ = itoa64[ l & 0x3f ];
  *p++ = itoa64[ (l>>=6) & 0x3f ];
  *p = '\0';

  /* Clear some memory */
  memset(final, 0, sizeof(final) );

  return passwd;
}

#endif /* HAVE_LIBNETTLE */
