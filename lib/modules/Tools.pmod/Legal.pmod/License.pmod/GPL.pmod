#pike __REAL_VERSION__

// $Id: GPL.pmod,v 1.2 2002/05/31 22:24:42 nilsson Exp $

static constant name = "GNU General Public License 2";
static constant text = #string "gpl.txt";

string get_name() {
  return name;
}

string get_text() {
  return text;
}

string get_xml() {
  return "<pre>\n" + get_text() + "</pre>\n";
}
