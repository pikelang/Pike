#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */
inherit "html.pike";


SGML split_tag(TAG t, TAG t2)
{
  switch(t2->tag)
  {
    case "section":
      t2->data=low_split(t2->data);
      sections[t2->params->number]=({t});
      return ({
	Sgml.Tag("h3",0,0,({
	  Sgml.Tag("link",(["to":t2->params->number]),0,
		   ({
		     sprintf("%s %s",
			     t2->params->number,
			     (string) t2->params->title),
		       })),
	    })
		 });
  }
  return ::split_tag(t,t2);
}

