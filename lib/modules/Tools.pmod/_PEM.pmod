#pike __REAL_VERSION__

/* _PEM.pmod
 *
 * Kludge.
 */

/* Regexp used to decide if an encapsulated message includes headers
 * (conforming to rfc 934). */

object rfc822_start_re = Regexp("^([-a-zA-Z][a-zA-Z0-9]*[ \t]*:|[ \t]*\n\n)");

/* Regexp used to extract the interesting part of an encapsulation
 * boundary. Also strips spaces, and requires that the string in the
 * middle between ---foo  --- is at least two characters long. Also
 * allow a trailing \r or other white space characters. */

object rfc934_eb_re = Regexp(
  "^-*[ \r\t]*([^- \r\t]"	/* First non dash-or-space character */
  ".*[^- \r\t])"		/* Last non dash-or-space character */
  "[ \r\t]*-*[ \r\t]*$");	/* Trailing space, dashes and space */

/* Start and end markers for PEM */

/* A string of at least two charecters, possibly surrounded by whitespace */
#define RE "[ \t]*([^ \t].*[^ \t])[ \t]*"

object begin_pem_re = Regexp("^BEGIN" RE "$");
object end_pem_re = Regexp("^END" RE "$");

array(string) dash_split(string data)
{
  /* Find suspected encapsulation boundaries */
  array parts = data / "\n-";
    
  //    werror(sprintf("Exploded: %O", parts));
    
  /* Put the newlines back */
  for (int i; i < sizeof(parts) - 1; i++)
    parts[i]+= "\n";
  return parts;
}

string dash_stuff(string msg)
{
  array parts = dash_split(msg);
    
  if (sizeof(parts[0]) && (parts[0][0] == '-'))
    parts[0] = "- " + parts[0];
  return parts * "- -";
}

#if 0
string chop_cr(string s)
{
  return (sizeof(s) && (s[-1] == '\r'))
    ? s[..sizeof(s) - 2]
    : s;
}
#endif

/* Strip dashes */
string extract_boundary(string s)
{
  array a = rfc934_eb_re->split(s);
  return a && a[0];
}
