/*
 * $Id: aes.pike,v 1.3 2004/02/03 13:49:21 nilsson Exp $
 *
 */

// Advanced Encryption Standard (AES), previously known as
// @[Crypto.rijndael].

#pike __REAL_VERSION__

inherit Nettle.AES_State;

string name() { return "AES"; }

int query_key_length() { return 32; }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
