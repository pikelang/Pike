#include "types.h"
import Sgml;

SGML low_make_concrete_wmml(SGML data);


static private int verify_any(SGML data, string in)
{
  int i=1;
  foreach(data,mixed x)
  {
    if(objectp(x))
    {
      if(strlen(x->tag) && x->tag[0]=='/')
      {
	werror("Unmatched "+x->tag+" near "+x->location()+"\n");
	werror(in);
	i=0;
	continue;
      }
      switch(x->tag)
      {
      default:
	werror("Unknown tag "+x->tag+" near "+x->location()+".\n");
	werror(in);
	i=0;
	break;

      case "font":
      case "firstpage":
      case "preface":
      case "introduction":

      case "chapter":
      case "appendix":
      case "i":
      case "b":
      case "a":
      case "anchor":
      case "tt":
      case "pre":
      case "tr":
      case "td":
      case "table":
      case "box":
      case "h1":
      case "h2":
      case "h3":
      case "dl":
      case "ul":
      case "section":
      case "center":
      case "ol":
      case "encaps":
      case "th":
      case "illustration":
      case "strong":
      case "link":

	case "man_nb":
	case "module":
	case "class":
	case "method":
	case "function":
	case "man_description":
	case "man_see":
	case "man_syntax":
	case "man_bugs":
	case "man_example":
	case "man_title":
	case "man_arguments":
	case "man_returns":
	case "man_note":

      case "ex_identifier":
      case "ex_keyword":
      case "ex_string":
      case "ex_comment":
      case "ex_meta":
      case "example":
	 
      case "aargdesc":
      case "aarg":
      case "adesc":
	if(!x->data)
	{
	  werror("Tag "+x->tag+" not closed near "+x->location()+".\n");
	  werror(in);
	  i=0;
	}

	break;

      case "ex_indent":
      case "ex_br":
      case "include":
      case "dt":
      case "dd":
      case "li":
      case "ref":
      case "hr":
      case "br":
      case "img":
      case "image":
      case "table-of-contents":
      case "index":
	if(x->data)
	{
	  werror("Tag "+x->tag+" should not be closed near "+x->location()+"\n");
	  werror(in);
	  i=0;
	}
      case "p":
      case "wbr":
      }

      if(x->data)
	if(!verify_any(x->data,"  In tag "+(x->tag=="anchor"?x->tag+" (name="+x->params->name+")":x->tag)+" near "+x->location()+"\n"+in))
	  i=0;
    }
  }
  return i;
}

int verify(SGML data)
{
  return verify_any(data,"");
}

INDEX_DATA collect_index(SGML data, void|INDEX_DATA index,void|mapping taken)
{
  if(!index) index=([]);
  if(!taken) taken=([]);

  foreach(data,TAG data)
  {
    if(objectp(data))
    {
      if(data->tag == "anchor" && !data->params->type)
      {
	if(string real_name=data->params->name)
	{
	  string new_name=real_name;
	  
	  if(taken[new_name])
	  {
	    int n=2;
//	    werror("Warning, duplicate "+real_name+" near "+data->location()+".\n");
	    while(taken[new_name+"_"+n]) n++;
	    new_name+="_"+n;
	  }
	  taken[new_name]++;
	  data->params->name=new_name;
	  
	  if(index[real_name])
	  {
	    index[real_name]+=({new_name});
	  }else{
	    index[real_name]=({new_name});
	  }
	}
      }
      if(data->data)
	collect_index(data->data,index,taken);
    }
  }
  return index;
}

INDEX newind;
INDEX_DATA index;

static private void build_index(string from, string fullname)
{
  mapping m=newind;

  if(string *to=index[fullname])
  {
    foreach(from/".",string tmp)
      {
	if(mapping m2=m[tmp])
	  m=m2;
	else
	  m=m[tmp]=([]);
      }
    if(!m[0]) m[0]=([]);
    m[0][fullname]=to;
  }
}

INDEX group_index(INDEX_DATA i)
{
  newind=([]);
  index=i;
  foreach(indices(index),string real_name)
  {
    string *to=index[real_name];
    build_index(real_name,real_name);
    string *from=real_name/".";
    for(int e=1;e<sizeof(from);e++)
      build_index(from[e], from[..e]*".");
  }
  index=0;
  return newind;
}

INDEX group_index_by_character(INDEX i)
{
  mapping m=([]);
  foreach(indices(i),string key)
    {
      int c;
      sscanf(lower_case(Html.unquote_param(key)),"%*[_ ]%c",c);
      string char=upper_case(sprintf("%c",c));
//      werror(char +" : "+key+"\n");
      if(!m[char]) m[char]=([]);
      m[char][key]=i[key];
    }
  return m;
}

string * reserved_pike =
({
  "array","break","case","catch","continue","default","do","else","float",
  "for","foreach","function","gauge","if","inherit","inline","int","lambda",
  "mapping","mixed","multiset","nomask","object","predef","private","program",
  "protected","public","return","sscanf","static","string","switch","typeof",
  "varargs","void","while"
});

string *reserved_c =
({
  "break","case","continue","default","do","else","float","double",
  "for","if","int","char","short","unsigned","long",
  "public","return","static","switch",
  "void","while"
});

object(Tag) parse_pike_code(string x,
	 int pos,
	 mapping(string:string) reserved_type)
{
  int p,e;
  int tabindented=-1;
  SGML ret=({""});

  while(e<strlen(x) && (x[e]=='\t' || x[e]=='\n' || x[e]==' ')) e++;
  while(e && (e>=strlen(x) || x[e]!='\n')) e--;

  for(;e<strlen(x);e++)
  {
    switch(x[e])
    {
    case '_':
    case 'a'..'z':
    case 'A'..'Z':
    {
      p=e;
      
      while(1)
      {
	switch(x[++e])
	{
	case '_':
	case 'a'..'z':
	case 'A'..'Z':
	case '0'..'9':
	  continue;
	}
	break;
      }
      
      string id=x[p..--e];
      ret+=({ Tag(reserved_type[id]||"ex_identifier",([]), pos+e, ({ id }) ) });
      break;
    }
    
    
    case '\'':
      p=e;
      while(x[++e]!='\'')
	if(x[e]=='\\')
	  e++;
      ret+=({ Tag("ex_string",([]), pos+e, ({ x[p..e]}) ) }); 
      break;
      
    case '"':
      p=e;
      while(x[++e]!='"')
	if(x[e]=='\\')
	  e++;
      
      ret+=({ Tag("ex_string",([]), pos+e, ({ x[p..e] }) ) });
      break;

    case '\n':
      if(tabindented!=-1)
	ret+=({ Tag("ex_br", ([]), pos+e) });

      if(tabindented)
      {
	for(int y=1;y<9;y++)
	{
	  switch(x[e+y..e+y])
	  {
	  case " ": continue;
	  case "\t": e+=y;
	    tabindented=1;
	  default:
	  }
	  break;
	}
	if(tabindented==-1) tabindented=0;
      }

      while(x[e+1..e+1]=="\t")
      {
	ret+=({
	  Tag("ex_indent", ([]), pos+e),
	  Tag("ex_indent", ([]), pos+e),
	  Tag("ex_indent", ([]), pos+e),
	  Tag("ex_indent", ([]), pos+e),
	    });
	e++;
      }

      while(x[e+1..e+2]=="  ")
      {
	ret+=({ Tag("ex_indent", ([]), pos+e) });
	e+=2;
      }
      break;

    case '/':
      if(x[e+1..e+1]=="/")
      {
	p=e++;
	while(x[e]!='\n') e++;
	e--;
	ret+=({ Tag("ex_comment",([]), pos+e, ({ x[p..e]}) ) }); 
	break;
      }
	
      if(x[e+1..e+1]=="*")
      {
	p=e++;
	while(x[e..e+1]!="*/") e++;
	e++;
	ret+=({ Tag("ex_comment",([]), pos+e, ({ x[p..e]}) ) }); 
	break;
      }
      
    default:
      ret[-1]+=x[e..e];
      continue;
    }
    ret+=({ "" });
  }

  return Tag("example",([]),pos,ret);
}

class Wmml
{
  SGML metadata;
  TOC toc;
  INDEX_DATA index_data;
  SGML data;
};

class Enumerator
{
  string *base;

  object push() { base+=({"1"}); return this_object(); }
  object pop()  { base=base[..sizeof(base)-2]; return this_object();  }
  object inc() 
  {
    switch(base[-1][0])
    {
      case '0'..'9':
	base[-1]=(string)( 1+ (int) base[-1]);
	break;

      default:
	base[-1]=sprintf("%c",base[-1][0]+1);
    }

    return this_object();
  }

  string query()
  {
    return base*".";
  }

  void create(mixed b)
  {
    if(objectp(b)) b=b->base;
    if(!arrayp(b)) b=({b});
    base=b;
  }
};

class Stacker
{
  inherit Enumerator;

  object push(string what) { base+=({what}); return this_object(); }
  object pop()  { base=base[..sizeof(base)-2]; return this_object();  }

  void create(void|mixed b)
  {
    ::create(b||({}));
  }
};

class TocBuilder
{
  import Sgml;

  TOC *stack=({ ({}) });

  void push() { stack+=({ ({}) });  }

  void pop(TAG t)
  {
    stack[-2]+=({
      Tag(t->tag+"_toc",
	  t->params+([]),
	  t->pos,
	  stack[-1],
	  t->file)
    });
    stack=stack[..sizeof(stack)-2];
  }

  TOC query() { return stack[0]; }
};


SGML metadata;
object(Enumerator) currentE;
object(Enumerator) appendixE;
object(Enumerator) chapterE;
object(Stacker) classbase;
object(TocBuilder) toker;

string name_to_link(string x)
{
   return replace(x,({"->","-&gt;"}),({".","."}));
}

string name_to_link_exp(string x)
{
   x=name_to_link(x);
   if (search(x,".")==-1) 
      x=classbase->query()+"."+x;
   return x;
}

SGML fix_anchors(TAG t)
{
  TAG ret=t;
  if(t->params->number)
  {
    ret=Tag("anchor",
	    (["name":t->params->number,"type":t->tag]),
	    t->pos,
	    ({ret}),
	    t->file);
  }

  if(t->params->name)
  {
    ret=Tag("anchor",
	    (["name":t->params->name,"type":t->tag]),
	    t->pos,
	    ({ret}),
	    t->file);
  }
  return ({ret});
}

SGML fix_section(TAG t, object(Enumerator) e)
{
  object(Enumerator) save=currentE;
  string num=e->query();
  e->push();
  currentE=e;

  toker->push();

  TAG ret=Tag(t->tag,
	      t->params+(["number":num]),
	      t->pos,
	      t->data=low_make_concrete_wmml(t->data),
	      t->file);
  toker->pop(ret);

  currentE->pop();
  currentE=save;

  return fix_anchors(ret);
}

SGML fix_class(TAG t, string name)
{
  classbase->push(name);
  TAG ret=Tag(t->tag,
	      t->params+(["name":classbase->query()]),
	      t->pos,
	      t->data=low_make_concrete_wmml(t->data),
	      t->file);

  SGML ret=
  ({
    Tag("anchor",
	(["name":classbase->query(),"type":t->tag]),
	t->pos,
	({t}),
	t->file)
      });
  classbase->pop();
  return ret;
}

SGML low_make_concrete_wmml(SGML data)
{
  if(!data) return 0;
  SGML ret=({});

  foreach(data, TAG tag)
  {
    if (stringp(tag))
    {
      ret+=({tag});
    }else if (objectp(tag)) {
      switch(tag->tag)
      {
	case "index":
	case "table-of-contents":
	  ret+=({
	    Tag("anchor",
		(["name":tag->params->name || tag->tag]),
		tag->pos,
		({
		  Tag(tag->tag,
		      tag->params,
		      tag->pos,
		      low_make_concrete_wmml(tag->data),
		      tag->file),
		    }),
		tag->file,
		)
	      });
	  toker->push();
	  toker->pop(tag);
	  continue;


	case "head":
	  metadata+=tag->data;
	  continue;
	  
	case "include":
	{
	  string filename=tag->params->file;
	  SGML tmp=group(lex(Stdio.read_file(filename),filename));
	  verify(tmp);
	  ret+=low_make_concrete_wmml(tmp);
	  continue;
	}
	  
	case "chapter":
	  ret+=fix_section(tag,chapterE);
	  chapterE->inc();
	  continue;

	case "section":
	  ret+=fix_section(tag,currentE);
	  currentE->inc();
	  continue;

	case "preface":
	  ret+=fix_section(tag,Enumerator("preface"));
	  continue;

	case "introduction":
	  ret+=fix_section(tag,Enumerator("introduction"));
	  continue;

	case "appendix":
	  ret+=fix_section(tag,appendixE);
	  appendixE->inc();
	  continue;
	  
	case "table":
	case "image":
	case "illustration":
	  if(tag->params->name)
	  {
	    TAG t=Tag(tag->tag,
		      tag->params,
		      tag->pos,
		      tag->data=low_make_concrete_wmml(tag->data),
		      tag->file);
	    ret+=({
	      Tag("anchor",
		  (["name":tag->params->name,"type":tag->tag]),
		  tag->pos,
		  ({ t }))
		});
	    continue;
	  }
	  break;
	  
	case "class":
	case "module":
	{
	   string name=tag->params->name;
	   sscanf(name,classbase->query()+"%*[.->]%s",name);
	   ret+=fix_class(tag, name);
	   continue;
	}
	
	case "man_syntax":
	case "man_example":
	case "man_nb":
	case "man_bugs":
	case "man_description":
	case "man_see":

	case "man_arguments":
	case "man_returns":
	case "man_note":
	{
	  string title=tag->tag;
	  SGML args=tag->data;
	  sscanf(title,"man_%s",title);
	  switch(title)
	  {
	    case "nb": title="nota bene"; break;
	    case "syntax":
	    case "example":
	      args=({Tag("tt",([]),tag->pos,tag->data)});
	      break;

	    case "see":
	    {
	      title="see also";
	      SGML tmp=({});
	      foreach(replace(get_text(args),({" ","\n"}),({"",""}))/",",string name)
		{
		  tmp+=({
		    Tag("link",(["to":
				 (tag->params->exp
				  ?name_to_link_exp
				  :name_to_link)(name)]),
			tag->pos,
			({
			  Tag("tt",([]),tag->pos,({name})),
			    })),
		      ", "
			});
		}
	      
	      tmp[-1]="";
	      if(sizeof(tmp)>3) tmp[-3]=" and ";
	      
	      args=tmp;
	      break;
	    }
	    case "arguments":
	       // insert argument parsing here 
	       // format:
	       // <aargdesc><aarg>int foo</aarg><aarg>int bar</aarg>...
               //         ...<adesc>description</adesc></aargdesc>
	       // 'desc' is formatted description text.
	       // 'arg' may contain <alink to...> tags

	       args=({Tag("arguments",([]),tag->pos,tag->data)});

	       break;
	  }
	  title=upper_case(title);
	  ret+=low_make_concrete_wmml(
	    ({
	    Tag("man_title",(["title":title]),tag->pos,args),
	      }));
	  continue;
	}

	case "method":
	case "function":
	{
	   array anchors=({}),fullnames=({});

	   foreach (replace(tag->params->name,"&gt;",">")/",",
		    string name)
	   {
	      string fullname;
	      if (name[0..strlen(classbase->query())-1]==
		  classbase->query() ||
		  tag->params->fullpath)
		 fullname=name;
	      else switch(tag->tag)
	      {
		 case "method":
		    fullname=classbase->query()+"->"+name;
		    break;
		 case "function":
		    fullname=classbase->query()+"."+name;
		    break;
	      }
	      anchors+=({name_to_link(fullname)});
	      fullnames+=({fullname});
	   }
	   array res=
		    ({Tag("dl",([]),tag->pos,
			  ({Tag("man_title",(["title":upper_case(tag->tag)]),
				tag->pos,
				Array.map(
				   fullnames,
				  lambda(string name,int pos)
				  { return ({Tag("tt",([]),pos,({name}))}); },
				   tag->pos)
				*({ ",", Tag("br") })
				+({ (tag->params->title
				     ?" - "+tag->params->title
				     :"")}))
			  }) + tag->data ) });
		   
	   foreach (anchors,string anchor)
	      res=({Tag("anchor",(["name":anchor]),
			tag->pos,res)});

	   ret+=
	      ({Tag(tag->tag,(["name":fullnames*",",
			       "title":tag->params->title]),tag->pos,
		    low_make_concrete_wmml(res))});
	   continue;
	}

	case "example":
	mapping reswords=([]);

	if(tag->params->meta)
	  foreach(tag->params->meta/",",string t)
	    reswords[t]="ex_meta";
	

	  switch(tag->params->language)
	  {
	    case "pike":
	      foreach(reserved_pike,string keyword)
		reswords[keyword]="ex_keyword";

	      ret+=({parse_pike_code(tag->data[0],
				     tag->pos,
				     reswords)});
	      continue;
	      
	    case "c":
	      foreach(reserved_c,string keyword)
		reswords[keyword]="ex_keyword";

	      ret+=({parse_pike_code(tag->data[0],
				      tag->pos,
				      reswords)});
	      continue;
	  }
      }
      ret+=({Tag(tag->tag,
		 tag->params,
		 tag->pos,
		 low_make_concrete_wmml(tag->data),
		 tag->file)});
    }
    else 
       throw(({"Tag or contents has illegal type: "+sprintf("%O\n",tag),
	       backtrace()}));
  }
  return ret;
}

object(Wmml) make_concrete_wmml(SGML data)
{
  classbase=Stacker();
  currentE=0;
  appendixE=Enumerator("A");
  chapterE=Enumerator("1");
  toker=TocBuilder();
  object(Wmml) ret= Wmml();
  metadata=({});
  ret->data=low_make_concrete_wmml(data);
  ret->index_data=collect_index(ret->data);
  ret->toc=toker->query();
  ret->metadata=metadata;
  return ret;
}

void save_image_cache();

int gifnum;
mapping gifcache=([]);
mapping srccache=([]);

string mkgif(mixed o)
{
  string g=stringp(o)?o:o->togif();

  int key=hash(g);

  foreach(gifcache[key]||({}),string file)
    {
      if(Stdio.read_file(file)==g)
      {
	werror("Cache hit in mkgif: "+file+"\n");
	return file;
      }
    }

  gifnum++;
  string gifname="illustration"+gifnum+".gif";
  rm(gifname);
  werror("Writing "+gifname+".\n");
  Stdio.write_file(gifname,g);

  if(gifcache[key])
    gifcache[key]+=({gifname});
  else
    gifcache[key]=({gifname});

  return gifname;
}


object render_illustration(string pike_code, mapping params, float dpi)
{
  werror("Rendering ");
  string src=params->src;
  object img=Image.image();

  if(params->dpi) dpi=(float)params->dpi;
  if(params->scale) dpi/=(float)params->scale;
  float scale=75.0/dpi;

  if(params->src) 
     img=srccache[params->src]||
	(srccache[params->src]=
	 img->fromppm(Process.popen("anytopnm 2>/dev/null "+src)));
  if(scale!=1.0) img=img->scale(scale);
  return compile_string("import Image;\n"
			"mixed `()(object src){ "+pike_code+" ; }")()(img);
}

private static string mkkey(mapping params, mixed ... other)
{
  params+=([]);
  m_delete(params,"align");
  m_delete(params,"alt");
  m_delete(params,"__from__");
  if(params->src)
    if(mixed x=file_stat(params->src))
      params->mtime=(string)(x[3]);
  string *keys=indices(params);
  string *values=values(params);
  sort(keys,values);

  return encode_value( ({keys,values,other}) );
}

mapping illustration_cache=([]);
string illustration_to_gif(TAG data, float dpi)
{
  mapping params=data->params;
  string pike_code=data->data[0];
  string key=mkkey(params,pike_code,dpi);

  string ret=illustration_cache[key];
  if(!ret)
  {
    mixed err=catch 
    {
       ret=mkgif(render_illustration(pike_code,params, dpi));
       illustration_cache[key]=ret;
       save_image_cache();
    };
    if (err)
    {
	werror("error while compiling and running\n"+pike_code+"\n");
	if (params->__from__) 
	   werror("from "+params->__from__+":\n");
	werror(master()->describe_backtrace(err)+"\n");
	return "failed to illustrate...";
    }
  }
  return ret;
}

string image_to_gif(TAG data, float dpi)
{
  mapping params=data->params;
  if(params->xfig)
    params->src=params->xfig+".fig";

  string key=mkkey(params,dpi);

  string ret=illustration_cache[key];
  if(!ret)
  {
    if(!params->src)
    {
      werror("Image without source near "+data->location()+".\n");
      return "";
    }
    string ext=reverse(params->src);
    sscanf(ext,"%s.",ext);
    switch(reverse(ext))
    {
    case "fig":
      werror("Converting ");
      Process.system("fig2dev -L ps "+params->src+" ___tmp.ps;echo showpage >>___tmp.ps");
      Process.system("gs -q -sDEVICE=pbmraw -r225 -g2500x2500 -sOutputFile=___tmp.ppm ___tmp.ps </dev/null >/dev/null");
      object o=Image.image()->fromppm(Stdio.read_file("___tmp.ppm"))->autocrop()->scale(1.0/3)->rotate(-90);
      o=Image.image(o->xsize()+40, o->ysize()+40, 255,255,255)->paste(o,20,20);
      rm("___tmp.ps");
      rm("___tmp.ppm");
      ret=mkgif(o);
      break;

    default:
      ret=mkgif(render_illustration("return src",params,dpi));
      break;
    }

    illustration_cache[key]=ret;
    save_image_cache();
  }
  return ret;
}

void save_image_cache()
{
  rm("illustration_cache");
  Stdio.write_file("illustration_cache",
		   encode_value(([
		     "gifnum":gifnum,
		     "gifcache":gifcache,
		     "illustration_cache":illustration_cache,
		     ])));
}

void create()
{
  if(file_stat("illustration_cache"))
  {
    mixed x=decode_value(Stdio.read_file("illustration_cache"));
    gifnum=x->gifnum;
    gifcache=x->gifcache;
    illustration_cache=x->illustration_cache;
  }
}

