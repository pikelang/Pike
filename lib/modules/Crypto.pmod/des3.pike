/* $Id: des3.pike,v 1.2 2004/02/03 13:49:35 nilsson Exp $
 *
 */

#pike __REAL_VERSION__

inherit Nettle.DES3_State;

string name() { return "DES"; } // Yep, it doesn't say DES3

array(int) query_key_length() { return ({ 8, 8, 8 }); }
int query_block_size() { return 8; }
int query_key_size() { return 16; }
string crypt_block(string p) { return crypt(p); }
