/* $Id: des3_cbc.pike,v 1.1 2003/03/19 17:46:30 nilsson Exp $
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
