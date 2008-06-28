#pike __REAL_VERSION__

// $Id: MPL.pmod,v 1.3 2008/06/28 16:37:02 nilsson Exp $

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
