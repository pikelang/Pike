#!/usr/local/bin/pike

// Parse BMML (Black Magic Markup Language) to HTML
// Written by Fredrik Hubinette, dark sourceror and inventor of BMML

#include <simulate.h>
#include <string.h>

multiset efuns = mklist(indices(all_efuns())) | (<"sscanf","gauge","catch">);
mapping pages = ([]);
mapping short_descs = ([]);
mapping keywords = ([]);
mapping subpages = ([]);

string new_path;
int writepages;
string docdir;
string prefix="";

/*
 * Make a 'header'
 */
string smallcaps(string foo)
{
  string *ret;
  ret=({"<b>"});
  foreach(explode(foo," "),foo)
  {
    ret+=({"<font size=+1>"+foo[0..0]+"</font><font size=-1>"+foo[1..0x7fffffff]+"</font>"});
  }
  return ret*" "+"</b>";
}

/*
 * convert original path to internal format
 */
string fippel_path(string path)
{
  sscanf(path,"./%s",path);
  path=replace(path,"/","_");
  if(path[strlen(path)-5..]==".bmml") path=path[..strlen(path)-6];
  if(path[strlen(path)-5..]!=".html") path+=".html";

  return docdir+path;
}

string implode3(string pre, string *stuff, string post)
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
    return replace(block,"\n","<br>\n");
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
      tmp[e]=implode3("<td> ",tmp[e]," </td>");

    return "<table border=0 cellpadding=0 cellspacing=0>\n"+
      implode3("<tr valign=top>",tmp,/* "<br>" */ "</tr>\n")+
	"</table>\n";
  }
}

string more_magic(string s, int quote)
{
  int e;
  string *tmp;
  int *ilevel=({0});
  string output="";
  string accumulator="";

  if(!quote && -1==search("\n"+s,"\n ") && -1==search(s,"\t"))
  {
    return s;
  }

#define FLUSH() do{ output+=even_more_magic(accumulator,ilevel[-1]); accumulator=""; }while(0)
#define POP() do{ output+="</dl>"; ilevel=ilevel[0..sizeof(ilevel)-2]; }while(0)

  tmp=s/"\n";
  for(e=0;e<sizeof(tmp);e++)
  {
    string spaces, rest;

    sscanf(tmp[e],"%[ ]%s",spaces,rest);
    if(strlen(spaces) > ilevel[-1])
    {
      FLUSH();
      output+="<dl><dt><dd>";
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

string magic(string s, int quote)
{
  string *ret;
  ret=({});

  foreach(s/"\n\n",s)
  {
    sscanf(s,"\t%s",s);
    s=replace(s,"\n\t","\n");
    ret += ({ more_magic(s, quote) });
  }

  return ret*"\n<p>";
}


/*
 * Magic to convert SYNTAX sections
 */
inherit Regexp : lastident;
inherit Regexp : megamagic;

string syntax_magic(string s)
{
  string *tmp;
  int e;

  while(tmp=megamagic::split(s))
  {
    s=tmp[0]+"<I>"+tmp[1]+"</I>"+tmp[2];
  }

  tmp=s/"\n";
  for(e=0;e<sizeof(tmp);e++)
  {
    string a,b;
    if(sscanf(tmp[e],"%s(%s",a,b) && strlen(b)>1 && b[-1]==';' && b[-2]==')')
    {
      string *tmp2;
      int d;
      tmp2=b[0..strlen(b)-3]/",";
      for(d=0;d<sizeof(tmp2);d++)
      {
	string *tmp3;
	if(tmp3=lastident::split(tmp2[d]))
	{
	  tmp2[d]=tmp3[0]+"<I>"+tmp3[1]+"</I>"+tmp3[2];
	}
      }
      tmp[e]=a+"("+tmp2*","+");";
    }
  }
  s=tmp*"\n";
  return "<tt>"+magic(s,1)+"</tt>";
}


/* HTML quoting / unquoting */
string *from=({"&","<",">"});
string *to=({"&amp;","&lt;","&gt;"});

string html_quote(string s)
{
  return replace(s,from,to);
}

string html_unquote(string s)
{
  return replace(s,to, from);
}

string url_quote(string s)
{
  return replace(s, 
		 ({" ","`","\"","%"}),
		 ({"%20","%60","%22","%37"}));
		 
}

string mkdocument(string s,string title)
{
  return ("<html>"+
	  "<title>"+
	  html_quote(title)+
	  "</title>"+
	  "<body bgcolor=\"#A0E0C0\" text=\"#000000\" link=blue vlink=purple>"+
	  s+
	  "</body>"+
	  "</html>");
}

string strip_prefix(string s)
{
  while(sscanf(s,"%*s__%s",s));
  return s;
}

string *my_sort(string *s)
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

list(string) indexes_done=(<>);
list(string) pages_done=(<>);

void done(string a)
{
  pages_done[a]=1;
}

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
      ret+="<li><a href="+pages[a]+">"+strip_prefix(a)+"</a>"+short(a)+"\n";
    
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
	
      ret+="<li><a href="+pages[a]+">"+strip_prefix(a)+"</a>"+short(a)+"\n";
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
      ret+="<li><a href="+pages[a]+">"+strip_prefix(a)+"</a>"+short(a)+"\n";
    }
    
    ret+="</ul>\n";
    break;
    
  case "other":
    head="<b>Other pages</b>\n";
    ret="<ul>\n";
//    perror(sprintf("all pages: %O\n",sort(m_indices(pages))));
//    perror(sprintf("pages done: %O\n",sort(m_indices(pages_done))));
    foreach(my_sort(m_indices(pages) - indices(pages_done) ),a)
    {
      if(a[0..4]=="index") continue;
      ret+="<li><a href="+pages[a]+">"+strip_prefix(a)+"</a>"+short(a)+"\n";
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
	ret+="<li><a href="+pages[a]+">"+strip_prefix(a)+"</a>"+short(a)+"\n";
      }else{
	if(writepages)
	  perror("Warning: no page for function: "+a+".\n");
      }
    }
    ret+="</ul>\n";
    break;
    
  default:
    if(!keywords[prefix+topic])
    {
      if(writepages)
	perror("Unknown keyword "+topic+".\n");
      return "";
    }

    head="<a name="+topic+">";
    head+="<b>"+capitalize(topic)+"</a>";
    head+=short(topic);
    head+="</b>\n";
    ret="<ul>\n";
    foreach(my_sort(keywords[prefix+topic]),a)
    {
      a=html_quote(a);
      ret+="<li><a href="+pages[a]+">"+strip_prefix(a)+"</a>"+ short(a) +"\n";
    }
    ret+="</ul></a>\n";
    break;
  }

  if(usehead) ret=head+ret;

  return ret;
}

/* Convert a page */
string convert_page(string path, string fname)
{
  string output, short;
  int headno;
  string cont, section, name, part;

  output="";

  cont=read_bytes(path);

//  perror("foo: "+path[strlen(path)-5..]+".\n");
  if(sscanf(cont,"NAME\n\t%s - %s\n",name,short))
  {
    int partno;
    string tmp;

    cont=html_quote(cont);
    path=fippel_path(path);

    short_descs[html_quote(name)]=short;
    pages[prefix+html_quote(name)]=path;
    if(sscanf(name,"/precompiled/%s",tmp))
      pages[prefix+html_quote(capitalize(tmp))]=path;


    string *parts=cont/"============================================================================\n";
    for(partno=0;partno<sizeof(parts);partno++)
    {
      string part_name="error";
      string *sections;
      string part;
      int section;

      part=parts[partno];
      if(!strlen(part)) continue;

      sections=part/"\n\n";
      
      /* Merge sections that does not have a header together */
      for(section=0;section<sizeof(sections);section++)
      {
	if(!strlen(sections[section]) ||
	   sections[section][0] < 'A' ||
	   sections[section][0] > 'Z')
	{
	  sections[section-1]+="\n\n"+sections[section];
	  sections=sections[0..section-1]+sections[section+1..0x7fffffff];
	  section--;
	}
      }

      for(headno=0;headno<sizeof(sections);headno++)
      {
	string type, rest;
	mixed a, b;
	sscanf(sections[headno],"%s\n%s",type,rest);

	switch(type)
	{
	case "NAME":
	  if(sscanf(rest,"\t%s - %s",part_name,b)!=2)
	    perror("Warning NAME section broken!\n");
	  rest="\t<tt>"+part_name+"</tt> - "+b;

	  if(partno)
	  {
	    subpages[prefix+fname+"-&gt;"+part_name]=path+"#"+part_name;
	  }

	case "RETURN VALUE":
	case "RETURN VALUES":
	case "DESCRIPTION":
	case "NOTA BENE":
	case "BUGS":
	  rest=magic(rest,0);
	  break;

	default:
	  perror("Warning: Unknown header on page "+path+": "+type+".\n");
	  rest=magic(rest,0);
	  break;

	case "KEYWORDS":
	  a=replace(rest,({"\n"," ","\t"}),({"","",""}))/",";
	  b=({});
	  foreach(a,a)
	  {
	    a=prefix+a;
	    keywords[a] = ( keywords[a] || ({}) ) | ({ prefix+name });
	    if(pages[a])
	    {
	      b+=({ "<a href="+pages[a]+">"+strip_prefix(a)+"</a>" });
	    }else{
	      b+=({ strip_prefix(a) });
	    }
	  }
	  rest=implode_nicely(b);
	  break;

	case "SEE ALSO":
	  rest=replace(rest,({"\n"," ","\t"}),({"","",""}));
	  a=rest/",";
	  b=({});
	  foreach(a,a)
	  {
	    string tmp;
	    tmp=a;
	    if(a[0]!='`' && a[0]!='/')
	      a=(a/"/")[-1];
	    a=prefix+a;
	    if(pages[a])
	    {
	      b+=({ "<a href="+pages[a]+">" + strip_prefix(a) + "</a>" });
	    }else if(subpages[a]){
	      b+=({ "<a href="+subpages[a]+">" + strip_prefix(a) + "</a>" });
	    }else if(subpages[fname+"-&gt;"+a]){
	      b+=({ "<a href="+subpages[name+"-&gt;"+a]+">" + strip_prefix(a) + "</a>" });
	    }else{
	      if(writepages)
		perror(path+": Warning, unlinked SEE ALSO: "+strip_prefix(a)+"\n");
	      b+=({ tmp });
	    }
	  }
	  rest=implode_nicely(b);
	  break;

	case "SYNTAX":
	case "SYNTAX EXAMPLE":
	  if(search(rest,name+"(")!=-1) efuns[name]=1;
	  rest=syntax_magic(rest);
	  break;

	case "EXAMPLES":
	case "EXAMPLE":
	case "DIRECTIVE":
	case "PREPROCESSOR DIRECTIVES":
	  rest="<tt>"+magic(rest,1)+"</tt>";
	  break;

	case "RELATED FUNCTIONS":
	  a=name;

	  sscanf(rest,"%*skeyword %[a-z/A-Z0-9]",a);
	  rest=mkindex(a, 0);
	}

	sections[headno]="<dt>"+
	  smallcaps(type)+
	    "<dd>\n"+rest+"\n<p>";
      }
      if(keywords[prefix+part_name])
      {
	sections+=({"<dt>"+
	  smallcaps("RELATED PAGES")+"\n"+
	    mkindex(part_name,0)+"<p>"});
      }
      parts[partno]="<dl>\n"+sections*"\n"+"\n</dl>\n";
      if(part_name)
      {
	parts[partno]="<a name="+url_quote(part_name)+">\n"+
	  parts[partno]+
	  "\n</a>\n";
      }
    }
    output=mkdocument(parts*"<hr noshade size=1>\n","Pike: "+name);
  }
  else if(path[strlen(path)-5..]==".bmml")
  {
    string *sections;
    string title;
    int section;

    pages[prefix+(path[..strlen(path)-6]/"/")[-1]]=fippel_path(path);

    cont=replace(cont,"$version",version());
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
	tmp=replace(tmp,"\n  ","\n");
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
	  tmp="<a href="+fippel_path(a)+">"+b+"</a>";
	  break;

	case "TAG":
	  pages[prefix+a]=fippel_path(path)+"#"+a;
	  done(a);
	  tmp="<a name="+a+">";
	  break;

	default:
	  perror("Unknown directive: "+pre+".\n");
	}

      }
      sections[section]=tmp;
    }
    cont=sections*"\n<p>\n";

    return mkdocument(cont, title || "Pike manual");
  }
  else if(path[strlen(path)-5..]==".html")
  {
    pages[prefix+(path[..strlen(path)-6]/"/")[-1]]=fippel_path(path);

    if(sscanf(cont,"<title>%s</title>",part))
      short_descs[(path/"/")[-1]]=part;
    output=cont;
  }
  else if(is_example::match(cont))
  {
    /** Hmm, this looks like an example file to me... */
    string line,tmp;
    int pre,p;

    pages[prefix+(path/"/")[-1]]=fippel_path(path);

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
	case '\t': p=1; sscanf(line,"\t",line); break;
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
	  case 0: output+=replace(tmp,"\n\n","\n<p>\n"); break;
	  }
	  pre=p;
	  tmp="";
	}
      }
      tmp+=line+"\n";
    }
    output=mkdocument(output,"Pike: "+
		      replace((fname/"/")[-1],"_"," "));
  }
  else
  {
    if(writepages)
    {
      string tmp;
      int l, i;
      
      foreach(cont/"\n", tmp)
      {
	if(is_example::match(tmp+"\n"))
	{
	  l++;
	}else{
	  i++;
	}
      }
      
      if(l > i*2)
      {
	int err;
	l=0;
	foreach(cont/"\n", tmp)
	{
	  l++;
	  if(!is_example::match(tmp+"\n"))
	  {
	    perror(path+":"+l+": not on example form.\n");
	    if(++err == 5) break;
	  }
	}
      }
      perror("Warning: not converting "+path+".\n");
    }
    output="";
  }
  return output;
}


void scanfiles(string path, string fname)
{
  string nf,np;
  nf=convert_page(path, fname);

  if(nf && strlen(nf) && writepages)
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
  perror("Doing "+path+"\n");
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

int main(int argc, string *argv)
{
  string np;
  int e;

  if(argc < 3)
  {
    perror("Usage: html_docs.pike to_path from_path [module_doc_path ...]\n");
    exit(0);
  }

//  perror(sprintf("argv = %O\n",argv));

  megamagic::create("^(.*)&lt;([a-z_0-9]+)&gt;(.*)$");
  lastident::create("^(.*[^<>a-z_0-9])([a-z_0-9]+)([^<>a-z_0-9]*)$");

#define BEGIN1 "[0-9]+(\\.[0-9]+)*(\\.|) "
#define BEGIN2 "\t"
#define BEGIN3 "  "
#define LEND "[^\n]*"
#define LINE "(" BEGIN1 LEND ")|(" BEGIN2 LEND ")|(" BEGIN3 LEND ")|()\n"

  is_example::create("^(" LINE ")+$");

  for(e=1;e<sizeof(argv);e++)
    argv[e]=combine_path(getcwd(),argv[e]);

  new_path=argv[1];

  write("Scanning pages for links and keywords.\n");
  writepages=0;
  for(e=2;e<sizeof(argv);e++) dodocs(argv[e],e-2);
  writepages=0;
  for(e=2;e<sizeof(argv);e++) dodocs(argv[e],e-2);

  foreach(indices(indexes_done),np)
    foreach(keywords[np] || ({}), np)
      done(np);


  write("Writing html files.\n");
  writepages=1;
  for(e=2;e<sizeof(argv);e++) dodocs(argv[e],e-2);
    
  foreach(indices(keywords) - indices(indexes_done),np)
  {
    perror("Keywords never indexed: "+np+"\n");
  }
}
