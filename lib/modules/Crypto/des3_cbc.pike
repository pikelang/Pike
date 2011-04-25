/* $Id$
 *
 */

#pike __REAL_VERSION__

//! Triple-DES CBC.
//!
//! @seealso
//!   @[cbc], @[des]

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.des3);
}
