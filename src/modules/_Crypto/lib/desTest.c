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
 *	Exercise the DES routines and collect performance statistics.
 */

#ifndef	lint
static char desTest_cRcs[] = "$Id$";
#endif

#include	"des.h"
#include <stdio.h>

/* define now(w) to be the elapsed time in hundredths of a second */

#ifndef __NT__
#include	<sys/time.h>
#include	<sys/resource.h>
extern int getrusage();
static struct rusage usage;
#define	now(w)	(						\
		(void)getrusage(RUSAGE_SELF, &usage),		\
		usage.ru_utime.tv_sec  * 100 +			\
		usage.ru_utime.tv_usec / 10000			\
	)
#else
#include       <windows.h>
#define now(w) 0
#endif

#define byte unsigned char

/* test data
 * the tests (key0-3, text0-3) are cribbed from code which is (c) 1988 MIT
 */

byte keyt[8] =		{0x5d, 0x85, 0x91, 0x73, 0xcb, 0x49, 0xdf, 0x2f};
byte key0[8] =		{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x80};
byte key1[8] =		{0x80, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
byte key2[8] =		{0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f};
byte key3[8] =		{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
byte textt[8] =		{0x67, 0x1f, 0xc8, 0x93, 0x46, 0x5e, 0xab, 0x1e};
byte text0[8] =		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte text1[8] =		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40};
byte text2[8] =		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte text3[8] =		{'N',  'o',  'w',  ' ',  'i',  's',  ' ',  't' };

/* work areas */

DesKeys keys;
byte cipher[8], output[8];

/* noisy interfaces to the routines under test */

static void
method(key)
byte *key;
{
	int j;

	(void)printf("\nkey:\t");
	for ( j = 0; j < 8; j++ )
		(void)printf("%02X ", key[j]);
	if ( des_key_sched(key, keys) )
		(void)printf("W");
	(void)printf("\t");
}

static void
encode(src, dst)
byte *src, *dst;
{
	int j;

	(void)printf("clear:\t");
	for (j = 0; j < 8; j++)
		(void)printf("%02X ", src[j]);

	(void)des_ecb_encrypt(src, dst, keys, 1);

	(void)printf("\tcipher:\t");
	for (j = 0; j < 8; j++)
		(void)printf("%02X ", dst[j]);
	(void)printf("\n");
}

static void
decode(src, dst, check)
byte *src, *dst, *check;
{
	int j;

	(void)printf("cipher:\t");
	for (j = 0; j < 8; j++)
		(void)printf("%02X ", src[j]);

	(void)des_ecb_encrypt(src, dst, keys, 0);

	(void)printf("\tclear:\t");
	for (j = 0; j < 8; j++)
		(void)printf("%02X ", dst[j]);

        if(!memcmp(dst,check,8))
           printf("Ok\n");
        else
           printf("FAIL\n");
}

/* run the tests */

int
main()
{
	int j, m, e, n;
	void (*f)();
	static char * expect[] = {
		"57 99 F7 2A D2 3F AE 4C", "9C C6 2D F4 3B 6E ED 74",
		"90 E6 96 A2 AD 56 50 0D", "A3 80 E0 2A 6B E5 46 96",
		"43 5C FF C5 68 B3 70 1D", "25 DD AC 3E 96 17 64 67",
		"80 B5 07 E1 E6 A7 47 3D", "3F A4 0E 8A 98 4D 48 15",
	};
	static void (*funcs[])() = {
		DesQuickCoreEncrypt, DesQuickFipsEncrypt,
		DesSmallCoreEncrypt, DesSmallFipsEncrypt,
		DesQuickCoreDecrypt, DesQuickFipsDecrypt,
		DesSmallCoreDecrypt, DesSmallFipsDecrypt };
	static char * names[] = {
		"QuickCore", "QuickFips",
		"SmallCore", "SmallFips" };

	n = 0;
	DesQuickInit();

	/* do timing info first */

	f = (void (*)())DesMethod;
	j = 10000;
	m = now(0);
	do
		(*f)(keys, keyt);
	while ( --j );
	m = now(1) - m;

	do {
		    DesCryptFuncs[0] = funcs[n+4];
		f = DesCryptFuncs[1] = funcs[n  ];
		j = 100000;
		e = now(0);
		do
			(*f)(cipher, keys, textt);
		while ( --j );
		e = now(1) - e;

		(void)printf(	"%s:  setkey,%5duS;  encode,%3d.%1duS.\n",
				names[n], m, e/10, e%10);

		/* now check functionality */

		method(key0);
		(void)printf("cipher?\t%s\n", expect[(n % 2) + 0]);
		encode(text0, cipher);
		decode(cipher, output, text0);

		method(key1);
		(void)printf("cipher?\t%s\n", expect[(n % 2) + 2]);
		encode(text1, cipher);
		decode(cipher, output, text1);

		method(key2);
		(void)printf("cipher?\t%s\n", expect[(n % 2) + 4]);
		encode(text2, cipher);
		decode(cipher, output, text2);

		method(key3);
		(void)printf("cipher?\t%s\n", expect[(n % 2) + 6]);
		encode(text3, cipher);
		decode(cipher, output, text3);

		(void)printf("%c", "\n\f\n\0"[n]);

	} while ( ++n < 4 );

	DesQuickDone();
	fflush(stdout);
	return 0;
}
