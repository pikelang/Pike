/* Crypto.pmod
 *
 */

//inherit _Crypto;

mixed `[](string name)
{
//  return (::`[](name) || ((program) ("Crypto/" + name)));
    return (_Crypto[name]
	    || ((program) ("Crypto/" + name))
	    || ((object) ("Crypto/" + name + ".pmod")));
}

