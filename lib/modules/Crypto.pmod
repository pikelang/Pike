/* Crypto.pmod
 *
 */

#pike __REAL_VERSION__

static private mixed crypto_module;

mixed `[](string name)
{
    catch {
      return (crypto_module[name]
	      || ((program) ("Crypto/" + name))
	      || ((object) ("Crypto/" + name + ".pmod")));
    };
    return ([])[0];	// UNDEFINED
}

array(string) _indices() {
  array tmp = ( __FILE__ / "/");
  tmp[-1] = "Crypto";
  tmp = get_dir(tmp*"/");
  return map(glob("*.pike",tmp)+glob("*.pmod",tmp),
	     lambda(string in){ return in[..sizeof(in)-6]; }) +
    indices(crypto_module);
}

void create()
{
  catch { crypto_module=master()->resolv("_Crypto"); };
  if(!crypto_module)
    crypto_module=master()->resolv("_Lobotomized_Crypto");
}

