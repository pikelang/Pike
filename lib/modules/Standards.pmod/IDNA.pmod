#pike __REAL_VERSION__

//! This module implements various algorithms specified by
//! the Internationalizing Domain Names in Applications (IDNA) drafts by
//! the Internet Engineering Task Force (IETF), see
//! @url{http://www.ietf.org/internet-drafts/draft-ietf-idn-idna-14.txt@}.

//! Punycode transcoder, see
//! @url{http://www.ietf.org/internet-drafts/draft-ietf-idn-punycode-03.txt@}.
//! Punycode is used by [to_ascii] as an "ASCII Compatible Encoding" when
//! needed.
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

//! Prepare a Unicode string for ACE transcoding.  Used by [to_ascii].
//! Nameprep is a profile of Stringprep, which is described in RFC 3454.
//!
//! @param s
//!   The string to prep.
//! @param allow_unassigned
//!   Set this flag the the string to transform is a "query string",
//!   and not a "stored string".  See RFC 3454.
//!
string nameprep(string s, int allow_unassigned)
{
  // FIXME...
  return s;
}

//! The to_ascii operation takes a sequence of Unicode code points that
//! make up one label and transforms it into a sequence of code points in
//! the ASCII range (0..7F).  If to_ascci succeeds, the original sequence
//! and the resulting sequence are equivalent labels.
//!
//! @param s
//!   The sequence of Unicode code points to transform.
//! @param allow_unassigned
//!   Set this flag the the string to transform is a "query string",
//!   and not a "stored string".  See RFC 3454.
//! @param use_std3_ascii_rules
//!   Set this flag to enforce the restrictions on ASCII characters in
//!   host names imposed by STD3.
//!
string to_ascii(string s, int(0..1)|void allow_unassigned,
		int(0..1)|void use_std3_ascii_rules)
{
  int is_ascii = max(@values(s)) < 128;
  if(!is_ascii)
    s = nameprep(s, allow_unassigned);
  if(use_std3_ascii_rules) {
    if(array_sscanf(s, "%*[^\0-,./:-@[-`{-\x7f]%n")[0] != sizeof(s))
      error("Label is not valid accoring to STD3: non-LDH codepoint\n");
    if(sizeof(s) && (s[0] == '-' || s[-1] == '-'))
      error("Label is not valid accoring to STD3: leading or trailing hyphen-minus\n");
  }
  if(!is_ascii) {
    if(lower_case(s[..3]) == "xn--")
      error("Label to encode begins with ACE prefix!\n");
    s = "xn--"+Punycode->encode(s);
  }
  if(sizeof(s)<1 || sizeof(s)>63)
    error("Label is not within the 1-63 character length range\n");
  return s;
}


//! The to_unicode operation takes a sequence of Unicode code points that
//! make up one label and returns a sequence of Unicode code points.  If
//! the input sequence is a label in ACE form, then the result is an
//! equivalent internationalized label that is not in ACE form, otherwise
//! the original sequence is returned unaltered.
//!
//! @param s
//!   The sequence of Unicode code points to transform.
string to_unicode(string s)
{
  if(max(@values(s)) >= 128 &&
     catch(s = nameprep(s, 1)))
    return s;
  if(lower_case(s[..3]) != "xn--")
    return s;
  string orig_s = s;
  catch {
    s = Punycode->decode(s[4..]);
    if(lower_case(to_ascii(s, 1)) == lower_case(orig_s))
      return s;
  };
  return orig_s;
}


//! Takes a sequence of labels separated by '.' and applies
//! [to_ascii] on each.
string zone_to_ascii(string s, int(0..1)|void allow_unassigned,
		     int(0..1)|void use_std3_ascii_rules)
{
  return to_ascii((s/".")[*], allow_unassigned, use_std3_ascii_rules)*".";
}


//! Takes a sequence of labels separated by '.' and applies
//! [to_unicode] on each.
string zone_to_unicode(string s)
{
  return to_unicode((s/".")[*])*".";
}
