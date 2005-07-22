#pike __REAL_VERSION__

// $Id: module.pmod,v 1.41 2005/07/22 20:35:32 nilsson Exp $

static constant fmts = ([
  "image/x-pnm" : "PNM",
  "image/jpeg" : "JPEG",
  "image/x-gimp-image" : "XCF",
  "image/png" : "PNG",
  "image/gif" : "GIF",
  "image/x-ilbm" : "ILBM",
  "image/x-MS-bmp" : "BMP",
  "image/x-sun-raster" : "RAS",
  "image/x-pvr" : "PVR",
  "image/x-tim" : "TIM",
  "image/x-xwd" : "XWD",
  "image/x-pcx" : "PCX",
  "application/x-photoshop" : "PSD",
]);

//! @belongs Image
//! Attempts to decode @[data] as image data. The heuristics
//! has some limited ability to decode macbinary files as well.
mapping _decode( string data )
{
  Image.image i, a;
  string format;

  if(!data)
    return 0;

  // macbinary decoding
  if (data[102..105]=="mBIN" ||
      data[65..68]=="JPEG" ||    // weird standard, that
      data[69..72]=="8BIM")
  {
     int i;
     sscanf(data,"%2c",i);
     // sanity check

     if (i>0 && i<64 && !has_value(data[2..2+i-1],"\0"))
     {
	int p,l;
	sscanf(data[83..86],"%4c",l);    // data fork size
	sscanf(data[120..121],"%2c",p);  // extra header size
	p=128+((p+127)/128)*128;         // data fork position

	if (p<sizeof(data) && l) // extra sanity check
	   data=data[p..p+l-1];
     }
  }

  // Use the low-level decode function to get the alpha channel.
#if constant(Image.GIF)
  catch
  {
    array chunks = Image.GIF->_decode( data );

    // If there is more than one render chunk, the image is probably
    // an animation. Handling animations is left as an exercise for
    // the reader. :-)
    foreach(chunks, mixed chunk)
      if(arrayp(chunk) && chunk[0] == Image.GIF.RENDER )
        [i,a] = chunk[3..4];
    format = "GIF";
  };
#endif

  if(!i) {
    catch {
      // PNM, JPEG, XCF, PNG, (GIF), ILBM, BMP, RAS, PVR,
      // TIM, XWD, PCX, PSD
      mapping res = Image.ANY._decode( data );
      i = res->image;
      a = res->alpha;
      format = fmts[res->format];
    };
  }

  if(!i && data[0..3]=="AC10") {
    catch {
      i = Image.DWG.decode(data);
      format = "DWG";
    };
  }

  if(!i && data[0..3]=="%!PS") {
    catch {
      i = Image.PS.decode(data);
      format = "PS";
    };
  }

  if(!i)
    foreach( ({ "TGA", "XBM", "XPM", "TIFF", "SVG", "NEO",
       /* Image formats low on headers below this mark */
                "DSI", "HRZ", "AVS", "WBF",
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

//! @belongs Image
//! Attempts to decode @[data] as image layer data. Additional
//! arguments to the various formats decode_layers method can
//! be passed through @[opt].
array(Image.Layer) decode_layers( string data, mapping|void opt )
{
  array i;
  function f;
  if(!data)
    return 0;

#if constant(Image.GIF)
  catch( i = Image["GIF"]->decode_layers( data ) );
#endif

  if(!i)
    foreach( ({ "XCF", "PSD" }), string fmt )
      if( (f=Image[fmt]["decode_layers"]) &&
	  !catch(i = f( data, opt )) && i )
	break;

  if(!i) // No image could be decoded at all.
    catch
    {
      mapping q = _decode( data );
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

//! @belongs Image
//! Reads the file @[file] and, if the file is compressed
//! with gzip or bzip, attempts to decompress it by calling
//! @tt{gzip@} and @tt{bzip2@} in a @[Process.create_process]
//! call.
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

//! @belongs Image
//! Loads in a file, which need not be an image file. If no
//! argument is given the data will be taken from stdin. If
//! a file object is given, it will be read to the end of the
//! file. If a string is given the function will first attempt
//! to load a file with that name, then try to download data
//! with the string as URL. Zero will be returned upon failure.
local string load_file( void|object|string file )
{
  string data;
  if (!file) file=Stdio.stdin;
  if (objectp(file))
    catch( data = file->read() );
  else
  {
    if( catch( data = read_file( file ) ) || !data || !sizeof(data) )
      catch( data = Protocols["HTTP"]["get_url_nice"]( file )[ 1 ] );
  }
  return data;
}

//! @belongs Image
//! Loads a file with @[load_file] and decodes it with @[_decode].
mapping _load(void|object|string file)
{
   string data = load_file( file );
   if( !data )
     error("Image._load: Can't open %O for input.\n",file);
   return _decode( data );
}

//! @belongs Image
//! Helper function to load an image layer from a file.
//! If no filename is given, Stdio.stdin is used.
//! The loaded file is decoded with _decode.
Image.Layer load_layer(void|object|string file)
{
   mapping m=_load(file);
   if (m->alpha)
      return Image.Layer( (["image":m->image,"alpha":m->alpha]) );
   else
      return Image.Layer( (["image":m->image]) );
}

//! @belongs Image
//! Helper function to load all image layers from a file.
//! If no filename is given, Stdio.stdin is used.
//! The loaded file is decoded with decode_layers. Extra
//! arguments to the image types layer decoder, e.g. for XCF
//! files, can be given in the @[opts] mapping.
array(Image.Layer) load_layers(void|object|string file, mixed|void opts)
{
  return decode_layers( load_file( file ), opts );
}

//! @belongs Image
//! Helper function to load an image from a file.
//! If no filename is given, Stdio.stdin is used.
//! The loaded file is decoded with _decode.
Image.Image load(void|object|string file)
{
   return _load(file)->image;
}

//! @decl Image.Image filled_circle(int d)
//! @decl Image.Image filled_circle(int xd,int yd)
//! @decl Image.Layer filled_circle_layer(int d)
//! @decl Image.Layer filled_circle_layer(int xd,int yd)
//! @decl Image.Layer filled_circle_layer(int d,Image.Color.Color color)
//! @decl Image.Layer filled_circle_layer(int xd,int yd,Image.Color.Color color)
//! @decl Image.Layer filled_circle_layer(int d,int r,int g,int b)
//! @decl Image.Layer filled_circle_layer(int xd,int yd,int r,int g,int b)
//! @belongs Image
//!
//!	Generates a filled circle of the 
//!	dimensions xd x yd (or d x d).
//!	The Image is a white circle on black background; the layer
//!	function defaults to a white circle (the background is transparent).

Image.Image filled_circle(int xd, void|int yd)
{
   int n;
   if (!yd) yd=xd;

   if (xd<10) n=25;
   else if (xd<100) n=35;
   else n=101;

   array x=map(map(map(enumerate(n,2*Math.pi/n),sin),`*,xd/2.0),`+,xd/2.0);
   array y=map(map(map(enumerate(n,2*Math.pi/n),cos),`*,yd/2.0),`+,yd/2.0);

  return 
     Image.Image(xd,yd,0,0,0)
     ->setcolor(255,255,255)
     ->polyfill( Array.splice( x,y ) );
}


Image.Layer filled_circle_layer(int xd,int|Image.Color.Color ...args)
{
   Image.Color.Color c;
   int yd=0;
   switch (sizeof(args))
   {
      case 4:
	 c=Image.Color(@args[1..]);
	 yd=args[0];
	 break;
      case 3:
	 c=Image.Color(@args);
	 break;
      case 2:
	 [yd,c]=args;
      case 1:
	 if (objectp(args[0])) c=args[0]; 
	 else yd=args[0],c=Image.Color.white;
      default:
	 c=Image.Color.white;
   }

   return Image.Layer(Image.Image(xd,yd||xd,c),
		      filled_circle(xd,yd));
}
