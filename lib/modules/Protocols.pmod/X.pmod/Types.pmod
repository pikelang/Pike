/* Types.pmod
 *
 */

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
	      
class Window
{
  inherit XResource;
  object visual;
  int currentInputMask;

  mapping(string:function) event_callbacks = ([ ]);
  
  object CreateSimpleWindow_req(int x, int y, int width, int height,
			 int border_width, int border, int background)
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
    req->attributes->background_pixel = background;
    req->attributes->border_pixel = border;
    return req;
  }

  object new(mixed ... args) /* Kludge */
  {
    return object_program(this_object())(@args);
  }
  
  object CreateSimpleWindow(int ... args)
  {
    object req = CreateSimpleWindow_req(@args);
    display->send_request(req);
    
    // object w = Window(display, req->wid);
    object w = new(display, req->wid);
    
    w->visual = visual;
    w->currentInputMask = 0;
    return w;
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
    req->attributes->event_mask = currentInputMask;
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


