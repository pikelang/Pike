#include "types.h"
import ".";
inherit "html.pike";

mapping what_bg=
(["method":"#c0c0ff",
  "function":"#c0c0ff",
  "variable":"#e0c0e0",
  "class":"#ffa296",
  "module":"#ffa296"]);

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
		 "link":"#110664",
		 "vlink":"##641831",
		 "alink":"#b20200",
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
// 			       "bgcolor":"#b1b2e6",
			       "align":"right"]),
			data->pos, 
			({Sgml.Tag(
			   "font",(["size":"-1"]),data->pos,
			   ({
			      " ",
			      data->params->title,
			      " "
			   }))})),
		     Sgml.Tag(
			      "td",([
// 				"bgcolor":"#151535"
			      ]),
			      data->pos, 
			      ({ Sgml.Tag(
					  "img",(["src":"/internal-roxen-unit.gif",
						  "width":"10","height":"1"])) })),
		     Sgml.Tag(
			      "td",([
// 				"bgcolor":"#151535",
				"valign":"top",
				     "width":"100%","align":"left"]),
			      data->pos, 
			      convert(data->data)),
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

	 case "arguments":
	    ret+=({ Sgml.Tag(
	       "table",(["border":"0",
			 "cellpadding":"0","cellspacing":"4"]),
	       data->pos,
	       convert(data->data||({}))) });
	    continue;

	 case "aargdesc":
	    ret+=({Sgml.Tag(
	       "tr",([]),data->pos,
	       ({Sgml.Tag("td",(["valign":"top","align":"left"]))})+
	       convert(data->data))});
	    continue;
	 case "adesc":
	    ret+=({"   ",Sgml.Tag("/td",([])),
		   Sgml.Tag("td",(["valign":"top","align":"left"]),
			    data->pos,convert(data->data))});
	    continue;
	 case "aarg": 
	    ret+=({Sgml.Tag("tt",([]),data->pos,
			    convert(data->data)),
		   Sgml.Tag("br")});
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
		  "table",(["width":"100%","cellpadding":"6",
			    "border":"0",
// 			    "bgcolor":"#ffdead", "align":"center"
		  ]),data->pos, 
		  ({
		     Sgml.Tag(
			"tr",([]),data->pos,
			({
			   Sgml.Tag("td",
				    ([
 				      "bgcolor":what_bg[data->tag],
					   "colspan":"4"]),
			      data->pos,
			      ({
				 Sgml.Tag(
				    "font",(["color":"black","size":"+2"]),
				    data->pos,
				    ({Sgml.Tag(
				       "b",([]),data->pos,
				       ({data->params->name||
					 data->params->title||"(?)"}))
				    })),
				    "    ",
				 Sgml.Tag(
				    "font",(["color":"#000030"]),
				    data->pos,
				    ((data->params->name && 
				      data->params->title)
				     ?({Sgml.Tag("i",([]),data->pos,
						 ({data->params->title+" "}))})
				     :({}))
				    // +({"("+data->tag+")"})
					  )
			      })), 
			})+convert(data->data))
		  }))
	    });
	    continue;
      }
      ret+=::convert(({data}));
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
