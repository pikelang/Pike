#!/usr/local/bin/pike
/*
 * $Id: pikedoc2.pike,v 1.2 1999/07/09 17:10:06 grubba Exp $
 *
 * Pike-doc extractor mk II
 *
 * Henrik Grubbström 1999-07-08
 */

/* Load spider. */
#if constant(spider)
#endif /* constant(spider) */

array(string) txt_to_wmml(array(string) lines)
{
  array(string) res = allocate(sizeof(lines)+1, "");

  array(string) a = lines[0]/" ";

  if (!(< "METHOD", "FUNCTION" >)[a[0]]) {
    werror(sprintf("%O\n"
		   "Not in txt format!\n", lines[0]));
    return lines;
  }

  if (sizeof(a) < 2) {
    werror(sprintf("%O\n"
		   "Missing name\n", lines[0]));
    return res;
  }

  string type = lower_case(a[0]);
  string name = a[1];

  string title;

  if (sizeof(a) > 2) {
    title = a[2..]*" ";
  }

  if (title) {
    res[0] = sprintf("<%s name=%O title=%O>", type, name, title);
  } else {
    res[0] = sprintf("<%s name=%O>", type, name);
  }

  res[-1] = sprintf("</%s>\n", type);

  string current_container;
  int i;
  for (i=1; i < sizeof(lines); i++) {
    if ((lines[i] != "") && (lines[i] == upper_case(lines[i]))) {
      if (current_container) {
	res[i-1] += sprintf("\n</man_%s>", current_container);
      }
      res[i] = sprintf("<man_%s>", current_container = lower_case(lines[i]));
    } else {
      if ((lines[i] != "") && (lines[i][0] == '\t')) {
	// Remove initial tab.
	res[i] = lines[i][1..];
      } else {
	res[i] = lines[i];
      }
    }
  }

  if (current_container) {
    if (res[-2] == "") {
      res[-2] = sprintf("</man_%s>", current_container);
    } else {
      res[-2] += sprintf("\n</man_%s>", current_container);
    }
  }

  return res;
}

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

  switch(lower_case(attrs->type || "wmml")) {
  case "wmml":
    // No need to do anything.
    break;
  case "txt":
    s = txt_to_wmml(s);
    break;
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
