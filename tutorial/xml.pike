#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */
inherit Stdio.File : out;


void output(string base, WMML data)
{
  SGML newdata=({
    Sgml.Tag("wmml",([]),0,
	     ({
	       /* FIXME:
		* add toc, index_data and links
		*/
	       "\n",
	       Sgml.Tag("metadata",([]),0,data->metadata),"\n",
	       Sgml.Tag("data",([]),0,data->data),"\n",
	     }))
  });
  Stdio.File(base+".xml","wct")->write(XML.generate(newdata));
}

Sgml.Tag illustration(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkgif(o,options)]),0);
}

Sgml.Tag illustration_jpeg(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkjpg(o,options)]),0);
}
