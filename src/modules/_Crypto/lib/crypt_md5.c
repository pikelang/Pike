/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 *  crypt-md5.c :  Implementation of the MD5 password hash function
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * Adapted to Pike by Andreas Lange
 *
 */

#include <string.h>
#include "crypto_types.h"
#include "md5.h"


static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void to64(char *s, unsigned long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v & 0x3f];
		v >>= 6;
	}
}


char *crypt_md5(const char *pw, const char *salt)
{
	static char *magic = "$1$"; /*
				     * This string is magic for
				     * this algorithm.  Having
				     * it this way, we can get
				     * better later on
				     */
	static char passwd[120], *p;
	static const char *sp, *ep;
	unsigned char final[MD5_DIGESTSIZE];
	int sl, pl, i;
	struct md5_ctx ctx, ctx1;
	unsigned long l;

	/* Refine the Salt first */
	sp = salt;

	/* If it starts with the magic string, then skip that */
	if (!strncmp(sp, magic, strlen(magic)))
		sp += strlen(magic);

	/* It stops at the first '$', max 8 chars */
	for (ep = sp; *ep && *ep != '$' && ep < (sp + 8); ep++)
		continue;

	/* get the length of the true salt */
	sl = ep - sp;

	md5_init(&ctx);

	/* The password first, since that is what is most unknown */
	md5_update(&ctx, (unsigned INT8 *) pw, strlen(pw));

	/* Then our magic string */
	md5_update(&ctx, (unsigned INT8 *) magic, strlen(magic));

	/* Then the raw salt */
	md5_update(&ctx, (unsigned INT8 *) sp, sl);

	/* Then just as many characters of the MD5(pw,salt,pw) */
	md5_init(&ctx1);
	md5_update(&ctx1, (unsigned INT8 *) pw, strlen(pw));
	md5_update(&ctx1, (unsigned INT8 *) sp, sl);
	md5_update(&ctx1, (unsigned INT8 *) pw, strlen(pw));
	md5_final(&ctx1);
	md5_digest(&ctx1, final);
	for(pl = strlen(pw); pl > 0; pl -= MD5_DIGESTSIZE)
	  md5_update(&ctx, (unsigned INT8 *) final, 
		     pl>MD5_DIGESTSIZE ? MD5_DIGESTSIZE : pl);

	/* Don't leave anything around in vm they could use. */
	memset(final, 0, sizeof(final) );

	/* Then something really weird... */
	for (i = strlen(pw); i ; i >>= 1)
		if(i&1)
		    md5_update(&ctx, (unsigned INT8 *) final, 1);
		else
		    md5_update(&ctx, (unsigned INT8 *) pw, 1);

	/* Now make the output string */
	strcpy(passwd, magic);
	strncat(passwd, sp, sl);
	strcat(passwd, "$");

	md5_final(&ctx);
	md5_digest(&ctx, final);

	for(i=0;i<1000;i++) {
		md5_init(&ctx1);
		if(i & 1)
		  md5_update(&ctx1, (unsigned INT8 *) pw, strlen(pw));
		else
		  md5_update(&ctx1, (unsigned INT8 *) final, MD5_DIGESTSIZE);

		if(i % 3)
		  md5_update(&ctx1, (unsigned INT8 *) sp, sl);

		if(i % 7)
		  md5_update(&ctx1, (unsigned INT8 *) pw, strlen(pw));

		if(i & 1)
		  md5_update(&ctx1, (unsigned INT8 *) final, MD5_DIGESTSIZE);
		else
		  md5_update(&ctx1, (unsigned INT8 *) pw, strlen(pw));

		md5_final(&ctx1);
		md5_digest(&ctx1, final);
	}

	p = passwd + strlen(passwd);

	l = (final[0] << 16) | (final[6] << 8) | final[12];
	to64(p, l, 4);
	p += 4;
	l = (final[1] << 16) | (final[7] << 8) | final[13];
	to64(p, l, 4);
	p += 4;
	l = (final[2] << 16) | (final[8] << 8) | final[14];
	to64(p, l, 4);
	p += 4;
	l = (final[3] << 16) | (final[9] << 8) | final[15];
	to64(p, l, 4);
	p += 4;
	l = (final[4] << 16) | (final[10] << 8) | final[5];
	to64(p, l, 4);
	p += 4;
	l = final[11];
	to64(p, l, 2);
	p += 2;
	*p = '\0';

	/* Don't leave anything around in vm they could use. */
	memset(final, 0, sizeof(final) );

	return passwd;
}
