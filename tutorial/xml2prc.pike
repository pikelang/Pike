#!/usr/local/bin/pike
/* $Id: xml2prc.pike,v 1.1 2000/03/30 05:57:48 neotron Exp $
 *
 * Converts tutorial.xml to the Palm DOC format for use in varios PalmOS
 * compatible handhelds.
 *
 * Written by David Hedbor <david@hedbor.org>
 * Requires the "makedoc" application.
 *
 */

import spider;
array from = ({"&#a0;","&amp;","&lt;","&gt;", "&nbsp;"});
array to = ({" ","&","<",">", " "});

#define BOOKMARK "*)"
string module;
string tag_type(string tag, mapping args, string data)
{
  switch(tag) {
   case "chapter":
   case "appendix":
    data = sprintf("$$chapter$$" BOOKMARK " %s: %s\n\n%s",args->number, args->title, data);
    break;
   case "section":
    if(!(int)args->number) {
      sscanf(args->number, "%*s.%s", args->number);
      args->number = "0."+args->number;
    }
    data = sprintf(BOOKMARK " %s: %s\n\n%s", args->number, args->title, data);
    break;
   case "preface":
   case "introduction":
    data = sprintf(BOOKMARK " %s\n\n%s", args->title, data);
    break;
   case "module":
    module = args->name;
    break;
   case "class":
   case "method":
   case "function":
    sscanf(args->name, module+".%s", args->name);
    data = sprintf(BOOKMARK " %s\n\n%s", args->name, data);
    
  }
  return data;
}

string tag_anchor(string tag, mapping args, string data)
{
  if(!args->type)
    sscanf(data, "<%s ", args->type);
  
  if(args->type)
    data = parse_html(data, ([]), ([ args->type: tag_type ]));
  return data;
}

string tag_pre(string tag, mapping args, string data)
{
  return "\n\n"+
    replace(data,
	    ({ " ", "\t", "\n",}),
	    ({ "&nbsp;", "&nbsp;&nbsp;&nbsp;", "<br/>" }))
    +"\n\n";
}

string tag_li(string tag, mapping args, string data)
{
  return "<br/>&nbsp;&nbsp;\x95 "+String.trim_all_whites(data);
}

string remove_html(string s)
{
  string p1, p2;
  s = " "+s;
  while(sscanf(s, "%s<%*s>%s", p1, p2) == 3)
  {
    write(replace(p1, from, to));
    s = p2;
  }
  write(replace(s, from, to));
}

string tag_remove(string tag, mapping args, string data) {
  return data||"";
}

string tag_example(string tag, mapping args, string data) {
  return "&nbsp;&nbsp;"+data;
}
string tag_indent(string tag, mapping args)
{
  return "&nbsp;&nbsp;";
}

string tag_mantitle(string tag, mapping args, string data)
{
  return "\n\n"+args->title+"\n\n"+data+"\n\n";
}


string tag_aarg(string tag, mapping args, string data)
{
  return "&nbsp;&nbsp;"+data;
}
string tag_adesc(string tag, mapping args, string data)
{
  return "&nbsp;&nbsp;&nbsp;&nbsp;"+data;
}

int li;
string tag_list(string tag, mapping args, string data, int|void count) {
  switch(tag) {
   case "dl":
    return "\n\n"+parse_html(data, ([]), ([ "dt":tag_list,
					    "dd":tag_list]))+"\n\n";
   case "dt":
    return "\n\n"+String.trim_all_whites(data)+"<br/>";
   case "dd":
    return "&nbsp;&nbsp;"+String.trim_all_whites(data)+"<br/>";
   case "ul":
    return "\n\n"+parse_html(data, ([]), ([ "li":tag_li]))+"\n\n";
   case "ol":
    li = 0;
    return "\n\n"+parse_html(data, ([]), ([ "li":tag_list]), 1)+"\n\n";
   case "li":
    return sprintf("<br/>&nbsp;&nbsp; %2d: %s", ++li,
		   String.trim_all_whites(data));
  }
}

string tag_ref(string tag, mapping args) {
  return args->to;
}

string tag_nl(string tag, mapping args, string data)
{
  return "\n\n"+String.trim_all_whites(data)+"\n\n";
}

string tag_tr(string tag, mapping args, string data)
{
  return "<br/>"+String.trim_all_whites(data);
}
string tag_td(string tag, mapping args, string data)
{
  return "  "+String.trim_all_whites(data);
}

string tag_data(string tag, mapping args, string data)
{
  switch(tag) {
   case "data_description":
    switch((args->type/ "(")[0]);
    {
     case "mapping":
      return "\n\n([\n"+parse_html(data, ([]), (["elem":tag_data]))+"])\n\n";
     case "array":
      return "\n\n({\n"+parse_html(data, ([]), (["elem":tag_data]))+"})\n\n";
     default:
      werror("Nonhandled datadesc type: %s\n", args->type);
      return "\n\n"+parse_html(data, ([]), (["elem":tag_data]))+"\n\n";
    }
    break;
   case "elem":
    string s = "&nbsp;&nbsp;";
    if(args->name)
      s+= "\""+args->name+"\" : ";
    if(args->type)
      s+= args->type;
    if(args->value)
      s+= " "+args->value;
    s+= "&nbsp;&nbsp;&nbsp;&nbsp;"+ String.trim_all_whites(replace(data, "<br/>", " "))+"<br/>";
    return s;
  }
}

void main(int argc, array (string) argv)
{
  string data = Stdio.read_file(argv[1]);
  werror("Parsing xml-file...");
  data = parse_html(data, ([
    "ex_indent/": tag_indent,
    "img": tag_remove,
    "ref": tag_ref, 
    "image": tag_remove,
  ]),([
    "anchor": tag_anchor,
    "pre": tag_pre,
    "ul": tag_list,
    "ol": tag_list,
    "example": tag_example,
    "tr": tag_tr,
    "data_description": tag_data,
    "td": tag_td,
    "th": tag_td,
    "ex_identifier": tag_remove,
    "ex_meta": tag_remove,
    "link": tag_remove,
    "man_title": tag_mantitle, 
    "ex_keyword": tag_remove,
    "ex_comment": tag_remove,
    "ex_string": tag_remove,
    "arguments": tag_remove,
    "aargdesc": tag_remove,
    "h1": tag_remove,
    "h2": tag_remove,
    "aarg": tag_aarg,
    "dl": tag_list,
    "font": tag_remove,
    "center": tag_nl,
    "exercises": tag_nl,
    "exercise": tag_nl,
    "table": tag_nl,
    "firstpage": tag_remove,
    "wmml": tag_remove,
    "metadata": tag_remove,
    "data": tag_remove,
    "tt": tag_remove, 
    "i": tag_remove,
    "b": tag_remove,
    "a": tag_remove,
    "adesc": tag_adesc,
  ]));
  werror("ok.\nCleaning up whitespaces...");
  data = replace(data, ({ "\n\n", "</p>" }),  ({ "<p>", "" }));
  data = (replace(data, ({"\r", "\t", "\n"}),
		  ({" ", " ", " "})) / " " - ({""}))*" ";
  data = replace(data, ({ "<p>", "<br/>", "<ex_br/>" }),
		 ({ "\n\n", "\n", "\n&nbsp;&nbsp;"}));  
  string tmp;
  data = map(data / "\n", String.trim_all_whites)*"\n";
  while(1) {
    tmp = replace(data, "\n\n\n", "\n\n");
    if(tmp == data) 
      break;
    data = tmp;
  }
  data = String.trim_all_whites(replace(data, from, to));
  write("ok.\n");
  array sections = data / "$$chapter$$";
  string convert="";
#define FN "palmdoc_chapter_"+num+".txt"
#define FN2 "palmdoc_chapter_"+num+".prc"
  foreach(sections, string sect)
  {
    string name, num;
    if(sscanf(sect, BOOKMARK " %s: %s\n", num, name) != 2) {
      name = "Pike Intro";
      num = "00";
    } else {
      if((int)num && (int)num< 10)
	num = "0"+num;
      name = "Pike: "+num+". "+name;
    }
    werror("Creating " FN2 "...");
    rm(FN);
    Stdio.write_file(FN, sect+"\n\n<" BOOKMARK ">");
    Process.popen("makedoc  " FN " " FN2 " '"+name+"'");
    rm(FN);
    werror("ok.\n");
  }
}  
