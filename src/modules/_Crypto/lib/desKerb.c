/*
 *	des - fast & portable DES encryption & decryption.
 *	Copyright (C) 1992  Dana L. How
 *	Please see the file `README' for the complete copyright notice.
 */

#include "des.h"

#include "RCSID.h"
RCSID2(desKerb_cRcs, "$Id: desKerb.c,v 1.3 1997/05/30 02:40:17 grubba Exp $");

/* permit the default style of des functions to be changed */

DesFunc *DesCryptFuncs[2] = { DesSmallFipsDecrypt, DesSmallFipsEncrypt };

/* kerberos-compatible key schedule function */

int
des_key_sched(unsigned INT8 *k, unsigned INT32 *s)
{
	return DesMethod(s, k);
}

/* kerberos-compatible des coding function */

int
des_ecb_encrypt(unsigned INT8 *s, unsigned INT8 *d, unsigned INT32 *r, int e)
{
	(*DesCryptFuncs[e])(d, r, s);
	return 0;
}
