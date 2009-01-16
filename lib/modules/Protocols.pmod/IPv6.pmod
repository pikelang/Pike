// Some IPv6 utilities.

protected array(int) parse_ipv6_seq (string seq)
{
  if (has_value (seq, "-"))
    // "-" could be interpreted as a minus sign.
    return 0;

  if (seq == "")
    return ({});
  if (sscanf (seq, "%4x%{:%4x%}%*c",
	      int first, array(array(int)) rest) == 2)
    return ({first}) + (rest * ({}));
}

protected array(int) parse_ipv6_seq_with_ipv4_suffix (string seq)
// seq is assumed to contain at least one ".".
{
  array(int) segments = ({});
  array(string) split = seq / ":";

  foreach (split[..<1], string part) {
    int val;
    if (sscanf (part, "%4x%*c", val) != 1) return 0;
    segments += ({val});
  }

  array(string|int) sub_segs = split[-1] / ".";

  foreach (sub_segs; int i; string|int v) {
    if (sscanf (v, "%D%*c", v) != 1) return 0;
    sub_segs[i] = v;
  }

  switch(sizeof(sub_segs)) {
    case 4:
      if (sub_segs[0] > 0xff ||
	  sub_segs[1] > 0xff ||
	  sub_segs[2] > 0xff ||
	  sub_segs[3] > 0xff) return 0;
      segments += ({ sub_segs[0]*256+sub_segs[1],
		     sub_segs[2]*256+sub_segs[3] });
      break;

    case 3:
      // From inet(3): "When a three part address is specified, the
      // last part is interpreted as a 16-bit quantity and placed in
      // the rightmost two bytes of the network address. This makes
      // the three part address format convenient for specifying Class
      // B network addresses as 128.net.host."
      if (sub_segs[0] > 0xff ||
	  sub_segs[1] > 0xff ||
	  sub_segs[2] > 0xffff) return 0;
      segments += ({ sub_segs[0]*256+sub_segs[1],
		     sub_segs[2] });
      break;

    case 2:
      // From inet(3): "When a two part address is supplied, the last
      // part is interpreted as a 24-bit quantity and placed in the
      // rightmost three bytes of the network address. This makes the
      // two part address format convenient for specifying Class A
      // network addresses as net.host."
      if (sub_segs[0] > 0xff ||
	  sub_segs[1] > 0xffffff) return 0;
      segments += ({ sub_segs[0]*256 + (sub_segs[1] >> 16),
		     sub_segs[1]&0xffff });
      break;

    default:
      return 0;
  }

  return segments;
}

array(int(0..65535)) parse_addr (string addr)
//! Parses an IPv6 address on the formatted hexadecimal
//! @expr{"x:x:x:x:x:x:x:x"@} form, or any of the shorthand varieties
//! (see RFC 2373, section 2.2).
//!
//! The address is returned as an 8-element array where each element
//! is the value of the corresponding field. Zero is returned if @[addr]
//! is incorrectly formatted.
//!
//! @seealso
//! @[format_addr]
{
  sscanf (addr, "%s::%s", string part1, string part2);
  array(int) sections;

  if (!part1) {
    sections = has_value (addr, ".") ?
      parse_ipv6_seq_with_ipv4_suffix (addr) : parse_ipv6_seq (addr);
    if (!sections) return 0;
  }

  else {
    sections = parse_ipv6_seq (part1);
    if (!sections) return 0;
    array(int) tail = has_value (part2, ".") ?
      parse_ipv6_seq_with_ipv4_suffix (part2) : parse_ipv6_seq (part2);
    if (!tail) return 0;
    int pad = 8 - sizeof(sections) - sizeof(tail);
    if (pad >= 0)
      sections += allocate (pad) + tail;
    else
      sections += tail;
  }

  return sizeof (sections) == 8 && sections;
}

string format_addr (array(int(0..65535)) bin_addr)
//! Formats an IPv6 address to the colon-separated hexadecimal form as
//! defined in RFC 2373, section 2.2. @[bin_addr] must be an 8-element
//! array containing the 16-bit fields.
//!
//! The returned address is on a canonical shortest form as follows:
//! The longest sequence of zeroes is shortened using @tt{"::"@}. If
//! there are several of equal length then the leftmost is shortened.
//! All hexadecimal letters are lower-case. There are no superfluous
//! leading zeroes in the fields.
//!
//! @seealso
//! @[parse_addr]
{
  string wide_addr = (string) bin_addr;

  // Optimize some common cases.
  if (wide_addr == "\0\0\0\0\0\0\0\0") return "::"; // ANY
  if (wide_addr == "\0\0\0\0\0\0\0\1") return "::1"; // loopback

  // Compress the longest sequence of zeros and format back to a
  // string with colon separated hex numbers.
  int maxlen, maxpos = -1;
  array(array(string)) split = array_sscanf (wide_addr, "%{%[\0]%[^\0]%}")[0];
  foreach (split; int pos; [string zeroes, string others]) {
    if (sizeof (zeroes) > maxlen) {
      maxlen = sizeof (zeroes);
      maxpos = pos;
    }
    split[pos][0] = "0:" * sizeof (zeroes);
    split[pos][1] = sprintf ("%{%x:%}", (array(int)) others);
  }

  switch (maxpos) {
    case -1:
      break;

    case 0:
      // Note: "::" special case above guarantees there's some later
      // field that isn't "".
      split[0][0] = "::";
      break;

    default:
      // Replace longest entry with a single colon since it gets
      // padded with the trailing one of the previous entry. We know
      // there's at least one earlier field that isn't "".
      split[maxpos][0] = ":";

      if (maxpos == sizeof (split) - 1 && split[maxpos][1] == "")
	// Only case when we've got no later field that isn't "" is
	// when maxpos is the last entry and the following non-zero
	// field is empty. Don't shave off a trailing ":" then.
	return (split * ({})) * "";
      break;
  }

  return ((split * ({})) * "")[..<1]; // Get rid of the trailing ":".
}

string normalize_addr_basic (string addr)
//! Normalizes a formatted IPv6 address to a string with eight
//! hexadecimal numbers separated by @tt{":"@}. @[addr] is given on
//! the same form, or any of the shorthand varieties as specified in
//! RFC 2373, section 2.2.
//!
//! All hexadecimal letters in the returned address are lower-case,
//! and there are no superfluous leading zeroes in the fields.
//!
//! Zero is returned if @[addr] is incorrectly formatted.
//!
//! @seealso
//! @[normalize_addr_short]
{
  // Shortcuts.
  if (addr == "::") return "0:0:0:0:0:0:0:0"; // ANY
  if (addr == "::1") return "0:0:0:0:0:0:0:1"; // loopback

  array(int(0..65535)) bin_addr = parse_addr (addr);
  if (!bin_addr) return 0;

  return sprintf ("%{%x:%}", bin_addr)[..<1];
}

string normalize_addr_short (string addr)
//! Normalizes a formatted IPv6 address to a canonical shortest form.
//! @[addr] is parsed according to the hexadecimal
//! @expr{"x:x:x:x:x:x:x:x"@} syntax or any of its shorthand varieties
//! (see RFC 2373, section 2.2).
//!
//! The returned address is normalized as follows: The longest
//! sequence of zeroes is shortened using @tt{"::"@}. If there are
//! several of equal length then the leftmost is shortened. All
//! hexadecimal letters are lower-case. There are no superfluous
//! leading zeroes in the fields.
//!
//! Zero is returned if @[addr] is incorrectly formatted.
//!
//! @seealso
//! @[normalize_addr_basic]
{
  if (addr == "::" || addr == "::1") return addr; // ANY and loopback shortcuts.

  array(int(0..65535)) bin_addr = parse_addr (addr);
  if (!bin_addr) return 0;

  return format_addr (bin_addr);
}
