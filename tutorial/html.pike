#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */
inherit Stdio.File : out;

SGML html_toc;
SGML html_index;
mapping(string:SGML) sections=([]);
mapping(string:string) link_to_page=([]);
mapping(string:TAG) link_to_data=([]);
string basename;
object files;

WMML global_data;

multiset exported=(<>);

void add_file_to_export_list(string f)
{
  if(!exported[f])
  {
    exported[f]=1;
    files->write(f+"\n");
  }
}

string mkfilename(string section)
{
  if(section=="") return basename+".html";
  return basename+"_"+section+".html";
}

string mklinkname(string section)
{
  string s=mkfilename(section);
  int q=sizeof(basename/"/");
  return (s/"/")[q-1..]*"/";
}


TAG mkimgtag(string file, mapping params)
{
  if(!file) return "<!-- error, no file -->";
  mapping p=([]);
  p->src=file;
  if(params->align) p->align=params->align;
  if(params->alt) p->align=params->alt;
  add_file_to_export_list(file);
  object o;
  // This could be optimized by using the Dims module /hubbe
  if (catch { o=Image.load(file); }) return "<!-- error -->";
  p->width=(string)o->xsize();
  p->height=(string)o->ysize();
  return Sgml.Tag("img",p,0);
}

string *srt(string *x)
{
  string *y=allocate(sizeof(x));
  for(int e=0;e<sizeof(y);e++)
  {
    y[e]=lower_case(x[e]);
    sscanf(y[e],"%*[ ,./_]%s",y[e]);
  }
  sort(y,x);
  return x;
}

int useful_tag(TAG t)
{
  return !stringp(t) || sscanf(t,"%*[ \n\r\t ]%*c")==2;
}

TAG get_tag(TAG t)
{
  while(1)
  {
    switch(t->tag)
    {
      default: return t;

      case "a":
      case "anchor":
    }
    if(!t->data) return t;

    SGML x=Array.filter(t->data,useful_tag);
    if(sizeof(x)==1 && objectp(t->data[0]))
      t=t->data[0];
    else
      break;
  }

  return t;
}

void add_target_to_links(SGML foo, string target)
{
  foreach(foo, TAG t)
    {
      if(objectp(t))
      {
	switch(t->tag)
	{
	case "link":
	case "ref":
	case "a":
	  if(!t->params->target)
	    t->params->target=target;
	}
	
	if(t->data)
	  add_target_to_links(t->data,target);
      }
    }
}

SGML low_toc_to_wmml(SGML|void toc)
{
  int app;
  SGML ret=({});

  foreach(toc, TAG t)
    {
      string name;
      string link;
      string title;

      switch(t->tag)
      {
      case "chapters_toc":
	ret+=low_toc_to_wmml(t->data);
	continue;

      case "index_toc":
      case "table-of-contents_toc":
      case "preface_toc":
      case "introduction_toc":
	name=0;
	title=t->params->title;
	sscanf(t->tag,"%s_toc",link);
	break;

      case "chapter_toc":
      case "section_toc":
	name=t->params->number;
	link=t->params->number;
	title=t->params->title;
	switch( (name/".")[0] )
	{
	case "introduction":
	case "preface":
	  name="";
	  break;
	}
	break;
	
      case "appendices_toc":
	name=0;
	link=0;
	title="Appendices";
	ret+=({Sgml.Tag("dt",([]),t->pos),
		 " "+title+"\n"})+
	  low_toc_to_wmml(t->data);
	continue;
	
      case "appendix_toc":
	name="Appendix "+t->params->number;
	link=t->params->number;
	title=t->params->title;
	break;

      default:
	werror("Error in generated TOC structure.\n");
      }
      
      mixed tmp=({ " "+title });
      if(name)
	tmp=({ Sgml.Tag("b",([]),t->pos,({name})) }) + tmp;

      if(link)
	tmp=({ Sgml.Tag("link",(["to":link]), t->pos, tmp) });

      ret+=({Sgml.Tag("dt",([]),t->pos) })+
	  tmp+
	    ({"\n"});

      if(t->data)
      {
	ret+=({
	  Sgml.Tag("dd",([]),t->pos),
	    Sgml.Tag("dl",([]),t->pos,
		     low_toc_to_wmml(t->data)),
	    "\n"
	    });
      }
    }
  return ret;
}

SGML toc_to_wmml(SGML toc)
{
  return ({
    Sgml.Tag("dl",([]),0, low_toc_to_wmml(toc))
	});
}

SGML low_index_to_wmml(INDEX data, string prefix)
{
  SGML ret=({});
  foreach(srt(indices(data)-({0})),string key)
    {
      ret+=({Sgml.Tag("dt")});
      
      if(data[key][0])
      {
	if(data[key][0][prefix+key])
	{
	  ret+=({
	    // FIXME: show all links
	    Sgml.Tag("link",(["to":data[key][0][prefix+key][0] ]),0, ({
	      Html.unquote_param(key),
		})),
	      "\n"
		});
	}else{
	  ret+=({Html.unquote_param(key)+"\n"});
	}

	foreach(srt(indices(data[key][0])), string key2)
	{
	  if(key2==prefix+key) continue;
	  ret+=({
	    Sgml.Tag("dd"),
	      // FIXME: show all links
	    Sgml.Tag("link",([ "to":data[key][0][key2][0] ]),0,
		     ({
		       Html.unquote_param(key2),
			 })),
	      "\n"
	  });
	}

      }else{
	ret+=({Html.unquote_param(key)+"\n"});
      }
	
      if(sizeof(data[key]) > !!data[key][0])
      {
	ret+=({
	    Sgml.Tag("dd"),
	      "\n",
	    Sgml.Tag("dl",([]),0,low_index_to_wmml(
	      data[key],
	      prefix+key+"."
	      )),
	      "\n",
	      });
      }
    }

  return ret;
}

int count_index_lines(SGML z)
{
  int ret;
  if(!z) return 0;
  foreach(z, TAG foo)
    {
      if(objectp(foo))
      {
	switch(foo->tag)
	{
	  case "h2": ret++;
	  case "dd":
	  case "dt": ret++;
	  default:   ret+=count_index_lines(foo->data);
	}
      }
    }
  return ret;
}

SGML index_to_wmml_low(INDEX data)
{
  SGML ret=({});
  foreach(srt(indices(data)-({0})),string key)
    {
      if(sizeof(data[key]) > !!data[key][0])
      {
	ret+=
	  ({
	  Sgml.Tag("dt"),
	    Sgml.Tag("h2",([]),0,({key})),
	      "\n",
	    Sgml.Tag("dd"),
	    Sgml.Tag("dl",([]),0,low_index_to_wmml(data[key],"")),
	      });
      }
    }
  return ret;
}

SGML index_to_wmml_onecol(INDEX data)
{
  SGML ret=index_to_wmml_low(data);
  return ({ Sgml.Tag("dl",([]),0,ret) });
}

string quote_string(string s)
{
  return replace(s, ({ "\\","\"" }),({"\\\\","\\\""}) );
}

string index_to_javascript(INDEX foo)
{
  string str="";
  str+="function gnapp() { }\n";
  str+="f=new gnapp();\n";
  foreach(sort(indices(foo)), string i)
    {
      if(string page=link_to_href(foo[i][0]))
      {
	str+=sprintf("f[\"%s\"]=\"%s\";\n",quote_string(i),quote_string(page));
      }
    }

  str+=#"

function do_search()
{
  text=document.forms.search.search.value;
  top.display.document.open(\"text/html\",\"replace\");
  top.display.document.write('<body ');
  top.display.document.write('bgcolor='+document.bgColor+' ');
  top.display.document.write('text='+document.fgColor+' ');
  top.display.document.write('link='+document.linkColor+' ');
  top.display.document.write('alink='+document.alinkColor+' ');
  top.display.document.write('vlink='+document.vlinkColor+' ');
  top.display.document.write('>\\n');

  top.display.document.write('<h2>Searching for '+text+'</h2>\\n');
  matches=0;

  for(a in f) {
    a+='';
    i=a.indexOf(text);
    if(i != -1 && (i==0 || a.charAt(i-1)=='.') && (i+text.length == a.length || a.charAt(i+text.length) == '.')) {
       found=f[a];
       top.display.document.write(\"<a href=\\\"\"+f[a]+\"\\\">\"+a+\"</a><br>\\n\");
       matches++;
    }
  }
  if(matches == 0)
    top.display.document.write('No matches found.');
  else
    top.display.document.write('Found '+matches+' matche(s).');

  top.display.document.write('</body>\\n');
  top.display.document.close();

  if(matches == 1)  top.display.location=found;
  return false;
}

document.write(\"<form method=PUT name=search onSubmit=\\\"return do_search()\\\">Index search: <input type=entry name=search><\form>\");

";

  return str;
}

SGML index_to_wmml(INDEX data)
{
  SGML ret=index_to_wmml_low(data);
  
#if 1
  int total_lines=count_index_lines(ret);
//  werror("\nTOTAL LINES: %d\n",total_lines);
  int lines,x;
  for(x=0;x<sizeof(ret);x+=2)
  {
    lines+=count_index_lines(ret[x..x+1]);
//    werror("LINES: %d\n",lines);
    if(lines*2>total_lines) break;
  }
  return ({
    Sgml.Tag("table",([]),0,
	     ({
	       Sgml.Tag("tr",(["valign":"top"]),0,
			({
			  Sgml.Tag("td",([]),0,
				   ({
				     Sgml.Tag("dl",([]),0, ret[..x-1])
				       })),
			  Sgml.Tag("td",([]),0,
				   ({
				     Sgml.Tag("dl",([]),0, ret[x..])
				       })),
			    })),
		 })),
	});
#else
  return ({
    Sgml.Tag("dl",([]),0, ret)
	});
#endif
}

string name_to_link(string x)
{
  return replace(x,"->",".");
}

int cpos;

SGML wmml_to_html(SGML data);

SGML preify(SGML in,int id)
{
   array r=({});
   foreach (in,TAG z)
      if (!stringp(z))
	 if (z->tag=="br")
	    r+=({"\n"+" "*id});
	 else
	 {
	    z->data=preify(z->data,id);
	    r+=({z});
	 }
      else 
	 r+=({replace(z,"\n","")});
   return r;
}

SGML data_description(mapping arg,int pos,array(object) data,
		      object orig)
{
   if (!data)
   {
      werror("Warning: empty data_description"
	     " (near "+Sgml.Tag("data_description",([]),pos)->location()+
	     ")\n");
      return ({});
   }

   array d=({}); 
   //    ({ type, name, value, pos, desc })
   // or ({ ?,    0,    0,     pos, group description })

   string type=arg->type||"",subtype="";
   sscanf(type,"%s(%s)",type,subtype);

   foreach (data,TAG data)
      if (!stringp(data))
	 switch (data->tag)
	 {
	    case "elem":
	       d+=({({data->params->type||subtype, /* 0 */
		      data->params->name,          /* 1 */
		      data->params->value,         /* 2 */
		      data->pos,                   /* 3 */
		      data->data})});              /* 4 */
	       break;
	    default:
	       werror("Warning: Found tag "+data->tag+" in data_description"
		      +" (near "+data->location()+")\n");
	 }
   switch (type)
   {
      case "array":
      {
	 SGML ret=({});
	 int ns,id,nl,ins,n=0;
	 ins=strlen(""+sizeof(column(d,2)-({0})))+2;
	 ns=max(@Array.map(d,
			   lambda(array z)
			   {
			      return strlen(z[0]||"")+strlen(z[2]||"");
			   }))+1;
	 if (ns>30) id=8,nl=1; else id=ns+5+ins,nl=0;
	 foreach (d,array t)
	    if (t[2])
	    {
	       ret+=({sprintf("   %-*s %*s ",
			      ns,t[0]+" "+t[2],ins,"["+n+"]")});
	       ret+=({Sgml.Tag("i",([]),t[3],
			       (nl?({"\n"+" "*id}):({}))+
			       preify(t[4],id)),"\n"});
	       n++;
	    }
	    else /* group title */
	       ret+=({"   ",Sgml.Tag("b",([]),t[3],preify(t[4],id)),"\n"});
	 return ({Sgml.Tag("pre",([]),pos,({"({\n"})+ret+({"})\n"}))});
      }

      case "mapping":
      {
	 SGML ret=({});
	 int ns,id,nl,ins;
	 ns=max(@Array.map(d,
			   lambda(array z)
			   {
			      if (z[0] && z[2])
				 return strlen(z[0])+
				    strlen(z[1])+
				    strlen(z[2])+1;
			      return strlen(z[0]||z[2]||"")+strlen(z[1]||"");
			   }))+5;
	 if (ns>30) id=8,nl=1; else id=ns+2,nl=0;
	 foreach (d,array t)
	    if (t[1])
	    {
	       if (t[0] && t[2]) 
		  ret+=({sprintf("   %-*s ",
				 ns,"\""+t[1]+"\" : "+t[0]+" "+t[2],ins)});
	       else
		  ret+=({sprintf("   %-*s ",
				 ns,"\""+t[1]+"\" : "+(t[2]||t[0]),ins)});
	       ret+=({Sgml.Tag("i",([]),t[3],
			       (nl?({"\n"+" "*id}):({}))+
			       preify(t[4],id)),"\n"});
	    }
	    else /* group title */
	       ret+=({"   ",Sgml.Tag("b",([]),t[3],preify(t[4],id)),"\n"});
	 return ({Sgml.Tag("pre",([]),pos,({"([\n"})+ret+({"])\n"}))});
      }
      default:
	 werror("Warning: Illegal/unimplemented data type %O"
		" (near "+orig->location()+")\n",type);
   }
   return ({});
}

string link_to_href(string to)
{
  if(!link_to_page[to])
  {
    string s=(to/".")[-1];
    if (link_to_page[s]) 
      to=s;
    else
      return 0;
  }
  return mklinkname(link_to_page[to])+"#"+
    replace(to,({"`",">","<","?"}),({"BQ-","GT","LT","QM"}));
}

/* Partially destructive! */
SGML convert(SGML data)
{
  if(!data) return 0;
  SGML ret=({});
  foreach(data,TAG data)
  {
    if(stringp(data))
    {
      ret+=({data});
    }
    else
    {
      if(!objectp(data))
      {
	error("Tag is neither string nor object: %O\n",data);
      }
      cpos=data->pos;
      switch(data->tag)
      {
	case "table":
	  data->data=convert(data->data);
	  if(data->params->nicer)
	  {
	    int foo;
	    data->params->cellpadding=(string) (
	      ((int) (data->params->cellspacing || 3))+
	      ((int) (data->params->cellpadding)) );

	    data->params->cellspacing="0";
	    if(!data->params->border)
	      data->params->border="0";

	    int max_rows;

	    foreach(data->data, TAG t)
	      {
		if(objectp(t) && t->tag=="tr")
		  {
		    int head;
		    foreach(t->data, TAG cell)
		      {
			if(objectp(cell) && cell->tag == "th")
			{
			  head=1;
			  break;
			}
		      }
		    
		    if(head)
		    {
		      t->params->bgcolor="#27215b";
		    }else{
		      t->params->bgcolor=(foo++&1) ? "#ffffff" : "#ddeeff";
		    }

		    SGML new_row=({});
		    foreach(t->data, TAG cell)
		      {
			if(objectp(cell))
			{
			  if(cell->tag == "th" || cell->tag=="td")
			  {
			    if(head)
			    {
			      if(!cell->params->align)
				cell->params->align="left";
			      cell->data=({
				Sgml.Tag("font",(["color":"#ffffff"]),0,
					 cell->data),
			      });
			    }else{
			      cell->data=({
				Sgml.Tag("font",(["color":"#000000"]),0,
					 cell->data),
			      });
			    }

			    // This adds some extra space, needed because
			    // we do not have any lines between columns
			    if(sizeof(new_row))
			      new_row+=({ Sgml.Tag("td",0,0,({"\240"})) });

			    new_row+=({cell});
			  }else{
			    werror("Tag within table row is not <th> or <td>, it is %s!\n",cell->tag);
			  }
			}
		      }
		    while(sizeof(new_row) < max_rows)
		      new_row+=({ Sgml.Tag("td",0,0,({"\240"})) });
		    if(sizeof(new_row) > max_rows)
		      max_rows=sizeof(new_row);
		    t->data=new_row;
		  }
	      }
	  }

	  if(data->params->framed || data->params->nicer)
	  {
	    m_delete(data->params,"nicer");
	    m_delete(data->params,"framed");
	    data=Sgml.Tag("table",(["border":"0",
				   "cellspacing":"0",
				   "cellpadding":"1",
				   "bgcolor":"#27215b",
				   ]),0,({
				     Sgml.Tag("tr",0,0,({
				       Sgml.Tag("td",0,0,({
					 data
				       }))
				     }))
				   }));
	  }
	  ret+=({data});
	  continue;

	 case "hr":
	    data->params->noshade=1;
	    data->params->size="1";
	    break;

	 case "man_title":
	    ret+=convert(({
	       Sgml.Tag("p"),
	       "\n",
	       Sgml.Tag("dt"),
	       Sgml.Tag("encaps",([]),data->pos, ({data->params->title})),
	       "\n",
	       Sgml.Tag("dd"),
	    })+data->data+ ({ "\n" }));
	    continue;

	 case "link":
	 {
	    data->tag="a";
	    string to=data->params->to;
	    m_delete(data->params,"to");
	    if(!(to=link_to_href(to)))
	    {
	      werror("Warning: Cannot find link "+to
		     +" (near "+data->location()+")\n");
	      data->tag="anchor";
	      break;
	    }
	    data->params->href=to;
	    break;
	 }
	
	 case "arguments":
	    ret+=convert(({
	       Sgml.Tag(
		  "table",(["border":"1","cellspacing":"0"]),data->pos,
		  ({Sgml.Tag(
		     "tr",([]),data->pos,
		     ({Sgml.Tag("td",(["align":"left"]),data->pos,
				({Sgml.Tag("font",(["size":"-2"]),
					   data->pos,({"argument(s)"}))})),
		       Sgml.Tag("td",(["align":"left"]),data->pos,
				({Sgml.Tag("font",(["size":"-2"]),
					   data->pos,({"description"}))}))}))})+
		  (data->data||({})))
	    }));
	    continue;

	 case "aargdesc":
	    ret+=convert(({Sgml.Tag("tr",([]),data->pos,
				    ({Sgml.Tag("td",(["valign":"top","align":"left"]))})+data->data)}));
	    continue;
	 case "adesc":
	    ret+=convert(({Sgml.Tag("td",(["valign":"top","align":"left"]),data->pos,data->data)}));
	    continue;
	 case "aarg":
	    ret+=convert(({Sgml.Tag("tt",([]),data->pos,data->data),
			   Sgml.Tag("br")}));
	    continue;

	 case "exercises":
	    ret+=convert(({Sgml.Tag("box",([]),data->pos,data->data)}));
	    continue;
	 case "exercise":
	    ret+=convert(({Sgml.Tag("li",([]),data->pos,data->data)}));
	    continue;

	 case "data_description":
	    ret+=data_description(data->params,data->pos,data->data,data);
	    continue;

	 case "ref":
	 {
	    string to=data->params->to;
	    TAG t2=link_to_data[to];
	    if(!t2)
	    {
	       werror("Warning: Cannot find ref "+to+" (near "+data->location()+")\n");
	    }
	    if(t2)
	       data->data=({t2->tag+" "+t2->params->number+" \""+t2->params->title+"\""});
	    else
	       data->data=({"unknown"});
	    data->tag="a";
	    data->params->href=mklinkname(link_to_page[to])+"#"+
	       replace(to,({"`",">","<","?"}),({"BQ-","GT","LT","QM"}));
	    break;
	 }
	
	 case "anchor":
	    data->tag="a";
	    if (!data->params->name)
	       werror("Warning: anchor without name (near "+data->location()+")\n");
	    data->params->name=
	       replace(data->params->name||"unnamed",
		       ({"`",">","<","?"}),({"BQ-","GT","LT","QM"}));
	    break;
	
	 case "ex_identifier":
	 case "ex_comment":
	    ret+=convert(data->data);
	    continue;
	 case "ex_string":
	   // FIXME: this doesn't work
	    ret+=convert(replace(data->data," "," "));
	    continue;
	
	 case "example": data->tag="blockquote";break;
	 case "ex_keyword": data->tag="b";break;
	 case "ex_meta": data->tag="i";break;
	 case "ex_br": data->tag="br"; break;

	 case "ex_indent":
	    ret+=({"    "});
	    continue;

	 case "table-of-contents":
	 {
	    SGML tmp=html_toc;
	    if(data->params->target)
	    {
	       werror("(targeting)");
	       tmp=Sgml.copy(tmp);
	       add_target_to_links(tmp,data->params->target);
	    }
	    ret+=({
	       Sgml.Tag("h1",([]),data->pos,
			({
			   data->title || "Table of contents",
			   
			})),
	       "\n"
	    });

	    if(data->params->target)
	    {
	      ret+=({
	       Sgml.Tag("script",(["language":"javascript"]),0,
			({
			  index_to_javascript(global_data->index_data),
			}) ),
	       "\n",
	      });
	    }
	    ret+=convert(tmp);
	    continue;
	 }

	 case "index":
	    ret+=({
	       Sgml.Tag("h1",([]),data->pos,
			({
			   data->title || "Index",
			}))
	    
	    })+convert(html_index);
	    continue;

	 case "preface":
	 case "introduction":
	    ret+=
	       ({
		  Sgml.Tag("h1",([]),data->pos,
			   ({
			      data->params->title,
			   })),
		  "\n",
	       })+
	       convert(data->data);
	       continue;

	 case "chapter":
	    ret+=
	       ({
		  Sgml.Tag("h1",([]),data->pos,
			   ({
			      "Chapter ",
			      data->params->number,
			      ", ",
			      data->params->title,
			   })),
		  "\n",
	       })+
	       convert(data->data);
	       continue;

	 case "appendix":
	    ret+=({
	       Sgml.Tag("h1",([]),data->pos,
			({
			   "Appendix ",
			   data->params->number,
			   ", ",
			   data->params->title||
			   lambda(){error("Appendix without title");}(),
			})),
	       "\n"
	    })+
	       convert(data->data)
		  ;
	       continue;

	 case "section":
	 {
	    string *tmp=data->params->number/".";
	    int level=sizeof(tmp);
	    switch(tmp[0])
	    {
	       case "introduction":
	       case "preface":
		  ret+=({
		     Sgml.Tag("h"+level,([]),data->pos,
			      ({
				 data->params->title,
			      })),
		     "\n"
		  })+
		     convert(data->data)
			;
		     continue;

	       default:
		  ret+=({
		     Sgml.Tag("h"+level,([]),data->pos,
			      ({
				 data->params->number,
				 " ",
				 data->params->title,
			      })),
		     "\n"
		  })+
		     convert(data->data)
			;
		     continue;
	    }
	 }

	 case "encaps":
	 {
	    SGML t=({});

	    foreach(data->data[0]/" "-({""}),string tmp)
	    {
	       t+=({
		  Sgml.Tag("font",(["size":"+1"]),data->pos,({tmp[0..0]})),
		  Sgml.Tag("font",(["size":"-1"]),data->pos,({tmp[1..]})),
		  " "
	       });
	    }
	
	    ret+=({ Sgml.Tag("b",([]),data->pos,t) });
	    continue;
	 }

	 case "img":
	 case "illustration":
	 case "image":
	   {
	     string file;
	     float dpi;
	     [file,dpi]=Gfx.convert(data->params,
					 "jpg|gif",
					 110.0,
					 data->data && Sgml.get_text(data->data));
	     // FIXME: what do we do if the returned file
	     // is in 300 dpi or something??

	    ret+=({ mkimgtag(file,data->params) });
	    continue;
	   }

	 case "box":
	    ret+=({
	       Sgml.Tag("table",
			(["cellpadding":"10",
			  "width":"100%",
			  "border":"1",
			  "cellspacing":"0"]),data->pos,
			({
			   "\n",
			   Sgml.Tag("tr",([]),data->pos,
				    ({
				       Sgml.Tag("td",([]),data->pos,
						convert(data->data))
				    }))
			})),
	       "\n",
	    });
	    continue;

	 case "method":
	 case "function":
	 case "class":
	 case "module":
	    // strip this metainformation
	    ret+=
	      ({Sgml.Tag("dl",([]),data->pos,
		    convert(data->data),
		    ) 
	      });
		      
	    continue;
      }
      data->data=convert(data->data);
      ret+=({data});
    }
  }
  return ret;
}

SGML wmml_to_html(SGML data)
{
  SGML ret=convert(data);
  if(!(objectp(data[0]) && 
       (data[0]->tag=="body" || data[0]->tag=="frameset")))
  {
    ret=({
      Sgml.Tag("body",
	       ([
		 "bgcolor":"#ffffff",
		 "text":"#000000",
		 "link":"blue",
		 "vlink":"purple",
		 ]),0,
	       ret),
    });
  }

  return ({
    Sgml.Tag("html",([]),0,
	     ({
	       "\n",
	       Sgml.Tag("head",([]),0,
			({
			  Sgml.Tag("title",([]),0,
				   ({
				     "Pike Manual",
				       })),
			    "\n"
			      })),
	       })+
	     ret),
      "\n",
      });
}

SGML split_tag(TAG t, TAG t2)
{
  array current=({});
  switch(t2->tag)
  {
    case "preface":
      if(!sections->introduction)
	sections->introduction=({t});
      else
	sections->introduction+=
	  ({
	    t,
	    Sgml.Tag("hr")
	  })+
	  sections->introduction;
	  return current;
	  
    case "introduction":
      if(!sections->introduction)
	sections->introduction=({});
      else
	sections->introduction+=
	  ({
	    Sgml.Tag("hr",(["size":"1","noshade":1]),0)
	  });
      
      
      sections->introduction+=({
	t
      });
      return current;
      
    case "index":
      sections[t2->params->name || "index"]=({ t });
      return current;
      
    case "chapter":
    case "appendix":
      t2->data=low_split(t2->data);
      sections[t2->params->number]=({ t });
      return current;
      
    case "firstpage":
      current+=
	t->data+
	({
	  Sgml.Tag("hr",(["size":"1","noshade":1]),0)
	});
      
      sections->firstpage=({
	t,
      });
      return current;

    case "table-of-contents":
      sections->frame=({
	Sgml.Tag("frameset",(["cols":"30%,*"]),0,
		 ({
		   "\n",
		   Sgml.Tag("frame",(["src":mklinkname("toc_frame"),"name":"toc"])),
		   "\n",
		   
		   Sgml.Tag("frame",(["src":mklinkname("firstpage"),"name":"display"])),
		   "\n",
		   
		 })),
	"\n",
      });
      
      sections->toc_frame=Sgml.copy(({ t }));
      TAG t3=get_tag(sections->toc_frame[0]);
      t3->params->target="display";
      sections->toc_frame[0]->params->name="toc_frame";
      break;
      
    default:
      if(t->data)
	t->data=low_split(t->data);
  }
  return ({t});
}


SGML low_split(SGML data)
{
  SGML current=({});
  foreach(data, TAG t)
    {
      if(objectp(t))
      {
	SGML tmp=split_tag(t,get_tag(t));
	if(search(tmp,0) != -1)
	{
	  error("Got zero! %O\n",tmp);
	}
	current+=tmp;
      }else{
	current+=({t});
      }
    }
  
  return current;
}

SGML split(SGML data)
{
  return low_split(data);
}

void low_collect_links(SGML data, string file)
{
  foreach(data,TAG t)
    {
      if(objectp(t))
      {
	if(t->tag == "anchor")
	{
	  if(t->params->name)
	  {
	    link_to_page[t->params->name]=file;
	    link_to_data[t->params->name]=get_tag(t);
	  }
	  if(t->params->number)
	    link_to_page[t->params->number]=file;
	}

	if(t->data)
	  low_collect_links(t->data,file);
      }
    }
}

string nextify(string num)
{
  if(!num) return "aslkdjf;alskdjfl;asdjfbasm,dbfa,msdfhkl";

  string *tmp=num/".";
  if((int)tmp[-1])
  {
    tmp[-1]=(string)(1+(int)tmp[-1]);
  }else{
    tmp[-1]=sprintf("%c",tmp[-1][0]+1);
  }
  return tmp*".";
}

string prevify(string num)
{
  if(!num) return "aslkdjf;alskdjfl;asdjfbasm,dbfa,msdfhkl";
  string *tmp=num/".";
  if((int)tmp[-1])
  {
    tmp[-1]=(string)(((int)tmp[-1])-1);
  }else{
    tmp[-1]=sprintf("%c",tmp[-1][0]-1);
  }
  return tmp*".";
}

void output(string base, WMML data)
{
  global_data=data;
  files=Stdio.File(base+".files","wct");

  basename=base;

  werror("Splitting ");
  sections=([]);
  sections[""]=split(data->data);

  werror("Finding links ");
  foreach(indices(sections), string file)
    low_collect_links(sections[file], file);

  werror("linking ");
  foreach(indices(sections), string file)
    {
      SGML data=sections[file];
      SGML links=({});
      string tmp1;
      if(sizeof(data)>0 && objectp(data[0]))
      {
	TAG t2=get_tag(data[0]);
	switch(t2->tag)
	{
	  string name;
	  string to;

	  case "preface":
	  case "index":
	  case "chapter":
	  case "appendix":
	  case "section":
	  case "introduction":
	  tmp1=t2->params->number;

	  
	  if(sections[to=prevify(tmp1)])
	  {
	    name="Previous "+t2->tag;
	  }
	  
	  if(name && sections[to])
	  {
	    links+=({ Sgml.Tag("a",(["href":mklinkname(to)]),0,
			       ({
				 Sgml.Tag("img",([
				   "src":"left.gif",
				   "alt":" < ",
				   "border":"0"])),
				 name,
			       })),
		      "\n",
	    });
	  }
	  name=0;

	  links+=({ Sgml.Tag("a",(["href":mklinkname("")]),0,
			     ({
			       Sgml.Tag("img",
					([
					  "src":"up.gif",
					  "alt":" ^ ",
					 "border":"0"])),
			       "To contents",
			     })),
		      "\n",
	  });
	  name=0;
	  


	  if(sections[to=nextify(tmp1)])
	  {
	    name="Next "+t2->tag;
	  }else{
	    switch(t2->tag)
	    {
	      case "chapter": name="To appendices"; to="A"; break;
	      case "appendix": name="To index"; to="index"; break;
	      case "introduction":
	      case "preface":
		name="To chapter one"; to="1";
		break;
	    }
	  }
	  
	  if(name && sections[to])
	  {
	    links+=({ Sgml.Tag("a",(["href":mklinkname(to)]),0,
			       ({
				 Sgml.Tag("img",([
				   "src":"right.gif",
				   "alt":" > ",
				   "border":"0"])),
				 name,
			       })),
		      "\n",
	    });
	  }
	  name=0;

	  links+=({
	    Sgml.Tag("br"),
	      "\n",
	      });

	  
	  sections[file]=links+
	    ({Sgml.Tag("hr")})+
	    data+
	    ({Sgml.Tag("hr")})+
	    links;
	}
      }
    }


  werror("Converting index to WMML\n");
  INDEX index=Wmml.group_index(data->index_data);
  index=Wmml.group_index_by_character(index);
  html_index=index_to_wmml(index);
  index=0;

  werror("Converting TOC to WMML\n");
  html_toc=toc_to_wmml(data->toc);


  if(sections->search_frame)
  {
    sections->search=({
    });
  }

  foreach(sort(indices(sections)),string file)
    {
      string filename=mkfilename(file);
      werror("Anchoring ");
      werror(filename+": WMML->HTML");
      SGML data=wmml_to_html(sections[file]);
      werror("->String");
      string data=Sgml.generate(data,Html.mktag);
      werror("->disk");
      add_file_to_export_list(filename);
      out::open(filename,"wct");
      out::write(data);
      out::close();
      werror(" done\n");
    }
}

Sgml.Tag illustration(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkgif(o,options)]),0);
}

Sgml.Tag illustration_jpeg(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkjpg(o,options)]),0);
}
