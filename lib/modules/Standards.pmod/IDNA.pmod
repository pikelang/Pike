#pike __REAL_VERSION__

//! This module implements various algorithms specified by
//! the Internet Domain Names in Applications (IDNA) drafts by
//! the Internet Engineering Task Force (IETF), see
//! @url[http://www.ietf.org/internet-drafts/draft-ietf-idn-idna-14.txt].

object Punycode = class {

    inherit String.Bootstring;

    string decode(string s) {
      array(string) x = s/"-";
      x[-1] = lower_case(x[-1]);
      return ::decode(x*"-");
    }

    static void create() { ::create(36, 1, 26, 38, 700, 72, 128, '-',
				    "abcdefghijklmnopqrstuvwxyz0123456789"); }
  }();

