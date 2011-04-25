#!/usr/local/bin/pike
/*
 *	des - fast & portable DES encryption & decryption.
 *	Copyright (C) 1992  Dana L. How
 *	Please see the file `README' for the complete copyright notice.
 *
 *	Generate tables used by desUtil.c and desCode.h.
 *
 *	Ported to Pike by Henrik Grubbström for cross-compilation use.
 *
 *	Based on
 *		desdata.c,v 1.4 1997/03/22 20:40:16 grubba
 *		desinfo.h,v 1.3 1997/04/18 20:09:24 nisse
 *		desCode.h,v 1.2 1997/03/15 04:51:31 nisse
 *
 *	$Id$
 */

/*
 *	Tables describing DES rather than just this implementation.
 *	These are used in desdata but NOT in runtime code.
 */

/* the initial permutation, E selection, and final permutation are hardwired */

/* Key Load: how to load the shift register from the user key */

constant KL = ({

	57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
	10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,

	63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
	14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4,
});

/* Key Shift: how many times to shift the key shift register */

constant KS = ({

	1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1,
});

/* Key Choose: which key bits from shift reg are used in the key schedule */

constant KC = ({

	14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
	23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,

	41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
	44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32,
});

/* S Boxes */

constant SB = ({
  ({
	14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
	 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
	 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
	15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,
  }),({
	15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
	 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
	 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,
  }),({
	10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
	 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,
  }),({
	 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
	 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,
  }),({
	 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
	 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,
  }),({
	12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
	 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
	 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,
  }),({
	 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
	 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
	 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,
  }),({
	13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
	 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
	 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
	 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
  })
});

/* Sbox Permutation */

constant SP = ({	

	16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
	 2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25,
});

/* From desCode.h */
#define ROR(d,c,o)      d = d >> c | d << o


/* list of weak and semi-weak keys

	 +0   +1   +2   +3   +4   +5   +6   +7
	0x01 0x01 0x01 0x01 0x01 0x01 0x01 0x01
	0x01 0x1f 0x01 0x1f 0x01 0x0e 0x01 0x0e
	0x01 0xe0 0x01 0xe0 0x01 0xf1 0x01 0xf1
	0x01 0xfe 0x01 0xfe 0x01 0xfe 0x01 0xfe
	0x1f 0x01 0x1f 0x01 0x0e 0x01 0x0e 0x01
	0x1f 0x1f 0x1f 0x1f 0x0e 0x0e 0x0e 0x0e
	0x1f 0xe0 0x1f 0xe0 0x0e 0xf1 0x0e 0xf1
	0x1f 0xfe 0x1f 0xfe 0x0e 0xfe 0x0e 0xfe
	0xe0 0x01 0xe0 0x01 0xf1 0x01 0xf1 0x01
	0xe0 0x1f 0xe0 0x1f 0xf1 0x0e 0xf1 0x0e
	0xe0 0xe0 0xe0 0xe0 0xf1 0xf1 0xf1 0xf1
	0xe0 0xfe 0xe0 0xfe 0xf1 0xfe 0xf1 0xfe
	0xfe 0x01 0xfe 0x01 0xfe 0x01 0xfe 0x01
	0xfe 0x1f 0xfe 0x1f 0xfe 0x0e 0xfe 0x0e
	0xfe 0xe0 0xfe 0xe0 0xfe 0xf1 0xfe 0xf1
	0xfe 0xfe 0xfe 0xfe 0xfe 0xfe 0xfe 0xfe
 */

/* key bit order in each method pair: bits 31->00 of 1st, bits 31->00 of 2nd */
/* this does not reflect the rotate of the 2nd word */

#define	S(box,bit)	(box*6+bit)
array(int) korder = ({
	S(7, 5), S(7, 4), S(7, 3), S(7, 2), S(7, 1), S(7, 0),
	S(5, 5), S(5, 4), S(5, 3), S(5, 2), S(5, 1), S(5, 0),
	S(3, 5), S(3, 4), S(3, 3), S(3, 2), S(3, 1), S(3, 0),
	S(1, 5), S(1, 4), S(1, 3), S(1, 2), S(1, 1), S(1, 0),
	S(6, 5), S(6, 4), S(6, 3), S(6, 2), S(6, 1), S(6, 0),
	S(4, 5), S(4, 4), S(4, 3), S(4, 2), S(4, 1), S(4, 0),
	S(2, 5), S(2, 4), S(2, 3), S(2, 2), S(2, 1), S(2, 0),
	S(0, 5), S(0, 4), S(0, 3), S(0, 2), S(0, 1), S(0, 0),
});

/* the order in which the algorithm accesses the s boxes */

array(int) sorder = ({
	7, 5, 3, 1, 6, 4, 2, 0,
});

#if !constant(has_suffix)
int has_suffix(string a, string b) {
  return a[sizeof(a)-sizeof(b)..]==b;
}
#endif

int main(int argc, array(string) argv)
{
	int d, i, j, k, l, m, n, s;
	array(int) b = allocate(256), ksr = allocate(56);


	if (has_suffix (argv[1], "parity.h")) {

	/*
	 * <<< make the key parity table >>>
	 */

	write(
"/* automagically produced - do not fuss with this information */\n\n");

	/* store parity information */
	for ( i = 0; i < 256; i++ ) {
		j  = i;
		j ^= j >> 4;	/* bits 3-0 have pairs */
		j ^= j << 2;	/* bits 3-2 have quads */
		j ^= j << 1;	/* bit  3 has the entire eight (no cox) */
		b[i] = 8 & ~j;	/* 0 is okay and 8 is bad parity */
	}

	/* only these characters can appear in a weak key */
	b[0x01] = 1;
	b[0x0e] = 2;
	b[0x1f] = 3;
	b[0xe0] = 4;
	b[0xf1] = 5;
	b[0xfe] = 6;

	/* print it out */
	for ( i = 0; i < 256; i++ ) {
		write(sprintf("%d,", b[i]));
		if ( (i & 31) == 31 )
			write("\n");
	}

	}
	else if (has_suffix (argv[1], "rotors.h")) {

	/*
	 * <<< make the key usage table >>>
	 */

	write("/* automagically made - do not fuss with this */\n\n");

	/* KL specifies the initial key bit positions */
	for (i = 0; i < 56; i++)
		ksr[i] = (KL[i] - 1) ^ 7;

	for (i = 0; i < 16; i++) {

		/* apply the appropriate number of left shifts */
		for (j = 0; j < KS[i]; j++) {
			m = ksr[ 0];
			n = ksr[28];
			for (k = 0; k < 27; k++)
				ksr[k     ] = ksr[k +  1],
				ksr[k + 28] = ksr[k + 29];
			ksr[27] = m;
			ksr[55] = n;
		}

		/* output the key bit numbers */
		for (j = 0; j < 48; j++) {
			m = ksr[KC[korder[j]] - 1];
			m = (m / 8) * 7 + (m % 8) - 1;
			m = 55 - m;
			write(sprintf(" %2d,", m));
			if ((j % 12) == 11)
				write("\n");
		}
		write("\n");
	}

	}
	else if (has_suffix (argv[1], "keymap.h")) {

	/*
	 * <<< make the keymap table >>>
	 */

	write("/* automagically made - do not fuss with this */\n\n");

	for ( i = 0; i <= 7 ; i++ ) {
		s = sorder[i];
		for ( d = 0; d <= 63; d++ ) {
			/* flip bits */
			k =	((d << 5) & 32) |
				((d << 3) & 16) |
				((d << 1) &  8) |
				((d >> 1) &  4) |
				((d >> 3) &  2) |
				((d >> 5) &  1) ;
			/* more bit twiddling */
			l =	((k << 0) & 32) |	/* overlap bit */
				((k << 4) & 16) |	/* overlap bit */
				((k >> 1) & 15) ;	/* unique bits */
			/* look up s box value */
			m = SB[s][l];
			/* flip bits */
			n =	((m << 3) &  8) |
				((m << 1) &  4) |
				((m >> 1) &  2) |
				((m >> 3) &  1) ;
			/* put in correct nybble */
			n <<= (s << 2);
			/* perform p permutation */
			for ( m = j = 0; j < 32; j++ )
				if ( n & (1 << (SP[j] - 1)) )
					m |= (1 << j);
			/* rotate right (alg keeps everything rotated by 1) */
			ROR(m, 1, 31) & 0xffffffff;
			/* print it out */
			write(sprintf(" 0x%08x,", m));
			if ( ( d & 3 ) == 3 )
				write("\n");
		}
		write("\n");
	}

	}
	else {
	  werror ("Don't know how to make %O\n", argv[1]);
	  return 1;
	}

	return 0;
}
