#pike __REAL_VERSION__

// This module contains utility functions for XML creation and
// some other useful stuff common to all the modules.

#include "./debug.h"

static constant DOC_COMMENT = "//!";

static int isDigit(int c) { return '0' <= c && c <= '9'; }

static int isDocComment(string s) {
  return has_prefix(s, DOC_COMMENT);
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

static string writeattributes(mapping(string:string) attrs)
{
  string s = "";
  foreach(sort(indices(attrs || ([]))), string attr)
    s += sprintf(" %s='%s'", attr, attributequote(attrs[attr]));
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
  string s = "<" + t + writeattributes(attributes) + "/>";
  return s;
}

class SourcePosition {
  string filename;
  int firstline;
  int lastline;

  static void create(string filename, int firstline,
                     int|void lastline)
  {
    if (!firstline) {
      werror("**********************************************************\n");
      werror("* NO FIRST LINE !!!!! \n");
      werror("**********************************************************\n");
      werror("%s", describe_backtrace(backtrace()));
      werror("**********************************************************\n");
      werror("**********************************************************\n");
    }
    this_program::filename = filename;
    this_program::firstline = firstline;
    this_program::lastline = lastline; 
  }

  SourcePosition copy() {
    return SourcePosition(filename, firstline, lastline);
  }

  string _sprintf() {
    string res = "SourcePosition(File: " + (filename ? filename : "?");
    if (firstline)
      if (lastline)
        res += sprintf(", lines: %d..%d", firstline, lastline);
      else
        res += sprintf(", line: %d", firstline);
    return res + ")";
  }

  string xml() {
    mapping(string:string) m = ([]);
    m["file"] = filename || "?";
    if (firstline) m["first-line"] = (string) firstline;
    if (lastline) m["last-line"] = (string) lastline;
    return xmltag("source-position", m);
  }
}

class AutoDocError {
  SourcePosition position;
  string part;     // which part of the autodoc system...
  string message;
  static void create(SourcePosition _position, string _part, string _message) {
    position = _position;
    part = _part;
    message = _message;
  }
  string _sprintf() {
    return sprintf("AutoDocError(%O, %O, %O)", position, part, message);
  }
}
