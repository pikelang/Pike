#pike __REAL_VERSION__

//! @appears Image.Fonts
//! Abstracted Font-handling support

constant ITALIC = 1;
//! The font is/should be italic

constant BOLD   = 2;
//! The font is/should be bold

constant BLACK  = 6; // Also enables BOLD..


constant NO_FAKE  = 256; 
//! Used in @[open_font]() to specify that no fake bold or italic
//! should be attempted.

#if constant(Image.FreeType.Face)
static class FTFont
{
  constant driver = "FreeType 2";

  Thread.Mutex lock = Thread.Mutex();
  Image.FreeType.Face face;
  static int size;
  static int xspacing;
  static int line_height;

  mapping(string:mixed) info()
  {
    mapping inf = face->info();
    inf->style = m_delete( inf, "style_flags" );
    return inf;
  }
  
  void set_xspacing( int x )
  {
    xspacing = x;
  }
  
  array text_extents( string what )
  {
    Image.Image o = write( what );
    return ({ o->xsize(), o->ysize() });
  }
  
  int height( )
  {
    if( !line_height ) write_row( "fx1!" );
    return line_height;
  }

  static mixed do_write_char( int c )
  {
    catch{
      return face->write_char( c );
    };
    return 0;
  }

  static Image.Image write_row( string text )
  {
    Image.Image res;
    int xp, ys;
    if( !strlen( text ) )
      text = " ";
    array(int) tx = (array(int))text;
    array chars = map( tx, do_write_char );
    int i;

    for( i = 0; i<sizeof( chars ); i++ )
      if( !chars[i] )
	tx[ i ] = 0;
      else if( !tx[i] )
	tx[ i ] = 20;

    tx  -= ({ 0 });
    chars  -= ({ 0 });
    
    if( !sizeof( chars ) )
      return Image.Image(1,1);

    int oc;
    array kerning = ({});
    foreach( tx, int c  )
    {
      if( oc )
        kerning += ({ face->get_kerning( oc, c )>>6 });
      else
        kerning += ({ 0 });
      oc = c;
    }
    kerning += ({ 0 });

    int w;
    for(int i = 0; i<sizeof(chars)-1; i++ )
      w += (int)(chars[i]->advance + kerning[i+1])+xspacing;

    w += (int)(chars[-1]->img->xsize()+chars[-1]->x);
    ys = chars[0]->ascender-chars[0]->descender;
    line_height = (int)chars[0]->height;
			   
    res = Image.Image( w, ys );

    for( int i = 0; i<sizeof( chars); i++ )
    {
      mapping c = chars[i];
      res->paste_alpha_color( c->img, ({255,255,255}),
                              xp+c->x, ys+c->descender-c->y );
      xp += (int)c->advance + kerning[i+1] + xspacing;
    }
    return res;
  }

  int advance( int character )
  {
    mapping m = do_write_char( character );
    if( m )
      return m->advance;
  }
  
  Image.Image write( string ... text )
  {
    if( !sizeof( text ) )
      return Image.Image( 1,height() );

    object key = lock->lock();
    face->set_size( 0, size );
    
    text = map( (array(string))text, replace, "\240", "" );
    
    array(Image.Image) res = map( text, write_row );

    Image.Image rr = Image.Image( max(0,@res->xsize()),
				  (int)(res[0]->ysize()+
					line_height*(sizeof(res)-1)));

    int start;
    foreach( res, object r )
    {
      rr->paste_alpha_color( r, 255,255,255, 0, start );
      start += line_height;
    }
    return rr;
  }

  void create( Image.FreeType.Face _f, int _s, string fn )
  {
    string fn2;
    face = _f;
    size = _s;

    if( (fn2=replace(fn,".pfa",".afm")) != fn && file_stat( fn2 ) )
      catch( face->attach_file( fn2 ) );
    
  }
}
#endif

#if constant(Image.TTF)
class TTFont
{
  constant driver = "FreeType 1";

  static object real;
  static int size;

  static int translate_ttf_style( string style )
  {
    switch( lower_case( (style-"-")-" " ) )
    {
      case "roman":
      case "normal": case "regular":        return 0;
      case "italic":                        return ITALIC;
      case "oblique":                       return ITALIC;
      case "bold":                          return BOLD;
      case "bolditalic":case "italicbold":  return ITALIC|BOLD;
      case "black":                         return BLACK;
      case "blackitalic":case "italicblack":return BLACK|ITALIC;
      case "light":                         return 0;
      case "lightitalic":case "italiclight":return ITALIC;
    }
    if(has_value(lower_case(style), "oblique"))
      return ITALIC; // for now.
    return 0;
  }

  mapping(string:mixed) info()
  {
    mapping in = real->face()->names();
    in->style = translate_ttf_style( in->style );
    return in;
  }
  
  int height() { return size; }

  void set_xspacing( int x ) { }

  array text_extents( string what )
  {
    Image.Image o = write( what );
    return ({ o->xsize(), o->ysize() });
  }
  
  Image.Image write( string ... what )
  {
    if( !sizeof( what ) )
      return Image.Image( 1,height() );

    what = map( (array(string))what, replace, "\240", "" );

    // cannot write "" with Image.TTF.
    what = replace( what, "", " " );

    array(Image.Image) res = map( what, real->write );

    Image.Image rr = Image.Image( max(0,@res->xsize()),
                                  abs(`+(0,@res->ysize())));
    
    int start = 0;
    foreach( res, object r )
    {
      rr->paste_alpha_color( r, 255,255,255, 0, (int)start );
      start += r->ysize();
    }
    return rr;
  }

  void create( object _r, int _s )
  {
    real = _r;
    size = _s;
    real->set_height( size );
  }
}
#endif


class Font( static string file,
	    static int size )
//! The abstract Font class. The @[file] is a valid font-file, @[size]
//! is in pixels, but the size of the characters to be rendered, not
//! the height of the finished image (the image is generally speaking
//! bigger then the size of the characters).
{
  object font;
  static object codec;
  
  static int fake_bold;
  static int fake_italic;

  static void open_font( )
  {
    switch( file )
    {
      case "fixed":
	font = Image.Font();
	break;

      default:
	if(file_stat(file+".properties"))
	  Parser.HTML()
	    ->add_container( "encoding", 
			     lambda(Parser.HTML p,
				    mapping m, string enc) {
			       codec=Locale.Charset.encoder(enc,"");
			     } )
	    ->feed( Stdio.read_file( file+".properties" ) )
	    ->read();

	if( !file_stat( file ) )
	  error("%s does not exist\n", file );

#if constant(Image.FreeType.Face)
	if(!font)
	  catch(font=FTFont(Image.FreeType.Face( file ),size,file));
#endif

#if constant(Image.TTF)
	if(!font) catch( font = TTFont( Image.TTF( file )(), size ) );
#endif
	if( !font )
	  catch( font = Image.Font( file ) );

	if( !font )
	  error("%s is not a font-file in any known format\n", file );
    }
  }

  static Image.Image post_process( Image.Image rr )
  {
    if( fake_bold > 0 )
    {
      object r2 = Image.Image( rr->xsize()+2, rr->ysize() );
      object r3 = rr*0.5;
      for( int i = 0; i<=fake_bold; i++ )
	for( int j = 0; j<=fake_bold; j++ )
	  r2->paste_alpha_color( r3,  255, 255, 255, i, j-2 );
      rr = r2->paste_alpha_color( rr, 255,255,255, 1, -1 );
    }

    // FIXME: Should skewx line-by-line.
    // Easily fixed in write().
    if( fake_italic )
      rr = rr->skewx( -(rr->ysize()/3), Image.Color.black );

    return rr;
  }

  
  int set_fake_bold( int fb )
  //! The amount of 'boldness' that should be added to the font when
  //! text is rendered. 
  {
    fake_bold = fb;
  }
  
  int set_fake_italic( int(0..1) fi )
  //! If true, make the font italic.
  {
    fake_italic = fi;
  }
  
  array(int) text_extents( string ... lines )
  //! Returns ({ xsize, ysize }) of the image that will result if
  //! @[lines] is sent as the argument to @[write].
  {
    if(!font) open_font();
    if( fake_bold || fake_italic )
    {
      Image.Image i = write( @lines );
      return ({ i->xsize(), i->ysize() });
    }
    return font->text_extents( @lines );
  }

  mapping info()
  //! Returns some information about the font.
  //! Always included:
  //!  file: Filename specified when the font was opened
  //!  size: Size specified when the font was opened
  //!  family: Family name
  //!  style: Bitwise or of the style flags, i.e. @[ITALIC] and @[BOLD].
  {
    if( !font) open_font();
    return ([
      "file":file,
      "size":size,
      "driver":(font->driver||"bitmap"),
      "family":file,
      "line-height":font->height(),
    ]) | (font->info ? font->info() : ([]));
  }
    
  
  Image.Image write( string ... line )
  //! Render the text strings sent as line as an alpha-channel image.
  {
    if(!font) open_font();
    if( codec )
      line = Array.map(line,
		       lambda(string s) {
			 return codec->clear()->feed(s)->drain();
		       });
    return post_process( font->write( @line ) );
  }
}

static mapping fontlist = ([]);
static array font_dirs = ({});

static void rescan_fontlist()
{
  fontlist = ([]);
  foreach( font_dirs, string f )
  {
    catch {
    // Do not recurse.
      foreach( get_dir( f ), string fn )
	catch
	{
	  Font f = Font( f+"/"+fn, 10 );
	  if( mapping n = f->info() )
	  {
	    if( !fontlist[ n->family ] )
	      fontlist[ n->family ]  = ({});
	  
	    fontlist[ n->family ] += ({
	      ([ "style":n->style,
		 "file":n->file,
		 "info":n
	      ]),
	    });
	  }
	};
    };
  }
}
  
Font open_font( string fontname, int size, int flags, int|void force )
//! Find a suitable font in the directories specified with
//! @[set_font_dirs].
//!
//! @[flags] is a bitwise or of 0 or more of @[ITALIC], @[BOLD] and
//! @[NO_FAKE].
//!
//! @[fontname] is the human-readable name of the font.
//! 
//! If @[force] is true, a font is always returned (defaulting to arial
//! or the pike builtin font), even if no suitable font is found.
{
  mapping res;
  int best;
  fontname = lower_case( fontname )-" ";

  foreach( ({ equal, has_prefix }), function cmpf )
  {
    if( res )
      break;
    foreach( indices( fontlist ), string fl )
      if( cmpf( lower_case( fl-" " ), fontname ) ) // match.
	foreach( fontlist[fl], mapping m )
	{
	  if( m->style&(BLACK|ITALIC) == (flags&(BLACK|ITALIC)) )
	    return Font( m->file, size );
	  if( !best )
	  {
	    res = m;
	    best =
	      ((m->style & BLACK) == (flags&BLACK)) +
	      ((m->style & ITALIC) == (flags&ITALIC));
	  }
	  else
	  {
	    int t;
	    if( (t=((m->style & BLACK) == (flags&BLACK)) +
		 ((m->style & ITALIC) == (flags&ITALIC)))
		< best )
	    {
	      best = t;
	      res = m;
	    }
	  }
	}
  }

  if( res )
  {
    Font f = Font( res->file, size );
    if( !(flags & NO_FAKE) )
    {
      if( (flags & BLACK) && !(res->style&BLACK) )
	f->set_fake_bold( (flags & BLACK) - 1 );
      if( (flags&ITALIC) && !(res->style&ITALIC) )
	f->set_fake_italic( 1 );
    }
    return f;
  }

  if( force && (fontname != "arial") )
    return open_font( "arial", size, flags, 1 );

  if( force && (fontname != "fixed") )
    return open_font( "fixed", size, flags );
}

mapping list_fonts( )
//! Returns a list of all the fonts as a mapping.
{
  return copy_value( fontlist );
}

void set_font_dirs( array(string) directories )
//! Set the directories where fonts can be found.
{
  font_dirs = directories;
  rescan_fontlist();
}


void create()
{
#ifdef __NT__
  string root = getenv("SystemRoot");
  if(root)
    root = replace(root, "\\", "/");
  else if(file_stat("C:/WINDOWS/"))
    root = "C:/WINDOWS/";
  else if(file_stat("C:/WINNT/"))
    root = "C:/WINNT/";

  if(root)
    set_font_dirs( ({ combine_path(root, "fonts/")}) );
#else
  array dirs = ({});
  foreach( ({"/usr/openwin/lib/X11/fonts/TrueType/",
	     "/c/windows/fonts","/c/winnt/fonts",
	     "/usr/X11R6/lib/X11/fonts/truetype" }), string d )
    if( file_stat( d ) )
      dirs += ({d});
  if( sizeof( dirs ) )
    set_font_dirs( dirs );
#endif
}
