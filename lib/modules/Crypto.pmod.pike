/* Crypto.pmod
 *
 */

inherit _Crypto;

mixed `[](string name)
{
  return (::`[](name) || ((program) "Crypto/" + name));
}

