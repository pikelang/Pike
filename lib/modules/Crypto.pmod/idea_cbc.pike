/* $Id: idea_cbc.pike,v 1.2 2003/11/30 17:07:59 nilsson Exp $
 *
 */

#pike __REAL_VERSION__

//! IDEA CBC.
//!
//! @seealso
//!   @[cbc], @[idea]

#if constant(Nettle.CBC)

inherit .CBC;
void create() { ::create(.IDEA()); }
string crypt_block(string data) { return crypt(data); }
int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }

#else

inherit .cbc;
void create() { ::create(.idea); }

#endif
