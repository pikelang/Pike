/* Crypto.pmod
 *
 */

#pike __REAL_VERSION__

private mixed crypto_module;

static mixed `[](string name)
{
    catch {
      return (crypto_module[name]
	      || ((program) ("Crypto/" + name))
	      || ((object) ("Crypto/" + name + ".pmod")));
    };
    return UNDEFINED;
}

private array(string) crypto_indices;

static array(string) _indices() {
  if(crypto_indices) return crypto_indices + ({});
  array tmp = ( __FILE__ / "/");
  tmp[-1] = "Crypto";
  tmp = get_dir(tmp*"/");
  crypto_indices = map(glob("*.pike",tmp)+glob("*.pmod",tmp),
		       lambda(string in){ return in[..sizeof(in)-6]; }) +
    indices(crypto_module);
  return crypto_indices + ({});
}

static void create()
{
#if constant(_Crypto)
  crypto_module=master()->resolv("_Crypto");
#else
  crypto_module=master()->resolv("_Lobotomized_Crypto");
#endif
}

