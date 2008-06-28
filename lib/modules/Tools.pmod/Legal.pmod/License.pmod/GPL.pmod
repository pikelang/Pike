#pike __REAL_VERSION__

// $Id: GPL.pmod,v 1.3 2008/06/28 16:37:01 nilsson Exp $

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
