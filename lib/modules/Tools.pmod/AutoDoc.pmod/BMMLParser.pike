#!/usr/local/bin/pike
#pike 7.2

// Parse BMML (Black Magic Markup Language) to AutoDoc XML.
// Written by Fredrik Hubinette, dark sourceror and inventor of BMML.

#include <stdio.h>
#include <string.h>

multiset efuns = mkmultiset(indices(all_constants())) |
  (<"sscanf","gauge","catch">);
mapping short_descs = ([]);
mapping keywords = ([]);

string new_path;
string docdir;
string prefix="";

// Constants that are known to have been documented in BMML.
protected constant known_constants = (< "PI" >);

/*
 * convert original path to internal format
 */
string fippel_path(string path)
{
  sscanf(path,"./%s",path);
  path=predef::replace(path,"/","_");
  if(path[strlen(path)-5..]==".bmml") path=path[..strlen(path)-6];
  if(path[strlen(path)-5..]!=".html") path+=".html";

  return docdir+path;
}

string implode3(string pre, array(string) stuff, string post)
{
  return pre+ stuff * (post+pre) + post;
}

/*
 * Three step conversion process...
 */
string even_more_magic(string block, int indent)
{
  if(-1==search(block,"\t"))
  {
    return predef::replace(block,"\n","<br />\n");
  }else{
    int e,d;
    mixed tmp,tmp2;

    tmp=block/"\n";
    for(e=0;e<sizeof(tmp);e++)
    {
      tmp[e]=tmp[e]/"\t";
      if(sscanf(tmp[e][0],"%*[ ]%s",tmp2) && tmp2=="")
      {
	int q;
	for(q=e-1;q>0;q--)
	{
	  if(!tmp[q]) continue;

	  if(sizeof(tmp[q])>=sizeof(tmp[e]))
	  {
	    for(d=1;d<sizeof(tmp[e]);d++)
	      tmp[q][d]+=" "+tmp[e][d];

	    tmp[e]=0;
	  }
	  break;
	}
      }
    }
    tmp-=({0});

    for(e=0;e<sizeof(tmp);e++)
      tmp[e]=implode3("<c> ",tmp[e]," </c>");

    return "<matrix>\n"+
      implode3("<r>",tmp,/* "<br />" */ "</r>\n")+
	"</matrix>\n";
  }
}

// Handle indentation.
string more_magic(string s, int quote)
{
  int e;
  array(string) tmp;
  array(int) ilevel=({0});
  string output="";
  string accumulator="";

  if(!quote && -1==search("\n"+s,"\n ") && -1==search(s,"\t"))
  {
    return s;
  }

#define FLUSH() do{ output+=even_more_magic(accumulator,ilevel[-1]); accumulator=""; }while(0)
#define POP() do{ output+="</text></group></dl>"; ilevel=ilevel[0..sizeof(ilevel)-2]; }while(0)

  tmp=s/"\n";
  for(e=0;e<sizeof(tmp);e++)
  {
    string spaces, rest;

    sscanf(tmp[e],"%[ ]%s",spaces,rest);
    if(strlen(spaces) > ilevel[-1])
    {
      FLUSH();
      output+="<dl><group><text>";
      ilevel+=({ strlen(spaces) });
    }
    else if(strlen(spaces) < ilevel[-1])
    {
      FLUSH();
      while(strlen(spaces) < ilevel[-1] && strlen(spaces) <= ilevel[-2])
	POP();
    }
    accumulator+=rest+"\n";
  }
  
  FLUSH();

  while(sizeof(ilevel)>1)
    POP();

  return output;
}

//! Convert a block of tab-indented text to a set of paragraphs.
string magic(string s, int quote)
{
  array(string) ret;
  ret=({});

  foreach(s/"\n\n",s)
  {
    sscanf(s,"\t%s",s);
    s=predef::replace(s,"\n\t","\n");
    ret += ({ more_magic(s, quote) });
  }

  return "<p>" + ret*"\n</p>\n<p>" + "</p>\n";
}

/*
 * Magic to convert SYNTAX sections
 */
protected inherit Regexp : lastident;
protected inherit Regexp : megamagic;

string syntax_magic(string s)
{
  array(string) tmp;
  int e;

  while(tmp=megamagic::split(s))
  {
    s=tmp[0]+"<i>"+tmp[1]+"</i>"+tmp[2];
  }

  tmp=s/"\n";
  for(e=0;e<sizeof(tmp);e++)
  {
    string a,b;
    if(sscanf(tmp[e],"%s(%s",a,b) && strlen(b)>1 && b[-1]==';' && b[-2]==')')
    {
      array(string) tmp2;
      int d;
      tmp2=b[0..strlen(b)-3]/",";
      for(d=0;d<sizeof(tmp2);d++)
      {
	array(string) tmp3;
	if(tmp3=lastident::split(tmp2[d]))
	{
	  tmp2[d]=tmp3[0]+"<i>"+tmp3[1]+"</i>"+tmp3[2];
	}
      }
      tmp[e]=a+"("+tmp2*","+");";
    }
  }
  s=tmp*"\n";
  return "<tt>"+magic(s,1)+"</tt>";
}


/* HTML quoting / unquoting */
array(string) from=({"&","<",">"});
array(string) to=({"&amp;","&lt;","&gt;"});

string html_quote(string s)
{
  return predef::replace(s,from,to);
}

string html_unquote(string s)
{
  return predef::replace(s,to, from);
}

string url_quote(string s)
{
  return predef::replace(s, 
		 ({" ","`","\"","%"}),
		 ({"%20","%60","%22","%37"}));
		 
}

string mkdocument(string s, string title, array(string)|void root)
{
  string namespace = "predef";
  if (root && sizeof(root) && has_suffix(root[0], "::")) {
    namespace = root[0][..<2];
  }
  return string_to_utf8("<?xml version='1.0' encoding='utf-8'?>\n"
			"<autodoc>"
			"<namespace name='" + namespace + "'>" +
			s+
			"</namespace>"+
			"</autodoc>\n");
}

string strip_prefix(string s)
{
  while(sscanf(s,"%*s__%s",s));
  return s;
}

array(string) my_sort(array(string) s)
{
  s+=({});
  sort(map(s,strip_prefix),s);
  return s;
}

string short(string s)
{
  s=strip_prefix(s);
  return short_descs[s] ? " - "+short_descs[s] : "";
}


inherit Regexp:is_example;

multiset(string) indexes_done=(<>);
multiset(string) pages_done=(<>);

void done(string a)
{
  pages_done[a]=1;
}

#if 0
string mkindex(string topic, int usehead)
{
  string head;
  string a,ret;
  ret="";

  indexes_done[prefix+topic]=1;

  switch(topic)
  {
  case "pages":
    head="<b>All pages:</b>\n";
    ret="<ul>\n";
    foreach(my_sort(m_indices(pages)),a)
      ret+="<li><a href='"+pages[a]+"'>"+strip_prefix(a)+"</a>"+short(a)+
	"</li>\n";
    
    ret+="</ul>\n";
    break;
    
  case "programs":
    head="<b>Builtin programs:</b>\n";
    ret="<ul>\n";
    foreach(my_sort(m_indices(pages)),a)
    {
      if(a[0]!='/') continue;
      done(a);
      if(sscanf(a,"/precompiled/%s",string tmp))
	done(capitalize(tmp));
	
      ret+="<li><a href='"+pages[a]+"'>"+strip_prefix(a)+"</a>"+short(a)+
	"</li>\n";
    }
    
    ret+="</ul>\n";
    break;

  case "examples":
    head="<b>examples:</b>\n";
    ret="<ul>\n";
    foreach(my_sort(m_indices(pages)),a)
    {
      if(search(a,"example")==-1) continue;
      done(a);
      ret+="<li><a href='"+pages[a]+"'>"+strip_prefix(a)+"</a>"+short(a)+
	"</li>\n";
    }
    
    ret+="</ul>\n";
    break;
    
  case "other":
    head="<b>Other pages</b>\n";
    ret="<ul>\n";
//    werror("all pages: %O\n",sort(m_indices(pages)));
//    werror("pages done: %O\n",sort(m_indices(pages_done)));
    foreach(my_sort(m_indices(pages) - indices(pages_done) ),a)
    {
      if(a[0..4]=="index") continue;
      ret+="<li><a href='"+pages[a]+"'>"+strip_prefix(a)+"</a>"+short(a)+
	"</li>\n";
    }
    ret+="</ul>\n";
    break;
    
  case "efuns":
    head="<b>All builtin functions:</b>\n";
    ret="<ul>\n";
    efuns-=(<"lambda","switch">);
    foreach(my_sort(m_indices(efuns)),a)
    {
      a=html_quote(a);
      done(a);
      if(pages[a])
      {
	ret+="<li><a href='"+pages[a]+"'>"+strip_prefix(a)+"</a>"+short(a)+
	  "</li>\n";
      }else{
	werror("Warning: no page for function: "+a+".\n");
      }
    }
    ret+="</ul>\n";
    break;
    
  default:
    if(!keywords[prefix+topic])
    {
      werror("Unknown keyword "+topic+".\n");
      return "";
    }

    head="<b><a name='"+topic+"'>";
    head+=capitalize(topic)+"</a>";
    head+=short(topic);
    head+="</b>\n";
    ret="<ul>\n";
    foreach(my_sort(keywords[prefix+topic]),a)
    {
      a=html_quote(a);
      ret+="<li><a href='"+pages[a]+"'>"+strip_prefix(a)+"</a>"+ short(a) +
	"</li>\n";
    }
    ret+="</ul>\n";
    break;
  }

  if(usehead) ret=head+ret;

  return ret;
}
#endif

/* Convert a page */
string convert_page(string path, string fname,
		    string|void cont, .Flags|void flags,
		    array(string)|void root)
{
  string output, short = "";
  int headno;
  string name;

  output="";

  cont = cont||read_bytes(path);

//  werror("foo: "+path[strlen(path)-5..]+".\n");
  if(sscanf(cont,"NAME\n\t%s - %s\n",name,short) ||
     // Both of the following are broken syntax.
     sscanf(cont,"NAME\t\n\t%s - %s\n",name,short) ||
     sscanf(cont,"NAME\n    %*[\t ]%s - %s\n",name,short))
  {
    cont=html_quote(cont);

    short_descs[html_quote(name)]=short;

    string header = "";
    string trailer = "";

    array(string) parts=cont/"============================================================================\n";

    array(string) module_path = name/".";

    if ((sizeof(parts) > 1) || (sizeof(module_path) > 1)) {
      if (name == "preprocessor") {
	root = ({ "cpp::" });
      } else {
	foreach(module_path; int i; string segment) {
	  string type = "module";
	  if (i == sizeof(module_path)-1) {
	    type = "class";
	  }
	  header += "<" + type + " name='" + html_quote(segment) + "'>\n";
	  trailer = "</" + type + ">\n" + trailer;
	}
      }
    } else if (known_constants[name]) {
      header += "<docgroup homogen-name='" + html_quote(name) +
	"' homogen-type='constant'>\n"
	"<constant name='" + html_quote(name) + "'>"
	"</constant>";
      trailer = "</docgroup>\n" + trailer;
    } else {
      header += "<docgroup homogen-name='" + html_quote(name) +
	"' homogen-type='method'>\n"
	"<method name='" + html_quote(name) + "'>" +
	// FIXME <returntype> & <arguments>.
	"</method>";
      trailer = "</docgroup>\n" + trailer;
    }

    for(int partno=0; partno<sizeof(parts); partno++)
    {
      string part_name="error";
      array(string) part_names;
      array(string) sections;
      string part;
      int section;
      array(string) section_path;
      string symbol_type = "method";

      part=parts[partno];
      if(!strlen(part)) continue;

      sections=part/"\n\n";
      
      /* Merge sections that do not have a header together */
      for(section=0;section<sizeof(sections);section++)
      {
	string section_header = "";
	if (has_prefix(sections[section], "\n"))
	  sections[section] = sections[section][1..];
	sscanf(sections[section], "%s\n", section_header);
	if(!strlen(String.trim_all_whites(section_header)) ||
	   upper_case(section_header) != section_header ||
	   lower_case(section_header) == section_header)
	{
	  sections[section-1]+="\n\n"+sections[section];
	  sections=sections[0..section-1]+sections[section+1..0x7fffffff];
	  section--;
	}
      }

      string term_prev = "";
      for(headno=0;headno<sizeof(sections);headno++)
      {
	string type, rest;
	mixed a, b;
	sscanf(sections[headno],"%s\n%s",type,rest);

	type = type && String.trim_all_whites(type);

	switch(type)
	{
	case "NAME\t":
	case "NAME":
	  if((sscanf(rest, "\t%s - %s", part_name, b)!=2) &&
	     (sscanf(rest,"    %*[\t ]%s - %s", part_name, b) != 3))
	    werror("Warning NAME section broken!\n");

	  section_path = module_path + part_name/".";
	  if (!partno) {
	    module_path = section_path;
	  }
	  
	  rest="\t<tt>"+part_name+"</tt> - "+b;

	  part_names = ({ part_name });

	  // FALL_THROUGH
	case "RETURN VALUE":
	case "RETURN VALUES":
	case "DESCRIPTION":
	case "DESCRIPITON":	// Common typo.
	case "COPYRIGHT":
	case "NOTA BENE":
	case "WARNING":
	case "THANKS":
	case "BUGS":
	  rest=magic(rest, 0);
	  break;
	case "DIRECTIVE":
	  if((sscanf(rest, "\t%s", part_name) != 1) &&
	     (sscanf(rest,"    %*[\t ]%s", part_name) != 2))
	    werror("Warning DIRECTIVE section broken!\n");

	  symbol_type = "directive";
	  part_names = map(part_name/"\n", String.trim_all_whites);

	  rest = "";
	  break;
	case "NOTES":
	case "NOTES, TODOs AND OTHER THINGS":
	  type = "NOTES";
	  rest=magic(rest, 0);
	  break;
	case "AUTHOR":
	  rest=magic("Author\n\n" + rest, 0);
	  break;

	default:
	  werror("Warning: Unknown section header on page %O: %O\n",
		 path, type);
	  rest=magic(type + "\n\n" + rest,0);
	  break;

	case "KEYWORD":
	case "KEYWORDS":
	  a=predef::replace(rest,({"\n"," ","\t"}),({"","",""}))/",";
	  b=({});
	  foreach(a,a)
	  {
	    a=prefix+a;
	    keywords[a] = ( keywords[a] || ({}) ) | ({ prefix+name });
	    b+=({ strip_prefix(a) });
	  }
	  rest=implode_nicely(b);
	  break;

	case "SEE ALSO":
	  rest=predef::replace(rest,({"\n"," ","\t"}),({"","",""}));
	  a=rest/",";
	  b = map(a, lambda(string tmp) {
		       string to = tmp;
		       foreach(({ "builtin/", "efun/", "files/", "math/",
				  "modules/math/", "modules/math", }),
			       string prefix) {
			 if (has_prefix(to, prefix)) {
			   to = "predef::" + to[sizeof(prefix)..];
			   break;
			 }
		       }
		       return "<ref to='" + to + "'>" + tmp + "</ref>";
		     });
	  rest = magic(implode_nicely(b), 0);
	  break;

	case "SYNTAX":
	case "SYNTAX EXAMPLE":
	case "SYNAX":	// Common typo.
	case "STRING":	// Common typo.
	  if(search(rest,name+"(")!=-1) efuns[name]=1;
	  rest=syntax_magic(rest);
	  break;

	case "EXAMPLES":
	case "EXAMPLE":
	case "PREPROCESSOR DIRECTIVES":
	  rest="<tt>"+magic(rest,1)+"</tt>";
	  break;

	case "RELATED FUNCTIONS":
#if 0
	  a=name;

	  sscanf(rest,"%*skeyword %[a-z/A-Z0-9]",a);
	  rest=mkindex(a, 0);
#endif
	  break;
	}

	type = ([
	  "RETURN VALUE":"<returns/>",
	  "RETURN VALUES":"<returns/>",
	  "NOTA BENE":"<note/>",
	  "WARNING":"<note/>",
	  "BUGS":"<bugs/>",
	  "SEE ALSO":"<seealso/>",
	  "EXAMPLE":"<example/>",
	  "EXAMPLES":"<example/>",
	  "THANKS":"<thanks/>",
	  "COPYRIGHT":"<copyright/>",
	])[type];

	if (type) {
	  // Start a new group with a section header.
	  if (headno) {
	    sections[headno-1] += term_prev;
	  }
	  sections[headno]="<group>" + type + "<text>\n" + rest;
	  term_prev = "</text></group>\n";
	} else {
	  // Continue the previous group (if any).
	  if (term_prev == "") {
	    rest = "<text>\n" + rest;
	    term_prev = "</text>\n";
	  }
	  sections[headno]=rest;
	}
      }
      if (sizeof(sections)) {
	sections[-1] += term_prev;
	parts[partno]="<doc placeholder='true'>\n"+sections*"\n"+"\n</doc>\n";
      } else {
	// Empty part!
	parts[partno] = "";
      }
      if(partno && part_names) {
	if (sizeof(part_names) == 1) {
	  parts[partno]="<docgroup homogen-name='" + part_name +
	    "' homogen-type='" + symbol_type + "'>\n"
	    "<" + symbol_type + " name='" + part_name + "'>\n" +
	    // FIXME <returntype> & <arguments>.
	    "</" + symbol_type + ">\n" + parts[partno] + "</docgroup>\n";
	} else {
	  // Multiple symbols documented at the same time.
	  // Currently only used by the preprocessor doc.
	  parts[partno]="<docgroup homogen-type='" + symbol_type + "'>\n"
	    "<" + symbol_type + " name='" + part_names *
	    ("'>\n" +
	     // FIXME <returntype> & <arguments>.
	     "</" + symbol_type + ">\n<" + symbol_type + " name='") + "'>\n" +
	    // FIXME <returntype> & <arguments>.
	    "</" + symbol_type + ">\n" + parts[partno] + "</docgroup>\n";
	}
      }
    }
    output = mkdocument(header + parts*"\n" + trailer,
			"Pike: " + name, root);
  }
  else if(path[strlen(path)-5..]==".bmml")
  {
    // Ignore these for now.
#if 0
    array(string) sections;
    string title;
    int section;

    cont=predef::replace(cont,"$version",version());
    cont=html_quote(cont);
    sections=cont/"\n\n";
    
    for(section=0;section<sizeof(sections);section++)
    {
      string tmp,pre,a,b;
      tmp=sections[section];
      sscanf(tmp,"%[\t ]",pre);
      
      switch(pre)
      {
      case "":
	title=tmp;
	tmp="<h1><center>"+tmp+"</center></h1>";
	break;
	
      case " ":
	sscanf(tmp," %s",tmp);
	tmp="<h2>"+tmp+"</h2>";
	break;
	
      case "  ":
	sscanf(tmp,"  %s",tmp);
	tmp=predef::replace(tmp,"\n  ","\n");
	tmp=more_magic(tmp,0);
	break;
	
      case "\t":
	sscanf(tmp,"\t%s %s",pre, a);
	switch(pre)
	{
	case "KEYWORD_INDEX":
	  sscanf(a,"%s\n",a);
	  tmp=mkindex(a, 1);
	  break;

	case "KEYWORD_LIST":
	  sscanf(a,"%s\n",a);
	  tmp=mkindex(a, 0);
	  break;

	case "LINK":
	  sscanf(a,"%s %s",a,b);
	  done((a/"/")[-1]);
	  tmp="<a href='"+fippel_path(a)+"'>"+b+"</a>";
	  break;

	case "TAG":
	  pages[prefix+a]=fippel_path(path)+"#"+a;
	  done(a);
	  tmp="<a name='"+a+"'>";
	  break;

	default:
	  werror("Unknown directive: "+pre+".\n");
	}

      }
      sections[section]=tmp;
    }
    cont="<doc placeholder='true'><text><p>" + sections*"\n</p>\n<p>\n" +
      "</p></text></doc>\n";

    return mkdocument(cont, title || "Pike manual", root);
#else
    return "";
#endif
  }
  else if(path[strlen(path)-5..]==".html")
  {
    // Ignore these for now.
#if 0
    string part;
    if(sscanf(cont,"<title>%s</title>",part))
      short_descs[(path/"/")[-1]]=part;
    output=cont;
#else
    return "";
#endif
  }
  else if(is_example::match(cont))
  {
    // Ignore these for now.
#if 0
    /** Hmm, this looks like an example file to me... */
    string part;
    string line,tmp;
    int pre,p;

    if(sscanf(cont,"%*[0-9.] %s\n",part)==2)
      short_descs[(path/"/")[-1]]=part;

    tmp="";
    pre=2;
    cont=html_quote(cont);
    foreach(cont/"\n"+({"."}),line)
    {
      if(strlen(line))
      {
	switch(line[0])
	{
	case ' ': p=0; sscanf(line,"  %s",line); break;
	case '\t': p=1; sscanf(line,"\t%s",line); break;
	default: p=2; break;
	}
	
	if(p!=pre)
	{
	  switch(pre)
	  {
	  case 2: output+="<h2>"+tmp+"</h2>"; break;
	  case 1:
	    if(tmp[-1]=='\n' && tmp[-2]=='\n')
	      tmp=tmp[..strlen(tmp)-2];
	    output+="<pre>\n"+tmp+"</pre>\n";
	    break;
	  case 0:
	    output+="<p>" + predef::replace(tmp,"\n\n","\n</p><p>\n") + "</p>\n";
	    break;
	  }
	  pre=p;
	  tmp="";
	}
      }
      tmp+=line+"\n";
    }
    output=mkdocument(output,"Pike: "+
		      predef::replace((fname/"/")[-1],"_"," "), root);
#else
    return "";
#endif
  }
  else
  {
    if ((flags & .FLAG_VERB_MASK) >= .FLAG_VERBOSE)
      werror("Warning: not converting "+path+".\n");
    output="";
  }
  return output;
}


void scanfiles(string path, string fname)
{
  string nf,np;
  nf=convert_page(path, fname);

  if(nf && strlen(nf))
  {
    np=combine_path(new_path,fippel_path(path));
//    write("Writing "+np+".\n");
    if(file_size(np)>=0)
      rm (np);
    write_file(np,nf);
  }
}

/** Traverse directory **/
void traversedir(string path)
{
  object tmp;
  string file,tmp2;
  string _prefix=prefix;


  tmp=clone(FILE);
  if(tmp->open(path+"/.bmmlrc","r"))
  {
    while(tmp2=tmp->gets())
    {
      string bar="";
      sscanf(tmp2,"%*[ \t]%s",tmp2);
      if(!strlen(tmp2) || tmp2[0]=='#') continue;
      sscanf(tmp2,"%s %s",tmp2,bar);
      switch(tmp2)
      {
      case "prefix":
	prefix+=bar+"__";
	break;
      }
    }
  }

  destruct(tmp);

  foreach(get_dir(path) - ({"CVS","RCS",".cvsignore",".bmmlrc"}),file)
  {
    string tmp;
    if(file[-1]=='~') continue;
    if(file[0]=='#' && file[-1]=='#') continue;
    if(file[0]=='.' && file[1]=='#') continue;

    tmp=path+"/"+file;

    if(file_size(tmp)==-2)
    {
      traversedir(tmp);
    }else{
      scanfiles(tmp,file);
    }
  }

  prefix=_prefix;
}

void dodocs(string path, int module)
{
  cd(path);
  werror("Doing "+path+"\n");
  if(!module)
  {
    docdir="";
  }else{
    docdir="module"+module;
    if(sscanf(reverse(path),"cod/%s",docdir))
    {
      sscanf(docdir,"%s/",docdir);
      docdir=reverse(docdir);
    }
    docdir+="_";
  }
  traversedir(".");
}

protected void create()
{
  // Set up the Regexps.

  megamagic::create("^(.*)&lt;([a-z_0-9]+)&gt;(.*)$");
  lastident::create("^(.*[^<>a-z_0-9])([a-z_0-9]+)([^<>a-z_0-9]*)$");

#define BEGIN1 "[0-9]+(\\.[0-9]+)*(\\.|) "
#define BEGIN2 "\t"
#define BEGIN3 "  "
#define LEND "[^\n]*"
#define LINE "(" BEGIN1 LEND ")|(" BEGIN2 LEND ")|(" BEGIN3 LEND ")|()\n"

  is_example::create("^(" LINE ")+$");
}

int main(int argc, array(string) argv)
{
  string np;
  int e;

  if(argc < 3)
  {
    werror("Usage: html_docs.pike to_path from_path [module_doc_path ...]\n");
    exit(0);
  }

//  werror("argv = %O\n",argv);

  for(e=1;e<sizeof(argv);e++)
    argv[e]=combine_path(getcwd(),argv[e]);

  new_path=argv[1];

  write("Scanning pages for links and keywords.\n");
  //writepages=0;
  for(e=2;e<sizeof(argv);e++) dodocs(argv[e],e-2);
  //writepages=0;
  for(e=2;e<sizeof(argv);e++) dodocs(argv[e],e-2);

  foreach(indices(indexes_done),np)
    foreach(keywords[np] || ({}), np)
      done(np);


  write("Writing html files.\n");
  //writepages=1;
  for(e=2;e<sizeof(argv);e++) dodocs(argv[e],e-2);
    
  foreach(indices(keywords) - indices(indexes_done),np)
  {
    werror("Keywords never indexed: "+np+"\n");
  }
}
