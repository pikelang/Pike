// This module contains utility functions for XML creation and
// some other useful stuff common to all the modules.

static constant DOC_COMMENT= "//!";

static int isDigit(int c) { return '0' <= c && c <= '9'; }

static int isDocComment(string s) {
  return (strlen(s) >= 3 && s[0..2] == DOC_COMMENT);
}

//FIXME: should it return 0 for keywords ??
static int isIdent(string s) {
  if (!s || s == "")
    return 0;
  int first = s[0];
  switch(s[0]) {
    case 'a'..'z':
    case 'A'..'Z':
    case '_':
    case '`':
      break;
    default:
      if (s[0] < 128)
        return 0;
  }
  return 1;
}

static string xmlquote(string s) {
  return replace(s, ({ "<", ">", "&" }), ({ "&lt;", "&gt;", "&amp;" }));
}

static string attributequote(string s) {
  return replace(s, ({ "<", ">", "\"", "'", "&" }),
                 ({ "&lt;", "&gt;", "&#34;", "&#39;", "&amp;" }));
}

static string writeattributes(mapping(string:string) attrs) {
  string s = "";
  foreach (indices(attrs || ([]) ), string attr) {
    s += sprintf(" %s='%s'", attr, attributequote(attrs[attr]));
  }
  return s;
}

static string opentag(string t, mapping(string:string)|void attributes) {
  return "<" + t + writeattributes(attributes) + ">";
}

static string closetag(string t) { return "</" + t + ">"; }

static string xmltag(string t, string|mapping(string:string)|void arg1,
                     string|void arg2)
{
  mapping attributes = mappingp(arg1) ? arg1 : 0;
  string content = stringp(arg1) ? arg1 : stringp(arg2) ? arg2 : 0;
  if (content && content != "")
    return opentag(t, attributes) + content + closetag(t);
  return "<" + t + "/>";
}
