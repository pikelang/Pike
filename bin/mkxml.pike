/* $Id: mkxml.pike,v 1.6 2001/05/05 20:42:39 mirar Exp $ */

import Stdio;
import Array;

mapping parse=([]);
int illustration_counter;

mapping manpage_suffix=
([
   "Image":"i",
   "Image.image":"i",
]);


function verbose=werror;

#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

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
				"decl" : array of textlines of declarations
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
   if (-1==search(dest->_order,name)) dest->_order+=({name});
   return dest[name] || (dest[name]=(["_line":line]));
}

string stripws(string s)
{
   return desc_stripws(s);
}

string desc_stripws(string s)
{
   if (s=="") return s;
   array lines = s / "\n";
   int m=10000;
   foreach (lines,string s)
      if (s!="")
      {
	 sscanf(s,"%[ ]%s",string a,string b);
	 if (b!="") m=min(strlen(a),m);
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

void report(string s)
{
   verbose("mkwmml:   "+s+"\n");
}

#define complain(X) (X)

mapping keywords=
(["module":lambda(string arg,string line) 
	  { classM=descM=nowM=moduleM=focM(parse,stripws(arg),line); 
	    methodM=0; 
	    if (!nowM->classes) nowM->classes=(["_order":({})]); 
	    if (!nowM->modules) nowM->modules=(["_order":({})]); 
	    report("module "+arg); },
  "class":lambda(string arg,string line) 
	  { if (!moduleM) return complain("class w/o module");
	    descM=nowM=classM=focM(moduleM->classes,stripws(arg),line); 
	    methodM=0; report("class "+arg); },
  "submodule":lambda(string arg,string line) 
	  { if (!moduleM) return complain("submodule w/o module");
	    classM=descM=nowM=moduleM=focM(moduleM->modules,stripws(arg),line);
	    methodM=0;
	    if (!nowM->classes) nowM->classes=(["_order":({})]); 
	    if (!nowM->modules) nowM->modules=(["_order":({})]); 
	    report("submodule "+arg); },
  "method":lambda(string arg,string line)
	  { if (!classM) return complain("method w/o class");
	    if (!nowM || methodM!=nowM || methodM->desc || methodM->args || descM==methodM) 
	    { if (!classM->methods) classM->methods=({});
	      classM->methods+=({methodM=nowM=(["decl":({}),"_line":line])}); }
	    methodM->decl+=({stripws(arg)}); descM=0; },
  "inherits":lambda(string arg,string line)
	  { if (!nowM) return complain("inherits w/o class or module");
  	    if (nowM != classM) return complain("inherits outside class or module");
	    if (!classM->inherits) classM->inherits=({});
	    classM->inherits+=({stripws(arg)}); },

  "variable":lambda(string arg,string line)
	  {
	     if (!classM) return complain("variable w/o class");
	     if (!classM->variables) classM->variables=({});
	     classM->variables+=({descM=nowM=(["_line":line])}); 
	     nowM->decl=stripws(arg); 
	  },
  "constant":lambda(string arg,string line)
	  {
	     if (!classM) return complain("constant w/o class");
	     if (!classM->constants) classM->constants=({});
	     classM->constants+=({descM=nowM=(["_line":line])}); 
	     nowM->decl=stripws(arg); 
	  },

  "arg":lambda(string arg,string line)
	  {
	     if (!methodM) return complain("arg w/o method");
	     if (!methodM->args) methodM->args=({});
	       methodM->args+=({argM=nowM=(["args":({}),"_line":line])}); 
	     argM->args+=({arg}); descM=argM;
	  },
  "note":lambda(string arg,string line)
	  {
	     if (!lower_nowM()) 
	        return complain("note w/o method, class or module");
	     descM=nowM->note||(nowM->note=(["_line":line]));
	  },
  "added":lambda(string arg,string line)
	  {
	     if (!lower_nowM()) 
	        return complain("added in: w/o method, class or module");
	     descM=nowM->added||(nowM->added=(["_line":line]));
	  },
  "bugs":lambda(string arg,string line)
	  {
	     if (!lower_nowM()) 
	        return complain("bugs w/o method, class or module");
	     descM=nowM->bugs||(nowM->bugs=(["_line":line]));
	  },
  "see":lambda(string arg,string line)
	  {
	     if (arg[0..3]!="also")
	        return complain("see w/o 'also:'\n");
	     if (!lower_nowM()) 
	        return complain("see also w/o method, class or module");
	     sscanf(arg,"also%*[:]%s",arg);
	     nowM["see also"]=map(arg/",",stripws)-({""});
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
      res+="         "[(strlen(res)%8)..7];
   }
   return res+s;
}


object(File) make_file(string filename)
{
   stderr->write("creating "+filename+"...\n");
   if (file_size(filename)>0)
   {
      rm(filename+"~");
      mv(filename,filename+"~");
   }
   object f=File();
   if (!f->open(filename,"wtc"))
   {
      stderr->write("failed.");
      exit(1);
   }
   return f;
}

string synopsis_to_html(string s,mapping huh)
{
   string type,name,arg;
   s=replace(s,({"<",">"}),({"&lt;","&gt;"}));
   if (sscanf(s,"%s%*[ \t]%s(%s",type,name,arg)!=4)
   {
      sscanf(s,"%s(%s",name,arg),type="";
      werror(sprintf(huh->_line+": suspicios method %O\n",(s/"(")[0]));
   }
   if (arg[..1]==")(") name+="()",arg=arg[2..];

   if (!arg) arg="";

   return 
      type+" <b>"+name+"</b>("+
      replace(arg,({","," "}),({", ","\240"}));
}

string htmlify(string s) 
{
#define HTMLIFY(S) \
   (replace((S),({"&lt;","&gt;",">","&","\240"}),({"&lt;","&gt;","&gt;","&amp;","&nbsp;"})))

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
   if (what==prefix[strlen(prefix)-strlen(what)-2..strlen(prefix)-3])
   {
      q=prefix[0..strlen(prefix)-3];
   }
   else if (what==prefix[strlen(prefix)-strlen(what)-1..strlen(prefix)-2])
   {
      q=prefix[0..strlen(prefix)-2];
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

string fixdesc(string s,string prefix,string where)
{
   s=desc_stripws(s);

   string t,u,v,q;

   t=s; s="";
   while (sscanf(t,"%s<ref%s>%s</ref>%s",t,q,u,v)==4)
   {
      if (search(u,"<ref")!=-1)
      {
	 werror("warning: unclosed <ref>\n%O\n",s);
	 u=replace(u,"<ref","&lt;ref");
      }
      
      if (sscanf(q," to=%s",q))
	 s+=htmlify(t)+make_nice_reference(q,prefix,u);
      else
	 s+=htmlify(t)+make_nice_reference(u,prefix,u);
      t=v;
   }
   if (search(s,"<ref")!=-1)
   {
      werror("%O\n",s);
      error("buu\n");
   }

   s+=htmlify(t);

   t=s; s="";
   for (;;)
   {
      string a,b,c;
      if (sscanf(t,"%s<%s>%s",a,b,c)<3) break;
      
      if (b[..11]=="illustration" &&
	  sscanf(t,"%s<illustration%s>%s</illustration>%s",t,q,u,v)==4)
      {
	 s+=replace(t,"\n\n","\n\n<p>")+
	    "<illustration __from__='"+where+"' src=image_ill.pnm"+q+">\n"
	    +replace(u,"lena()","src")+"</illustration>";
	 t=v;
      }
      else if (b[..2]=="pre" &&
	  sscanf(t,"%s<pre%s>%s</pre>%s",t,q,u,v)==4)
      {
	 s+=replace(t,"\n\n","\n\n<p>")+
	    "<pre"+q+">\n"+u+"</pre>";
	 t=v;
      }
      else
      {
	 s+=replace(a,"\n\n","\n\n<p>")+"<"+b+">";
	 t=c;
      }
   }
   s+=replace(t,"\n\n","\n\n<p>");

   return s;
}


multiset(string) get_method_names(string *decls)
{
   string decl,name;
   multiset(string) names=(<>);
   foreach (decls,decl)
   {
      sscanf(decl,"%*s%*[\t ]%s%*[\t (]%*s",name);
      names[name]=1;
   }
   return names;
}

string *nice_order(string *arr)
{
   sort(map(arr,replace,({"_","`"}),({"ÿ","þ"})),
	arr);
   return arr;
}

string addprefix(string suffix,string prefix)
{
   return prefix+suffix;
}

array fix_dotstuff(array(string) in)
{
   if (!sizeof(in)) return ({});
   array(string) last;
   in=Array.map(in,replace,({"->",">","<"}),({".","&lt;","&gt;"}));
   last=in[0]/"."; 
   last=last[..sizeof(last)-2];
   int i;
   array res=in[..0];
   for (i=1; i<sizeof(in); i++)
   {
      array(string) z=in[i]/".";
      if (equal(z[..sizeof(z)-2],last))
	 res+=({"."+z[-1]});
      else
      {
	 last=z[..sizeof(z)-2];
	 res+=in[i..i];
      }
   }
   return res;
}

#define S(X) ("'"+(X)+"'") /* XML arg quote */

string doctype(string type,void|string indent)
{
   array(string) endparan(string in)
   {
      int i;
      int q=1;
      for (i=0; i<strlen(in); i++)
	 switch (in[i])
	 {
	    case '(': q++; break;
	    case ')': q--; if (!q) return ({in[..i-1],in[i+1..]}); break;
	 }
      return ({in,""});
   };
   string combine_or(string a,string b)
   {
      if (b[..3]=="<or>") b=b[4..strlen(b)-6];
      return "<or>"+a+b+"</or>";
   };
   array(string) paramlist(string in,string indent)
   {
      int i;
      int q=0;
      array res=({});
      for (i=0; i<strlen(in); i++)
	 switch (in[i])
	 {
	    case '(': q++; break;
	    case ')': q--; break;
	    case ':':
	    case ',':
	       if (!q) return ({doctype(in[..i-1],indent+"  ")})+
			  paramlist(in[i+1..],indent);
	 }
      return ({in});
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
	    z+=({"mixed","mixed"});
	 return nindent+"<mapping><indextype>"+z[0]+"</indextype>"+
	    nindent+"   <valuetype>"+z[1]+"</valuetype></mapping>";
      case "object":
	 return nindent+"<object>"+b+"</object>";
      case "function":
	 z=paramlist(b,nindent);
	 if (sizeof(z)<1)
	    werror("warning: confused function type: %O\n",type),
	    z+=({"mixed"});
	 return
	    nindent+
	    "<function>"+
	    map(z[..sizeof(z)-2],
		lambda(string s)
		{
		   return nindent+"  <argtype>"+s+"<argtype>";
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

void docdecl(string enttype,
	     string decl,
	     object f)
{
   string rv,name,params=0;
   sscanf(decl,"%s %s(%s",rv,name,params);

   f->write("<"+enttype+" name="+S(name)+">");
   
   if (params)
   {
      string paramlist(string in)
      {
	 int i;
	 int q=1;
	 array res=({});
	 string t;
	 if (in=="" || in[..0]==")") return "";
	 for (i=0; i<strlen(in); i++)
	    switch (in[i])
	    {
	       case '(': q++; break;
//  	       case ')': q--; if (q==-1) return ""; break;
	       case ')': q--; if (q) break;
	       case ':':
	       case ',':
		  array z=in[..i-1]/" "-({""});
		  if (sizeof(z)==1)
		     return "\n     <argument><type>"+doctype(z[0],"\n      ")+
			"</type></argument>"+
			paramlist(in[i+1..]);
		  else
		     return "\n     <argument name="+S(z[-1])+
			"><type>"+doctype(z[0..sizeof(z)-2]*"","\n      ")+
			"</type></argument>"+
			paramlist(in[i+1..]);
	    }
	 array z=in[..i-1]/" "-({""});
	 if (sizeof(z)==1)
	    return "\n     <argument><type>"+doctype(z[0])+
	       "</type></argument>";
	 else
	    return "\n     <argument name="+S(z[-1])+
	       "><type>"+doctype(z[0..sizeof(z)-2]*"");
      };

      f->write("\n   <returntype>"+doctype(rv,"\n      ")+"</returntype>\n"
	       "   <arguments>"+paramlist(params)+"\n   </arguments>\n");
   }
   else
   {
      f->write("<typevalue>"+doctype(rv)+"</typevalue>\n");
   }

   f->write("</"+enttype+">");
}

void document(string enttype,
	      mapping huh,string name,string prefix,
	      object f)
{
   string *names;

   if (huh->names)
      names=map(indices(huh->names),addprefix,name);
   else
      names=({name});

   verbose("mkwmml: "+name+" : "+names*","+"\n");

   array v=name/".";
   string canname=v[-1];
   sscanf(canname,"%s->",canname);
   sscanf(canname,"%s()",canname);

   switch (enttype)
   {
      case "class":
      case "module":
	 f->write("<"+enttype+" name="+S(canname)+">\n");
	 break;
      default:
	 f->write("<docgroup homogen-type="+S(enttype)+
		  " homogen-name="+S(canname)+">\n");

	 if (huh->decl)
	 {
	    foreach (arrayp(huh->decl)?huh->decl:({huh->decl}),string decl)
	    {
	       docdecl(enttype,decl,f);
	    }
	 }
	 else
	    foreach (names,string name)
	    {
	       f->write("<"+enttype+" name="+S(name)+">\n");
	       f->write("</"+enttype+">");
	    }
	 break;
   }
   f->write("<source-position " + huh->_line + "/>\n");

// [SYNTAX]

//     if (huh->decl)
//     {
//        f->write("<man_syntax>\n");

//        if (enttype=="function" ||
//  	  enttype=="method")
//  	 f->write(replace(htmlify(map(huh->decl,synopsis_to_html,huh)*
//  				  "<br>\n"),"\n","\n\t")+"\n");
//        else
//  	 f->write(huh->decl);

//        f->write("</man_syntax>\n\n");
//     }

// [DESCRIPTION]

   if (huh->desc || huh->returns || huh->bugs || huh->added ||
       huh["see_also"])
      f->write("<doc>\n");

   if (huh->desc)
   {
      f->write("<text>\n");

      if (huh->inherits)
      {
	 string s="";
	 foreach (huh->inherits,string what)
	    f->write("inherits "+make_nice_reference(what,prefix,what)+
		     "<br>\n");
	 f->write("<br>\n");
      }

      f->write(fixdesc(huh->desc,prefix,huh->_line)+"\n");
      f->write("</text>\n");
   }

// [ARGUMENTS]

#if 0
   if (huh->args)
   {
      string rarg="";
      f->write("<man_arguments>\n");
      mapping arg;
      foreach (huh->args, arg)
      {
	 if (arg->desc)
	 {
	    f->write("\t<aargdesc>\n"
		     +fixdesc(rarg+"\t\t<aarg>"
			      +arg->args*"</aarg>\n\t\t<aarg>"
			      +"</aarg>",prefix,arg->_line)
		     +"\n<adesc>"
		     +fixdesc(arg->desc,prefix,arg->_line)
		     +"</adesc></aargdesc>\n\n");
	    rarg="";
	 }
	 else
	 {
	    rarg+="\t\t<aarg>"
	       +arg->args*"</aarg>\n\t\t<aarg>"+
	       "</aarg>\n";
	 }
      }
      if (rarg!="") error("trailing args w/o desc on "+arg->_line+"\n");

      f->write("</man_arguments>\n\n");
   }
#endif

// [RETURN VALUE]

   if (huh->returns)
   {
      f->write("<group><returns/>\n");
      f->write(fixdesc(huh->returns,prefix,huh->_line)+"\n");
      f->write("</group>\n");
   }


// [NOTE]

   if (huh->note && huh->note->desc)
   {
      f->write("<group><note/>\n");
      f->write(fixdesc(huh->note->desc,prefix,huh->_line)+"\n");
      f->write("</group>\n");
   }

// [BUGS]

   if (huh->bugs && huh->bugs->desc)
   {
      f->write("<group><bugs/>\n");
      f->write(fixdesc(huh->bugs->desc,prefix,huh->_line)+"\n");
      f->write("</group>\n");
   }

// [ADDED]

   if (huh->added && huh->added->desc)
   {
      /* noop */
   }

// [SEE ALSO]

   if (huh["see also"])
   {
      f->write("<group><see_also/>\n");
      f->write(htmlify(huh["see also"]*", "));
      f->write("</group>\n");
   }

   if (huh->desc || huh->returns || huh->bugs || huh->added ||
       huh["see_also"])
      f->write("</doc>\n");

// ---childs----

   if (huh->constants)
   {
      foreach(huh->constants,mapping m)
      {
	 sscanf(m->decl,"%s %s",string type,string name);
	 sscanf(name,"%s=%s",name,string value);
	 document("constant",m,prefix+name,prefix+name+".",f);
      }
   }

   if (huh->variables)
   {
      foreach(huh->variables,mapping m)
      {
	 sscanf(m->decl,"%s %s",string type,string name);
	 if (!name) name=m->decl,type="mixed";
	 sscanf(name,"%s=%s",name,string value);
	 document("variable",m,prefix+name,prefix+name+".",f);
      }
   }

   if (huh->methods)
   {
      // postprocess methods to get names

      multiset(string) method_names=(<>);
      string *method_names_arr,method_name;
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
		  document("method",method,prefix,prefix,f);
		  method_names-=method->names;
	       }
	    if (method_names[method_name])
	       stderr->write("failed to find "+method_name+" again, wierd...\n");
	 }
   }

   if (huh->classes)
   {
      foreach(huh->classes->_order,string n)
      {
//  	 f->write("\n\n\n<section title=\""+prefix+n+"\">\n");
	 document("class",huh->classes[n],
		  prefix+n,prefix+n+"->",f);
//  	 f->write("</section title=\""+prefix+n+"\">\n");
      }
   }

   if (huh->modules)
   {
      foreach(huh->modules->_order,string n)
      {
	 document("module",huh->modules[n],
		  prefix+n,prefix+n+".",f);
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
	 f->write("</docgroup>\n\n");
	 break;
   }
}

void make_doc_files()
{
   stderr->write("modules: "+sort(indices(parse))*", "+"\n");
   
   foreach (sort(indices(parse)-({"_order"})),string module)
      document("module",parse[module],module,module+".",stdout);
}

int main(int ac,string *files)
{
   string s,t;
   int line;
   string *ss=({""});
   object f;

   string currentfile;

   nowM=parse;

   stderr->write("reading and parsing data...\n");

   files=files[1..];

   if (sizeof(files) && files[0]=="--nonverbose") 
      files=files[1..],verbose=lambda(){};

   stderr->write("mkwmml: reading files...\n");

   for (;;)
   {
      int i;
      int inpre=0;

      if (!f) 
      {
	 if (!sizeof(files)) break;
	 verbose("mkwmml: reading "+files[0]+"...\n");
	 f=File();
	 currentfile=files[0];
	 files=files[1..];
	 if (!f->open(currentfile,"r")) { f=0; continue; }
	 t=0;
	 ss=({""});
	 line=0;
      }

      if (sizeof(ss)<2)
      {
	 if (t=="") { f=0; continue; }
	 t=f->read(8192);
	 if (!t) 
	 {
	    werror("mkwmml: failed to read %O\n",currentfile);
	    f=0;
	    continue;
	 }
	 s=ss[0];
	 ss=t/"\n";
	 ss[0]=s+ss[0];
      }
      s=ss[0]; ss=ss[1..];

      s=getridoftabs(s);

      line++;
      if ((i=search(s,"**!"))!=-1 || (i=search(s,"//!"))!=-1)
      {
	 string kw,arg;

	 sscanf(s[i+3..],"%*[ \t]%[^: \t\n\r]%*[: \t]%s",kw,arg);
	 if (keywords[kw])
	 {
	    string err;
	    if ( (err=keywords[kw](arg,"file='"+currentfile+"' line='"+line+"'")) )
	    {
	       stderr->write("mkwmml: "+
			     currentfile+"file='"+currentfile+"' line="+line);
	       return 1;
	    }
	    inpre=0;
	 }
	 else if (s[i+3..]!="")
	 {
	    string d=s[i+3..];
//  	    sscanf(d,"%*[ \t]!%s",d);
//	    if (search(s,"$Id")!=-1) report("Id: "+d);
	    if (!descM) descM=methodM;
	    if (!descM)
	    {
	       stderr->write("mkwmml: "+
			     currentfile+" line "+line+
			     ": illegal description position\n");
	       return 1;
	    }
	    if (!descM->desc) descM->desc="";
	    else descM->desc+="\n";
	    d=getridoftabs(d);
	    descM->desc+=d;
	 }
	 else
	 {
	    if (!descM->desc) descM->desc="";
	    else descM->desc+="\n";
	 }
      }
   }

//   stderr->write(sprintf("%O",parse));

   stderr->write("mkwmml: making docs...\n\n");

   make_doc_files();

   return 0;
}
