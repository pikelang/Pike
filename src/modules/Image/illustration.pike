/* $Id: illustration.pike,v 1.1 1997/05/29 22:51:19 mirar Exp $ */

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

string doit(string name)
{
   object o=foo();
   if (o->toppm()==lena()->toppm()) 
      return "<img src=lena.gif width=67 height=67>";
   
   rm("doc/"+name);
   write_file("doc/"+name,o->togif());
   return "<img src="+name+" width="+o->xsize()+" height="+o->ysize()+">";
}
