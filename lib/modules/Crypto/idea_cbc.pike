/* idea_cbc.pike
 *
 */

inherit Crypto.cbc : cbc;

void create()
{
  cbc::create(Crypto.idea);
}
