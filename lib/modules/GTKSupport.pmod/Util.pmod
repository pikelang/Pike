#if constant(GTK.parse_rc)

// This function will be easier to write in newer pikes, where there
// will be a Image.ANY.decode function, but this will do for now. It
// decoded an image from a string and returns a mapping with the
// image, and optinally the alpha channel.
// 
// Currently supports most commonly used image formats, but only PNG
// and GIF supports alpha channels
//

array(int) invert_color(array color )
{
  return ({ 255-color[0], 255-color[1], 255-color[2] });
}


mapping low_decode_image(string data, mixed tocolor)
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
    array chunks = Image.GIF._decode( data );

    // If there is more than one render chunk, the image is probably
    // an animation. Handling animations is left as an exercise for
    // the reader. :-)
    foreach(chunks, mixed chunk)
      if(arrayp(chunk) && chunk[0] == Image.GIF.RENDER )
        [i,a] = chunk[3..4];
    format = "GIF";
  };

  if(!i) catch
  {
    i = Image.GIF.decode( data );
    format = "GIF";
  };

  // The JPEG format is only available if JPEG library was installed
  // when pike was compiled, thus we have to conditionally compile
  // this part of the code.
#if constant(Image.JPEG) && constant(Image.JPEG.decode)
  if(!i) catch
  {
    i = Image.JPEG.decode( data );
    format = "JPEG";
  };

#endif

#if constant(Image.XCF) && constant(Image.XCF._decode)
  if(!i) catch
  {
    if(!opts) opts = ([]);
    if( tocolor )
    {
      opts->background = (array)tocolor;
      tocolor = 0;
    }
    mixed q = Image.XCF._decode( data, opts );
    format = "XCF Gimp file";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.PSD) && constant(Image.PSD._decode)
  if(!i) catch
  {
    mixed q = Image.PSD._decode( data, ([
      "background":tocolor,
      ]));
    tocolor=0;
    format = "PSD Photoshop file";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.PNG) && constant(Image.PNG._decode)
  if(!i) catch
  {
    mixed q = Image.PNG._decode( data );
    format = "PNG";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.BMP) && constant(Image.BMP._decode)
  if(!i) catch
  {
    mixed q = Image.BMP._decode( data );
    format = "Windows bitmap file";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.TGA) && constant(Image.TGA._decode)
  if(!i) catch
  {
    mixed q = Image.TGA._decode( data );
    format = "Targa";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.PCX) && constant(Image.PCX._decode)
  if(!i) catch
  {
    mixed q = Image.PCX._decode( data );
    format = "PCX";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.XBM) && constant(Image.XBM._decode)
  if(!i) catch
  {
    mixed q = Image.XBM._decode( data, (["bg":tocolor||({255,255,255}),
                                    "fg":invert_color(tocolor||({255,255,255})) ]));
    format = "XBM";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.XPM) && constant(Image.XPM._decode)
  if(!i) catch
  {
    mixed q = Image.XPM._decode( data );
    format = "XPM";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.TIFF) && constant(Image.TIFF._decode)
  if(!i) catch
  {
    mixed q = Image.TIFF._decode( data );
    format = "TIFF";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.ILBM) && constant(Image.ILBM._decode)
  if(!i) catch
  {
    mixed q = Image.ILBM._decode( data );
    format = "ILBM";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.RAS) && constant(Image.RAS.decode)
  if(!i) catch
  {
    i = Image.RAS.decode( data );
    format = "Sun Raster";
  };
#endif

#if constant(Image.PS) && constant(Image.PS._decode)
  if(!i) catch
  {
    mixed q = Image.PS._decode( data );
    format = "Postscript";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.XWD) && constant(Image.XWD.decode)
  if(!i) catch
  {
    i = Image.XWD.decode( data );
    format = "XWD";
  };
#endif

#if constant(Image.AVS) && constant(Image.AVS._decode)
  if(!i) catch
  {
    mixed q = Image.AVS._decode( data );
    format = "AVS X";
    i = q->image;
    a = q->alpha;
  };
#endif

  if(!i)
    catch{
      i = Image.PNM.decode( data );
      format = "PNM";
    };


/* Image formats without headers */
#if constant(Image.HRZ) && constant(Image.HRZ._decode)
  if(!i) catch
  {
    mixed q = Image.HRZ._decode( data );
    format = "HRZ";
    i = q->image;
    a = q->alpha;
  };
#endif

#if constant(Image.WBF) && constant(Image.WBF._decode)
  if(!i) catch
  {
    mixed q = Image.WBF._decode( data );
    format = "WBF";
    i = q->image;
    a = q->alpha;
  };
#endif

  if(!i) // No image could be decoded at all. 
    return 0;

  if( tocolor && i && a )
  {
    object o = Image.image( i->xsize(), i->ysize(), @tocolor );
    o->paste_mask( i,a );
    i = o;
  }

  return ([
    "format":format,
    "alpha":a,
    "img":i,
  ]);
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

// This function loads and decodes and image from disk.
// Returns Image.image objects
mapping low_load_image( string filename, mapping|array|void bgcol )
{
  string data;
  catch(data = read_file(filename));
  return low_decode_image(data,bgcol);
}

// returns GDK image objects.
mapping load_image( string filename, array|void bgcol )
{
  if(mapping a = low_load_image( filename, bgcol ) )
  return ([
    "format":a->format,
    "alpha": a->alpha && GDK.Bitmap( a->alpha ),
    "img":  GDK.Pixmap( a->img ),
  ]);
}

// returns GDK image objects.
mapping decode_image( string data, mapping|array|void tocolor )
{
  if(mapping a = low_decode_image( data,tocolor ) )
  return ([
    "format":a->format,
    "alpha": a->alpha && GDK.Bitmap( a->alpha ),
    "img":   GDK.Pixmap( a->img ),
  ]);
}

string low_encode_image( mapping img, string fmt )
{
  if(img->alpha) return 0;
  catch {
    return Image[fmt||"PNG"]->encode( img->img );
  };
  return 0;
}

string encode_image( mapping img, string fmt )
{
}




// Some shortcut handling stuff.

mapping parse_shortcut_file( string f )
{
  mapping ss = ([]);
  Stdio.File o = Stdio.File();
  if(o->open(f, "r"))
  {
    string p,k;
    foreach(o->read()/"\n", string l)
      if(sscanf(l, "\"%s\" \"%s\"", p, k) == 2)
	ss[p] = k;
  }
  return ss;
}

void save_shortcut_file( string f, mapping ss )
{
  Stdio.File o = Stdio.File();
  if(o->open(f, "wct"))
  {
    foreach(sort(indices(ss)), string s)
      o->write( "\""+s+"\" \""+ss[s]+"\"\n" );
  }
}


class signal_handling
{
  class Fun
  {
    mixed arg;
    function tocall;

    void `()(mixed ... args)
    {
      array err;
      if(!tocall)
        destruct();
      else
        if(err=catch(tocall(arg,@args)))
        {
          if(err == 1)
          {
            destruct();
            return;
          }
          werror("signal error: %s\n", describe_backtrace( err ) );
        }
    }
  
    void create(function f, mixed a)
    {
      tocall = f;
      arg = a;
    }
  }

  mapping signals = ([]);
  mapping r_signals = ([]);

  mixed signal_connect( string signal, function tocall, mixed arg )
  {
    object ret;
    if(signals[signal])
      signals[signal]+=({ (ret=Fun( tocall, arg)) });
    else
      signals[signal]=({ (ret=Fun( tocall, arg)) });
    r_signals[ret] = signal;
    return ret;
  }

  void signal_disconnect( Fun what )
  {
    if(r_signals[what])
    {
      signals[r_signals[what]] -= ({ what });
      m_delete( r_signals, what );
      destruct(what);
    }
  }

  void signal_broadcast( string signal, mixed ... args)
  {
    if(signals[signal]) 
    {
      signals[signal]-=({ 0 });
      signals[signal]( this_object(), @args );
      signals[signal]-=({ 0 });
    }
  }
}

#endif
