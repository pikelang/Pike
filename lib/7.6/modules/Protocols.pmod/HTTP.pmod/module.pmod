#pike 7.7

// RFC 1738, 2.2. URL Character Encoding Issues
protected constant url_non_corresponding = enumerate(0x21) +
  enumerate(0x81,1,0x7f);
protected constant url_unsafe = ({ '<', '>', '"', '#', '%', '{', '}',
				'|', '\\', '^', '~', '[', ']', '`' });
protected constant url_reserved = ({ ';', '/', '?', ':', '@', '=', '&' });

// Encode these chars
protected constant url_chars = url_non_corresponding + url_unsafe +
  url_reserved + ({ '+', '\'' });
protected constant url_from = sprintf("%c", url_chars[*]);
protected constant url_to   = sprintf("%%%02x", url_chars[*]);

string http_encode_string(string in)
{
  string res = replace(in, url_from, url_to);
  if (String.width(res) == 8) return res;

  // Wide string handling.
  // FIXME: Could be more efficient...
  mapping(string:string) wide = ([]);
  foreach(res/"", string char) {
    if (char[0] & ~255) {
      // Wide character.
      if (!wide[char] && !(char[0] & ~65535)) {
	// UCS-2 character.
	// This encoding is used by eg Safari.
	wide[char] = sprintf("%%u%04x", char[0]);
      } else {
	// FIXME: UCS-4 character.
      }
    }
  }
  return replace(res, wide);
}
