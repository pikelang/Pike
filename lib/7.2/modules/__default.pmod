// Compatibility module
// $Id: __default.pmod,v 1.5 2002/11/28 22:02:53 nilsson Exp $

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

object new(string|program prog, mixed ... args)
{
  if(stringp(prog))
  {
    if(program p=(program)(prog, backtrace()[-2][0]))
      return p(@args);
    else
      error("Failed to find program %s.\n", prog);
  }
  return prog(@args);
}

function(string|program, mixed ... : object) clone = new;

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);
  ret->all_constants=all_constants;
  ret->dirname=dirname;
  ret->default_yp_domain=default_yp_domain;
  ret->new=new;
  ret->clone=clone;
  return ret;
}
