/* Crypto.pmod
 *
 */

#pike __REAL_VERSION__

private mixed crypto_module;

static mixed `[](string name)
{
    mixed err = catch {
      return (crypto_module[name]
	      || ((program) combine_path( __FILE__, "..", "Crypto" , name + ".pike"))
	      || ((object) combine_path( __FILE__, "..","Crypto", name + ".pmod")));
    };
    if( file_stat("Crypto/"+name) || file_stat("Crypto/"+name+".pmod") )
      throw(err);
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
  crypto_module = _Crypto;
#else
  crypto_module = _Lobotomized_Crypto;
#endif
}

