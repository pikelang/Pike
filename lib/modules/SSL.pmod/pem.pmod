/* pem.pmod
 *
 * Primitive PEM decoder. (Extracted from roxen/server/protocols/ssl3.pike).
 */

object begin_pem = Regexp("-----BEGIN (.*)----- *$");
object end_pem = Regexp("-----END (.*)----- *$");

mapping(string:string) parse_pem(string f)
{
#ifdef PEM_DEBUG
  werror(sprintf("parse_pem: '%s'\n", f));
#endif
#if 0
if(!f)
  {
    werror("PEM: No certificate found.\n");
    return 0;
  }
#endif
  array(string) lines = f / "\n";
  string name = 0;
  int start_line;
  mapping(string:string) parts = ([ ]);

  for(int i = 0; i < sizeof(lines); i++)
  {
    array(string) res;
    if (res = begin_pem->split(lines[i]))
    {
#ifdef PEM_DEBUG
      werror(sprintf("Matched start of '%s'\n", res[0]));
#endif
      if (name) /* Bad syntax */
	return 0;
      name = res[0];
      start_line = i + 1;
    }
    else if (res = end_pem->split(lines[i]))
    {
#ifdef PEM_DEBUG
      werror(sprintf("Matched end of '%s'\n", res[0]));      
#endif
      if (name != res[0]) /* Bad syntax */
	return 0;
      parts[name] = MIME.decode_base64(lines[start_line .. i - 1] * "");
      name = 0;
    }
  }
  if (name) /* Bad syntax */
    return 0;
#ifdef PEM_DEBUG
  werror(sprintf("pem contents: %O\n", parts));
#endif
  return parts;
}

string build_pem(string tag, string data)
{
  return sprintf("-----BEGIN %s-----\n\n"
		 "%s\n"
		 "-----END %s-----\n",
		 tag, MIME.encode_base64(data), tag);
}
