// Compatibility module
// $Id: __default.pmod,v 1.3 2002/11/28 06:05:07 nilsson Exp $

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

string default_yp_domain() {
  return Yp.default_domain();
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);
  ret->all_constants=all_constants;
  ret->dirname=dirname;
  ret->default_yp_domain=default_yp_domain;
  return ret;
}
