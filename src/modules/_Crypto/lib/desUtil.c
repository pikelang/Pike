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

#include	"desCode.h"

#include "RCSID.h"
RCSID2(desUtil_cRcs, "$Id$");

/* various tables */

unsigned INT32 des_keymap[] = {
#include	"keymap.h"
};

static unsigned INT8 rotors[] = {
#include	"rotors.h"
};
static char parity[] = {
#include	"parity.h"
};

RCSID2(ego, "\n\nFast DES Library Copyright (c) 1991 Dana L. How\n\n");


/* set up the method list from the key */

int
DesMethod(unsigned INT32 *method, unsigned INT8 *k)
{
	register unsigned INT32 n, w;
	register char * b0, * b1;
	char bits0[56], bits1[56];

	/* check for bad parity and weak keys */
	b0 = parity;
	n  = b0[k[0]]; n <<= 4;
	n |= b0[k[1]]; n <<= 4;
	n |= b0[k[2]]; n <<= 4;
	n |= b0[k[3]]; n <<= 4;
	n |= b0[k[4]]; n <<= 4;
	n |= b0[k[5]]; n <<= 4;
	n |= b0[k[6]]; n <<= 4;
	n |= b0[k[7]];
	w  = 0X88888888L;
	/* report bad parity in key */
	if ( n & w )
		return -1;
	/* report a weak or semi-weak key */
	if ( !((n - (w >> 3)) & w) ) {	/* 1 in 10^10 keys passes this test */
		if ( n < 0X41415151 ) {
			if ( n < 0X31312121 ) {
				if ( n < 0X14141515 ) {
					/* 01 01 01 01 01 01 01 01 */
					if ( n == 0X11111111 ) return -2;
					/* 01 1F 01 1F 01 0E 01 0E */
					if ( n == 0X13131212 ) return -2;
				} else {
					/* 01 E0 01 E0 01 F1 01 F1 */
					if ( n == 0X14141515 ) return -2;
					/* 01 FE 01 FE 01 FE 01 FE */
					if ( n == 0X16161616 ) return -2;
				}
			} else {
				if ( n < 0X34342525 ) {
					/* 1F 01 1F 01 0E 01 0E 01 */
					if ( n == 0X31312121 ) return -2;
					/* 1F 1F 1F 1F 0E 0E 0E 0E */	/* ? */
					if ( n == 0X33332222 ) return -2;
				} else {
					/* 1F E0 1F E0 0E F1 0E F1 */
					if ( n == 0X34342525 ) return -2;
					/* 1F FE 1F FE 0E FE 0E FE */
					if ( n == 0X36362626 ) return -2;
				}
			}
		} else {
			if ( n < 0X61616161 ) {
				if ( n < 0X44445555 ) {
					/* E0 01 E0 01 F1 01 F1 01 */
					if ( n == 0X41415151 ) return -2;
					/* E0 1F E0 1F F1 0E F1 0E */
					if ( n == 0X43435252 ) return -2;
				} else {
					/* E0 E0 E0 E0 F1 F1 F1 F1 */	/* ? */
					if ( n == 0X44445555 ) return -2;
					/* E0 FE E0 FE F1 FE F1 FE */
					if ( n == 0X46465656 ) return -2;
				}
			} else {
				if ( n < 0X64646565 ) {
					/* FE 01 FE 01 FE 01 FE 01 */
					if ( n == 0X61616161 ) return -2;
					/* FE 1F FE 1F FE 0E FE 0E */
					if ( n == 0X63636262 ) return -2;
				} else {
					/* FE E0 FE E0 FE F1 FE F1 */
					if ( n == 0X64646565 ) return -2;
					/* FE FE FE FE FE FE FE FE */
					if ( n == 0X66666666 ) return -2;
				}
			}
		}
	}

	/* explode the bits */
	n = 56;
	b0 = bits0;
	b1 = bits1;
	do {
		w = (256 | *k++) << 2;
		do {
			--n;
			b1[n] = 8 & w;
			w >>= 1;
			b0[n] = 4 & w;
		} while ( w >= 16 );
	} while ( n );

	/* put the bits in the correct places */
	n = 16;
	k = rotors;
	do {
		w   = (b1[k[ 0   ]] | b0[k[ 1   ]]) << 4;
		w  |= (b1[k[ 2   ]] | b0[k[ 3   ]]) << 2;
		w  |=  b1[k[ 4   ]] | b0[k[ 5   ]];
		w <<= 8;
		w  |= (b1[k[ 6   ]] | b0[k[ 7   ]]) << 4;
		w  |= (b1[k[ 8   ]] | b0[k[ 9   ]]) << 2;
		w  |=  b1[k[10   ]] | b0[k[11   ]];
		w <<= 8;
		w  |= (b1[k[12   ]] | b0[k[13   ]]) << 4;
		w  |= (b1[k[14   ]] | b0[k[15   ]]) << 2;
		w  |=  b1[k[16   ]] | b0[k[17   ]];
		w <<= 8;
		w  |= (b1[k[18   ]] | b0[k[19   ]]) << 4;
		w  |= (b1[k[20   ]] | b0[k[21   ]]) << 2;
		w  |=  b1[k[22   ]] | b0[k[23   ]];

		method[0] = w;

		w   = (b1[k[ 0+24]] | b0[k[ 1+24]]) << 4;
		w  |= (b1[k[ 2+24]] | b0[k[ 3+24]]) << 2;
		w  |=  b1[k[ 4+24]] | b0[k[ 5+24]];
		w <<= 8;
		w  |= (b1[k[ 6+24]] | b0[k[ 7+24]]) << 4;
		w  |= (b1[k[ 8+24]] | b0[k[ 9+24]]) << 2;
		w  |=  b1[k[10+24]] | b0[k[11+24]];
		w <<= 8;
		w  |= (b1[k[12+24]] | b0[k[13+24]]) << 4;
		w  |= (b1[k[14+24]] | b0[k[15+24]]) << 2;
		w  |=  b1[k[16+24]] | b0[k[17+24]];
		w <<= 8;
		w  |= (b1[k[18+24]] | b0[k[19+24]]) << 4;
		w  |= (b1[k[20+24]] | b0[k[21+24]]) << 2;
		w  |=  b1[k[22+24]] | b0[k[23+24]];

		ROR(w, 4, 28);		/* could be eliminated */
		method[1] = w;

		k	+= 48;
		method	+= 2;
	} while ( --n );

	return 0;
}
