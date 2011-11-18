
#pike __REAL_VERSION__

/* MirarDoc documentation extractor.
 */

string IMAGE_DIR;

// Alternative makepic implementations, current first.
constant makepic = ({
  // Pike 0.7.11 and later:
  #"// Illustration.
#pike __REAL_VERSION__
  string fn;

  int verbosity;

  void create(string _fn, int|void _verbosity, void|string type) {
    fn = _fn;
    verbosity = _verbosity;
    string ext;
    if(type==\"image/gif\") ext=\"gif\";
    fn += \".\" + (ext||\"png\");
  }

  object lena() {
    catch { return Image.load(IMAGE_DIR + \"image_ill.pnm\"); };
    catch { return Image.load(IMAGE_DIR + \"lena.ppm\"); };
    catch { return Image.load(IMAGE_DIR + \"lenna.rs\"); };
    return Image.load(IMAGE_DIR + \"lena.gif\");
  }

  object|string render();

  string make() {
#if constant(Image.PNG.encode)
    object|string o=render();
    if(objectp(o))
      o=Image.PNG.encode(o);
    Stdio.write_file(fn, o);
    if(verbosity > 1)
      werror(\"Wrote %s.\\n\", fn);
#endif
    return \"<image>\"+fn+\"</image>\";
  }

  object|string render() {
",
  // Prior to 7.3.11 there were implicit imports of Image
  // and Stdio. cf src/modules/Image/illustration.pike.
  //
  // There was also a compat layer for various functions
  // that later were removed.
  #"// Pike 7.3 illustration, implicit imports.
#pike 7.3

  import Image;
  import Stdio;
  string fn;

  int verbosity;

  void create(string _fn, int|void _verbosity, void|string type) {
    fn = _fn;
    verbosity = _verbosity;
    string ext;
    if(type==\"image/gif\") ext=\"gif\";
    fn += \".\" + (ext||\"png\");
  }

  object lena() {
    catch { return load(IMAGE_DIR + \"image_ill.pnm\"); };
    catch { return load(IMAGE_DIR + \"lena.ppm\"); };
    catch { return load(IMAGE_DIR + \"lenna.rs\"); };
    return load(IMAGE_DIR + \"lena.gif\");
  }

  object|string render();

  string make() {
#if constant(Image.PNG.encode)
    object|string o=render();
    if(objectp(o))
      o=PNG.encode(o);
    Stdio.write_file(fn, o);
    if(verbosity > 1)
      werror(\"Wrote %s.\\n\", fn);
#endif
    return \"<image>\"+fn+\"</image>\";
  }

  object|string render() {
",
  // Prior to 0.7.3 the Image module classes were all lower-case.
  #"// Pike 0.6 illustration, implicit imports, only Image.image.
#pike 0.6

  import 0.6::Image;
  import 0.6::Stdio;
  string fn;

  constant Image = 7.6::Image;

  int verbosity;

  void create(string _fn, int|void _verbosity, void|string type) {
    fn = _fn;
    verbosity = _verbosity;
    string ext;
    if(type==\"image/gif\") ext=\"gif\";
    fn += \".\" + (ext||\"png\");
  }

  object lena() {
    catch { return load(IMAGE_DIR + \"image_ill.pnm\"); };
    catch { return load(IMAGE_DIR + \"lena.ppm\"); };
    catch { return load(IMAGE_DIR + \"lenna.rs\"); };
    return load(IMAGE_DIR + \"lena.gif\");
  }

  object|string render();

  string make() {
#if constant(Image.PNG.encode)
    object|string o=render();
    if(objectp(o))
      o=PNG.encode(o);
    Stdio.write_file(fn, o);
    if(verbosity > 1)
      werror(\"Wrote %s.\\n\", fn);
#endif
    return \"<image>\"+fn+\"</image>\";
  }

  object|string render() {
",
});

string execute;

mapping parse=([ ]);
int illustration_counter;

int verbosity = 1;

/*

module : mapping <- moduleM
	"desc" : text
	"see also" : array of references
	"note" : mapping of "desc": text
	"modules" : same as classes (below)
	"classes" : mapping 
		class : mapping <- classM
	        	"see also" : array of references
			"desc" : text
			"note" : mapping of "desc": text
			"methods" : array of mappings <- methodM
				"decl" : array of array of type, name[, params]
				"desc" : text
				"returns" : textline
				"see also" : array of references
				"note" : mapping of "desc": text
				"known bugs" : mapping of "desc": text
				"args" : array of mappings <- argM
					"args" : array of args names and types
					"desc" : description
				"names" : multiset of method name(s)

Quoting: Only '<' must be quoted as '&lt;'.

*/

mapping moduleM, classM, methodM, argM, nowM, descM;

mapping focM(mapping dest,string name,string line)
{
   if (!dest->_order) dest->_order=({});
   if (!has_value(dest->_order,name)) dest->_order+=({name});
   return dest[name] || (dest[name]=(["_line":line]));
}

string stripws(string s)
{
  if (s=="") return s;

  array lines = s / "\n";

  lines = map(lines, lambda(string s) {
		       s = reverse(s);
		       sscanf(s, "%*[ \t\r]%s", s);
		       s = reverse(s);
		       return s; });

  int m=10000;
  foreach (lines,string s)
    if (s!="")
    {
      sscanf(s,"%[ ]%s",string a,string b);
      if (b!="") m=min(sizeof(a),m);
    }

  return map(lines,lambda(string s) { return s[m..]; })*"\n";
}

mapping lower_nowM()
{
   if (nowM &&
       (nowM==parse
	|| nowM==classM
	|| nowM==methodM
	|| nowM==moduleM)) return nowM;
   else return nowM=methodM;
}

void report(int level, string currentfile, int line,
	    sprintf_format msg, sprintf_args ... args)
{
  if (verbosity >= level) {
    if (sizeof(args)) msg = sprintf(msg, @args);
    werror("%s:%d: %s\n", currentfile, line, msg);
  }
}

#define complain(X) (X)

string file_version = "";

string format_line(string currentfile, int line)
{
  return "file='"+currentfile+"' first-line='"+line+"'";
}

mapping keywords=
(["$""Id":lambda(string arg, string currentfile, int line)
	{
	  file_version = " version='Id: "+arg[..search(arg, "$")-1]+"'";
	},
  "module":lambda(string arg, string currentfile, int line)
	  { classM=descM=nowM=moduleM=focM(parse,stripws(arg),format_line(currentfile,line));
	    methodM=0;
	    if (!nowM->classes) nowM->classes=(["_order":({})]);
	    if (!nowM->modules) nowM->modules=(["_order":({})]);
	    report(.FLAG_VERBOSE,currentfile,line,"module "+arg); },
  "class":lambda(string arg, string currentfile, int line)
	  { if (!moduleM) return complain("class w/o module");
	    descM=nowM=classM=focM(moduleM->classes,stripws(arg),format_line(currentfile,line));
	    methodM=0; report(.FLAG_VERBOSE,currentfile,line,"class "+arg); },
  "submodule":lambda(string arg, string currentfile, int line)
	  { if (!moduleM) return complain("submodule w/o module");
	    classM=descM=nowM=moduleM=focM(moduleM->modules,stripws(arg),format_line(currentfile,line));
	    methodM=0;
	    if (!nowM->classes) nowM->classes=(["_order":({})]);
	    if (!nowM->modules) nowM->modules=(["_order":({})]);
	    report(.FLAG_VERBOSE,currentfile,line,"submodule "+arg); },
  "method":lambda(string arg, string currentfile, int line)
	  { if (!classM) return complain("method w/o class");
	    if (!nowM || methodM!=nowM || methodM->desc || methodM->args || descM==methodM)
	    { if (!classM->methods) classM->methods=({});
	      classM->methods+=({methodM=nowM=(["decl":({}),"_line":format_line(currentfile,line)])}); }
	    methodM->decl+=({parse_decl(arg)}); descM=0; },
  "inherits":lambda(string arg, string currentfile, int line)
	  { if (!nowM) return complain("inherits w/o class or module");
  	    if (nowM != classM) return complain("inherits outside class or module");
	    if (!classM->inherits) classM->inherits=({});
	    classM->inherits+=({stripws(arg)}); },

  "variable":lambda(string arg, string currentfile, int line)
	  {
	     if (!classM) return complain("variable w/o class");
	     if (!classM->variables) classM->variables=({});
	     classM->variables+=({descM=nowM=(["_line":format_line(currentfile,line)])});
	     nowM->decl=({parse_decl(arg)});
	  },
  "constant":lambda(string arg, string currentfile, int line)
	  {
	     if (!classM) return complain("constant w/o class");
	     if (!classM->constants) classM->constants=({});
	     classM->constants+=({descM=nowM=(["_line":format_line(currentfile,line)])});
	     nowM->decl=({parse_decl(arg)});
	  },

  "arg":lambda(string arg, string currentfile, int line)
	  {
	     if (!methodM) return complain("arg w/o method");
	     if (!methodM->args) methodM->args=({});
	       methodM->args+=({argM=nowM=(["args":({}),"_line":format_line(currentfile,line)])});
	     argM->args+=({arg}); descM=argM;
	  },
  "note":lambda(string arg, string currentfile, int line)
	  {
	     if (!lower_nowM())
	        return complain("note w/o method, class or module");
	     descM=nowM->note||(nowM->note=(["_line":format_line(currentfile,line)]));
	  },
  "added":lambda(string arg, string currentfile, int line)
	  {
	     if (!lower_nowM())
	        return complain("added in: w/o method, class or module");
	     descM=nowM->added||(nowM->added=(["_line":format_line(currentfile,line)]));
	  },
  "bugs":lambda(string arg, string currentfile, int line)
	  {
	     if (!lower_nowM())
	        return complain("bugs w/o method, class or module");
	     descM=nowM->bugs||(nowM->bugs=(["_line":format_line(currentfile,line)]));
	  },
  "see":lambda(string arg, string currentfile, int line)
	  {
	     if (arg[0..3]!="also")
	        return complain("see w/o 'also:'\n");
	     if (!lower_nowM())
	        return complain("see also w/o method, class or module");
	     sscanf(arg,"also%*[:]%s",arg);
	     nowM["see also"]+=map(arg/",",stripws)-({""});
	     if (!nowM["see also"])
	        return complain("empty see also\n");
	  },
  "returns":lambda(string arg)
	  {
	     if (!methodM)
	        return complain("returns w/o method");
	     methodM->returns=stripws(arg);
	     descM=0; nowM=0;
	  }
]);

string getridoftabs(string s)
{
   string res="";
   while (sscanf(s,"%s\t%s",string a,s)==2)
   {
      res+=a;
      res+="         "[(sizeof(res)%8)..7];
   }
   return res+s;
}

string htmlify(string s)
{
#define HTMLIFY(S) \
   (replace((S),({"&lt;","&gt;",">","&","\240"}),({"&lt;","&gt;","&gt;","&amp;"," "})))

   string t="",u,v;
   while (sscanf(s,"%s<%s>%s",u,v,s)==3)
      t+=HTMLIFY(u)+"<"+v+">";
   return t+HTMLIFY(s);
}

#define linkify(S) \
   ("\""+replace((S),({"->","()","&lt;","&gt;"}),({".","","<",">"}))+"\"")

string make_nice_reference(string what,string prefix,string stuff)
{
   string q;
   if (what==prefix[sizeof(prefix)-sizeof(what)-2..<2])
   {
      q=prefix[..<2];
   }
   else if (what==prefix[sizeof(prefix)-sizeof(what)-1..<1])
   {
      q=prefix[..<1];
   }
   else if (search(what,".")==-1 &&
	    search(what,"->")==-1 &&
	    !parse[what])
   {
      q=prefix+what;
   }
   else 
      q=what;

   return "<ref to="+linkify(q)+">"+htmlify(stuff)+"</ref>";
}

// Mapping from tag to their required parent tag.
constant dtd_nesting = ([
  "dt":"dl",
  "dd":"dl",
  "tr":"table",
  "td":"tr",
  "th":"tr",
]);

constant self_terminating = (< "br", "hr", "wbr" >);

ADT.Stack nesting;

array(string) pop_to_tag(string tag)
{
  array(string) res = ({});
  string top;
  while ((top = nesting->top()) && (top != tag)) {
    res += ({ "</" + top + ">" });
    nesting->pop();
  }
  if (!top && tag) {
    error("Missing container tag %O\n", tag);
  }
  return res;
}

array(string) fix_tag_nesting(Parser.HTML p, string value)
{
  // werror("fix_nesting(%O, %O)\n", p, value);

  if ((nesting->top() == "pre") && !has_prefix(value, "</pre>")) {
    return ({ _Roxen.html_encode_string(value) });
  }

  string tag = p->parse_tag_name(value[1..<1]);

  array(string) ret = ({});

  if (has_prefix(tag, "/")) {
    // End tag. Pop to starttag.
    tag = tag[1..];
    if (!has_value(nesting->arr[..nesting->ptr-1], tag)) {
      // Extraneous end-tag -- remove it.
      return ({""});
    }
    ret = pop_to_tag(tag);
    nesting->pop();
  } else {
    if (dtd_nesting[tag]) {
      if (has_value(nesting, dtd_nesting[tag])) {
	ret = pop_to_tag(dtd_nesting[tag]);
      } else {
	// Surrounding tag is missing. Add it.
	nesting->push(dtd_nesting[tag]);
	ret += ({ "<" + dtd_nesting[tag] + ">" });
      }
    }
    if (has_suffix(value, " />")) {
      // Self-terminating tag.
    } else if (self_terminating[tag]) {
      value = value[..<1] + " />";
    } else if (nesting->top() == tag) {
      // Probably a typo. Convert to a close tag.
      nesting->pop();
      value = "</" + value[1..];
    } else {
      nesting->push(tag);
    }
  }
  ret += ({ value });

  return ret;
}

.Flags flags;

Parser.HTML nesting_parser;

Parser.HTML parser;

string fixdesc(string s,string prefix,void|string where)
{
   s = stripws(replace(s, "<p>", "\n"));

   // Take care of some special cases (shifts and arrows):
   s = replace(s,
	       ({ "<<", ">>", "<->", "<=>",
		  "<-", "->", "<=", "=>",
		  "<0", "<1", "<2", "<3", "<4",
		  "<5", "<6", "<7", "<8", "<9",
	       }),
	       ({ "&lt;&lt;", "&gt;&gt;", "&lt;-&gt;", "&lt;=&gt;",
		  "&lt;-", "-&gt;", "&lt;=", "=&gt;",
		  "&lt;0", "&lt;1", "&lt;2", "&lt;3", "&lt;4",
		  "&lt;5", "&lt;6", "&lt;7", "&lt;8", "&lt;9",
	       }));

   nesting = ADT.Stack();
   nesting->push(0);	// End sentinel.

   nesting_parser->set_extra(where);

   s = nesting_parser->finish(s)->read() + pop_to_tag(UNDEFINED) * "";

   parser->set_extra(where);

   s = parser->finish(s)->read();
   s = "<p>" + (s/"\n\n")*"</p>\n\n<p>" + "</p>";
   s = htmlify(s);

   if (where && !(flags & .FLAG_NO_DYNAMIC))
      return "<source-position " + where + "/>\n"+s;

   return s;
}

multiset(string) get_method_names(array(array(string)) decls)
{
   multiset(string) names=(<>);
   foreach (decls, array(string) decl)
   {
      names[decl[1]] = 1;
   }
   return names;
}

array(string) nice_order(array(string) arr)
{
   sort(map(arr,replace,({"_","`"}),({"ÿ","þ"})),
	arr);
   return arr;
}

string addprefix(string suffix,string prefix)
{
   return prefix+suffix;
}

#define S(X) ("'"+(X)+"'") /* XML arg quote */

string doctype(string type,void|string indent)
{
   array(string) endparan(string in)
   {
      int i;
      int q=1;
      for (i=0; i<sizeof(in); i++)
	 switch (in[i])
	 {
	    case '(': q++; break;
	    case ')': q--; if (!q) return ({in[..i-1],in[i+1..]}); break;
	 }
      return ({in,""});
   };
   string combine_or(string a,string b)
   {
     b = String.trim_all_whites(b);
     if (b[..3]=="<or>") b=b[4..<5];
     return "<or>"+a+b+"</or>";
   };
   array(string) paramlist(string in,string indent)
   {
      int i;
      int q=0;
      array res=({});
      for (i=0; i<sizeof(in); i++)
	 switch (in[i])
	 {
	    case '(': q++; break;
	    case ')': q--; break;
	    case ':':
	    case ',':
	       if (!q) return ({doctype(in[..i-1],indent+"  ")})+
			  paramlist(in[i+1..],indent);
	 }
      return ({doctype(in,indent+"  ")});
   };

   if (!indent) indent="\n ";
   string nindent=indent+"  ";

   if (type[..2]=="...")
      return nindent+"<varargs>"+doctype(type[3..])+"</varargs>";

   string a=type,b=0,c,o=0;
   sscanf(type,"%s(%s",a,b);
   if (b) [b,c]=endparan(b);

   if (sscanf(a,"%s|%s",string d,string e)==2)
   {
      if (b) e+="("+b+")"+c;
      return nindent+combine_or(doctype(d,nindent),
			       doctype(e,nindent));
   }
   if (b && c!="" && sscanf(c,"%s|%s",string d,string e)==2)
      return nindent+combine_or(doctype(a+"("+b+")"+d,nindent),
			       doctype(e,nindent));

   switch (a)
   {
      case "int":
      case "float":
      case "string":
      case "void":
      case "program":
      case "array":
      case "mapping":
      case "multiset":
      case "function":
      case "object":
      case "mixed":
	 if (!b) return "<"+a+"/>";
	 break;
   }
   switch (a)
   {
      case "array":
	 return nindent+"<array><valuetype>"+
	    doctype(b,nindent)+"</valuetype></array>";
      case "multiset":
	 return nindent+"<multiset><indextype>"+
	    doctype(b,nindent)+"</indextype></multiset>";
      case "mapping":
	 array z=paramlist(b,nindent);
	 if (sizeof(z)!=2)
	    werror("warning: confused mapping type: %O\n",type),
	    z+=({"<mixed/>","<mixed/>"});
	 return nindent+"<mapping><indextype>"+z[0]+"</indextype>"+
	    nindent+"   <valuetype>"+z[1]+"</valuetype></mapping>";
      case "object":
	 return nindent+"<object>"+b+"</object>";
      case "function":
	 z=paramlist(b,nindent);
	 if (sizeof(z)<1)
	    werror("warning: confused function type: %O\n",type),
	    z+=({"<mixed/>"});
	 return
	    nindent+
	    "<function>"+
	    map(z[..<1],
		lambda(string s)
		{
		   return nindent+"  <argtype>"+s+"</argtype>";
		})*""+
	    nindent+"   <returntype>"+z[-1]+"</returntype>"+
	    nindent+"</function>";
   }
   if (b && a=="int")
   {
      if (sscanf(b,"%d..%d",int min,int max)==2)
	 return nindent+"<int><min>"+min+"</min><max>"+max+"</max></int>";
      if (sscanf(b,"..%d",int max)==1)
	 return nindent+"<int><max>"+max+"</max></int>";
      if (sscanf(b,"%d..%",int min)==1)
	 return nindent+"<int><min>"+min+"</min></int>";
   }

   if (b)
      werror("warning: confused type: %O\n",type);

   return nindent+"<object>"+type+"</object>";
}

constant convname=
([
   "`>":"`&gt;",
   "`<":"`&lt;",
   "`>=":"`&gt;=",
   "`<=":"`&lt;=",
   "`&":"`&amp;",
]);

array(string) parse_decl(string raw_decl)
{
   string rv,name,params=0;
   array tokens =
      Parser.Pike.split(replace(raw_decl, ({ "&lt;", "&gt;" }), ({ "<", ">"})));
   tokens = Parser.Pike.tokenize(tokens);
   tokens = (tokens/({"="}))[0];	// Handle constants.
   tokens = Parser.Pike.group(tokens);
   tokens = Parser.Pike.hide_whitespaces(tokens);

   if (tokens[-1] == ";") {
      tokens = tokens[..<1];
   }
   if (arrayp(tokens[-1])) {
      params = Parser.Pike.simple_reconstitute(tokens[-1]);
      tokens = tokens[..<1];
   }
   name = objectp(tokens[-1])?tokens[-1]->text:tokens[-1];
   rv = "mixed";
   if (sizeof(tokens) > 1) {
      rv = Parser.Pike.simple_reconstitute(tokens[..<1]);
   }

   if (params) {
      return ({ rv, name, params });
   }
   return ({ rv, name });
}

void docdecl(string enttype,
	     array(string) decl,
	     object f)
{
   string rv,name,params=0;
   rv = decl[0];
   name = decl[1];
   if (sizeof(decl) == 3) params = decl[2][1..];

#if 0
   sscanf(decl,"%s %s(%s",rv,name,params) == 3 ||
     sscanf(decl+"\n","%s %s\n",rv,name);

   if (name == "`" && params && has_prefix(params, ")")) {
     name = "`()";
     sscanf(params[1..], "%*s(%s", params);
   }
#endif

   if (convname[name]) name=convname[name];

   f->write("<"+enttype+" name="+S(name)+">");

   if (params)
   {
     string paramlist(string in) {
       int i;
       string res = "";

       while(i<sizeof(in)) {

	 // Find type
	 string t = "";
	 int br;
	 for (; i<sizeof(in); i++) {
	   t += in[i..i];
	   if(in[i]=='(') br++;
	   if(in[i]==')') br--;
	   if(in[i]==' ' && !br) {
	     t = doctype(t[..<1]);
	     break;
	   }
	   if(br==-1 || (in[i]==',' && !br)) {
	     if(String.trim_all_whites(t)==")")
	       return res;
	     if(String.trim_all_whites(t[..<1])=="void" && res=="")
		return "<argument/>\n";

	     if(t[-1]==')')
	       return res += "<argument><value>" + t[..<1] + "</value></argument>";
	     if(t[-1]==',')
	       break;
	   }
	 }

	 if(t[-1]==',') {
	   res += "<argument><value>" + t[..<1] + "</value></argument>";
	   i++;
	   continue;
	 }

	 // Find name
	 string n = "";
	 i++;
	 for (; i<sizeof(in); i++) {
	   if(in[i]==')') {
	     if(!sizeof(String.trim_all_whites(n)))
	       throw( ({ "Empty argument name. ("+in+")\n", backtrace() }) );
	     return res + "<argument name=" + S(n) + "><type>" + t +
	       "</type></argument>\n";
	   }
	   if(in[i]==',') {
	     if(!sizeof(String.trim_all_whites(n)))
		throw( ({ "Empty argument name. ("+in+")\n", backtrace() }) );
	     res += "<argument name=" + S(n) + "><type>" + t +
	       "</type></argument>\n";
	     break;
	   }
	   if(in[i]==' ') {
	     if(n=="...") {
	       n = "";
	       t = "<varargs>" + t  + "</varargs>";
	     }
	   }
	   else
	     n += in[i..i];
	 }
	 i++;
       }

       throw( ({ "Malformed argument list \"(" + in + "\".\n",
		 backtrace() }) );
     };

     f->write("\n   <returntype>"+doctype(rv,"\n      ")+"</returntype>\n"
	      "   <arguments>"+paramlist(params)+"\n   </arguments>\n");
   }
   else if (enttype == "variable")
   {
      f->write("<type>"+doctype(rv)+"</type>\n");
   }
   else
   {
      f->write("<typevalue>"+doctype(rv)+"</typevalue>\n");
   }

   f->write("</"+enttype+">");
}

void document(string enttype,
	      mapping huh,string name,string prefix,
	      object f, string currentfile, int line)
{
  int(0..1) has_doc;
   array(string) names;

   if (huh->_line) {
     sscanf(huh->_line, "file='%s' first-line='%d'", currentfile, line);
   }

   if (huh->names)
      names=map(indices(huh->names),addprefix,name);
   else
      names=({name});

   report(.FLAG_VERBOSE,currentfile,line,name+" : "+names*",");

   array v=name/".";
   string canname=v[-1];
   sscanf(canname,"%s->",canname);
   sscanf(canname,"%s()",canname);

   string presname = replace(sort(names)[0],
			     ([ ".":"_", ">":"_" ]) );

   if (convname[canname]) canname=convname[canname];

   switch (enttype)
   {
      case "class":
      case "module":
	 f->write("<"+enttype+" name="+S(canname)+">\n");
	 break;
      default:
	f->write("<docgroup homogen-type="+S(enttype));
	if(huh->decl && sizeof(names)==1) {
	  lambda() {
	    string m,n;
	    foreach(huh->decl, array(string) decl) {
	      n = decl[1];
	      if(!m) { m=n; continue; }
	      if(n!=m) return;
	    }
	    if(convname[m]) m=convname[m];
	    f->write(" homogen-name="+S(m));
	  }();
	}
	f->write(">\n");

	if (huh->decl) {
	  foreach (huh->decl, array(string) decl)
	    docdecl(enttype,decl,f);
	}
	else
	  foreach (names,string name) {
	    if (convname[name]) name=convname[name];
	    f->write("<"+enttype+" name="+S(name)+">\n");
	    f->write("</"+enttype+">");
	  }
	break;
   }
   if (!(flags & .FLAG_NO_DYNAMIC))
     f->write("<source-position " + huh->_line + "/>\n");

// [DESCRIPTION]

   string res="";

   if (huh->desc)
   {
      res+="<text>\n";

      if (huh->inherits)
      {
	 string s="";
	 foreach (huh->inherits,string what)
	    res+="inherits "+make_nice_reference(what,prefix,what)+
	      "<br/>\n";
	 res+="<br/>\n";
      }

      res+=fixdesc(huh->desc,prefix,huh->_line)+"\n";
      res+="</text>\n";
   }

// [ARGUMENTS]

   if (huh->args)
   {
     mapping arg;
      array(string) v = ({});

      foreach (huh->args, arg)
      {
	v += arg->args;
	 if (arg->desc)
	 {
	    res+="<group>\n";
	    foreach (v,string arg)
	    {
	       sscanf(arg,"%*s %s",arg);
	       sscanf(arg,"%s(%*s",arg); // type name(whatever)
	       res+="   <param name="+S(arg)+"/>\n";
	    }
	    res+=
	       "<text>"+
	       fixdesc(arg->desc,prefix,arg->_line)+
	       "</text></group>\n";
	    v = ({});
	 }
      }
      if(sizeof(v))
	werror("Parameters without description on line %O.\n%O\n",
	       arg->_line, v);
   }

// [RETURN VALUE]

   if (huh->returns)
   {
      res+="<group><returns/><text>\n";
      res+=fixdesc(huh->returns,prefix,huh->_line)+"\n";
      res+="</text></group>\n";
   }

// [NOTE]

   if (huh->note && huh->note->desc)
   {
      res+="<group><note/><text>\n";
      res+=fixdesc(huh->note->desc,prefix,huh->_line)+"\n";
      res+="</text></group>\n";
   }

// [BUGS]

   if (huh->bugs && huh->bugs->desc)
   {
      res+="<group><bugs/><text>\n";
      res+=fixdesc(huh->bugs->desc,prefix,huh->_line)+"\n";
      res+="</text></group>\n";
   }

// [ADDED]

   if (huh->added && huh->added->desc)
   {
      /* noop */
   }

// [SEE ALSO]

   if (huh["see also"])
   {
      res+="<group><seealso/><text>\n";
      res+=fixdesc(
	 map(huh["see also"],
	       lambda(string s)
	       {
		  return "<ref>"+htmlify(s)+"</ref>";
	       })*", ",
	 prefix,0);
      res+="</text></group>\n";
   }

   if (res!="")
   {
     has_doc = 1;
      f->write("<doc>\n"+res+"\n</doc>\n");
   }

// ---childs----

   if (huh->constants)
   {
      foreach(huh->constants,mapping m)
      {
	 string type = m->decl[0][0];
	 string name = m->decl[0][1];
	 document("constant",m,prefix+name,prefix+name+".",f,currentfile,line);
      }
   }

   if (huh->variables)
   {
      foreach(huh->variables,mapping m)
      {
	 string type = m->decl[0][0];
	 string name = m->decl[0][1];
	 document("variable",m,prefix+name,prefix+name+".",f,currentfile,line);
      }
   }

   if (huh->methods)
   {
      // postprocess methods to get names

      multiset(string) method_names=(<>);
      array(string) method_names_arr;
      string method_name;
      mapping method;

      if (huh->methods) 
	 foreach (huh->methods,method)
	    method_names|=(method->names=get_method_names(method->decl));

       method_names_arr=nice_order(indices(method_names));

      // alphabetically

      foreach (method_names_arr,method_name)
	 if (method_names[method_name])
	 {
	    // find it
	    foreach (huh->methods,method)
	       if ( method->names[method_name] )
	       {
		  document("method",method,prefix,prefix,f,currentfile,line);
		  method_names-=method->names;
	       }
	    if (method_names[method_name])
	       werror("failed to find "+method_name+" again, weird...\n");
	 }
   }

   if (huh->classes)
   {
      foreach(huh->classes->_order,string n)
      {
//  	 f->write("\n\n\n<section title=\""+prefix+n+"\">\n");
	 document("class",huh->classes[n],
		  prefix+n,prefix+n+"->",f,currentfile,line);
//  	 f->write("</section title=\""+prefix+n+"\">\n");
      }
   }

   if (huh->modules)
   {
      foreach(huh->modules->_order,string n)
      {
	 document("module",huh->modules[n],
		  prefix+n,prefix+n+".",f,currentfile,line);
      }
   }
// end ANCHOR

   switch (enttype)
   {
      case "class":
      case "module":
	 f->write("</"+enttype+">\n\n");
	 break;
      default:
	if(!has_doc) f->write("<doc/>");
	 f->write("</docgroup>\n\n");
	 break;
   }
}

string make_doc_files(string builddir, string imgdest, string|void namespace)
{
   if (verbosity > 0)
     werror("modules: " +
	    sort(indices(parse))*", " +
	    "\n");

   namespace = namespace || "predef::";
   if (has_suffix(namespace, "::")) {
      namespace = namespace[..<2];
   }

   object f = class {
      string doc = "";
      int write(string in) {
	 doc += in;
	 return sizeof(in);
      }
      string read() {
	 return "<autodoc>\n" + doc + "</autodoc>";
      }
   }();

   string here = getcwd();
   cd(builddir);

   mixed err = catch {
       // Module documentation exists in a namespace...
       f->write("<namespace name='" + namespace + "'>\n");
       foreach (sort(indices(parse)-({"_order"})),string module)
	 document("module",parse[module],module,module+".", f, "-", 1);
       f->write("</namespace>\n");
     };

   cd(here);
   if (err) throw(err);
   string autodoc = f->read();
   err = catch {
       return Tools.AutoDoc.ProcessXML.moveImages(autodoc, builddir, imgdest,
						  (flags & .FLAG_VERB_MASK) <
						  .FLAG_VERBOSE);
     };
   werror("Autodoc:\n%O\n", autodoc);
   throw(err);
}

void process_line(string s,string currentfile,int line)
{
   s=getridoftabs(s);

   int i;
   if ((i=search(s,"**""!"))!=-1 || (i=search(s,"//""!"))!=-1)
   {
      string kw,arg;

      sscanf(s[i+3..],"%*[ \t]%[^: \t\n\r]%*[: \t]%s",kw,arg);
      if (keywords[kw])
      {
	 string err;
	 if ( (err=keywords[kw](arg,currentfile,line)) )
	 {
	   report(.FLAG_QUIET, currentfile, line,
		  "process_line failed: %O", err);
	   error("process_line failed: %O\n", err);
	 }
      }
      else if (s[i+3..]!="")
      {
	 string d=s[i+3..];
   //  	    sscanf(d,"%*[ \t]!%s",d);
   //	    if (search(s,"$""Id")!=-1)
   //          report(.FLAG_VERBOSE,currentfile,line,"Id: "+d);
	 if (!descM) descM=methodM;
	 if (!descM)
	 {
	    report(.FLAG_NORMAL, currentfile, line,
		   "illegal description position.");
	    //error("process_line failed: Illegal description position.\n");
	    return;
	 }
	 if (!descM->desc) descM->desc="";
	 else descM->desc+="\n";
	 d=getridoftabs(d);
	 descM->desc+=d;
      }
      else if (descM)
      {
	 if (!descM->desc) descM->desc="";
	 else descM->desc+="\n";
      }
   }
}

string safe_newlines(string in) {
  string old;
  do {
    old = in;
    in = replace(in, "\n\n", "\n \n");
  } while(old!=in);
  return in;
}

array(string) tag_quote_args(Parser.HTML p, mapping args) {
  return ({ sprintf("<%s%{ %s='%s'%}>", p->tag_name(), (array)args) });
}

array(string) tag_preserve_ws(Parser.HTML p, mapping args, string c) {
  return ({ sprintf("<%s%{ %s='%s'%}>%s</%s>", p->tag_name(),
		    (array)args, safe_newlines(p->clone()->finish(c)->read()),
		    p->tag_name()) });
}

class CompilationHandler
{
  array(string) lines = ({});
  void compile_warning(string filename, int linenumber, string message)
  {
    lines += ({ sprintf("%s:%d:%s\n", filename, linenumber, message) });
  }
  void compile_error(string filename, int linenumber, string message)
  {
    lines += ({ sprintf("%s:%d:%s\n", filename, linenumber, message) });
  }
}

array(program|string) try_compile_illustration(array(string) templates,
					       string illustration_code,
					       string where)
{
  CompilationHandler handler;
  string code;
  mixed err;
  string defines = sprintf("#define IMAGE_DIR %O\n", IMAGE_DIR);
  foreach(templates; int t; string template) {
    if ((flags & .FLAG_COMPAT) && !t &&
	(has_value(illustration_code, "->map_closest") ||
	 has_value(illustration_code, "->map_fs"))) {
      continue;
    }
    code = defines + template + illustration_code + "\n;\n}\n";
    handler = CompilationHandler();
    err = catch {
	return ({ compile_string(code, "-", handler), code });
      };
    if (!(flags & .FLAG_COMPAT)) break;
  }
  werror("Compilation of illustration at %O failed:\n"
	 "%s\n"
	 "******\n", where, handler->lines * "");
  array(string) rows = code/"\n";
  for(int i; i<sizeof(rows); i++)
    werror("%04d: %s\n", i+1, rows[i]);
  werror("******\n");
  if (flags & .FLAG_KEEP_GOING)
    return ({ UNDEFINED, code });
  throw(err);
}

void create(string image_dir, void|.Flags flags)
{
  verbosity = (flags & .FLAG_VERB_MASK);
  this_program::flags = flags;

  IMAGE_DIR = image_dir;

  nesting_parser = Parser.HTML();
  nesting_parser->_set_tag_callback(fix_tag_nesting);

  parser = Parser.HTML();

  Parser.HTML p = Parser.HTML();

  parser->add_containers( ([ "pre":tag_preserve_ws,
			     "table":tag_preserve_ws,
			     "ul":tag_preserve_ws,
			     "ol":tag_preserve_ws,
  ]) );

  parser->add_container("li",
    lambda(Parser.HTML p, mapping m, string c) {
      return "<group><item/><text>" + c + "</text></group>";
    });

  parser->add_container("illustration",
    lambda(Parser.HTML p, mapping args, string c, string where)
    {
      c = replace(c, ([ "&gt;":">", "&lt;":"<" ]));
      string name;
      sscanf(where, "file='%s'", name);
      name = (name/"/")[-1];
      [program ip, string code] = try_compile_illustration(makepic, c, where);

      if (mixed err = catch {
	  if (ip) {
	    object g = ip(name+(illustration_counter++), verbosity, args->type);
	    return ({ g->make() });
	  }
	}) {
	werror("Execution of illustration at %O failed:\n"
	       "******\n", where);
	array(string) rows = code/"\n";
	for(int i; i<sizeof(rows); i++)
	  werror("%04d: %s\n", i+1, rows[i]);
	werror("******\n");
	if (!(flags & .FLAG_KEEP_GOING)) {
	  throw(err);
	}
	werror("%s\n", describe_backtrace(err));
      }
      return ({ "" });	// FIXME: Broken image?
    });

  parser->add_container("execute",
    lambda(Parser.HTML p, mapping args, string c, string where)
    {
      c = replace(c, ([ "&gt;":">", "&lt;":"<", "&amp;":"&",
			// NB: src/modules/Image/layers.c had absolute paths...
			"/home/mirar/pike7/tutorial/":IMAGE_DIR,
		  ]));
      // Repair some known typos in old versions.
      c = replace(c, ({
		    // src/modules/Image/layers.c:1.33
		    // Parenthesis/brace matching error.
		    // The corresponding code is just longdesc at HEAD.
		    "replace(longdesc,({\",\",\";\",\")\",({\",<wbr />\",\";<wbr />\",\")<wbr />\"}))))/",
		    "\"<wbr />\"/1*({mktag(\"wbr\")})));",
		    // src/modules/Image/layers.c:1.36
		    // longdesc still needs to be quoted.
		    "mktag(\"td\",([\"align\":\"left\",\"valign\":\"center\"]),longdesc)",
		  }), ({
		    "replace(longdesc, ({ \"<\",\">\",\"&\" }), ({\"&lt;\",\"&gt;\",\"&amp;\"}))",
		    "));",
		    "mktag(\"td\",([\"align\":\"left\",\"valign\":\"center\"]),replace(longdesc, ({ \"<\",\">\",\"&\" }), ({\"&lt;\",\"&gt;\",\"&amp;\"})))",
		  }));
      string name;
      sscanf(where, "file='%s'", name);
      name = (name/"/")[-1];
      array err;
      object g;

      err = catch {
	g = compile_string(execute + c)
	  (illustration_counter, name);
	g->main();
      };

      if(err) {
	werror("%O\n", where);
	array rows = (execute+c)/"\n";
	werror("******\n");
	for(int i; i<sizeof(rows); i++)
	  werror("%04d: %s\n", i+1, rows[i]);
	werror("******\n");
	throw(err);
      }

      illustration_counter = g->img_counter;

      if (flags & .FLAG_COMPAT) {
	// NB: We may need to repair the output...
	nesting = ADT.Stack();
	nesting->push(0);	// End sentinel.
	nesting_parser->set_extra(where);
	return nesting_parser->finish(g->write->get())->read() +
	  pop_to_tag(UNDEFINED) * "";
      }
      return g->write->get();
    });

  parser->add_container("data_description",
    lambda(Parser.HTML p, mapping args, string c)
    {
      if(args->type=="mapping") {
	Parser.HTML i = Parser.HTML()->
	  add_container("elem",
	    lambda(Parser.HTML p, mapping args, string c)
	    {
	      if(!args->type)
		throw("mkxml: Type attribute missing on elem tag.");
	      if(args->type!="int" && args->type!="float" &&
		 args->type!="string")
		throw("mkxml: Unknown type "+args->type+" in elem type attribute.\n");
	      if(!args->name)
		throw("mkxml: Name attribute missing on elem tag.");
	      return "<group>\n<member><type><" + args->type +
		"/></type><index>\"" + args->name + "\"</index></member>\n"
		"<text><p>" + c + "</p></text>\n</group>\n";
	    });
	return ({ "<mapping>\n " +
		  safe_newlines(i->finish(c)->read()) +
		  "</mapping>\n " });
      }
      throw("mkxml: Unknown data_description type "+args->type+".\n");
    });

  parser->add_tag("br",lambda(mixed...) { return ({"<br/>"}); });
  parser->add_tag("wbr",lambda(mixed...) { return ({"<br/>"}); });

  parser->add_tags( ([ "dl":tag_quote_args,
		       "dt":tag_quote_args,
		       "dd":tag_quote_args,
		       "tr":tag_quote_args,
		       "th":tag_quote_args,
		       "td":tag_quote_args,
		       "ref":tag_quote_args ]) );

  parser->add_container(
    "text",
    lambda(Parser.HTML p,mapping args, string c)
    {
      c=p->clone()->finish(c)->read();
      string res="<text><p>"+c+"</p></text>";
      string t;
      do
      {
	t=res;
	res=replace(res,"<p></p>","");
	res=replace(res,"<br/></p>","</p>");
      }
      while (t!=res);
      return ({res});
    });

  parser->add_container("link",
    lambda(Parser.HTML p, mapping args, string c)
    {
      return ({ sprintf("<ref%{ %s=\"%s\"%}>%s</ref>", (array)args, c) });
    });

  parser->add_container("a",
    lambda(Parser.HTML p, mapping args, string c)
    {
      return ({ sprintf("<url%{ %s=\"%s\"%}>%s</url>", (array)args, c) });
    });

  // Normalize IMAGE_DIR.
  IMAGE_DIR = combine_path(getcwd(), IMAGE_DIR);
  if (!sizeof(IMAGE_DIR)) IMAGE_DIR="./";
  else if (IMAGE_DIR[-1] != '/') IMAGE_DIR += "/";

  execute = #"
  class Interceptor {
    string buffer = \"\";

    void `()(string in) {
      buffer += in;
    }

    string get() {
      return buffer;
    }
  }

  Interceptor write = Interceptor();

  int img_counter;
  string prefix;
  void create(int _img_counter, string _prefix) {
    img_counter = _img_counter;
    prefix = _prefix;
  }

  string illustration(string|Image.Image img, mapping extra, void|string suffix) {
    string fn = prefix + \".\" + (img_counter++) + (suffix||\".png\");
#if constant(Image.PNG.encode)
    if(!stringp(img))
      img = Image.PNG.encode(img);
    Stdio.write_file(fn, img);"+
    ((verbosity > 1)?#"
    werror(\"Wrote %s from execute.\\n\", fn);":"") + #"
#endif
    return \"<image>\"+fn+\"</image>\";
  }

  string illustration_jpeg(Image.Image img, mapping|void extra) {
#if constant(Image.JPEG.encode)
    return illustration(Image.JPEG.encode(img, extra||([])), extra, \".jpeg\");
#else
    return illustration(img, extra);
#endif
  }

  string mktag(string name, void|mapping args, void|string c) {
    if(!args) args = ([]);
    if(!c)
      return sprintf(\"<%s%{ %s='%s'%} />\", name, (array)args);
    return sprintf(\"<%s%{ %s='%s'%}>%s</%s>\", name, (array)args, c, name);
  }

  array(string) tag_stack = ({});

  string begin_tag(string name, void|mapping args) {
    if(!args) args = ([]);
    tag_stack += ({ name });
    return sprintf(\"<%s%{ %s='%s'%}>\", name, (array)args);
  }

  string end_tag() {
    if(!sizeof(tag_stack)) throw( ({ \"Tag stack underflow.\\n\", backtrace() }) );
    string name = tag_stack[-1];
    tag_stack = tag_stack[..<1];
    return \"</\" + name + \">\";
  }

  string fix_image_path(string name) {
    return \"" + IMAGE_DIR + #"\" + name;
  }

  Image.Image load(string name) {
    if (!Stdio.exist(name)) name = fix_image_path(name);
    return Image.load(name);
  }

  Image.Layer load_layer(string name) {
    if (!Stdio.exist(name)) name = fix_image_path(name);
    return Image.load_layer(name);
  }

";

  nowM = parse;
}
