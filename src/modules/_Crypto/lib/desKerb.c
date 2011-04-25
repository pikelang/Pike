/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 *	des - fast & portable DES encryption & decryption.
 *	Copyright (C) 1992  Dana L. How
 *	Please see the file `README' for the complete copyright notice.
 */

#include "des.h"

#include "RCSID.h"
RCSID2(desKerb_cRcs, "$Id$");

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
