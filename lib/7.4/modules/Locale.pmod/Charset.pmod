#pike 7.6

inherit Locale.Charset;

//! This class does not exist in the Locale module in Pike 7.6.
//! @deprecated ASCIIDec
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

//! @deprecated Encoder
constant _encoder = Encoder;
