#include "types.h"
inherit "html";

SGML wmml_to_html(SGML data)
{
  return convert(data);
}

string mkfilename(string section)
{
  string s=::mkfilename(section);
  return replace(s,".md.html",".md");
}


SGML split(SGML data)
{
  SGML ret=html::split(data);
  foreach(indices(sections),string file)
    {
      sections[file+".md"]=({
	"string title=\"Pike Manual\";\n"
	"string template=\"manual.tmpl\";\n"
      });
    }
  return ret;
}
