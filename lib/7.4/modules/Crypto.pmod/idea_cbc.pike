/* $Id: idea_cbc.pike,v 1.1 2004/02/07 04:14:05 nilsson Exp $
 *
 */

#pike __REAL_VERSION__

//! IDEA CBC.
//!
//! @seealso
//!   @[cbc], @[idea]

inherit Nettle.CBC;
void create() { ::create(Crypto.IDEA()); }
string crypt_block(string data) { return crypt(data); }
int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }
