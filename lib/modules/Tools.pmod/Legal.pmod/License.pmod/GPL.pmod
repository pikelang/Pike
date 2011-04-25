#pike __REAL_VERSION__

// $Id$

protected constant name = "GNU General Public License 2";
protected constant text = #string "gpl.txt";

string get_name() {
  return name;
}

string get_text() {
  return text;
}

string get_xml() {
  return "<pre>\n" + get_text() + "</pre>\n";
}
