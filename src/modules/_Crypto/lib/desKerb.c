/*
 *	des - fast & portable DES encryption & decryption.
 *	Copyright (C) 1992  Dana L. How
 *	Please see the file `README' for the complete copyright notice.
 */

#include "des.h"

#include "RCSID.h"
RCSID2(desKerb_cRcs, "$Id: desKerb.c,v 1.2 1997/03/15 04:51:44 nisse Exp $");

/* permit the default style of des functions to be changed */

DesFunc *DesCryptFuncs[2] = { DesSmallFipsDecrypt, DesSmallFipsEncrypt };

/* kerberos-compatible key schedule function */

int
des_key_sched(INT8 *k, INT32 *s)
{
	return DesMethod(s, k);
}

/* kerberos-compatible des coding function */

int
des_ecb_encrypt(INT8 *s, INT8 *d, INT32 *r, int e)
{
	(*DesCryptFuncs[e])(d, r, s);
	return 0;
}
