#include "types.h"
inherit Stdio.File : out;

void make_pages(string base, SGML data, string ind);

void make_page(string base, TAG tag, string ind, string fbase)
{
   ind="";
   werror(ind+tag->tag+" "+tag->params->name+"\n");
   make_pages(base,tag->data,ind+" ");
}

void make_pages(string base, SGML data, string ind, string fbase)
{
   if (arrayp(data))
      foreach (data, TAG tag)
	 if (objectp(tag))
	    if ((<"method","function","class","module">)[tag->tag])
	       make_page(base,tag,ind);
	    else
	       make_pages(base,tag->data,ind+" ");
}

void output(string base, WMML data)
{
   make_pages(base,data->data,"","");
}
