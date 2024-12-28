#pike __REAL_VERSION__

//! GNU General Public License 2

protected constant name = "GNU General Public License 2";
protected constant text = #string "gpl.txt";

//! Returns the name of the license.
string get_name() {
  return name;
}

//! Returns the text of the license.
string get_text() {
  return text;
}

//! Returns an XML @tt{<pre>@}-fragment with the text of the license.
string get_xml() {
  return "<pre>\n" + Parser.XML.text_quote(get_text()) + "</pre>\n";
}
