/*
 * $Id: _Crypto.pmod,v 1.1 2005/05/22 11:57:26 grubba Exp $
 *
 * Compat for _Crypto.
 *
 * 2005-05-22 Henrik Grubbström
 */

/* Note: We base this module on Crypto, which is the reverse of
 *       the way a true Pike 7.4 did.
 */
#pike 7.4
#if constant(Crypto)

constant cast = Crypto.cast;
constant pipe = Crypto.pipe;
constant invert = Crypto.invert;
constant arcfour = Crypto.arcfour;
constant des = Crypto.des;
constant cbc = Crypto.cbc;
constant crypto = Crypto.crypto;
constant sha = Crypto.sha;
constant md4 = Crypto.md4;
constant md5 = Crypto.md5;
constant crypt_md5 = Crypto.crypt_md5;
constant string_to_hex = Crypto.string_to_hex;
constant des_parity = Crypto.des_parity;
constant hex_to_string = Crypto.hex_to_string;
constant md2 = Crypto.md2;
constant rijndael = Crypto.rijndael;

#endif /* constant(Crypto) */