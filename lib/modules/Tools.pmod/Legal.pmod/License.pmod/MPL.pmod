#pike __REAL_VERSION__

// $Id: MPL.pmod,v 1.2 2002/05/31 22:24:42 nilsson Exp $

static constant name = "Mozilla Public License 1.1";
static constant text = #string "mpl.txt";

string get_name() {
  return name;
}

string get_text() {
  return text;
}

string get_xml() {
  return "<pre>\n" + get_text() + "</pre>\n";
}
