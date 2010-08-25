#pike __REAL_VERSION__

//! Codec for the ISO-8859-1 character encoding.

class decoder {
  constant charset = "iso88591";
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
  constant charset = "iso88591";
  protected string s = "";
  protected string|void replacement;
  protected function(string:string)|void repcb;
  protected string low_convert(string s, string|void r,
			     function(string:string)|void rc)
  {
    if (String.width(s) <= 8) return s;

    String.Buffer res = String.Buffer();
    function(string ...:void) add = res->add;

    string rr;
    int l;
    foreach(s; int i; int c) {
      if(c>255) {
	if (l < i)
	  add (s[l..i - 1]);
	l = i + 1;
	if(rc && (rr = rc(s[i..i])))
	  add(low_convert(rr,r));
	else if(r)
	  add(r);
	else
	  Locale.Charset.encode_error (s, i, charset,
				       "Character unsupported by encoding.\n");
      }
    }
    if (l < sizeof (s))
      add (s[l..]);
    return res->get();
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
    replacement = r && low_convert(r);
    repcb = rc;
  }
}
