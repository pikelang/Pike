// Compatibility module
// $Id: __default.pmod,v 1.2 2002/10/27 13:52:54 nilsson Exp $

#pike 7.2

string dirname(string x)
{
  array(string) tmp=explode_path(x);
  return tmp[..sizeof(tmp)-2]*"/";
}

void sleep(float|int t, void|int abort)
{
  delay(t, abort);
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);

  ret->all_constants=all_constants;
  ret->dirname=dirname;
  return ret;
}
