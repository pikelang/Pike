// Compatibility module
// $Id: __default.pmod,v 1.1 2002/09/14 00:54:17 nilsson Exp $

#pike 7.2

string dirname(string x)
{
  array(string) tmp=explode_path(x);
  return tmp[..sizeof(tmp)-2]*"/";
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);

  ret->all_constants=all_constants;
  ret->dirname=dirname;
  return ret;
}
