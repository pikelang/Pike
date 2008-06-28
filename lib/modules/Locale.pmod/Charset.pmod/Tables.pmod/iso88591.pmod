#pike __REAL_VERSION__

//! Codec for the ISO-8859-1 character encoding.

class decoder {
  protected private string s = "";
  this_program feed(string ss)
  {
    s += ss;
    return this;
  }
  string drain()
  {
    string ss = s;
    s = "";
    return ss;
  }
  this_program clear()
  {
    s = "";
    return this;
  }
}

class encoder
{
  protected string s = "";
  protected string|void replacement;
  protected function(string:string)|void repcb;
  protected string low_convert(string s, string|void r,
			     function(string:string)|void rc)
  {
    int i = sizeof(s);
    string rr;
    while(--i>=0)
      if(s[i]>255)
	if(rc && (rr = rc(s[i..i])))
	  s=s[..i-1]+low_convert(rr,r)+s[i+1..];
	else if(r)
	  s=s[..i-1]+low_convert(r)+s[i+1..];
	else
	  error("Character unsupported by encoding.\n");
    return s;
  }
  this_program feed(string ss)
  {
    s += low_convert(ss, replacement, repcb);
    return this;
  }
  string drain()
  {
    string ss = s;
    s = "";
    return ss;
  }
  this_program clear()
  {
    s = "";
    return this;
  }
  void set_replacement_callback(function(string:string) rc)
  {
    repcb = rc;
  }
  protected void create(string|void r, string|void rc)
  {
    replacement = r;
    repcb = rc;
  }
}


