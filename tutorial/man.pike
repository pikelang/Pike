#include "types.h"
inherit Stdio.File : out;

void make_page(string base, TAG tag, string ind, string fbase);

string make_manpage(string base, SGML data, string ind, string fbase)
{
   string res="";

   if (stringp(data))
      res+=data;
   else if(arrayp(data))
      foreach (data, TAG tag)
	 if (objectp(tag))
	 {
	    switch (tag->tag)
	    {
	       case "method":
	       case "function":
	       case "class":
	       case "module":
		  make_page(base,tag,ind,fbase);
		  continue;

	       case "man_title":
		  res+="\n.SH "+tag->params->title+"\n";
		  break;
	    }
	    res+=make_manpage(base,tag->data,ind,fbase);
	 }
	 else if (stringp(tag))
	    res+=tag;

   return res;
}

void make_page(string base, TAG tag, string ind, string fbase)
{
   werror(ind+tag->tag+" "+tag->params->name+"\n");

   string *outfiles;

   outfiles=Array.map(tag->params->name/",",
		      lambda(string s,string t,string u)
		      {
			 sscanf(replace(s,"->","."),t+".%s",s);
			 return u+"/"+s;
		      },fbase,base);

   werror("files: "+outfiles*", "+"\n");

   if (tag->params->mansuffix)
   {
      base+=tag->params->mansuffix;
      fbase=(tag->params->name/",")[0];
   }

   string page="";

   page+=".TH "+(tag->params->name/",")[0]+" "+base[3..]+"\n";

   page+=make_manpage(base,tag->data,ind+" ",fbase);

   werror(page);

   exit(0);
}

void make_pages(string base, SGML data, string ind, string fbase)
{
   if (arrayp(data))
      foreach (data, TAG tag)
	 if (objectp(tag))
	    if ((<"method","function","class","module">)[tag->tag])
	       make_page(base,tag,ind,fbase);
	    else
	       make_pages(base,tag->data,ind,fbase);
}

void output(string base, WMML data)
{
   make_pages(base,data->data,"","");
}
