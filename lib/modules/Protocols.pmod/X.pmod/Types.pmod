/* Types.pmod
 *
 */

#include "error.h"

class XResource
{
  object display;
  int id;
  int autofree = 1;

  void create(object d, int i)
  {
    display = d;
    id = i;

    display->remember_id(id, this_object());
  }

  void Free()
  {
    object req = this_object()->FreeRequest();
    werror("free "+id+"\n");
    req->id = id;
    display->send_request( req );
  }

  void destroy()
  {
    if(autofree) Free();
  }

}

class Font
{
  inherit XResource;

  object QueryTextExtents_req(string str)
  {
    object req = Requests.QueryTextExtents();
    req->font = id;
    req->str = str;
    return req;
  }

  mapping(string:int) QueryTextExtents16(string str)
  {
    object req = QueryTextExtents_req(str);
    return display->blocking_request(req)[1];
  }

  mapping(string:int) QueryTextExtents(string str)
  {
    return QueryTextExtents16("\0"+str/""*"\0");
  }

  object CloseFont_req()
  {
    object req = Requests.CloseFont();
    req->id = id;
    return req;
  }

  void CloseFont()
  {
    object req = CloseFont_req();
    display->send_request(req);
  }

  object CreateGlyphCursor(int sourcechar, array(int)|void foreground,
			   array(int)|void background)
  {
    return display->CreateGlyphCursor(this_object(), sourcechar, 0, 0,
				      foreground, background);
  }

}

class Cursor
{
  inherit XResource;
  constant FreeRequest = Requests.FreeCursor;
}

class Visual
{
  inherit XResource;

  int depth;

  int c_class;
  int bitsPerRGB;
  int colorMapEntries;
  int redMask;
  int greenMask;
  int blueMask;

  void create(mixed ... args)
  {
    ::create(@args);
    autofree=0;
  }
}

class GC
{
  inherit XResource;
  constant FreeRequest = Requests.FreeGC;

  mapping(string:mixed) values;

  object ChangeGC_req(mapping attributes)
  {
    object req = Requests.ChangeGC();
    req->gc = id;
    req->attributes = attributes;
    return req;
  }

  void ChangeGC(mapping attributes)
  {
    object req = ChangeGC_req(attributes);
    display->send_request(req);
    values |= attributes;
  }

  mapping(string:mixed) GetGCValues()
  {
    return values;
  }

  void create(mixed ... args)
  {
    ::create(@args);
    values = mkmapping(_Xlib.gc_attributes,
		       ({ 3, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
			  0, 0, 1, 0, 0, 0, 0, 4, 1 }));
  }
}

class Rectangle
{
  int x, y;
  int width, height;

  void create(int ... args)
  {
    switch(sizeof(args))
      {
      case 0:
	x = y = width = height = 0;
	break;
      case 4:
	x = args[0];
	y = args[1];
	width = args[2];
	height = args[3];
	break;
      default:
	error("Types.Rectangle(): To many arguments.\n");
      }
  }

  string to_string()
  {
    return sprintf("%2c%2c%2c%2c", x, y, width, height);
  }
}

class Point
{
  int x, y;

  void create(int ... args)
  {
    switch(sizeof(args))
      {
      case 0:
	x = y = 0;
	break;
      case 2:
	x = args[0];
	y = args[1];
	break;
      default:
	error("Types.Point(): To many arguments.\n");
      }
  }

  string to_string()
  {
    return sprintf("%2c%2c", x, y);
  }
}

class Colormap
{
  inherit XResource;
  object visual;
  constant FreeRequest = Requests.FreeColormap;
  mapping alloced = ([]);

  object FreeColors_req(int pixel)
  {
    object req = Requests.FreeColors();
    req->colormap = id;
    req->plane_mask=0;
    req->colors = ({ pixel });
    return req;
  }

  void FreeColor( int pixel )
  {
    display->send_request( FreeColors_req( pixel ) );
  }

  void Free()
  {
    foreach(values(alloced), mapping f) FreeColor( f->pixel );
    ::Free();
  }

    

  int AllocColor(int r, int g, int b)
  {
    if(alloced[sprintf("%2c%2c%2c", r, g, b)])
      return alloced[sprintf("%2c%2c%2c", r, g, b)];
    object req = Requests.AllocColor();
    req->colormap = id;
    req->red = r;
    req->green = g;
    req->blue = b;

    array a = display->blocking_request( req );
    return a[0] && (alloced[sprintf("%2c%2c%2c", r, g, b)]=a[1]);
  }


  void create(object disp, int i, object vis)
  {
    display = disp;
    id = i;
    visual = vis;
  }
}


class Drawable
{
  inherit XResource;
  int depth;
  object colormap, parent, visual;

  object CreateGC_req()
  {
    object req = Requests.CreateGC();

    req->drawable = id;
    req->gc = display->alloc_id();

    return req;
  }

  object CreateGC(mapping|void attributes)
  {
    object req = CreateGC_req();
    if (attributes)
      req->attributes = attributes;
    display->send_request(req);

    return GC(display, req->gc);
  }

  object CreatePixmap_req(int width, int height, int depth)
  {
    object req = Requests.CreatePixmap();

    req->pid = display->alloc_id();
    req->drawable = id;
    req->width = width;
    req->height = height;
    req->depth = depth;
    return req;
  }

  object CreatePixmap(int width, int height, int depth)
  {
    object req = CreatePixmap_req(width, height, depth);
    display->send_request( req );
    object p = Pixmap(display, req->pid, this_object(), colormap );
    p->depth = depth;
    return p;
  }

  object FillPoly_req(int gc, int shape, int coordMode, array(object) p)
  {
    object req = Requests.FillPoly();
    req->drawable = id;
    req->gc = gc;
    req->shape = shape;
    req->coordMode = coordMode;
    req->points = p;
    return req;
  }

  void FillPoly(object gc, int shape, int coordMode, array(object) points)
  {
    object req = FillPoly_req(gc->id, shape, coordMode, points);
    display->send_request(req);
  }

  object FillRectangles_req(int gc, array(object) r)
  {
    object req = Requests.PolyFillRectangle();
    req->drawable = id;
    req->gc = gc;
    req->rectangles = r;
    return req;
  }

  void FillRectangles(object gc, object ... r)
  {
    object req = FillRectangles_req(gc->id, r);
    display->send_request(req);
  }

  object DrawLine_req(int gc, int coordMode, array(object) points)
  {
    object req = Requests.PolyLine();
    req->drawable = id;
    req->gc = gc;
    req->coordMode = coordMode;
    req->points = points;
    return req;
  }

  void DrawLine(object gc, int coordMode, object ... points)
  {
    display->send_request(DrawLine_req(gc->id, coordMode, points));
  }

  object PutImage_req(object gc, int depth,
		      int tx, int ty, int width, int height, string data)
  {
    object r = Requests.PutImage();
    r->drawable = id;
    r->gc = gc->id;
    r->depth = depth;
    r->dst_x = tx;
    r->dst_y = ty;
    r->width = width;
    r->height = height;
    r->data = data;

    return r;
  }
  
  void PutImage(object gc, int depth,
		int tx, int ty, int width, int height, string data,
		int|void format)
  {
    object req = PutImage_req(gc, depth, tx, ty, width, height, data);
    req->format = format;
    display->send_request( req );
  }

  object ImageText8_req(int gc, int x, int y, string str)
  {
    object req = Requests.ImageText8();
    req->drawable = id;
    req->gc = gc;
    req->x = x;
    req->y = y;
    req->str = str;
    return req;
  }

  void ImageText8(object gc, int x, int y, string str)
  {
    if(sizeof(str)>255)
      error("ImageText8: string to long\n");
    object req = ImageText8_req(gc->id, x, y, str);
    display->send_request(req);    
  }

  object ImageText16_req(int gc, int x, int y, string str)
  {
    object req = Requests.ImageText16();
    req->drawable = id;
    req->gc = gc;
    req->x = x;
    req->y = y;
    req->str = str;
    return req;
  }

  void ImageText16(object gc, int x, int y, string str)
  {
    if(sizeof(str)>510)
      error("ImageText16: string to long\n");
    object req = ImageText16_req(gc->id, x, y, str);
    display->send_request(req);    
  }
}

class Pixmap
{
  inherit Drawable;
  constant FreeRequest = Requests.FreePixmap;

  // Init function.
  void create(mixed ... args)
  {
    ::create( @args );
    if(sizeof(args)>2 && objectp(args[2]))  parent = args[2];
    if(sizeof(args)>3 && objectp(args[3]))  colormap = args[3];
  }
}

class Window
{
  inherit Drawable;
  int currentInputMask;
  constant FreeRequest = lambda(){}; // FIXME!!
  mapping(string:array(function)) event_callbacks = ([ ]);

  int alt_gr, num_lock, shift, control, caps_lock;
  int meta, alt, super, hyper;

  mapping attributes;

#include "keysyms.h"

  void ReleaseKey( int sym )
  {
    switch(sym)
    {
     case XK_Scroll_Lock: return 0; break;
     case XK_Mode_switch:  alt_gr = 0; return;
     case XK_Num_Lock:    num_lock = 0; return;
     case XK_Shift_L:
     case XK_Shift_Lock:
     case XK_Shift_R:     shift=0; return;
     case XK_Control_L: case XK_Control_R:   control=0; return;
     case XK_Caps_Lock:  caps_lock=0; return;

     case XK_Meta_L: case XK_Meta_R:   meta=0; return;
     case XK_Alt_L:  case XK_Alt_R:   alt=0; return;
     case XK_Super_L:case XK_Super_R:   super=0; return;
     case XK_Hyper_L: case XK_Hyper_R:   hyper=0; return;
    }
  }

  string _LookupKeysym( int keysym )
  {
    switch(keysym)
    {
     case XK_BackSpace:   return "";
     case XK_Tab:         return "\t";
     case XK_Linefeed:    return "\n";
     case XK_Return:      return "\n";
     case XK_Escape:      return "";
     case XK_Delete:      return "";
       /* Kanji here ... */
       /* Korean here ... */
       /* Cursors ... */
     case XK_KP_Space:       return " ";
     case XK_KP_Tab:         return "\t";
     case XK_KP_Equal:       return "=";
     case XK_KP_Multiply:    return "*";
     case XK_KP_Add:         return "+";
     case XK_KP_Subtract:    return "-";
     case XK_KP_Decimal:     return ".";
     case XK_KP_Divide:      return "/";

     case XK_KP_0:      return "0";
     case XK_KP_1:      return "1";
     case XK_KP_2:      return "2";
     case XK_KP_3:      return "3";
     case XK_KP_4:      return "4";
     case XK_KP_5:      return "5";
     case XK_KP_6:      return "6";
     case XK_KP_7:      return "7";
     case XK_KP_8:      return "8";
     case XK_KP_9:      return "9";

     case XK_space..XK_at:     return sprintf("%c", keysym);
     case XK_A..XK_Z: 
     case XK_a..XK_z:  
       keysym = keysym&0xdf;
       if(control) return sprintf("%c", (keysym-'A')+1);
       if(shift || caps_lock) return sprintf("%c",keysym);
       return sprintf("%c",keysym+0x20);

     default:
       if(keysym < 256)
	 return sprintf("%c", keysym);
       return 0;
       /*Latin2 .. latin3 .. latin4 .. katakana .. arabic .. cyrillic ..
	 greek .. technical .. special .. publishing .. APL .. hebrew ..
	 thai .. korean .. hangul .. */
    }
  } 
  mapping compose_patterns;
  string compose_state = "";
  string LookupKeysym( int keysym )
  {
    if(!compose_patterns) compose_patterns =  display->compose_patterns;
    switch(keysym)
    {
     case XK_A..XK_Z: 
     case XK_a..XK_z:
       keysym = keysym&0xdf;      // Upper..
       if(!shift && !caps_lock)  //
	 keysym=keysym+0x20;    // .. and lower ..
       break;

     case XK_Mode_switch:  alt_gr = 1; return 0;
     case XK_Num_Lock:    num_lock = 1; return 0;
     case XK_Shift_L:
     case XK_Shift_Lock:
     case XK_Shift_R:     shift=1; return 0;
     case XK_Control_L:
     case XK_Control_R:   control=1; return 0;
     case XK_Caps_Lock:  caps_lock=1; return 0;
     case XK_Meta_L:
     case XK_Meta_R:   meta=1; return 0;
     case XK_Alt_L:
     case XK_Alt_R:   alt=1; return 0;
     case XK_Super_L:
     case XK_Super_R:   super=1; return 0;
     case XK_Hyper_L:
     case XK_Hyper_R:   hyper=1; return 0;
    }

    compose_state += sprintf("%4c", keysym);

    if(arrayp(compose_patterns[compose_state]))  return 0; // More to come..

    if(compose_patterns[compose_state])
    {
      keysym = compose_patterns[compose_state];
      compose_state="";
      return _LookupKeysym( keysym );
    }
    if(strlen(compose_state)>4)
    {
      string res="";
      while(strlen(compose_state)
	    && (sscanf(compose_state, "%4c%s", keysym, compose_state)==2))
	res += _LookupKeysym( keysym ) || "";
      return strlen(res)?res:0;
    }
    compose_state="";
    return _LookupKeysym( keysym );
  }

  string handle_keys(mapping evnt)
  {
    int code=evnt->detail, keysym;
    evnt->keycode = code;

    array keysymopts = display->key_mapping[ code ];
    keysym = keysymopts[ alt_gr*2+shift ];

    if( keysym )
    {
      if(evnt->type == "KeyPress")
	evnt->data = LookupKeysym( keysym );
      else
	foreach(keysymopts, int k)
	  ReleaseKey( k );
      evnt->keysym = keysym;
    }
  }
  
  object CreateWindow_req(int x, int y, int width, int height,
			  int border_width, int depth, object visual)
  {
    object req = Requests.CreateWindow();
    req->depth = depth;  /* CopyFromParent */
    req->wid = display->alloc_id();
    req->parent = id;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->borderWidth = border_width;
    req->c_class = 0 ;  /* CopyFromParent */
    req->visual = visual && visual->id;
    return req;
  }

  object new(mixed ... args) /* Kludge */
  {
    return object_program(this_object())(@args);
  }

  object CreateColormap(object visual, int|void alloc)
  {
    object req = Requests.CreateColormap();
    req->cid = display->alloc_id();
    req->alloc = alloc;
    req->visual = visual->id;
    req->window = id;
    display->send_request(req);
    return Colormap(display, req->cid, visual);
  }

  object CreateWindow(int x, int y, int width, int height,
		      int border_width, mapping attributes,
		      object|void visual, int|void d,
		      int|void c_class)
  {
    object req = CreateWindow_req(x, y, width, height,
				  border_width,d,visual);
    object c = colormap||parent->defaultColorMap;

    if (attributes)
    {
      req->attributes = attributes;
      if(attributes->Colormap) c= attributes->Colormap;
    }    

    // object w = Window(display, req->wid);
    display->send_request(req);
    object w = new(display, req->wid, visual, this_object());
    w->depth = d||depth||this_object()->rootDepth;
    w->colormap = c;
    w->currentInputMask = req->attributes->EventMask;
    w->attributes = attributes;
    return w;
  }
    
  object CreateSimpleWindow(int x, int y, int width, int height,
			    int border_width,int border, int background)
  {
    object req = CreateWindow_req(x, y, width, height,
				  border_width,0,0);
    req->attributes->BackPixel = background;
    req->attributes->BorderPixel = border;

    display->send_request(req);
    
    // object w = Window(display, req->wid);
    object w = new(display, req->wid, 0, this_object());
    
    w->visual = visual;
    w->currentInputMask = 0;
    return w;
  }

  object ChangeAttributes_req(mapping m)
  {
    object req = Requests.ChangeWindowAttributes();
    req->window = id;
    req->attributes = m;
    return req;
  }

  void ChangeAttributes(mapping m)
  {
    attributes |= m;
    display->send_request(ChangeAttributes_req(m));
  }

  object Configure_req(mapping m)
  {
    object req = Requests.ConfigureWindow();
    req->window = id;
    req->attributes = m;
    return req;
  }

  void Configure(mapping m)
  {
    display->send_request(Configure_req(m));
  }

  void Raise()
  {
    Configure((["StackMode":0]));
  }

  void Lower()
  {
    Configure((["StackMode":1]));
  }

  void set_event_callback(string type, function f)
  {
    event_callbacks[type] = (event_callbacks[type] || ({ }) )
      + ({ f });
  }
  
  object SelectInput_req()
  {
    object req = Requests.ChangeWindowAttributes();
    req->window = id;
    req->attributes->EventMask = currentInputMask;
    attributes->EventMask = currentInputMask;
    return req;
  }

  int event_mask_to_int(array(string) masks)
  {
    int mask = 0;
    
    foreach(masks, string type)
      mask
	|= _Xlib.event_masks[type];
    return mask;
  }

  int SelectInput(string ... types)
  {
    // int old_mask = currentInputMask;
    currentInputMask |= event_mask_to_int(types);
    display->send_request(SelectInput_req());
    return currentInputMask;
  }

  int DeselectInput(string ... types)
  {
    // int old_mask = currentInputMask;
    currentInputMask &= ~event_mask_to_int(types);

    display->send_request(SelectInput_req());
    return currentInputMask;
  }

  object GrabButton_req(int button, int modifiers, array(string) types)
  {
    object req = Requests.GrabButton();
    req->ownerEvents = 1;
    req->grabWindow = id;
    req->eventMask = event_mask_to_int(types);
    // req->pointerMode = 1;
    // req->keyboardMode = 1;
    req->button = button;
    req->modifiers = modifiers || 0x8000;
    
    return req;
  }

  void GrabButton(int button, int modifiers, string ... types)
  {
    object req = GrabButton_req(button, modifiers, types);
    display->send_request(req);
  }

  object Map_req()
  {
    object req = Requests.MapWindow();
    req->id = id;
    return req;
  }

  void Map()
  {
    display->send_request(Map_req());
  }

  object Unmap_req()
  {
    object req = Requests.UnmapWindow();
    req->id = id;
    return req;
  }

  void Unmap()
  {
    display->send_request(Unmap_req());
  }

  object ListProperties_req()
  {
    object req = Requests.ListProperties();
    req->window = id;
    return req;
  }

  array ListProperties()
  {
    object req = ListProperties_req();
    array a = display->blocking_request(req);

    if (!a[0])
      return 0;
    
    a = a[1];
    for(int i = 0; i<sizeof(a); i++)
      {
	object atom = display->lookup_atom(a[i]);
	if (!atom)
	  return 0;
	a[i] = atom;
      }
    return a;
  }

  object ChangeProperty_req(object property, object type,
			    int format, array(int)|string data)
  {
    object req = Requests.ChangeProperty();
    req->window = id;
    req->property = property->id;
    req->type = type->id;
    req->format = format;
    req->data = data;

    return req;
  }

  void ChangeProperty(object property, object type,
		      int format, array(int)|string data)
  {
    display->send_request(ChangeProperty_req(property, type, format, data));
  }
  
  object GetProperty_req(object property, object|void type)
  {
    object req = Requests.GetProperty();
    req->window = id;
    req->property = property->id;
    if (type)
      req->type = type->id;
    
    return req;
  }

  mapping GetProperty(object property, object|void type)
  {
    object req = GetProperty_req(property, type);
    array a = display->blocking_request(req);

    if (!a[0])
      return 0;

    mapping m = a[1];
    if (type && (m->type == type->id))
      m->type = type;
    else
      m->type = display->lookup_atom(m->type);
    return m;
  }

  object ClearArea_req(int x, int y, int width, int height, int exposures)
  {
    object req = Requests.ClearArea();
    req->window = id;
    req->exposures = exposures;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;

    return req;
  }

  void ClearArea(int x, int y, int width, int height, int|void exposures)
  {
    display->send_request(ClearArea_req(x, y, width, height, exposures));
  }
  
  // Shape extension
  void ShapeRectangles( string kind, int xo, int yo, string operation,
			array (object(Rectangle)) rectangles )
  {
    if(kind == "both")
    {
      ShapeRectangles( "clip", xo, yo, operation, rectangles);
      ShapeRectangles( "bounding", xo, yo, operation, rectangles);
      return;
    }
    if(!display->extensions["SHAPE"])
      error("No shape extension available.\n");
    display->extensions["SHAPE"]->
      ShapeRectangles( this_object(), xo, yo, kind, operation, rectangles );
  }

  void ShapeMask( string kind, int xo, int yo, string operation,
		  object (Pixmap) mask )
  {
    if(kind == "both")
    {
      ShapeMask( "clip", xo, yo, operation, mask);
      ShapeMask( "bounding", xo, yo, operation, mask);
      return;
    }
    if(!display->extensions["SHAPE"])
      error("No shape extension available.\n");
    display->extensions["SHAPE"]->
      ShapeMask( this_object(), xo, yo, kind, operation, mask );
  }

  void ShapeOffset( string kind, int xo, int yo )
  {
    if(kind == "both")
    {
      ShapeOffset( "clip", xo, yo );
      ShapeOffset( "bounding", xo, yo );
      return;
    }
    if(!display->extensions["SHAPE"])
      error("No shape extension available.\n");
    display->extensions["SHAPE"]->ShapeOffset( this_object(), kind, xo, yo );
  }


  // Init function.
  void create(mixed ... args)
  {
    ::create( @args );
    if(sizeof(args)>2 && objectp(args[2]))  visual = args[2];
    if(sizeof(args)>3 && objectp(args[3]))  parent = args[3];
    set_event_callback("_KeyPress", handle_keys);
    set_event_callback("_KeyRelease", handle_keys);
  }
}

class RootWindow
{
  inherit Window;

  object defaultColorMap;
  int whitePixel;
  int blackPixel;
  int pixWidth;
  int pixHeight;
  int mmWidth;
  int mmHeight;
  int minInstalledMaps;
  int maxInstalledMaps;
  int backingStore;
  int saveUnders;
  int rootDepth;
  mapping depths;

  void create(mixed ... args)
  {
    ::create(@args);
    autofree=0;
  }
  
  object new(mixed ... args) /* Kludge */
  {
    return Window(@args);
  }
}


