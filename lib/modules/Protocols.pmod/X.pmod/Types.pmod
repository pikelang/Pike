/* Types.pmod
 *
 */

#include "error.h"

class XResource
{
  object display;
  int id;

  void create(object d, int i)
  {
    display = d;
    id = i;

    display->remember_id(id, this_object());
  }
}

class Visual
{
  inherit XResource;

  int c_class;
  int bitsPerRGB;
  int colorMapEntries;
  int redMask;
  int greenMask;
  int blueMask;
}

class GC
{
  inherit XResource;

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

class Drawable
{
  inherit XResource;

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
}
  
class Window
{
  inherit Drawable;
  object visual;
  int currentInputMask;

  mapping(string:function) event_callbacks = ([ ]);
  
  object CreateWindow_req(int x, int y, int width, int height,
			  int border_width)
  {
    object req = Requests.CreateWindow();
    req->depth = 0;  /* CopyFromParent */
    req->wid = display->alloc_id();
    req->parent = id;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->borderWidth = border_width;
    req->c_class = 0 ;  /* CopyFromParent */
    req->visual = 0;    /* CopyFromParent */
    return req;
  }

  object new(mixed ... args) /* Kludge */
  {
    return object_program(this_object())(@args);
  }

  /* FIXME: Supports only visual CopyFromParent */
  object CreateWindow(int x, int y, int width, int height,
		      int border_width, mapping attributes)
  {
    object req = CreateWindow_req(x, y, width, height,
				  border_width);
    if (attributes)
      req->attributes = attributes;
    display->send_request(req);
    
    // object w = Window(display, req->wid);
    object w = new(display, req->wid);
    
    w->visual = visual;
    w->currentInputMask = req->attributes->EventMask;
    return w;
  }
    
  object CreateSimpleWindow(int x, int y, int width, int height,
			    int border_width,
			    int border, int background)
  {
    object req = CreateWindow_req(x, y, width, height,
				  border_width);
    req->attributes->BackPixel = background;
    req->attributes->BorderPixel = border;

    display->send_request(req);
    
    // object w = Window(display, req->wid);
    object w = new(display, req->wid);
    
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


  void set_event_callback(string type, function f)
  {
    event_callbacks[type] = f;
  }

  function remove_event_callback(string type)
  {
    function f = event_callbacks[type];
    m_delete(event_callbacks, type);
  }

  object SelectInput_req()
  {
    object req = Requests.ChangeWindowAttributes();
    req->window = id;
    req->attributes->EventMask = currentInputMask;
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
}

class Colormap
{
  inherit XResource;
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
  
  object new(mixed ... args) /* Kludge */
  {
    return Window(@args);
  }
}


