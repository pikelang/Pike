import Stdio;
import Array;

mapping parse=([]);

/*

module : mapping <- moduleM
	"desc" : text
	"see also" : array of references 
	"note" : mapping of "desc": text
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

#define complain(X) (X)

mapping keywords=
(["module":lambda(string arg,int line) 
	  { nowM=moduleM=focM(parse,stripws(arg),line); classM=methodM=0; 
	    if (!nowM->classes) nowM->classes=([]); },
  "class":lambda(string arg,int line) 
	  { if (!moduleM) return complain("class w/o module");
	    nowM=classM=focM(moduleM->classes,stripws(arg),line); 
	    methodM=0; },
  "method":lambda(string arg,int line)
	  { if (!classM) return complain("method w/o class");
	    if (!nowM || methodM!=nowM || methodM->desc || methodM->args) 
	    { if (!classM->methods) classM->methods=({});
	      classM->methods+=({methodM=nowM=(["decl":({}),"_line":line])}); }
	    methodM->decl+=({stripws(arg)}); },
  "arg":lambda(string arg,int line)
	  {
	     if (!methodM) return complain("arg w/o method");
	     if (nowM!=argM || !argM)
	     { if (!methodM->args) methodM->args=({});
	       methodM->args+=({argM=nowM=(["args":({}),"_line":line])}); }
	     argM->args+=({arg});
	  },
  "note":lambda(string arg,int line)
	  {
	     if (!lower_nowM()) 
	        return complain("note w/o method, class or module");
	     nowM=nowM->note||(nowM->note=(["_line":line]));
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
	     nowM=0;
	     methodM->returns=stripws(arg);
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

string addprefix(string suffix,string prefix)
{
   return prefix+suffix;
}

string make_nice_reference(string refto,string my_prefix)
{
   return "<tt><a href=#>"+refto+"</a></tt>";
}

object(File) make_file(string filename)
{
   stdout->write("creating "+filename+"...\n");
   if (file_size(filename)>0)
   {
      rm(filename+"~");
      mv(filename,filename+"~");
   }
   object f=File();
   if (!f->open(filename,"wtc"))
   {
      stdout->write("failed.");
      exit(1);
   }
   return f;
}

string fixdesc(string s,string prefix)
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
   return replace(s,"\n\n","\n\n<p>");
}

string standard_doc(mapping info,string myprefix)
{
   string res="";

   if (info->desc && stripws(info->desc)!="")
      res+="\n\n<blockquote>\n"+info->desc+"\n</blockquote>\n";
   
   if (info->note)
      res+="\n\n<h4>NOTE</h4>\n<blockquote>\n"+
	 fixdesc(info->desc,myprefix)+"\n</blockquote>\n";
   
   if (info["see also"])
   {
      res+=
	 "\n\n<h4>SEE ALSO</h4>\n<blockquote>     " +
	 map(info["see also"],make_nice_reference,myprefix)*",\n     " +
	 "\n</blockquote>\n";
   }
   
   return res;
}

void make_an_index(string title,
		   string file,
		   mapping info,
		   string prefix,
		   string *refs)
{
   object f=make_file(file);
   f->write("<title>Pike documentation: "+title+"</title>\n"+
	    "<h2>"+title+"</h2>\n"+
	    standard_doc(info,prefix));
   if (sizeof(refs))
   {
      f->write("<h3>More documentation:</h3>\n <i><tt>" +
	       map(map(refs,addprefix,prefix),
		   make_nice_reference,prefix)*"</tt></i><br>\n <i><tt>" +
	       "</tt></i>\n\n");
   }
   f->close();
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

string synopsis_to_html(string s)
{
   string type,name,arg;
   if (sscanf(s,"%s%*[ \t]%s(%s",type,name,arg)!=4)
      sscanf(s,"%s(%s",name,arg),type="";

   return 
      type+" <b>"+name+"</b>("+
      replace(arg,({","," "}),({", ","&nbsp"}));
}

void document_method(object(File) f,
		     mapping method,
		     string prefix)
{
   stdout->write("documenting "+prefix+" methods "+
		 sort(indices(method->names))*", "+"...\n");
   
   f->write("\n<hr>\n"
	    // anchors goes here
	    "<h4>SYNOPSIS</h4>\n"
	    "<blockquote>\n"
	    "<tt>"+map(method->decl,synopsis_to_html)*
	              "<br>\n"+
	    "</tt>\n</blockquote>\n\n");

   if (method->desc && stripws(method->desc)!="")
   {
      f->write("<h4>DESCRIPTION</h4>\n"
	       "\n\n<blockquote>\n"+
	       fixdesc(method->desc,prefix)+
	       "\n</blockquote>\n");
   }

   if (method->args)
   {
      mapping arg;
      f->write("<h4>ARGUMENTS</h4>\n<blockquote><dl>\n");
      foreach (method->args,arg)
      {
	 f->write("<dt><tt>"+arg->args*"</tt>\n<dt><tt>"+
		  "</tt>\n  <dd>"+
		  fixdesc(arg->desc,prefix)+"\n");
      }
      f->write("</dl></blockquote>\n");
   }

   if (method->returns)
   {
      f->write("<h4>RETURNS</h4>\n"
	       "\n\n<blockquote>\n"+method->returns+"\n</blockquote>\n");
   }

   if (method->note)
   {
      f->write("\n\n<h4>NOTE</h4>\n<blockquote>\n"+
	       method->desc+"\n</blockquote>\n");
   }

   if (method["see also"])
   {
       f->write("\n\n<h4>SEE ALSO</h4>\n<blockquote>     " +
		map(method["see also"],
		    make_nice_reference,prefix)*",\n     " +
		"\n</blockquote>\n");
   }
}

void document_class(string title,
		    string file,
		    mapping info,
		    string prefix)
{
   stdout->write("documenting "+title+"...\n");

   object(File) f=make_file(file);

   f->write("<title>Pike documentation: "+title+"</title>\n"+
	    "<h2>"+title+"</h2>\n"+
	    standard_doc(info,prefix));

   // postprocess methods to get names

   multiset(string) method_names=(<>);
   string *method_names_arr,method_name;
   mapping method;

   foreach (info->methods,method)
      method_names|=(method->names=get_method_names(method->decl));

   method_names_arr=sort(indices(method_names));

   f->write("\n<hr>\n   <i><tt>"+
	    map(method_names_arr,make_nice_reference,prefix)*
	       "</tt></i><br>\n   <i><tt>"+"</tt></i><br>\n\n");
   // alphabetically

   foreach (method_names_arr,method_name)
      if (method_names[method_name])
      {
	 // find it
	 foreach (info->methods,method)
	    if ( method->names[method_name] )
	    {
	       document_method(f,method,prefix);
	       method_names-=method->names;
	    }
	 if (method_names[method_name])
	    stderr->write("failed to find "+method_name+" again, wierd...\n");
      }

   f->close();

}

void make_doc_files(string dir)
{
   stdout->write("modules: "+sort(indices(parse))*", "+"\n");

   string module,clas;
   
   foreach (sort(indices(parse)),module)
   {
      make_an_index("module "+module,
		    dir+module+".html", parse[module],
		    module+".", sort(indices(parse[module]->classes)));

      stdout->write("module "+module+" class(es): "+
		    sort(indices(parse[module]->classes))*", "+"\n");

      foreach (sort(indices(parse[module]->classes)),clas)
	 document_class(module+"."+clas,
			dir+module+"."+clas+".html",
			parse[module]->classes[clas],
			module+"."+clas);
   }
}

int main()
{
   stdout->write("reading data...\n");

   string doc=stdin->read(10000000);
   string s;
   int line;

   nowM=parse;

   stdout->write("parsing data...\n");

   foreach (doc/"\n",s)
   {
      int i;
      line++;
      if ((i=search(s,"**!"))!=-1)
      {
	 string kw,arg;
	 sscanf(s[i+3..],"%*[ \t]%s%*[ \t]%s",kw,arg);
	 if (keywords[kw])
	 {
	    string err;
	    if ( (err=keywords[kw](arg,line)) )
	    {
	       stderr->write("Error on line "+line+": "+err+"\n");
	       return 1;
	    }
	 }
	 else 
	 {
	    if (!nowM && !(nowM=descM) )
	    {
	       stderr->write("Error on line "+line+": illegal description position\n");
	       return 1;
	    }
	    if (!nowM->desc) nowM->desc="";
	    else nowM->desc+="\n";
	    s=getridoftabs(s);
	    nowM->desc+=s[search(s,"**!")+3..];
	    descM=nowM; nowM=0;
	 }
      }
   }

//   stderr->write(sprintf("%O",parse));

   stdout->write("making docs...\n\n");

   make_doc_files("doc/");
   return 0;
}
