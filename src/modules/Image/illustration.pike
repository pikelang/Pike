/* $Id: illustration.pike,v 1.3 1997/11/03 02:06:18 mirar Exp $ */

import Image;
import Stdio;

object lena_image;

object lena()
{
   return lena_image;
}

object foo()
{
***the string***
}

string doit(string name,mapping has,object f,string src)
{
   rm("doc/"+name); // clean up

   object o=foo();
   string s=o->toppm();

   if (s==lena()->toppm()) 
      return "<img src=lena.gif width=67 height=67>";

   if (has[s]) return has[s];
   
   write_file("doc/"+name,Image.GIF.encode(o));

   f->write(
      "<a name="+name+">"
      "<img border=0 src="+name+" width="+o->xsize()+" height="+o->ysize()+" align=right>"
      "<pre>"+src+"</pre>"
      "</a><br clear=all><hr>"
      );

   return has[s]=
      "<a href=illustrations.html#"+name+">"
      "<img border=0 src="+name+" width="+o->xsize()+" height="+o->ysize()+">"
      "</a>";
}
