/* $Id: idea_cbc.pike,v 1.2 1997/05/31 22:03:48 grubba Exp $
 *
 */

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.idea);
}
