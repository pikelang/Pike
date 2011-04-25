#pike __REAL_VERSION__

// $Id$

protected constant name = "Mozilla Public License 1.1";
protected constant text = #string "mpl.txt";

string get_name() {
  return name;
}

string get_text() {
  return text;
}

string get_xml() {
  return "<pre>\n" + get_text() + "</pre>\n";
}
