/* $Id$
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
