/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * rijndael-alg-fst.h   v2.3   April '2000
 *
 * Optimised ANSI C code
 *
 * #define INTERMEDIATE_VALUE_KAT to generate the Intermediate Value Known Answer Test.
 */

#ifndef __RIJNDAEL_ALG_FST_H
#define __RIJNDAEL_ALG_FST_H

#define MAXKC			(256/32)
#define MAXROUNDS		14

#define RIJNDAEL_BLOCK_SIZE	16

#ifndef USUAL_TYPES
#define USUAL_TYPES
typedef unsigned char	byte;
typedef unsigned char	word8;	
typedef unsigned short	word16;	
typedef unsigned int	word32;
#endif /* USUAL_TYPES */

int rijndaelKeySched(word8 k[MAXKC][4], word8 rk[MAXROUNDS+1][4][4], int ROUNDS);

int rijndaelKeyEncToDec(word8 W[MAXROUNDS+1][4][4], int ROUNDS);

int rijndaelEncrypt(word8 a[16], word8 b[16], word8 rk[MAXROUNDS+1][4][4], int ROUNDS);

#ifdef INTERMEDIATE_VALUE_KAT
int rijndaelEncryptRound(word8 a[4][4], word8 rk[MAXROUNDS+1][4][4], int ROUNDS, int rounds);
#endif /* INTERMEDIATE_VALUE_KAT */

int rijndaelDecrypt(word8 a[16], word8 b[16], word8 rk[MAXROUNDS+1][4][4], int ROUNDS);

#ifdef INTERMEDIATE_VALUE_KAT
int rijndaelDecryptRound(word8 a[4][4], word8 rk[MAXROUNDS+1][4][4], int ROUNDS, int rounds);
#endif /* INTERMEDIATE_VALUE_KAT */

#endif /* __RIJNDAEL_ALG_FST_H */
