#pike __REAL_VERSION__

inherit Locale.Charset;

class ascii {
  static private string s = "";
  object(this_program) feed(string ss)
  {
    s += ss;
    return this_object();
  }
  string drain()
  {
    string ss = s;
    s = "";
    return ss;
  }
  object(this_program) clear()
  {
    s = "";
    return this_object();
  }
}

constant _encoder = Encoder;
