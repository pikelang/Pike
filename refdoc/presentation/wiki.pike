// $Id$

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

string create_reference(string from, string to) {
      return "<font face='courier'><a href='" +
	cquote(to) + "'>" + to + "</a></font>";
}

int main(int x, array y) {

  lay->docgroup = "<hr/>";
  lay->_docgroup = "";
  lay->dochead = "";
  lay->_dochead = "";
  lay->ndochead = "<p><br />";
  lay->_ndochead = "</p>";
  lay->dochead = "\n<h2>";
  lay->_dochead = "</h2>";
  lay->typehead = 0;
  lay->_typehead = 0;
  lay->docbody = "";
  lay->_docbody = "";
  lay->fixmehead = "\n<h2>";
  lay->_fixmehead = "</h2>\n";

  lay->example = "<pre>";
  lay->_example = "</pre>";
  lay->code = "<pre>";
  lay->_code = "</pre>";

  return ::main(x,y);
}
