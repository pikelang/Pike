/* $Id: idea_cbc.pike,v 1.5 2001/11/08 01:45:39 nilsson Exp $
 *
 */

//! IDEA CBC.
//!
//! @seealso
//!   @[cbc], @[idea]

#pike __REAL_VERSION__

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.idea);
}
