/* $Id: mkwmml.pike,v 1.1 1997/11/10 05:41:13 mirar Exp $ */

import Stdio;
import Array;

mapping parse=([]);
int illustration_counter;

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
		
*/

mapping moduleM, classM, methodM, argM, nowM, descM;

mapping focM(mapping dest,string name,int line)
{
   return dest[name] || (dest[name]=(["_line":line]));
}

string stripws(string s)
{
   sscanf(s,"%*[ \t\n\r]%s",s);
   s=reverse(s);
   sscanf(s,"%*[ \t\n\r]%s",s);
   return reverse(s);
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
   stderr->write(s+"\n");
}

#define complain(X) (X)

mapping keywords=
(["module":lambda(string arg,int line) 
	  { classM=descM=nowM=moduleM=focM(parse,stripws(arg),line); 
	    methodM=0; 
	    if (!nowM->classes) nowM->classes=([]); 
	    if (!nowM->modules) nowM->modules=([]); 
	    report("module "+arg); },
  "class":lambda(string arg,int line) 
	  { if (!moduleM) return complain("class w/o module");
	    descM=nowM=classM=focM(moduleM->classes,stripws(arg),line); 
	    methodM=0; report("class "+arg); },
  "submodule":lambda(string arg,int line) 
	  { if (!moduleM) return complain("submodule w/o module");
	    classM=descM=nowM=moduleM=focM(moduleM->modules,stripws(arg),line);
	    methodM=0;
	    if (!nowM->classes) nowM->classes=([]); 
	    if (!nowM->modules) nowM->modules=([]); 
	    report("submodule "+arg); },
  "method":lambda(string arg,int line)
	  { if (!classM) return complain("method w/o class");
	    if (!nowM || methodM!=nowM || methodM->desc || methodM->args || descM==methodM) 
	    { if (!classM->methods) classM->methods=({});
	      classM->methods+=({methodM=nowM=(["decl":({}),"_line":line])}); }
	    methodM->decl+=({stripws(arg)}); descM=0; },
  "arg":lambda(string arg,int line)
	  {
	     if (!methodM) return complain("arg w/o method");
	     if (!methodM->args) methodM->args=({});
	       methodM->args+=({argM=nowM=(["args":({}),"_line":line])}); 
	     argM->args+=({arg}); descM=argM;
	  },
  "note":lambda(string arg,int line)
	  {
	     if (!lower_nowM()) 
	        return complain("note w/o method, class or module");
	     descM=nowM->note||(nowM->note=(["_line":line]));
	  },
  "bugs":lambda(string arg,int line)
	  {
	     if (!lower_nowM()) 
	        return complain("bugs w/o method, class or module");
	     descM=nowM->bugs||(nowM->bugs=(["_line":line]));
	  },
  "see":lambda(string arg,int line)
	  {
	     if (arg[0..4]!="also:")
	        return complain("see w/o 'also:'\n");
	     if (!lower_nowM()) 
	        return complain("see also w/o method, class or module");
	     nowM["see also"]=map(arg[5..]/",",stripws);
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
   for (;;)
   {
      int i;
      if ((i=search(s,"\t"))==-1) return res+s;
      res+=s[0..i-1]; 
      s=s[i+1..];
      res+="         "[(strlen(res)%8)..7];
   }
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

string synopsis_to_html(string s)
{
   string type,name,arg;
   if (sscanf(s,"%s%*[ \t]%s(%s",type,name,arg)!=4)
      sscanf(s,"%s(%s",name,arg),type="";

   return 
      type+" <b>"+name+"</b>("+
      replace(arg,({","," "}),({", ","\240"}));
}

string htmlify(string s) 
{
#define HTMLIFY(S) \
   (replace((S),({"->","&","\240"}),({"-&gt;","&amp;","&nbsp;"})))

   string t="",u,v;
   while (sscanf(s,"%s<%s>%s",u,v,s)==3)
      t+=HTMLIFY(u)+"<"+v+">";
   return t+HTMLIFY(s);
}

#define linkify(S) (replace((S),({"->","()"}),({".",""})))

string make_nice_reference(string what,string prefix)
{
   string q;
   if (search(what,".")==-1 &&
       search(what,"->")==-1 &&
       !parse[what])
      q=prefix+what;
   else 
      q=what;

   return "<link to="+linkify(q)+">"+what+"</link>";
}

string fixdesc(string s,string prefix,string where)
{
   s=stripws(s);

   string t,u,v;

   t=s; s="";
   while (sscanf(t,"%s<ref>%s</ref>%s",t,u,v)==3)
   {
      s+=t+make_nice_reference(u,prefix);
      t=v;
   }
   s+=t;

   t=s; s="";
   while (sscanf(t,"%s<illustration>%s</illustration>%s",t,u,v)==3)
   {
      s+=htmlify(replace(t,"\n\n","\n\n<p>"));

      s+="<illustration __from__='"+where+"' src=lena.gif>\n"
	 +replace(u,"lena()","src")+"</illustration>";

      t=v;
   }
   s+=htmlify(replace(t,"\n\n","\n\n<p>"));

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


string addprefix(string suffix,string prefix)
{
   return prefix+suffix;
}

void document(mapping huh,string name,string prefix,object f)
{
   string *names;

   if (huh->names)
      names=map(indices(huh->names),addprefix,name);
   else
      names=({name});

   werror(name+" : "+names*","+"\n");

// ANCHOR

   f->write("<hr newpage>\n");
   
   foreach (names,string n)
      f->write("<anchor name="+linkify(n)+">\n");

   f->write("<dl>\n");

// NAME

   f->write("<dt><encaps>NAME</encaps><dd>\n");

   foreach (names,n)
      f->write("\t<tt>"+n+"</tt><br>\n");

   f->write("<p>\n");

// [SYNTAX]

   if (huh->decl)
   {
      f->write("<dt><encaps>SYNTAX</encaps><dd>\n");

      f->write(replace(htmlify(map(huh->decl,synopsis_to_html)*
			       "<br>\n"),"\n","\n\t")+"\n");

      f->write("<p>\n\n");
   }

// [DESCRIPTION]

   if (huh->desc)
   {
      f->write("<dt><encaps>DESCRIPTION</encaps><dd>\n");
      f->write(fixdesc(huh->desc,prefix,huh->_line)+"\n");
      f->write("<p>\n\n");
   }

// [ARGUMENTS]

   if (huh->args)
   {
      string rarg="";
      f->write("<dt><encaps>ARGUMENTS</encaps><dd>\n");
      
      f->write("\t<table border=1 cellspacing=0><tr>\n"
	       "\t<td align=left><font size=-2>argument(s)</font></td>"
	       "\t<td align=left><font size=-2>description</font></td>"
	       "</tr>\n\n");

      foreach (huh->args,mapping arg)
      {
	 if (arg->desc)
	 {
	    f->write("\t<tr align=left><td valign=top>\n"
		     +rarg+"\t\t<tt>"
 		     +arg->args*"</tt><br>\n\t\t<tt>"
		     +"</tt></td>\n"
		     +"\t<td valign=bottom>"
		     +fixdesc(arg->desc,prefix,arg->_line)
		     +"</td></tr>\n\n");
	    rarg="";
	 }
	 else
	 {
	    rarg+="\t\t<tt>"
	       +arg->args*"</tt><br>\n\t\t<tt>"+
	       "</tt><br>\n";
	 }
      }
      if (rarg!="") error("trailing args w/o desc on "+arg->_line+"\n");

      f->write("</table><p>\n\n");
   }

// [RETURN VALUE]

   if (huh->returns)
   {
      f->write("<dt><encaps>RETURN VALUE</encaps><dd>\n");
      f->write(fixdesc(huh->returns,prefix,huh->_line)+"\n");
      f->write("<p>\n\n");
   }


// [NOTE]

   if (huh->note && huh->note->desc)
   {
      f->write("<dt><encaps>NOTA BENE</encaps><dd>\n");
      f->write(fixdesc(huh->note->desc,prefix,huh->_line)+"\n");
      f->write("<p>\n\n");
   }

// [SEE ALSO]

   if (huh["see also"])
   {
      f->write("<dt><encaps>SEE ALSO</encaps><dd>\n");
      f->write(map(huh["see also"],
		    make_nice_reference,prefix)*",\n     "+"\n");
      f->write("<p>\n\n");
   }

   f->write("</dl>\n\n");

// ---childs----

   if (huh->methods)
   {
      // postprocess methods to get names

      multiset(string) method_names=(<>);
      string *method_names_arr,method_name;
      mapping method;

      if (huh->methods) 
	 foreach (huh->methods,method)
	    method_names|=(method->names=get_method_names(method->decl));

      method_names_arr=sort(indices(method_names));

      // alphabetically

      foreach (method_names_arr,method_name)
	 if (method_names[method_name])
	 {
	    // find it
	    foreach (huh->methods,method)
	       if ( method->names[method_name] )
	       {
		  document(method,prefix,prefix,f);
		  method_names-=method->names;
	       }
	    if (method_names[method_name])
	       stderr->write("failed to find "+method_name+" again, wierd...\n");
	 }

      foreach(huh->methods,mapping child)
      {
	 document(child,prefix,prefix,f);
      }
   }

   if (huh->classes)
   {
      foreach(indices(huh->classes),string n)
      {
	 f->write("\n\n\n<section title=\""+prefix+n+"\">\n");
	 document(huh->classes[n],
		  prefix+n,prefix+n+"->",f);
	 f->write("</section title=\""+prefix+n+"\">\n");
      }
   }

   if (huh->modules)
   {
      foreach(indices(huh->modules),string n)
      {
	 f->write("\n\n\n<section title=\""+prefix+n+"\">\n");
	 document(huh->modules[n],
		  prefix+n,prefix+n+".",f);
	 f->write("</section title=\""+prefix+n+"\">\n");
      }
   }
// end ANCHOR

   foreach (names,string n)
      f->write("</anchor name="+linkify(n)+">\n");
   f->write("\n\n\n");
}

void make_doc_files()
{
   stderr->write("modules: "+sort(indices(parse))*", "+"\n");
   
   foreach (sort(indices(parse)),string module)
      document(parse[module],module,module+".",stdout);
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

   for (;;)
   {
      int i;

      if (!f) 
      {
	 if (!sizeof(files)) break;
	 stderr->write("reading "+files[0]+"...\n");
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
	 s=ss[0];
	 ss=t/"\n";
	 ss[0]=s+ss[0];
      }
      s=ss[0]; ss=ss[1..];

      line++;
      if ((i=search(s,"**!"))!=-1)
      {
	 string kw,arg;

	 sscanf(s[i+3..],"%*[ \t]%s%*[ \t]%s",kw,arg);
	 if (keywords[kw])
	 {
	    string err;
	    if ( (err=keywords[kw](arg,currentfile+" line "+line)) )
	    {
	       stderr->write(currentfile+" line "+line+": "+err+"\n");
	       return 1;
	    }
	 }
	 else 
	 {
//	    if (search(s,"$Id")!=-1) report("Id: "+s);
	    if (!descM) descM=methodM;
	    if (!descM)
	    {
	       stderr->write("Error on line "+line+": illegal description position\n");
	       return 1;
	    }
	    if (!descM->desc) descM->desc="";
	    else descM->desc+="\n";
	    s=getridoftabs(s);
	    descM->desc+=s[search(s,"**!")+3..];
	 }
      }
   }

//   stderr->write(sprintf("%O",parse));

   stderr->write("making docs...\n\n");

   make_doc_files();

   return 0;
}
