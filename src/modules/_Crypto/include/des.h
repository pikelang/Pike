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
 *
 * Slightly edited by Niels Möller, 1997
 */

#ifndef DES_H_INCLUDED
#define DES_H_INCLUDED

#include "crypto_types.h"

#include "RCSID.h"
RCSID2(desCore_hRcs, "$Id$");

#define DES_KEYSIZE 8
#define DES_BLOCKSIZE 8
#define DES_EXPANDED_KEYLEN 32

typedef unsigned INT8 DesData[DES_BLOCKSIZE];
typedef unsigned INT32 DesKeys[DES_EXPANDED_KEYLEN];
typedef void DesFunc(unsigned INT8 *d, unsigned INT32 *r, unsigned INT8 *s);

extern int DesMethod(unsigned INT32 *method, unsigned INT8 *k);
extern void DesQuickInit(void);
extern void DesQuickDone(void);
extern DesFunc DesQuickCoreEncrypt;
extern DesFunc DesQuickFipsEncrypt;
extern DesFunc DesQuickCoreDecrypt;
extern DesFunc DesQuickFipsDecrypt;
extern DesFunc DesSmallCoreEncrypt;
extern DesFunc DesSmallFipsEncrypt;
extern DesFunc DesSmallCoreDecrypt;
extern DesFunc DesSmallFipsDecrypt;

extern DesFunc *DesCryptFuncs[2];
extern int des_key_sched(unsigned INT8 *k, unsigned INT32 *s);
extern int des_ecb_encrypt(unsigned INT8 *s, unsigned INT8 *d, unsigned INT32 *r, int e);

#endif /*  DES_H_INCLUDED */
