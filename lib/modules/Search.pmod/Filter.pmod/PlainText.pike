// Filter for text/plain
// Copyright © 2000, Roxen IS.

inherit Search.Filter.Base;

constant contenttypes = ({ "text/plain" });

constant whtspaces = ({ "\n", "\r", "\t" });
constant interpunc = ({ ".", ",", ";", ":", "-", "_", "!", "\"", "?", "/",
			"\\", "(", ")", "{", "}", "[", "]" });

string _sprintf()
{
  return "Search.Filter.PlainText";
}

inline string normalize(string text)
{
  return replace(text, whtspaces+interpunc,
		 ({" "})*sizeof(whtspaces+interpunc));
}

array(string) content=({});
array(int) context=({});
array(int) offset=({});

void set_content(string c)
{
  content=({ normalize(c) });
  context=({ 0 });
  offset=({ 1 });
}

void add_content(string c, int t)
{
  content+=({ normalize(c) });
  context+=({ t });
  offset+=({ 0 });
}
			    
array(array) get_filtered_content()
{
  return ({ content, context, offset });
}

array get_anchors() { return 0; }
string get_title() { return ""; }
string get_keywords() { return ""; }
string get_description() { return ""; }
