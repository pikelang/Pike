/* Crypto.pmod
 *
 */

//inherit _Crypto;

mixed `[](string name)
{
//  return (::`[](name) || ((program) ("Crypto/" + name)));
    catch {
      return (_Crypto[name]
	      || ((program) ("Crypto/" + name))
	      || ((object) ("Crypto/" + name + ".pmod")));
    };
    return 0;
}

