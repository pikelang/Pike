/* $Id: des3_cbc.pike,v 1.5 2001/11/08 01:45:38 nilsson Exp $
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
