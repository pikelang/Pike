/* _PEM.pmod
 *
 * Kludge.
 */

/* Regexp used to decide if an encapsulated message includes headers
 * (conforming to rfc 934). */

object rfc822_start_re = Regexp("^([-a-zA-Z][a-zA-Z0-9]*[ \t]*:|[ \t]*\n\n)");

/* Regexp used to extract the interesting part of an encapsulation
 * boundary. Also strips spaces, and requires that the string in the
 * middle between ---foo  --- is at least two characters long. */

object rfc934_eb_re = Regexp("^-*[ \t]*([^- \t].*[^- \t])[ \t]*-*$");

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

/* Strip dashes */
string extract_boundary(string s)
{
  array a = rfc934_eb_re->split(s);
  return a && a[0];
}
