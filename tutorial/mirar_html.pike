#include "types.h"
import ".";
inherit "html.pike";

mapping what_bg=
(["method":"#c0c0ff",
  "function":"#c0c0ff",
  "variable":"#e0c0e0",
  "class":"white",
  "module":"white"]);

SGML wmml_to_html(SGML data)
{
  SGML ret=convert(data);
  if(!(objectp(data[0]) && 
       (data[0]->tag=="body" || data[0]->tag=="frameset")))
  {
    ret=({
      Sgml.Tag("body",
	       ([
		 "bgcolor":"#000020",
		 "text":"#ffffff",
		 "link":"#f0e0ff",
		 "vlink":"#c0c0f0",
		 "alink":"#ffffff",
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
	TAG t2=get_tag(t);
	switch(t2->tag)
	{
	case "preface":
	  if(!sections->introduction)
	    sections->introduction=({t});
	  else
	    sections->introduction+=
	      ({
		t,
		Sgml.Tag("hr",(["size":"1","noshade":1]),0)
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
	  sections[t2->params->name || "index"]=({ t });
	  continue;

	case "chapter":
	case "appendix":
	  sections[t2->params->number]=({ t });
	  if (this_object()->split_and_remove_section)
	     this_object()->split_and_remove_section(t);
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
      }
      current+=({t});
    }
  
  return current;
}

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


	 case "man_title":
	    if (data->params->title=="METHOD"||
		data->params->title=="FUNCTION"||
		data->params->title=="CLASS"||
		data->params->title=="VARIABLE"||
		data->params->title=="MODULE")
	       continue;

	    if (data->params->title=="specdesc")
	    {
	       // is this needed?
	       ret+=({
		  "\n",
		  Sgml.Tag(
		     "tr",([]),data->pos, 
		     ({ Sgml.Tag(
			"td",(["valign":"top","width":"100%",
			       "align":"left","colspan":"4"]),
			data->pos, 
			convert(data->data))})),
		  "\n"
	       });
	       continue;
	    }

	    ret+=({
	       "\n",
	       Sgml.Tag(
		  "tr",([]),data->pos, 
		  ({
		     Sgml.Tag(
			"td",(["valign":"top","width":"1",
			       "bgcolor":"#202040","align":"right"]),
			data->pos, 
			({Sgml.Tag(
			   "font",(["size":"-1"]),data->pos,
			   ({
			      " ",
			      data->params->title,
			      " "
			   }))})),
		     Sgml.Tag(
			"td",(["bgcolor":"#151535"]),
			data->pos, 
			({ Sgml.Tag(
			   "img",(["src":"/internal-roxen-unit.gif",
				   "width":"10","height":"1"])) })),
		     Sgml.Tag(
			"td",(["bgcolor":"#151535","valign":"top",
			       "width":"100%","align":"left"]),
			data->pos, 
			convert(data->data)),
		     Sgml.Tag(
			"td",(["bgcolor":"#151535"]),
			data->pos, 
			({ Sgml.Tag(
			   "img",(["src":"/internal-roxen-unit.gif",
				   "width":"30","height":"1"])) })),
		  })),
	       "\n",
	       Sgml.Tag(
		  "tr",([]),data->pos, 
		  ({
		     Sgml.Tag(
			"td",(["colspan":"4","width":"100%",
			       // "bgcolor":"#151535"
			]),
			data->pos, 
			({ Sgml.Tag(
			   "img",(["src":"/internal-roxen-unit.gif",
				   "width":"1","height":"1"])) })),
		  })),
	       "\n",
	       });
	    continue;

	 case "link":
	 {
	    data->tag="a";
	    string to=data->params->to;
	    m_delete(data->params,"to");
	    if(!link_to_page[to])
	    {
	       string s=(to/".")[-1];
	       if (link_to_page[s]) 
		  to=s;
	       else
	       {
		  werror("Warning: Cannot find link "+to
			 +" (near "+data->location()+")\n");
		  data->tag="anchor";
		  break;
	       }
	    }
// 	    werror("--- %O --- %O\n",to,link_to_page[to]);
	    data->params->href=mklinkname(link_to_page[to])+"#"+to;
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
	    data->params->href=mklinkname(link_to_page[to])+"#"+to;
	    break;
	 }
	
	 case "anchor":
	    data->tag="a";
	    break;
	
	 case "ex_identifier":
	 case "ex_commend":
	    ret+=convert(data->data);
	    continue;
	 case "ex_string":
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
			   data->params->title||error("Appendix without title"),
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
	    add_file_to_export_list(data->params->src);
	    break;

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

	 case "method":
	 case "function":
	 case "variable":
	 case "class":
	 case "module":
	    data->data=clean_up_item(data->data);
	    ret+=({
	       "\n",
	       Sgml.Tag(
		  "table",(["width":"100%","cellpadding":"2",
			    "cellspacing":"0","border":"0"]),data->pos, 
		  ({
		     Sgml.Tag(
			"tr",([]),data->pos,
			({
			   Sgml.Tag(
			      "td",(["bgcolor":what_bg[data->tag],
				     "colspan":"4"]),
			      data->pos,
			      ({
				 Sgml.Tag(
				    "font",(["color":"black"]),
				    data->pos,
				    ({Sgml.Tag(
				       "b",([]),data->pos,
				       ({data->params->name||
					 data->params->title||"(?)"}))
				    })),
				    "    ",
				 Sgml.Tag(
				    "font",(["color":"#404040"]),
				    data->pos,
				    ((data->params->name && 
				      data->params->title)
				     ?({Sgml.Tag("i",([]),data->pos,
						 ({data->params->title+" "}))})
				     :({}))
				    // +({"("+data->tag+")"})
					  )
			      })), 
			}))})+
		  convert(data->data)
		  )});
	    continue;
      }
      data->data=convert(data->data);
      ret+=({data});
    }
  }
  return ret;
}

array clean_up_item(array in)
{
   array res=({});
   array buf=({});
   int mode=0;
   int got;
   foreach (in,object|string s)
   {
      if (stringp(s)) got=1;
      else if (s->tag!="man_title") got=1;
      else got=0;
      if (mode!=got) 
      {
	 if (mode==0 && stringp(s) && s-" "-"\n"-"\t"-"\r"=="")
	    continue; // ignore white

	 if (mode==0) // man_title's
	    res+=buf;
	 else
	    res+= ({ Sgml.Tag("man_title",(["title":"specdesc"]),0,buf) });
	 mode=got;
      }
      buf+=({s});
   }
   if (mode==0) // man_title's
      res+=buf;
   else
      res+= ({ Sgml.Tag("man_title",(["title":"specdesc"]),0,buf) });
   return res;
}
