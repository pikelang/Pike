//! module Image
//! $Id: module.pmod,v 1.6 2000/02/24 01:11:22 marcus Exp $

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

mapping _decode( string data, mixed|void tocolor )
{
  Image.image i, a;
  string format;
  mapping opts;
  if(!data)
    return 0;

  if( mappingp( tocolor ) )
  {
    opts = tocolor;
    tocolor = 0;
  }
  // Use the low-level decode function to get the alpha channel.
  catch
  {
    array chunks = Image["GIF"]->_decode( data );

    // If there is more than one render chunk, the image is probably
    // an animation. Handling animations is left as an exercise for
    // the reader. :-)
    foreach(chunks, mixed chunk)
      if(arrayp(chunk) && chunk[0] == Image.GIF.RENDER )
        [i,a] = chunk[3..4];
    format = "GIF";
  };

  if(!i)
    foreach( ({ "GIF", "JPEG", "XWD", "PNM" }), string fmt )
    {
      catch {
        i = Image[fmt]->decode( data );
        format = fmt;
      };
      if( i )
        break;
    }

  if(!i)
    foreach( ({ "XCF", "PSD", "PNG",  "BMP",  "TGA", "PCX",
                "XBM", "XPM", "TIFF", "ILBM", "PS", "PVR",
       /* Image formats low on headers below this mark */
                "HRZ", "AVS", "WBF",
       /* "XFace" Always succeds*/
    }), string fmt )
    {
      catch {
        mixed q = Image[fmt]->_decode( data );
        format = fmt;
        i = q->image;
        a = q->alpha;
      };
      if( i )
        break;
    }

  if(!i) // No image could be decoded at all.
    return 0;

  if( arrayp(tocolor) && (sizeof(tocolor)==3) && objectp(i) && objectp(a) )
  {
    Image.Image o = Image.Image( i->xsize(), i->ysize(), @tocolor );
    i = o->paste_mask( i, a );
  }

  return  ([
    "format":format,
    "alpha":a,
    "img":i,
    "image":i,
  ]);
}

array(Image.Layer) decode_layers( string data, mixed|void tocolor )
{
  array i;

  if(!data)
    return 0;

  foreach( ({ "GIF", "JPEG", "XWD", "PNM",
              "XCF", "PSD", "PNG",  "BMP",  "TGA", "PCX",
              "XBM", "XPM", "TIFF", "ILBM", "PS",
              "HRZ", "AVS", "WBF",
  }), string fmt )
    if( !catch(i = Image[fmt]->decode_layers( data )) && i )
      break;

  if(!i) // No image could be decoded at all.
    catch
    {
      mapping q = _decode( data, tocolor );
      i = ({
        Image.Layer( ([
          "image":q->img,
          "alpha":q->alpha
        ]) )
      });
    };

  return i;
}


string read_file(string file)
{
  string ext="";
  sscanf(reverse(file),"%s.%s",ext,string rest);
  string dcc;

  switch(lower_case(reverse(ext)))
  {
   case "gz":
   case "z":
     dcc="gzip";
     break;
   case "bz":
   case "bz2":
     dcc = "bzip2";
     break;
  }

  if( dcc )
  {
    object f = Stdio.File();
    object p=f->pipe(Stdio.PROP_IPC);
    Process.create_process(({dcc,"-c","-d",file}),(["stdout":p]));
    destruct( p );
    return f->read();
  }
  return Stdio.read_file( file );
}

local string load_file( void|object|string file )
{
  string data;
  if (!file) file=Stdio.stdin;
  if (objectp(file))
    data = file->read();
  else
  {
    if( catch( data = read_file( file ) ) || !data || !strlen(data) )
      catch( data = Protocols.HTTP.get_url_nice( file )[ 1 ] );
  }
  return data;
}

mapping _load(void|object|string file, mixed|void opts)
{
   string data = load_file( file, opts );
   if( !data )
     error("Image._load: Can't open %O for input.\n",file);
   return _decode( data,opts );
}

object(Image.Layer) load_layer(void|object|string file)
{
   mapping m=_load(file);
   if (m->alpha)
      return Image.Layer( (["image":m->image,"alpha":m->alpha]) );
   else
      return Image.Layer( (["image":m->image]) );
}

array(Image.Layer) load_layers(void|object|string file, mixed|void opts)
{
  string d = load_file( file );
  return decode_layers( file, opts );
}

object(Image.Image) load(void|object|string file)
{
   return _load(file)->image;
}
