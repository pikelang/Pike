#!/usr/local/bin/pike
//.
//. File:	pikedoc.pike
//. RCSID:	$Id: pikedoc.pike,v 1.1 1999/07/05 10:36:40 grubba Exp $
//. Author:	David Kågedal (kg@infovav.se)
//.
//. Synopsis:	The Pikedoc parser.
//.
//. ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. This object converts pike source commented according to the
//. pikedoc standard to a HTML file.

#include <stdio.h>

#ifdef ROXEN
#include <roxen.h>
#else
void perror(string s, mixed ... args)
{
  werror(sprintf(s, @args));
}
#endif

//. o Pikedoc
//.
//. This class contains the documentation in a Pike source file.
//.
class Pikedoc
{
#ifndef ROXEN
  void perror(string s, mixed ... args)
  {
    werror(sprintf(s, @args));
  }
#endif

  //. o Function
  //.
  //. A class containing documentation about a function.
  //.
  class Function
  {
    //. + name
    //.   The name of the function.
    string name;
    //. + ret_type
    //.   The return type of the function.
    string ret_type;
    //. + args
    //.   The arguments as an array of arrrays.
    array args;
    //. + locals
    //.   An array of objects describing local variables.
    array locals;
    //. + doc
    //.   The documentation string.
    string doc;
    //. + rest
    //.   The remaining string.
    string rest;

    //. - create
    //. > _name
    //.   The name of the function.
    //. > _ret_type
    //.   The return type of the function.
    //. > _args
    //.   The arguments as an array of arrrays.
    //. > _locals
    //.   An array of objects describing local variables.
    //. > _doc
    //.   The documentation string.
    //. > _rest
    //.   The remaining string.
    void create(string _name, string _ret_type, array _args,
		array _locals, string _doc, string _rest)
    {
      name = _name;
      ret_type = _ret_type;
      args = _args;
      locals = _locals;
      doc = _doc;
      rest = _rest;
    }

    //. - magic
    //.
    //. Output the function documentation in magic format.
    //.
    string magic()
    {
      string tag, result;
      if (name == "create")
	tag = "constructor";
      else
	tag = "function";
      result = "<"+tag+" name='"+name+
	"' type='"+ret_type+ "'>\n";
      foreach(args, array a)
	result += "<arg name='"+a[1]+"' type='"+a[0]+"'>"+
	  (stringp(a[2])?a[2]:a[2]->doc)+
	  "</arg>\n";
      result += doc;
      foreach(locals, mapping p)
	result += p->magic();

      result += "</"+tag+">\n";
      return result;
    }
  }
  
  //. o Variable
  //.
  //. A class containing documentation about a variable.
  //.
  class Variable
  {
    string name, var_type, doc, rest;

    //. - create
    //.
    void create(string _name, string _var_type, string _doc, string _rest)
    {
      name = _name;
      var_type = _var_type;
      doc = _doc;
      rest = _rest;
    }

    //. - magic
    //.
    //. Output the function documentation in magic format.
    //.
    string magic()
    {
      return "<variable name="+name+
	" type='"+var_type+"'>\n" +
	doc +
	"</variable>\n";
    }
  }
  
  //. o Class
  //.
  //. A class containing documentation about a class.
  //.
  class Class
  {
    string name, doc, rest;
    array locals;

    //. - create
    //.
    void create(string _name, array _locals, string _doc, string _rest)
    {
      name = _name;
      locals = _locals;
      doc = _doc;
      rest = _rest;
    }

    //. - magic
    //.
    //. Output the function documentation in magic format.
    //.
    string magic()
    {
      string result = "<class name="+name+">\n" + doc;
      foreach(locals, mapping p)
	result += p->magic();
      result += "</class>\n";
      return result;
    }
  }

  //. o Module
  //.
  //. A class containing documentation about a module.
  //.
  class Module
  {
    string name, doc, rest;
    array locals;

    //. - create
    //.
    void create(string _name, array _locals, string _doc, string _rest)
    {
      name = _name;
      locals = _locals;
      doc = _doc;
      rest = _rest;
    }

    //. - magic
    //.
    //. Output the function documentation in magic format.
    //.
    string magic()
    {
      string result = "<module name='"+name+"'>\n" + doc;
      foreach(locals, mapping p)
	result += p->magic();
      result += "</module>\n";
      return result;
    }
  }

  private int in_header = 0;

  //. + headers
  //.
  //. A mapping from header name to header content.
  //.
  mapping(string:string) headers = ([ ]);

  //. + parts
  //.
  //. An array of the parts of the file.
  array(mapping(string:mixed)) parts = ({ });
  
  //. + functions
  //.
  //. A mapping from function name to function documentation.
  //.
  mapping(string:mixed) functions = ([ ]);

  //. + variables
  //.
  //. A mapping from variable name to variable documentation.
  //.
  mapping(string:mixed) variables = ([ ]);

  //. + classes
  //.
  //. A mapping from variable name to variable documentation.
  //.
  mapping(string:mixed) classes = ([ ]);

  //. * Functions

  array (string) find_type(string in);  
  array parse(string s);

  //. - match_brace
  //.
  //. Find the closing brace for the first opening brase in a string.
  //.
  int match_brace(string s)
  {
    int level = 0, pos, len=sizeof(s), i, slash;
    for(pos=0;pos<len;pos++)
    {
      switch(s[pos])
      {
      case '\'':
	for(i=pos+1;i<len;i++)
	  switch(s[i])
	  {
	  case '\'':
	    pos = i; i = len; break;
	  case '\\':
	    i++; break;
	  }
	slash = 0;
	break;
      case '"':
	for(i=pos+1;i<len;i++)
	  switch(s[i])
	  {
	  case '"':
	    pos = i; i = len; break;
	  case '\\':
	    i++; break;
	  }
	slash = 0;
	break;
      case '/':
	if (slash)
	{
	  pos = search(s, "\n", pos);
	  if (pos == -1)
	    pos = len;
	}
	slash = !slash;
	break;
      case '*':
	if (slash)
	  pos = search(s, "*/", pos);
	slash = 0;
	break;
      case '{':
	level++;
	slash = 0;
	break;
      case '}':
	level--;
	if (level == 0)
	  return pos+1;
	slash = 0;
	break;
      default:
	slash = 0;
      }
    }
    perror("No matching brace found!\n");
    return len;
  }

  //. - parse_function
  //.
  //. Parse a function and its documentation.
  //.
  object(Function) parse_function(string rest)
  {
    string fn, as, tp, n, doc = "", fn_doc, s, argname;
    array tmp, args;
    mapping arg_doc = 0;
    int pos;

    sscanf(rest[1..],"%*[ \t]%[^\n]\n%s",n,rest);
    while (1)
    {
      sscanf(rest, "%*[ \t]%s", rest);
      if (sscanf(rest, "//.%[^\n]\n%s", s, rest) == 2)
      {
	sscanf(s,"%*[ \t]%s", s);

	// Is this an argument doc?
	if (sizeof(s) && (s[0] == '>'))
	{
	  if (arg_doc)
	    arg_doc[argname] = doc;
	  else
	  {
	    fn_doc = doc;
	    arg_doc = ([ ]);
	  }
	  sscanf(s[1..], "%*[ \t]%s", argname);
	  doc = "";
	}
	else
	  doc += s+"\n";
      }
      else if (sscanf(rest, "%*[ \t]\n%s", rest) == 2)
	;
      else
	break;
    }
    if (arg_doc)
      arg_doc[argname] = doc;
    else
    {
      fn_doc = doc;
      arg_doc = ([ ]);
    }
    
    // Find out the return type
    tmp = find_type(rest);
    
    tmp[2] -= " ";
    if (tmp[2] != n)
      perror("Name mismatch: '%s' <-> '%s'\n", n, tmp[2]);
    tp = tmp[1];
    args = ({ });
    if (sscanf(tmp[3],"%*[ ];%*s") == 2) {
      // variable.
      werror(sprintf("%s is a variable, and not a function.\n", n));

      if ((sscanf(tp, "function%*[ ](%s", tp) == 2) &&
	  (tp[-1] == ')')) {
	tp = tp[..sizeof(tp)-2];

	string funargs, rettype;
	sscanf(reverse(tp), "%s:%s", rettype, funargs);
	if (rettype) {
	  tp = reverse(rettype);

	  if (funargs) {
	    funargs = reverse(funargs);

	    args += ({ funargs, "unknown", "" });
	  }
	} else {
	  tp = "mixed";
	}
      }
    } else {
      sscanf(tmp[3], "%*[ ](%s", as);
      if (as) {
	while(sscanf(as, "%*[ ])%*s") != 2)
	{
	  tmp = find_type(as);
	  if (!tmp)
	  {
	    perror("find_type returned 0 while parsing parameters for %s\n", n);
	    break;
	  }
	  // perror("argument: %s %s\n", tmp[1], tmp[2]);
	  sscanf(tmp[2], "%*[ ]%s", tmp[2]);
	  args += ({ tmp[1..2]+({ arg_doc[tmp[2]] || "" }) });
	  as = tmp[3];
	  rest = tmp[3];
	} 
      }
      // perror("%O\n", args);
      pos = match_brace(rest);
    }
    return Function(n, tp, args, parse(rest[..pos-1]), fn_doc, rest[pos..]);
  }
  
  //. - parse_variable
  //.
  //. Parse a variable and its documentation.
  //.
  //. > rest
  //.   A string starting with the variable to parse.
  //.
  object(Variable) parse_variable(string rest)
  {
    string n, s, doc="", typ;
    array tmp;

    sscanf(rest[1..],"%*[ \t]%[^\n]\n%s",n,rest);
    while (1)
    {
      sscanf(rest, "%*[ \t]%s", rest);
      if (sscanf(rest, "//.%[^\n]\n%s", s, rest) == 2)
      {
	doc += s+"\n";
      }
      else if (sscanf(rest, "%*[ \t]\n%s", rest) == 2)
	;
      else
	break;
    }
    tmp = find_type(rest);
    if (!tmp)
      perror("find_type returned 0\n");
    else
    {
      typ = tmp[1];
      rest = tmp[3];
    }
    return Variable(n, typ, doc, rest);
  }

  //. - parse_class
  //.
  //. Parse a class definition and its documentation.
  //.
  //. > rest
  //.   A string starting with the class to parse.
  //.
  object(Class) parse_class(string rest)
  {
    string fn, as, tp, n, doc = "", s;
    array tmp, args;
    mapping arg_doc = ([ ]);
    int pos;

    sscanf(rest[1..],"%*[ \t]%[^\n]\n%s",n,rest);
    while (1)
    {
      sscanf(rest, "%*[ \t]%s", rest);
      if (sscanf(rest, "//.%[^\n]\n%s", s, rest) == 2)
      {
	doc += s+"\n";
      }
      else if (sscanf(rest, "%*[ \t]\n%s", rest) == 2)
	;
      else
	break;
    }
    
    pos = match_brace(rest);

    return Class(n, parse(rest[..pos-1]), doc, rest[pos..]);
  }

  //. - parse_module
  //.
  //. Parse a module definition and its documentation.
  //.
  //. > rest
  //.   A string starting with the class to parse.
  //.
  object(Module) parse_module(string rest)
  {
    string fn, as, tp, n, doc = "", s;
    array tmp, args;
    mapping arg_doc = ([ ]);
    int pos;

    sscanf(rest[1..],"%*[ \t]%[^\n]\n%s",n,rest);
    while (1)
    {
      sscanf(rest, "%*[ \t]%s", rest);
      if ((sscanf(rest, "//.%[^\n]\n%s", s, rest) == 2)
	  && (s[..1] != "o "))
      {
	doc += s+"\n";
      }
      else if (sscanf(rest, "%*[ \t]\n%s", rest) == 2)
	;
      else
	break;
    }
    
    return Module(n, parse(rest), doc, "");
  }

  //. - parse
  //.
  //. The parser function. It takes a source text mass and returns an
  //. HTML text. It is a really ugly piece of work and would do good
  //. from a total rewrite.
  //.
  //. > s
  //.
  //.   The string to parse.
  //.
  array parse(string s)
  {
    // array(string) lines = filedata/"\n";
    string header, value, ss, line;
    int level = 0;
    array(mixed) stack = allocate(10); 
    object part;
    array(object) parts;
    
    parts = ({ });
    
    s += "\n";
    while(sizeof(s))
    {
      sscanf(s,"%[^\n]\n%s", line,s);
      sscanf(line,"%*[ \t]%s",line);
      
      if((sizeof(line)>2) && (line[0..2] == "//."))
      {
	sscanf(line[3..],"%*[ \t]%s",line);
	if (sizeof(line))
	{
	  if (in_header)
	  {
	    if (line[0] == '+')
	    {
	      in_header = 0;
	    }
	    else if (sscanf(line, "%s:%*[ \t]%s",
			    header, value) == 3)
	    {
	      header = lower_case(header);
	      headers[header] = value;
	    }
	  }
	  else
	  {
	    switch (line[0])
	    {
	    case '-':
	      part = parse_function(line+"\n"+s);
	      parts += ({ part });
	      s = part->rest;
	      break;
	    case '+':
	      part = parse_variable(line+"\n"+s);
	      parts += ({ part });
	      s = part->rest;
	      break;
	    case 'o':
	      part = parse_class(line+"\n"+s);
	      parts += ({ part });
	      s = part->rest;
	      break;
	    case '*':
	      part = parse_module(line+"\n"+s);
	      parts += ({ part });
	      s = part->rest;
	      break;
	    default:
	    }
	  }
	}
      }
      else // Not a pikedoc comment
      {
	// if (part->type != "code")
	// {
	//   part = ([ "type":"code", "code":"" ]);
	//   parts += ({ part });
	// }
	// part->code += line + "\n";
      }
    }
    return parts;
  }

  //. - create
  //.
  void create(string filedata)
  {
    parts = parse(filedata);
  }

  //. - tohtml
  //.
  //. Turn the documentation into HTML.
  //.
  string tohtml()
  {
    array(string) result;
    string header;
    mapping(string:mixed) part;

    result = ({ "<h1>"+headers->file+"</h1>\n" +
		headers->synopsis +
		"<hr>\n<table>\n" });
    
    foreach(indices(headers)-({"file","synopsis"}), header)
    {
      result += ({ "<tr><td>"+header+":</th><td>"+
		     headers[header]+"</td></tr>" });
    }
    
    result += ({ "\n</table>\n" });

    foreach(parts, part)
    {
      switch (part->type)
      {
      case "doc":
	result += ({ part->doc });
	break;
      case "class":
	result += ({ "<h3><i>class</i> "+part->name+"</h3>\n"+part->doc });
	break;
      case "function":
	result += ({ "<h3><i>function</i> "+part->name+"</h3>\n"+part->doc });
	break;
      case "variable":
	result += ({ "<h3><i>variable</i> "+part->name+"</h3>\n"+part->doc });
	break;
      case "code":
	result += ({ ("<dl><dd>\n"+"<pike-example light>" +
		      part->code +
		      "</pike-example>"
		      "</dl>\n"
		       ) });
	break;
      }
    }
    return result*"";
  }

  //. - tomagic
  //.
  //. Turn the documentation into magic markup.
  //.
  string tomagic()
  {
    string result;
    string header;
    mapping(string:mixed) part;

    result = "<pike-doc name='"+headers->file-".pike"+"'>\n";
    // result = "";
    result += (headers->synopsis||"")+"<p>";
    
    foreach(indices(headers)-({"file","synopsis"}), header)
    	 result += "<em>"+header+":</em> "+ headers[header]+"<br>";
    result += "<p>";
    
    foreach(parts, part)
      result += part->magic();

    return result + "<index></pike-doc>\n";
  }

  constant types=({"mapping","function","multiset","array","object","program","float","int","mixed","string","void"});
  
  //. - find_decl
  //.
  //. Parse stuff
  //.
  array (string) find_decl(string in)
  {
    string pre,decl,p2;
    sscanf(in, "%[ \t\r\n]%s", pre, in);
    if(!strlen(in)) return ({"",pre+in});
    if(in[0]==')') // Cast
      return ({"",pre+in});
    if(sscanf(in, "%[^(),; \t\n\r]%s", decl,in)==2)
      return ({ pre+decl, in });
    return ({ "", pre+in });
  }
  
  string find_complex_type(string post)
  {
    string p="";
    if(strlen(post) && post[0]=='(')
    {
      int level=1, i=1;
      while(level && i < strlen(post))
      {
	switch(post[i++])
	{
	case '(': level++;break;
	case ')': level--;break;
	}
      }
      p = p+post[..i-1];
      post = post[i..];
      if(sscanf(post, "|%s", post))
      {
	string q;
	if(sscanf(post, "%s(", q))
	{
	  p+=q;
	  post = post[strlen(q)..];
	} else if(sscanf(post, "%s%*[ \t\n\r]", post)>1) {
	  p+="|"+post;
	  return p;
	}
	p+="|"+find_complex_type(post);
      }
      return p;
    }
    if(sscanf(post, "|%s", post))
    {
      string q;
      if(sscanf(post, "%[^,();| \t]", q) && search(types,q)!=-1)
      {
	p+=q;
	post = post[strlen(q)..];
	sscanf(post, "%*[ \t\n\r]%s", post);
	return p+"|"+find_complex_type(post);
      }
    }
    if(sscanf(post, "...%s", post))
      return "...";
    return p;
  }

  array (string) find_type(string in)
  {
    string s,pre,post,decl;
    int min=10000000000,i;
    string mt;

    while (sscanf(in, "%*[ ]//%*[^\n]\n%s", in) == 3);
    foreach(types, s)
      if(((i=search(in, s))!=-1) && i<min)
      {
	if(i) switch(in[i-1])
	{
	default:
	  //perror("Invalid type thingie: '"+in[i..i]+"'\n");
	  continue;
	case ' ':
	case '\n':
	case '\r':
	case '\t':
	case '(':
	case ')':
	case '|':
	case '.':
	}
	min = i;
	mt = s;
      }

    if(!(s=mt)) return 0;

    if(sscanf(in, "%s"+s+"%s", pre, post)==2)
    {
      string op = post;
      string p="";
      sscanf(post, "%[ \t\n\r]%s", p, post);

//      if(post[0]=='|') post=post[1..];

      p += find_complex_type(post);

      p = op[..strlen(p)-1];
      post = op[strlen(p)..];
      // perror("Found type: '%s' (left: %s)\n", s+p, post-"\n");
      return ({ pre, s+p, @find_decl(post) });
    }
  }
};

//. - parse
//.
//. Called from Roxen to produce the answer.
//.
mixed parse(object id)
{
  object f, doc;
  string filename = id->query;

  if (!filename)
    return "No file";
  id->not_query = filename;
  f = File();
  f->open(filename,"r");
  doc = Pikedoc(f->read(100000));
  f->close();
  return doc->tomagic();
}

//. - main

#ifndef ROXEN
int main(int argc, array argv)
{
  object f;
  if (argc == 0)
    f = stdin;
  else 
  {
    f = File();
    f->open(argv[1],"r");
  }
  object doc;
  doc = Pikedoc(f->read(100000));
  f->close();
  write(doc->tomagic());
}
#endif
