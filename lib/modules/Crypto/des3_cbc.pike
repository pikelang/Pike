/* $Id: des3_cbc.pike,v 1.2 1997/05/31 22:03:46 grubba Exp $
 *
 */

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.des3);
}
