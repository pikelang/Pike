/* Crypto.pmod
 *
 */

static private mixed crypto_module;

mixed `[](string name)
{
//  return (::`[](name) || ((program) ("Crypto/" + name)));
    catch {
      return (crypto_module[name]
	      || ((program) ("Crypto/" + name))
	      || ((object) ("Crypto/" + name + ".pmod")));
    };
    return 0;
}

void create()
{
  catch { crypto_module=master()->resolv("_Crypto"); };
  if(!crypto_module)
    crypto_module=master()->resolv("_Lobotomized_Crypto");
}

