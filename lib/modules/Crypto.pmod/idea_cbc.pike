/* $Id: idea_cbc.pike,v 1.1 2003/03/19 17:46:30 nilsson Exp $
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
