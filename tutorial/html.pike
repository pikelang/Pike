#include "types.h"
inherit Stdio.File : out;

SGML html_toc;
SGML html_index;
mapping(string:SGML) sections=([]);
mapping(string:string) link_to_page=([]);
string basename;

string mkfilename(string section)
{
  if(section=="") return basename+".html";
  return basename+"_"+section+".html";
}


TAG mkimgtag(string file, mapping params)
{
  mapping p=([]);
  p->src=file;
  if(params->align) p->align=params->align;
  if(params->alt) p->align=params->alt;
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

varargs SGML low_toc_to_wmml(SGML toc)
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
	break;
	
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
	    Sgml.Tag("link",(["to":data[key][0][prefix+key][0] ]),0, ({key})),
	      "\n"
		});
	}else{
	  ret+=({key+"\n"});
	}

	foreach(srt(indices(data[key][0])), string key2)
	{
	  if(key2==prefix+key) continue;
	  ret+=({
	    Sgml.Tag("dd"),
	      // FIXME: show all links
	    Sgml.Tag("link",([ "to":data[key][0][key2][0] ]),0,({key2})),
	      "\n"
	  });
	}

      }else{
	ret+=({key+"\n"});
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

SGML index_to_wmml(INDEX data)
{
  SGML ret=({});
  foreach(srt(indices(data)-({0})),string key)
    {
      if(sizeof(data[key]) > !!data[key][0])
      {
	ret+=({
	  Sgml.Tag("dt"),
	    Sgml.Tag("h2",([]),0,({key})),
	      "\n",
	    Sgml.Tag("dd"),
	    Sgml.Tag("dl",([]),0,low_index_to_wmml(data[key],"")),
	      });
      }
    }

  return ({
    Sgml.Tag("dl",([]),0, ret)
	});
}

int cpos;

SGML wmml_to_html(SGML data);

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
      cpos=data->pos;
      switch(data->tag)
      {
      case "hr":
	data->params->noshade=1;
	data->params->size="1";
	break;

      case "link":
      {
	data->tag="a";
	string to=data->params->to;
	m_delete(data->params,"to");
	if(!link_to_page[to])
	{
//	  werror("Warning: Cannot find link "+to+" (near pos "+data->pos+")\n");
	}
	data->params->href=mkfilename(link_to_page[to])+"#"+to;
	break;
      }

      case "anchor": data->tag="a"; break;
// case "ref":
	
      case "ex_identifier":
      case "ex_string":
      case "ex_commend":
	ret+=convert(data->data);
	continue;

      case "example": data->tag="blockquote";break;
      case "ex_keyword": data->tag="b";break;
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
		       }))
	    
	    })+convert(tmp);
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
		       data->params->title,
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

      case "illustration":
	ret+=({ mkimgtag(Wmml.illustration_to_gif(data,75.0),data->params) });
	continue;

      case "image":
	ret+=({ mkimgtag(Wmml.image_to_gif(data,75.0),data->params) });
	continue;

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
  if(!(objectp(data[0]) && (data[0]->tag=="body" || data[0]->tag=="frameset")))
  {
    ret=({
      Sgml.Tag("body",
	       ([
		 "bgcolor":"#A0E0C0",
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

SGML low_split(SGML data)
{
  SGML current=({});
  foreach(data, TAG t)
    {
      if(objectp(t))
      {
	switch(t->tag)
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
	  continue;

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
	  continue;

	case "index":
	case "chapter":
	case "appendix":
	  sections[t->params->number]=({ t });
	  continue;

	case "firstpage":
	  current+=
	    t->data+
	    ({
	      Sgml.Tag("hr",(["size":"1","noshade":1]),0)
		    });

	  sections->firstpage=({
	    t,
	  });
	  continue;

	case "table-of-contents":
	  sections->frame=({
	    Sgml.Tag("frameset",(["cols":"30%,*"]),0,
		     ({
		       "\n",
		       Sgml.Tag("frame",(["src":mkfilename("toc_frame"),"name":"toc"])),
		       "\n",

		       Sgml.Tag("frame",(["src":mkfilename("firstpage"),"name":"display"])),
		       "\n",

		     })),
	    "\n",
	  });

	  sections->toc_frame=Sgml.copy(({ t }));
	  sections->toc_frame[0]->params->target="display";
	  sections->toc_frame[0]->params->name="toc_frame";
	  break;

	default:
	  if(t->data)
	    t->data=low_split(t->data);
	}
      }
      current+=({t});
    }
  
  return current;
}

void de_abstract_anchors(SGML data)
{
  for(int e=0;e<sizeof(data);e++)
  {
    TAG t=data[e];
    if(objectp(t))
    {
      if(Wmml.islink(t->tag))
      {
	if(string tmp=t->params->name)
	  data[e]=Sgml.Tag("anchor",(["name":tmp]),t->pos,({data[e]}));

	if(string tmp=t->params->number)
	  data[e]=Sgml.Tag("anchor",(["name":tmp]),t->pos,({data[e]}));
      }else{
	switch(t->tag)
	{
	case "index":
	case "preface":
	case "introduction":
	case "table-of-contents":
	  data[e]=Sgml.Tag("anchor",
			   ([
			     "name":t->namet || t->tag
			     ]),t->pos,({data[e]}));
	}
      }

      if(t->data)
	de_abstract_anchors(t->data);
    }
  }
}


void low_collect_links(SGML data, string file)
{
  foreach(data,TAG t)
    {
      if(objectp(t))
      {
	if(Wmml.islink(t->tag))
	{
	  if(t->params->name)
	    link_to_page[t->params->name]=file;
	  if(t->params->number)
	    link_to_page[t->params->number]=file;
	}
	switch(t->tag)
	{
	case "index":
	case "introduction":
	case "preface":
	case "table-of-contents":
	  link_to_page[t->params->name || t->tag]=file;
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

void output(string base, SGML data, SGML toc, INDEX_DATA index_data)
{
  basename=base;

  werror("Splitting ");
  sections=([]);
  sections[""]=low_split(data);

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
	switch(data[0]->tag)
	{
	  string name;
	  string to;

	  case "preface":
	  case "index":
	  case "chapter":
	  case "appendix":
	  case "section":
	  case "introduction":
	  tmp1=data[0]->params->number;

	  
	  if(sections[to=prevify(tmp1)])
	  {
	    name="Previous "+data[0]->tag;
	  }
	  
	  if(name && sections[to])
	  {
	    links+=({ Sgml.Tag("a",(["href":mkfilename(to)]),0,
			       ({
				 Sgml.Tag("img",(["src":"left.gif",
					       "border":"0"])),
				 name,
			       })),
		      "\n",
	    });
	  }
	  name=0;

	  links+=({ Sgml.Tag("a",(["href":mkfilename("")]),0,
			     ({
			       Sgml.Tag("img",(["src":"up.gif",
					       "border":"0"])),
			       "To contents",
			     })),
		      "\n",
	  });
	  name=0;
	  


	  if(sections[to=nextify(tmp1)])
	  {
	    name="Next "+data[0]->tag;
	  }else{
	    switch(data[0]->tag)
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
	    links+=({ Sgml.Tag("a",(["href":mkfilename(to)]),0,
			       ({
				 Sgml.Tag("img",(["src":"right.gif",
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


  werror("Converting TOC to WMML\n");
  html_toc=toc_to_wmml(toc);

  werror("Converting index to WMML\n");
  INDEX index=Wmml.group_index(index_data);
  index=Wmml.group_index_by_character(index);
  html_index=index_to_wmml(index);
  index=0;

  foreach(indices(sections),string file)
    {
      string filename=mkfilename(file);
      werror("Anchoring ");
      de_abstract_anchors(sections[file]);
      werror(filename+": WMML->HTML");
      data=wmml_to_html(sections[file]);
      werror("->String");
      string data=Sgml.generate(data);
      werror("->disk");
      out::open(filename,"wct");
      out::write(data);
      out::close();
      werror(" done\n");
    }
}
