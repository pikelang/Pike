#!/usr/local/bin/pike
/*
 * $Id: pikedoc2.pike,v 1.1 1999/07/08 21:57:23 grubba Exp $
 *
 * Pike-doc extractor mk II
 *
 * Henrik Grubbström 1999-07-08
 */

/* Load spider. */
#if constant(spider)
#endif /* constant(spider) */

string _extract_pikedoc(string tag, mapping attrs, string contents,
			mapping res)
{
  /* NOTE: Drop the first line. */
  array(string) s = (contents/"\n")[1..];
  int i;

  for(i=0; i < sizeof(s); i++) {
    int j = search(s[i], "*:");

    if (j >= 0) {
      s[i] = s[i][j+2..];
      if (sizeof(s[i]) && (s[i][0] == ' ')) {
	s[i] = s[i][1..];
      }
    }
  }

  res->res += s*"\n" + "\n";

  return("");
}

string extract_pikedoc(string input)
{
  mapping res = (["res":""]);

  parse_html(input, ([]), (["pikedoc":_extract_pikedoc]), res);

  return(res->res);
}

int main(int argc, array(string) argv)
{
  string raw = Stdio.stdin->read();

  write(extract_pikedoc(raw));

  return(0);
}
