// $Id: wiki.pike,v 1.1 2002/02/14 21:21:32 nilsson Exp $

inherit "tree-split-autodoc.pike";

string cquote(string n)
{
  // http_encode_url from roxenlib
  return replace(n,
		 ({"\000", " ", "\t", "\n", "\r", "%", "'", "\"", "#",
		   "&", "?", "=", "/", ":", "+", }),
		 ({"%00", "%20", "%09", "%0a", "%0d", "%25", "%27", "%22", "%23",
		   "%26", "%3f", "%3d", "%2f", "%3a", "%2b"}));
}
