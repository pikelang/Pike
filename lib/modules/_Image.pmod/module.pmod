//! module Image
//! $Id: module.pmod,v 1.2 1999/05/28 13:35:02 mirar Exp $


//! method object(Image.Image) load()
//! method object(Image.Image) load(object file)
//! method object(Image.Image) load(string filename)
//! method object(Image.Layer) load_layer()
//! method object(Image.Layer) load_layer(object file)
//! method object(Image.Layer) load_layer(string filename)
//! method mapping _load()
//! method mapping _load(object file)
//! method mapping _load(string filename)
//!	Helper function to load an image from a file.
//!	If no filename is given, Stdio.stdin is used.
//! 	The result is the same as from the decode functions
//!	in <ref>Image.ANY</ref>.
//! note:
//! 	All data is read, ie nothing happens until the file is closed.
//!	Throws upon error.

mapping _load(void|object|string file)
{
   if (!file) file=Stdio.stdin;
   else if (stringp(file))
   {
      object f=Stdio.File();
      if (!f->open(file,"r"))
	 error("Image._load: Can't open %O for input.\n",file);
      file=f;
   }
   return Image.ANY._decode(file->read());
}

object(Image.Layer) load_layer(void|object|string file)
{
   mapping m=_load(file);
   if (m->alpha)
      return Image.Layer( (["image":m->image,"alpha":m->alpha]) );
   else
      return Image.Layer( (["image":m->image]) );
}

object(Image.Image) load(void|object|string file)
{
   return _load(file)->image;
}
