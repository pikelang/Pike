#pike __REAL_VERSION__

//! module Image
//! $Id: module.pmod,v 1.17 2001/03/30 01:42:24 per Exp $

//! method Image.Layer load()
//! method Image.Image load(object file)
//! method Image.Image load(string filename)
//! method Image.Layer load_layer()
//! method Image.Layer load_layer(object file)
//! method Image.Layer load_layer(string filename)
//! method array(Image.Layer) load_layers()
//! method array(Image.Layer) load_layers(object file)
//! method array(Image.Layer) load_layers(string filename)
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

  // macbinary decoding
  if (data[102..105]=="mBIN" ||
      data[65..68]=="JPEG" ||    // wierd standard, that
      data[69..72]=="8BIM")
  {
     int i;
     sscanf(data,"%2c",i);
     // sanity check

     if (i>0 && i<64 && -1==search(data[2..2+i-1],"\0"))
     {
	int p,l;
	sscanf(data[83..86],"%4c",l);    // data fork size
	sscanf(data[120..121],"%2c",p);  // extra header size
	p=128+((p+127)/128)*128;         // data fork position

	if (p<strlen(data)) // extra sanity check
	   data=data[p..p+l-1];
     }
  }

  // Use the low-level decode function to get the alpha channel.
#if constant(Image.GIF) && constant(Image.GIF.RENDER)
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
#endif

  if(!i)
    foreach( ({ "JPEG", "XWD", "PNM", "RAS" }), string fmt )
    {
      catch {
        i = Image[fmt]->decode( data );
        format = fmt;
      };
      if( i )
        break;
    }

  if(!i)
    foreach( ({ "ANY", "XCF", "PSD", "PNG",  "BMP",  "TGA", "PCX",
                "XBM", "XPM", "TIFF", "ILBM", "PS", "PVR",
       /* Image formats low on headers below this mark */
                "DSI", "TIM", "HRZ", "AVS", "WBF",
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
  function f;
  if(!data)
    return 0;
  foreach( ({ "GIF", "XCF", "PSD","ILBM" }), string fmt )
    if( (f=Image[fmt]["decode_layers"]) && !catch(i = f( data,tocolor )) && i )
      break;

  if(!i) // No image could be decoded at all.
    catch
    {
      mapping q = _decode( data, tocolor );
      if( !q->img )
	return 0;
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
  return decode_layers( load_file( file ), opts );
}

object(Image.Image) load(object|string file)
{
   return _load(file)->image;
}
