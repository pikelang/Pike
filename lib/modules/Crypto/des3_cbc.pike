/* $Id: des3_cbc.pike,v 1.4 2000/09/28 03:38:38 hubbe Exp $
 *
 */

#pike __REAL_VERSION__

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.des3);
}
