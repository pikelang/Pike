#pike __REAL_VERSION__

// This function will be easier to write in newer pikes, where there
// will be a Image.ANY.decode function, but this will do for now. It
// decoded an image from a string and returns a mapping with the
// image, and optinally the alpha channel.
// 
// Currently supports most commonly used image formats, but only PNG
// and GIF supports alpha channels
//
mapping low_decode_image(string data, mixed|void tocolor)
{
  return Image._decode( data, tocolor );
}


// This function loads and decodes and image from disk.
// Returns Image.image objects
mapping low_load_image( string filename, mapping|array|void bgcol )
{
  return Image._load( filename, bgcol );
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
      array|int(1..1) err;
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
  
    void create(function f, mixed|void a)
    {
      tocall = f;
      arg = a;
    }
  }

  mapping signals = ([]);
  mapping r_signals = ([]);

  mixed signal_connect( string signal, function tocall, mixed|void arg )
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
      signals[signal]( this, @args );
      signals[signal]-=({ 0 });
    }
  }
}

