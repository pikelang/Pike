/* $Id: des3_cbc.pike,v 1.2 2003/11/30 17:07:05 nilsson Exp $
 *
 */

#pike __REAL_VERSION__

//! Triple-DES CBC.
//!
//! @seealso
//!   @[cbc], @[des]

#if constant(Nettle.CBC)

inherit .CBC;
void create() { ::create(.DES3()); }
string crypt_block(string data) { return crypt(data); }
int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }

#else

inherit .cbc;
void create() { ::create(.des3); }

#endif
