/* $Id: des3_cbc.pike,v 1.3 2000/09/26 18:59:27 hubbe Exp $
 *
 */

#pike __VERSION__

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.des3);
}
