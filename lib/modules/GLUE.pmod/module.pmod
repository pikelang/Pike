//
// $Id: module.pmod,v 1.5 2004/04/14 20:20:02 nilsson Exp $

#pike __REAL_VERSION__
#if constant(GL)

//! GL Universal Environment

import GL;


// --- Driver related code

static .Driver.Interface driver;

#if !constant(GL.GL_TEXTURE_RECTANGLE_NV)
# define GL_TEXTURE_RECTANGLE_NV 0x864E
#endif

static constant drivers = ({ "SDL", "GTK" });

//! Returns the name of the available drivers.
//! @seealso
//! @[init]
array(string) get_drivers() {
  return drivers;
}


// --- Callback handling

static function(.Events.Event:void) event_callback;
static function(float,int(0..1),float:void) resize_callback;

static array reinit_callbacks = ({});

//! Add a callback that will be called every time the resolution
//! is about to change.
//! @seealso
//! @[remove_reinit_callback]
void add_reinit_callback( function(void:void) f ) {
  if(!f) return;
  reinit_callbacks |= ({ f });
}

//! Removes a reinitialization callback.
//! @seealso
//! @[add_reinit_callback]
void remove_reinit_callback( function(void:void) f ) {
  reinit_callbacks -= ({ f });
}

// --- GL extension related code
static multiset extensions;
int(0..1) has_extension( string ext )
{
  if( !extensions )
    extensions = (multiset)(glGetString( GL_EXTENSIONS )/" ");
  werror("%O\n", extensions );
  return extensions[ ext ];
}



// --- Drawing area related code

static int(0..1) fullscreen = 1;
static array(int) resolution = ({ 800, 600 });
static float aspect = 4/3.0;
static int gl_flags = 0;
static int depth = 32;
static float global_z_rotation;
static int(0..1) mirrorx, mirrory;

static void pre_modeswitch() {
  reinit_callbacks();
}

static void modeswitch()
{
  glEnable( GL_TEXTURE_2D );
  glEnable( GL_BLEND );
  glDisable( GL_CULL_FACE );
  glDisable( GL_DEPTH_TEST );
  glTexEnv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glShadeModel( GL_FLAT );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

#ifdef __NT__
  if(sizeof(all_textures))
    foreach( sort(indices(all_textures)), object o )
     o->modeswitch();

  if(sizeof(all_lists))
    foreach( sort(indices(all_lists)), object o )
      if(o->modeswitch) 
	o->modeswitch();
      else 
      {
	all_lists[o]=0;
	destruct(o);
      }
#endif // __NT__
}

// width==height==0 means that no real change has been done.
// hidden==1 means that the window is hidden.
void configure_event( int(0..) width, int(0..) height,
		      int(0..1)|void hidden )
{
  int noactual;
  if( !width && !height )
  {
    [width,height] = resolution;
    noactual=1;
  }
  float oa = resolution[0] / (float)resolution[1];
  float na = width / (float)height;
  resolution = ({ width, height });
  aspect = (na / oa) * aspect;
  if( !noactual )
    modeswitch();
  glViewport( 0,0, width, height );
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if( mirrorx )   glScale( -1.0, 1.0, 1.0 );
  if( mirrory )   glScale( 1.0, -1.0, 1.0 );

  float l_aspect = aspect;
  switch( global_z_rotation )
  {
    case 45.0 .. 145.0:
    case 210.0..275.0:
      l_aspect = 1.0/l_aspect;
      break;
  }
  if( global_z_rotation )
    glRotate( global_z_rotation, 0.0, 0.0, 1.0 );

  resize_callback( l_aspect, hidden, global_z_rotation );
  glScale( 1.0/l_aspect, -1.0, 1.0 );
  glMatrixMode(GL_MODELVIEW);
  driver->flush();
}


//! Returns 1 if in fullscreen mode, otherwise 0.
//! @seealso
//! @[toggle_fullscreen]
int(0..1) get_screen_mode() { return fullscreen; }

//! Toggles between fullscreen and window mode.
//! If a screen mode is provided, that mode will be assumed.
//! @seealso
//! @[get_screen_mode]
void toggle_fullscreen(void|int(0..1) _fullscreen) {
  if(query_num_arg())
    fullscreen = !!_fullscreen;
  else
    fullscreen ^= 1;
  set_mode();
}

//! Returns the GL flags currently used.
//! @seealso
//! @[set_gl_flags]
int get_gl_flags() { return gl_flags; }

//! Sets the GL flags.
//! @seealso
//! @[get_gl_flags]
void set_gl_flags(int _gl_flags) {
  gl_flags = _gl_flags;
  set_mode();
}

//! Returns the current color depth.
//! @seealso
//! @[set_depth]
int get_depth() { return depth; }

//! Sets the color depth.
//! @seealso
//! @[get_depth]
void set_depth(int _depth) {
  depth = _depth;
  set_mode();
}

static void set_mode() {
  // FIXME: Run pre_modeswitch here?
  call_out( lambda() {
    driver->set_mode( fullscreen, depth, @resolution, gl_flags );
    // set_mode calls configure_event
  }, 0.0 );
}

//! Returns the screen width/height.
//! @seealso
//! @[set_resolution]
int xsize() { return resolution[0]; }
int ysize() { return resolution[1]; }

//! Sets the resolution to @[w]x@[h] pixels.
//! @seealso
//! @[xsize], @[ysize]
void set_resolution( int w, int h )
{
  resolution = ({ w, h });
  call_out( lambda() {
    pre_modeswitch();
    driver->set_resolution( @resolution );
    // set_resolution calls configure_event.
  }, 0.0 );
}

//! Returns the screen aspect.
//! @seealso
//! @[set_aspect]
float get_aspect() { return aspect; }

//! @decl void set_aspect(float asp)
//! @decl void set_aspect(int w, int h)
//! Set the aspect of the draw area. Does nothing if the provided aspect
//! is equal to the one currently used.
//! @seealso
//! @[get_aspect]
void set_aspect( float|int w, void|int h )
{
  if(intp(w)) {
    if(!h) error("set_aspect(int,int) or set_aspect(float)\n");
    w /= (float)h;
  }
  if( aspect==w ) return;
  aspect = w;
  if( h != -1 )
    configure_event( @resolution );
}

//! Rotates the drawing area @[deg] degrees. Useful e.g. when drawing
//! for tilted monitors.
void set_screen_rotation( float deg )
{
  global_z_rotation = deg;
}

//! Mirrors the screen in x and/or y axis. Useful e.g. when drawing
//! for backlight projection.
//! @param how
//! A string that contains the mirror axis, e.g. @expr{"x"@} or
//! @expr{"xy"@}.
void mirror_screen( string how )
{
  mirrorx = mirrory = 0;
  if( has_value( how, "x" ) )
    mirrorx = 1;
  if( has_value( how, "y" ) )
    mirrory = 1;
}


// --- Init code

//! Initializes GLUE and loads a driver from a list of drivers.
//! If a driver fails to load or initialize, the next driver
//! is tried.
//! @throws
//! @[driver_names] not listed in the result from @[get_drivers]
//! will cause an error to be thrown.
//! @param options
//!   @mapping
//!     @member string|array(string) "driver_names"
//!       The name of a driver or a list of drivers to try, in given
//!       order. If no driver name is given, the list given by
//!       @[get_drivers] is used.
//!     @member function(.Events.Event:void) "event_callback"
//!       This callback is called with a @[Events.Event] object
//!       whenever an event is trapped by the driver.
//!     @member function(float,int(0..1):void) "resize_callback"
//!       This callback is called with the aspect whenever the drawing
//!       area is resized, either by an event or explicitly by the program.
//!     @member int(0..1) "fullscreen"
//!       Set fullscreen/window mode. 1 is fullscreen, 0 is window.
//!       Defaults to fullscreen.
//!     @member array(int) "resolution"
//!       Sets the resolution of the drawing area. Defaults to
//!       ({ 800, 600 }).
//!     @member float "aspect"
//!       Sets the aspect of the drawing area. Defaults to 1.333333
//!       (4:3).
//!     @member int "depth"
//!       Sets the color depth of the drawing area. Defaults to 32.
//!     @member string "title"
//!       Sets the window title to this string.
//!     @member string "icon_title"
//!       Sets the icon title to this string.
//!     @member int(0..1) "fast_mipmap"
//!       Use GL_NEAREST_MIMAP_NEAREST instead of GL_LINEAR_MIPMAP_LINEAR,
//!       which also is the default.
//!     @member float "rotation"
//!       The rotation in z-axis of the drawing field.
//!     @member string "mirror"
//!       Mirroring in x and/or y axis.
//!   @endmapping
//! @seealso
//! @[get_drivers]
void init(void|mapping(string:mixed) options) {

  if(!options)
    options = ([]);

  if(options->event_callback)
    event_callback = options->event_callback;
  else
    event_callback = lambda(.Events.Event evt){
		       if(evt->name=="Exit") exit(0);
		     };
  if(options->resize_callback)
    resize_callback = options->resize_callback;
  else
    resize_callback = lambda(){};

  start_driver(options->driver_names, options->title, options->icon_title);

  if( !zero_type(options->fullscreen) ) {
    fullscreen = options->fullscreen;
    if( !(< 0,1 >)[fullscreen] )
      error("options->fullscreen should be 1 or 0.\n");
  }
  if( options->resolution ) {
    if(!arrayp(options->resolution) || sizeof(options->resolution)!=2)
      error("options->resolution should be an array of size two.\n");
    resolution = options->resolution;
  }
  if( options->aspect ) {
    if(!floatp(options->aspect))
      error("options->aspect should be a float.\n");
    aspect = options->aspect;
  }
  if( options->depth ) {
    if(!intp(options->depth))
      error("options->depth should be an integer.\n");
    depth = options->depth;
  }
  if( options->fast_mipmap ) {
    fast_mipmap = options->fast_mipmap;
    if( !(< 0,1 >)[fast_mipmap] )
      error("options->fast_mipmap should be 1 or 0.\n");
  }
  // FIXME: gl_flags
  if( options->rotation ) {
    if(!floatp(options->rotation))
      error("options->rotation should be a float.\n");
    global_z_rotation = options->rotation;
  }
  if( options->mirror ) {
    if(!stringp(options->mirror))
      error("options->mirror should be string.\n");
    mirror_screen( options->mirror );
  }

  toggle_fullscreen(fullscreen);
}

static void start_driver(string|array(string)|
			 object(.Driver.Interface) driver_names,
			 string title, string icon_title) {
  if (objectp(driver_names)) {
    driver=driver_names;
    if (driver->setup)
      driver->setup(event_callback, configure_event);
    if (driver->init)
      driver->init(title,icon_title);
    if (driver->exit)
      atexit(driver->exit);
    return;
  }

  if(!driver_names)
    driver_names = get_drivers();
  else if(stringp(driver_names))
    driver_names = ({ driver_names });

  foreach(driver_names, string name) {
    if(!has_value(drivers, name))
      error("Driver %O is not available.\n");
    catch {
      driver = .Driver[name](event_callback, configure_event);
      if(driver->init)
	driver->init(title, icon_title);
      if(driver->exit)
	atexit(driver->exit);
      return;
    };
  }
  error("Failed to load any of the specified drivers.\n");
}


// --- Driver proxies

//! Swap the drawing buffer and the viewing buffer.
void swap_buffers() {
  driver->swap_buffers();
}

//! Show the mouse cursor.
void show_cursor() {
  driver->show_cursor();
}

//! Hide the mouse cursor.
void hide_cursor() {
  driver->hide_cursor();
}


// --- ID generation

static class IDGenerator {
  static int next;
  static array(int) reuse = ({});
  static int max_id;

  static void create(void|int start_id, void|int max) {
    next = start_id;
    max_id = max;
    if (max>start_id)
      max_id = start_id;
  }

  int get() {
    if(sizeof(reuse)) {
      int ret = reuse[0];
      reuse = reuse[1..];
      return ret;
    }
    if(!max_id || next<max_id)
      return next++;
    error("Out of unallocated id:s.\n");
  }

  void free(int id) {
    reuse += ({ id });
  }
}

#if constant( GL.glDeleteLists ) && constant( GL.glGenLists )
class ListIDGenerator
{
  int get()         { return GL.glGenLists(1); }
  void free(int id) { GL.glDeleteLists(id,1);  }
}
#else
#define ListIDGenerator IDGenerator
#endif

#if constant( GL.glDeleteTextures ) && constant( GL.glGenTextures )
class TextureIDGenerator
{
  int get()          { return GL.glGenTextures(1)[0];  }
  void free(int id)  { GL.glDeleteTextures(id);        }
}
#else
#define TextureIDGenerator IDGenerator
#endif

static IDGenerator texture_ids = TextureIDGenerator();
static IDGenerator list_ids    = ListIDGenerator();
#ifdef __NT__
static IDGenerator light_ids   = IDGenerator(0,7);
#else
static IDGenerator light_ids   = IDGenerator(0,glGet( GL_MAX_LIGHTS )-1);
#endif

//! Allocate a hardwareaccelerated lightsource from OpenGL.
//! @returns
//! an id which may be added to the GL.GL_LIGHT0 constant.
//! @seealso
//! @[free_light]
int allocate_light() {
  return light_ids->get();
}

//! Call this function to free a lightsource that has been allocated with
//! @[allocate_light].
//! @param l
//! Id which has been allocated using @[allocate_light].
//! @seealso
//! @[allocate_light]
void free_light(int l) {
  light_ids->free(l);
}

// --- GL PushPop

static int(0..) push_depth;

//! Returns the PushPop depth, i.e. the number of pushes awaiting
//! corresponding pops.
int(0..) pushpop_depth() {
  return push_depth;
}

//! Performs function @[f] between @[GL.glPushMatrix] and
//! @[GL.glPopMatrix] calls.
//! @example
//!   PushPop() {
//!     GL.glTranslate( 0.01, -0.9, 0.0 );
//!     write_text( "Press esc to quit" );
//!   };
void PushPop( function f ) {
  GL.glPushMatrix();
  push_depth++;
  mixed err = catch(f());
  GL.glPopMatrix();
  push_depth--;
  if( err )
    werror("Error in PushPop: %s\n",
	   describe_backtrace( err ) );
}


// --- GL Lists

#ifdef __NT__
static multiset(List) all_lists = set_weak_flag( (<>), Pike.WEAK );
#endif

//! A display list abstraction. Automatically allocates a display list
//! id upon creation and correctly deallocate it upon destruction.
//! @seealso
//!   @[DynList]
class List {
  static int id;

  //! When creating a new list, the list code can be compiled upon
  //! creation by supplying a function @[f] that performs the GL
  //! operations.
  //! @seealso
  //!   @[call]
  //! @example
  //! List list = List() {
  //!   // GL code
  //! };
  static void create( void|function f ) {
    id = list_ids->get();
#ifdef __NT__
    all_lists[this] = 1;
#endif
    if(f) compile(f);
  }

  //! Deletes this list and frees the list id from the id pool.
  static void destroy() {
#ifdef __NT__
    all_lists[this] = 0;
#endif
    GL.glDeleteLists(id, 1);
    list_ids->free(id);
  }

  //! Returns this lists' id.
  int get_id() { return id; }

  static int __hash() { return id; }

  //! @[List] objects can be sorted according to list id.
  //! @seealso
  //!   @[get_id]
  static int(0..1) `>(mixed x) {
    if(!objectp(x) || !x->get_id) return 1;
    return (int)id > (int)x->get_id();
  }

  //! Start defining the list. If @[run] is provided, the list will
  //! be executed as it is compiled (@[GL.GL_COMPILE_AND_EXECUTE]).
  //! @seealso
  //!   @[end], @[compile]
  void begin( int(0..1)|void run ) {
    if(run)
      GL.glNewList( id, GL.GL_COMPILE_AND_EXECUTE );
    else
      GL.glNewList( id, GL.GL_COMPILE );
  }

  //! Finish the list definition.
  //! @seealso
  //!   @[begin], @[compile]
  void end() {
    GL.glEndList();
  }

  //! Compile a list be executing the list code @[f]. Exceptions in
  //! @[f] will be thrown after @[GL.glEndList] has been called.
  //! @seealso
  //!   @[begin]
  void compile( function f ) {
    begin();
    mixed err = catch(f());
    end();
    if(err) error(err);
  }

  //! Execute the commands in the list.
  void call() {
    GL.glCallList( id );
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%O)", this_program, id);
  }
}

// Debug code 
static int global_list_begin;

//! A displaylist that is generated on demand.
//! @note
//! On Windows lists needs to be regenerated when the video driver
//! mode is changed. Thus the DynList is to prefer over @[List], since
//! regeneration is done automatically upon video mode change.
class DynList {
  inherit List;
  int(0..1) generated;
  function generator;

  //! Called by videodriver when a video mode change occurs.
  void modeswitch() {
    if( !generator )
      destruct(this);
    else
      init();
  }

  //! Sets a function which can generate a displaylist.
  //! Hint: Use implicit lambda...
  void set_generator( function _generator ) {
    generator = _generator;
  }

  //! Generates the displaylist, ie calls the function set in
  //! @[set_generator]. Called only when the display list needs to
  //! be generated.
  void init() {
    if( global_list_begin )
      error( "Initializing displaylist while initializing display list\n");
    mixed err;
    global_list_begin++;
    begin();
    generated = 1;
    if( err = catch(generator()) )
      werror(describe_backtrace(err));
#ifndef __NT__
    generator = 0;
#endif
    end();
    global_list_begin--;
  }

  //! Call the displaylist, ie draw it.
  void call() {
    if( !generated )
      init();
    GL.glCallList( id ); // ::call();
  }

  //! Create a new DynList object and optionally set a function
  //! that can generate the displaylist
  //! @param f
  //! Function which contains the GL commands that generates
  //! the displaylist.
  void create( function|void f ) {
    ::create();
    if( f )
      set_generator( f );
  }
}

#ifdef __NT__
//! Returns all defined lists. Only available on Windows.
array(List) get_all_lists() {
  return indices(all_lists);
}

//! Returns @expr{1@} if all defined lists are @[DynList] lists.
int(0..1) only_dynlists() {
  foreach(indices(all_lists), List o)
    if(!o->modeswitch) return 0;
  return 1;
}
#endif

// --- Textures

static int fast_mipmap;
static int texture_mem;
static multiset(BaseTexture) all_textures = set_weak_flag( (<>), Pike.WEAK );

//! The texture base class. Using e.g. @[Texture] might be more
//! convenient.
class BaseTexture {

  static int id;

  int t_width, t_height; //! Texture dimensions

  int i_width, i_height; //! Image dimensions

  float width_u, height_u; //! Utilization in percent.

  //! The texture type, e.g. @[GL.GL_TEXTURE_2D].
  int texture_type = GL_TEXTURE_2D;

  static int alpha, mode;
  static int(0..1) is_mipmapped;

  string debug; //! A string to identify the texture.

#ifdef __NT__
  static mapping(string:mixed) backingstore;
#endif;

  //! Construct a new texture. Processes @[_alpha], @[_mode] and
  //! @[debug_text] and calls @[resize].
  //! @param _alpha
  //! The alpha mode the texture is operating in.
  //! @int
  //!   @value 0
  //!     RGB
  //!   @value 1
  //!     RGBA
  //!   @value 2
  //!     ALPHA
  //!   @value 3
  //!     LUM
  //!   @value 4
  //!     LUM+ALPHA
  //! @endint
  //! @param _mode
  //! The mode the texture is operating in. Autoselected wrt @[_alpha]
  //! if @expr{0@}.
  //! @param debug_text
  //! A string that can be used to identify this texture.
  void construct( int width, int height, int _alpha,
		  mapping|void imgs, int(0..1)|void mipmap, int|void _mode,
		  string|void debug_text )
  {
    if(id) error("construct called twice\n");
    id = texture_ids->get();
    if(debug_text) debug = debug_text;
    alpha = _alpha;
    mode = _mode;
    resize(width, height, imgs, mipmap);
    all_textures[this] = 1;
  }

  //! Calls @[construct] with @[args].
  static void create(mixed ... args ) {
    construct( @args );
  }

  //! Properly deallocates the texture.
  static void destroy()
  {
    all_textures[this] = 0;
    texture_ids->free(id);
    texture_mem -= _sizeof();
  }

  //! Resizes/creates a texture to meet the dimensions @[width] and
  //! @[height]. If @[nocreate] isn't given, @[create_texture] is
  //! called to actually perform the resize/creation.
  //! @seealso
  //!   @[construct]
  void resize(int width, int height,
	      mapping|void imgs, int(0..1)|void mipmap,
	      int(0..1)|void nocreate)
  {
    texture_mem -= _sizeof();
    t_width = t_height = 1;
    i_width = width; i_height = height;
    while( t_width < i_width ) t_width <<= 1;
    while( t_height < i_height ) t_height <<= 1;
    width_u = (float)i_width / t_width;
    height_u = (float)i_height / t_height;
    texture_mem += _sizeof();
    if(!nocreate) 
      create_texture( imgs, mipmap, width, height );
  }

  //! Actually creates the texture.
  //! @param imgs
  //! If zero, a black texture with the dimensions @[width] *
  //! @[height] will be generated. Otherwise @[imgs] should be a
  //! mapping as follows.
  //! @mapping
  //!   @member Image.Image "image"
  //!     The actual image to be used as texture. It will be
  //!     cropped/padded to meet the dimensions given in @[width] and
  //!     @[height].
  //!   @member Image.Image "alpha"
  //!     Optional image to be used as alpha channel, depending on the
  //!     alpha value given to @[create]/@[construct].
  //! @endmapping
  //! @param mipmap
  //!   If @expr{1@}, the texture will be mipmapped.
  //! @param width
  //! @param height
  //!   The dimensions of the texture. If omitted the dimensions of
  //!   the images in @[imgs] will be used.
  //! @seealso
  //!   @[resize]
  void create_texture( mapping|void imgs, int(0..1)|void mipmap,
		       int|void width, int|void height )
  {
    if(imgs) {
      int w,h;
      if(imgs->image) {
	w = imgs->image->xsize();
	h = imgs->image->ysize();
      }
      else if(imgs->alpha) {
	w = imgs->alpha->xsize();
	h = imgs->alpha->ysize();
      }

#ifdef __NT__
      backingstore = ([ "image":imgs->image, "alpha":imgs->alpha ]);
#endif

      if(imgs->image)
	imgs->image = imgs->image->copy(0, 0, t_width-1, t_height-1);
      if(imgs->alpha)
	imgs->alpha = imgs->alpha->copy(0, 0, t_width-1, t_height-1);
      // FIXME: Generate imgs->image/alpha if missing?

      i_width = w;
      i_height = h;
    }
    else
    {
#ifdef __NT__
      imgs = ([ "image":Image.Image(t_width, t_height),
		"alpha":Image.Image(t_width, t_height) ]);

      switch( alpha ) {
      case 4: // LUM+ALPHA
      case 1: // RGBA
	break;
      case 3: // LUM
      case 0: // RGB
	m_delete(imgs, "alpha");
	break;
      case 2: // ALPHA
	m_delete(imgs, "image");
	break;
      }
      backingstore = imgs;
#else
      imgs = ([]);
      imgs->alpha = imgs->image = Image.Image(t_width, t_height);
#endif
      i_width = width;
      i_height = height;
    }

    mapping m;
    switch( alpha ) {
    case 4: // LUM+ALPHA
      mode = GL_LUMINANCE_ALPHA;
      m = ([ "luminance":imgs->image, "alpha":imgs->alpha ]);
      break;
    case 3: // LUM
      mode = GL_LUMINANCE;
      m = ([ "luminance":imgs->image ]);
      break;
    case 2: // ALPHA
      mode = GL_ALPHA;
      m = ([ "alpha":imgs->alpha ]);
      break;
    case 1: // RGBA
      mode = GL_RGBA;
      m = ([ "rgb":imgs->image, "alpha":imgs->alpha ]);
      break;
    case 0: // RGB
      mode = GL_RGB;
      m = ([ "rgb":imgs->image ]);
      break;
    }

    int imode;
    if( depth == 16 )
      switch( alpha ) {
      case 2: // ALPHA
	imode = GL_ALPHA; break;
      case 1: // RGBA
	imode = GL_RGBA4; break;
      case 0: // RGB
	imode = GL_RGB5;  break;
      }
    else
      imode = mode;

    use();
    if( global_list_begin )
      error("Creating texture while creating list\n");
    if( texture_type == GL_TEXTURE_2D ) {
      glTexParameter(texture_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameter(texture_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    if(mipmap) {
      glTexParameter(texture_type, GL_TEXTURE_MAG_FILTER,
		     fast_mipmap ? GL_NEAREST_MIPMAP_NEAREST :
		     GL_LINEAR_MIPMAP_LINEAR);
      glTexParameter(texture_type, GL_TEXTURE_MIN_FILTER,
		     fast_mipmap ? GL_NEAREST_MIPMAP_NEAREST :
		     GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
      glTexParameter(texture_type, GL_TEXTURE_MAG_FILTER,
		     fast_mipmap ? GL_NEAREST : GL_LINEAR);
      glTexParameter(texture_type, GL_TEXTURE_MIN_FILTER,
		     fast_mipmap ? GL_NEAREST : GL_LINEAR);
    }

    glTexImage2D( texture_type, 0, mode || imode, 0, m );
    if(mipmap)
      make_mipmap(imgs, imode);
    else
      is_mipmapped = 0;
  }

  //! Renders a mipmap of the image/partial image @[imgs].
  //! @param imgs
  //!   Image data mapping to feed @[GL.glTexImage2D] or
  //!   @[GL.glTexSubImage2D].
  //! @param imode
  //!   Internal format to feed @[GL.glTexImage2D], or UNDEFINED for
  //!   partial images.
  //! @param dx
  //! @param dy
  //!   Xoffs, yoffs to feed @[GL.glTexSubImage2D] for partial images.
  //! @seealso
  //!   @[create_texture]
  void make_mipmap(mapping imgs, int|void imode,
		   int|void dx, int|void dy) {
    is_mipmapped = 1;
    int level, done;
    mapping m;
    use();

    Image.Image i = imgs->image;
    Image.Image a = imgs->alpha;

    while(!done) {
      if( i ) { i = i->scale(0.5); done = min(i->xsize(), i->ysize()) < 2; }
      if( a ) { a = a->scale(0.5); done = min(a->xsize(), a->ysize()) < 2; }
      switch( alpha )
      {
	case 4: m = ([ "luminance":i, "alpha":a ]); break;
	case 3: m = ([ "luminance":i ]); break;
	case 2: m = ([ "alpha":a ]); break;
	case 1: m = ([ "rgb":i, "alpha":a ]); break;
	case 0: m = ([ "rgb":i ]); break;
      }
      if(zero_type(dx))
	glTexImage2D( texture_type, ++level, mode || imode, 0, m );
      else
	glTexSubImage2D( texture_type, ++level, dx >>= 1, dy >>= 1, m);
    }
  }

  //! Clears the texture.
  void clear() {
    // FIXME: Clear backingstore as well?
    use();
    glTexSubImage2D( texture_type, 0, 0, 0, Image.Image(t_width, t_height) );
    if(is_mipmapped) {
      int w = t_width, h = t_height, level;
      while(w + h) {
	w >>= 1; h >>= 1;
	glTexSubImage2D( texture_type, ++level, 0, 0,
			 Image.Image(w || 1, h || 1) );
      }
    }
  }

  //! Paste the image @[i] with alpha channel @[a] at coordinates @[x]
  //! and @[y] in the current texture.
  void paste( Image.Image i, Image.Image a, int x, int y ) {
#ifdef __NT__	
    // We need backing store.  Why, you ask? Because when we switch
    // mode or somebody else does for us, all textures become invalid.
    if( backingstore->image && i && backingstore->image != i )
      backingstore->image = backingstore->image->paste( i, x, y );
    if( backingstore->alpha && a && backingstore->alpha != a )
      backingstore->alpha = backingstore->alpha->paste( a, x, y );
#endif
    mapping m = ([]);
    //    if( colormap && i )
    //      i = colormap->map( i );
    switch( mode ) {
      case GL_LUMINANCE_ALPHA:
	m = ([ "luminance":i, "alpha":a ]);
	break;
      case GL_LUMINANCE:
	m = ([ "luminance":i ]);
	break;
      case GL_ALPHA:
	m->alpha = a;
	break;
      case GL_RGBA:
	if( i )
	  m->rgb = i;
	if( a )
	  m->alpha = a;
	break;
      case GL_RGB:
	m->rgb = i;
    }
    use();

    glTexSubImage2D( texture_type, 0, x, y, m );
    if(is_mipmapped)
      make_mipmap(m, UNDEFINED, x, y);
  }

  //! Returns the size of memory allocated by the texture.
  int _sizeof() {
    int multiplier = 1, size = t_width * t_height;

    if( depth == 16 ) {
      if( (mode == GL_ALPHA) || (mode == GL_LUMINANCE) )
	multiplier = 1;
      else
	multiplier = 2;
    }
    else
      switch( mode ) {
	case GL_ALPHA:
	case GL_LUMINANCE:
	  multiplier = 1; break;
	case GL_LUMINANCE_ALPHA:
	  multiplier = 2; break;
	case GL_RGB:
	  multiplier = 3; break;
	case GL_RGBA:
	  multiplier = 4; break;
      }

    if(is_mipmapped) {
      int w = t_width, h = t_height;
      while(w + h) {
	w >>= 1; h >>= 1;
	size += (w||1) * (h||1);
      }
    }

    return size * multiplier;
  }

  //! Returns the id of this texture.
  int get_id() { return id; }

  static int __hash() { return id; }

  //! Textures can be sorted according to texture id.
  static int(0..1) `>(mixed x) {
    if(!objectp(x) || !x->get_id) return 1;
    return (int)id > (int)x->get_id();
  }

  //! Draw the texture at @[x],@[y],@[z] with dimensions @[w]*@[h].
  void draw( float x, float y, float z, float w, float h ) {
    use();

    glBegin( GL_QUADS );
     coords( 0.0, 0.0 ); glVertex( x, y, z );
     coords( 1.0, 0.0 ); glVertex( x+w, y, z );
     coords( 1.0, 1.0 ); glVertex( x+w, y+h, z );
     coords( 0.0, 1.0 ); glVertex( x, y+h, z );
    glEnd();
  }

  //! Draw texture region @[s0],@[q0] - @[ss],@[qs] at @[x],@[y],@[z]
  //! with dimensions @[w]*@[h].
  void draw_region( float x, float y, float z, float w, float h,
		    float s0, float q0, float ss, float qs ) {
    use();

    glBegin( GL_QUADS );
     coords( s0, q0 );       glVertex( x, y, z );
     coords( s0+ss, q0 );    glVertex( x+w, y, z );
     coords( s0+ss, q0+qs ); glVertex( x+w, y+h, z );
     coords( s0, q0+qs );    glVertex( x, y+h, z );
    glEnd();
  }

  //! Use the generated texture (@[GL.glBindTexture]).
  void use() {
    glBindTexture( texture_type, id );
  }

  //! Sets the texture coordinates to @[x]*width,@[y]*height.
  void coords( float x, float y ) {
    glTexCoord( x*width_u, y*height_u );
  }

#ifdef __NT__
  void modeswitch() {
    use();
    create_texture( backingstore );
  }
#endif

  static string _sprintf( int f ) {
    if(f!='O') return 0;

    string ms;
    switch( mode ) {
    case GL_LUMINANCE_ALPHA:
      ms = "la"; break;
    case GL_LUMINANCE:
      ms = "l"; break;
    case GL_RGB:
      ms = "rgb"; break;
    case GL_ALPHA:
      ms = "a"; break;
    case GL_RGBA:
      ms = "rgba"; break;
    default:
      ms = "unknown mode"; break;
    }

    return sprintf( "%O(%d %dx%d (%dx%d) %s%s %s; %dKb)", this_program, id,
		    t_width, t_height, i_width, i_height,
		    ms, (is_mipmapped?"(m)":""), (debug?"("+debug+")":""),
		    _sizeof()/1024 );
  }

  void set_image_data( Image.Image|mapping(string:mixed) data,
		       int(0..1)|void no_resize )
  //! Set the contents (and size) of the texture from the supplied
  //! data. The @[data] is identical to what would normally be sent as
  //! the last argument to glTex[Sub]Image2D() or an Image.Image object.
  //! 
  //! If @[no_resize] is specified, it is assumed that the data will
  //! fit in the texture, otherwise the parts that extend beyond it
  //! will be discarded.
  //! @param data
  //! Besides being an @[Image.Image] object, @[data] can be either of
  //! two types of mappins. First it can be a mapping with Image data.
  //! @mapping
  //!   @member Image.Image "rgb"
  //!     Texture image data.
  //!   @member Image.Image "alpha"
  //!     Optional alpha channel.
  //!   @member Image.Image "luminance"
  //!     Optional luminance channel.
  //! @endmapping
  //! Second it can be a mapping pointing out a shared memory segment.
  //! @mapping
  //!   @member System.Memory "mem"
  //!     The shared memory segment.
  //!   @member int "mem_w"
  //!   @member int "mem_h"
  //!     The width and height of the memory segment.
  //!   @member int "mem_format"
  //!     The format of the memory segment, e.g. @[GL.GL_RGB].
  //!   @member int "mem_type"
  //!     The low level format of the memory segment, e.g.
  //!     @[GL.GL_UNSIGNED_BYTE].
  //! @endmapping
  {
    if( global_list_begin )
      error("Creating texture while creating list\n");
    if( objectp( data ) )
      data = ([ "rgb":data ]);
    use();
    if( !no_resize )
    {
      int nw, nh, do_create;
      if( nw = data->mem_w ) // This is most common (movie playback)
	nh = data->mem_h;
      else if( data->rgb )
      {
	nw = data->rgb->xsize();
	nh = data->rgb->ysize();
      }
      else if( data->alpha )
      {
	nw = data->alpha->xsize();
	nh = data->alpha->ysize();
      }
      else if( data->luminance )
      {
	nw = data->luminance->xsize();
	nh = data->luminance->ysize();
      }
      if( nw!=i_width || nh!=i_height )
      {
	if( texture_type == GL_TEXTURE_RECTANGLE_NV 
	    || (nw > t_width) || (nh > t_height) ||
	    (t_width>>1) >= nw || (t_height>>1) >= nh )
	  do_create = 1;
	resize( nw, nh, 0, 0, 1 );
	if( do_create )
	{
	  int imode = mode;
	  if( depth == 16 )
	    switch( alpha )
	    { // the others are identical (alpha/lum/luma).
	      case 1: imode = GL_RGBA4; break;
	      case 0: imode = GL_RGB5;  break;
	    }
	  // It is not possible to use 'data' here since it's most
	  // likely not a power of 2 large.
	  if(texture_type == GL_TEXTURE_RECTANGLE_NV)	  
	  {
	    glTexImage2D( texture_type, 0, imode, 0, data );
	    return;
	  }
	  else
	  {
	    glTexImage2D( texture_type, 0, imode, 0,
			([ "mem":System.Memory( t_width*t_height*4 ),
			   "mem_w":t_width, "mem_h":t_height,
			   "mem_format":imode,
			   "mem_type":GL.GL_UNSIGNED_BYTE,
			]) );
	  }
	}
      }
    }
    glTexSubImage2D( texture_type, 0,0,0, data );
  }
}

//! Uses the NVidia RECT texture extension for non-power-of-two
//! textures.
class RectangleTexture
{
  inherit BaseTexture;

  void resize(int width, int height,
	      mapping|void imgs, int(0..1)|void mipmap,
	      int(0..1)|void nocreate)
  {
    texture_type = GL_TEXTURE_RECTANGLE_NV;
    texture_mem -= _sizeof();
    width_u = (float)(t_width = width);
    height_u = (float)(t_height = height);
    texture_mem += _sizeof();
    if(!nocreate) create_texture( imgs, 0, width, height );
  }
}

//! A mixin class with a dwim create function.
class BaseDWIM {

  void construct( int width, int height, int _alpha,
		  mapping|void imgs, int(0..1)|void mipmap, int|void _mode,
		  string|void debug_text );

  //! This create function has the following heuristic:
  //!
  //! If a mapping is encountered, the following information will be
  //! attempted to be extracted.
  //! @mapping
  //!   @member Image.Image "image"
  //!     The texture image.
  //!   @member int "xsize"
  //!   @member int "ysize"
  //!   @member int "height"
  //!   @member int "width"
  //!     The image dimensions. If not provided, the dimensions of the
  //!     @expr{"image"@} member will be used.
  //!   @member int "alpha"
  //!     The alpha mode.
  //!   @member int(0..1) "mipmap"
  //!     Should the texture be mipmapped or not.
  //!   @member int "mode"
  //!     The texture mode.
  //!   @member string "debug"
  //!     The debug name associated with this texture.
  //! @endmapping
  //!
  //! If an object is encountered in the argument list, the first
  //! object will be used as texture image and the second as texture
  //! alpha.
  //!
  //! If a string is encountered in the argument list, it will be used
  //! as debug name associated with this texture.
  //!
  //! Once all mappings, strings and objects are removed from the
  //! argument list, the remaining integers will be interpreted as
  //! width, height, alpha, mipmap and mode, unless there is only one
  //! argument. In that case it will be interpreted as the alpha mode.
  void create(mixed ... args) {
    mapping imgs;
    int height, width, alpha=-1, mipmap, mode;
    string debug;

    // Get imgs and debug
    foreach(args, mixed arg) {
      if(mappingp(arg)) {
	  if(arg->image) {
	    if(imgs)
	      error("Error on arguments. More than one image provided.\n");
	    imgs = arg;
	    if(!imgs->xsize) imgs->xsize = imgs->image->xsize();
	    if(!imgs->ysize) imgs->ysize = imgs->image->ysize();
	    height = imgs->ysize;
	    width = imgs->xsize;
	}
	args -= ({ arg });
	if(arg->height) height = m_delete(arg, "height");
	if(arg->width) width = m_delete(arg, "width");
	if(arg->alpha) alpha = m_delete(arg, "alpha");
	if(arg->mipmap) mipmap = m_delete(arg, "mipmap");
	if(arg->mode) mode = m_delete(arg, "mode");
	if(arg->debug) debug = m_delete(arg, "debug");
	continue;
      }
      if(objectp(arg)) {
	if(imgs) {
	  if(imgs->alpha)
	    error("Error on arguments. "
		  "Both image and alpha already provided.\n");
	  if(height != arg->ysize() || width != arg->xsize())
	    error("Alpha has different size than image.\n");
	  imgs->alpha = arg;
	}
	else {
	  imgs = ([ "image":arg,
		    "xsize":arg->xsize(),
		    "ysize":arg->ysize() ]);
	  height = imgs->ysize;
	  width = imgs->xsize;
	}
#if 0 // It is not possible to compare Image.Image with e.g. strings.
	args -= ({ arg });
#else
	foreach(args; int pos; mixed value)
	  if(objectp(value) && value==arg) {
	    args[pos] = "\0";
	    args -= ({ "\0" });
	    break;
	  }
#endif
	continue;
      }
      if(stringp(arg)) {
	debug = arg;
	args -= ({ arg });
      }
    }

    // Now there should only be ints left.
    foreach(args, mixed arg)
      if(!intp(arg)) error("Unexpected type in argument.\n");

    switch(sizeof(args)) {
    case 0:
      break;
    case 1:
      alpha = args[0];
      break;
    case 2:
      [ width, height ] = args;
      break;
    case 3:
      [ width, height, alpha ] = args;
      break;
    case 4:
      [ width, height, alpha, mipmap ] = args;
      break;
    case 5:
      [ width, height, alpha, mipmap, mode ] = args;
      break;
    default:
      error("Too many arguments.\n");
    }

    if(width<1 || height<1)
      error("Dimensions must not be less than 1x1.\n");
    if(alpha==-1) {
      if(imgs && imgs->alpha)
	alpha=1;
      else
	alpha=0;
    }

    construct(width, height, alpha, imgs, mipmap, mode, debug);
  }
}

//! Convenience version of the @[Texture] class.
class Texture 
{
  //! Texture base
  inherit BaseTexture;

  //! Convenience methods
  inherit BaseDWIM;
}

//! Convenience version of the @[RectangleTexture] class.
class RectangleDWIMTexture
{
  //! Texture base
  inherit RectangleTexture;

  //! Convenience methods
  inherit BaseDWIM;
}

//! Create a texture.  Mainly here for symetry with @[make_rect_texture]
//! @seealso
//! @[Texture], @[make_rect_texture]
BaseTexture make_texture(  mapping|Image.Image image, string|void name )
{
  return Texture( image, name );
}

//! Create a texture with the specified image as contents.  Will try
//! to use the TEXTURE_RECTANGLE_NV extension if available, otherwise 
//! normal textures will be used (like @[make_texture]).
//! @seealso
//!   @[make_texture]
BaseTexture make_rect_texture( mapping|Image.Image image, string|void name )
{
  
//   if( has_extension( "GL_NV_texture_rectangle") )
//       return RectangleDWIMTexture( image, name );
  return Texture( image, name );
}


//! Returns a list of all current textures.
array(BaseTexture) get_all_textures() {
  return indices(all_textures);
}

//! Returns the number of bytes used by the textures.
int get_texture_mem_usage() {
  return texture_mem;
}


// --- Rectangle ADT

//! A rectangle. Used by the text routines to avoid drawing outside
//! the current region.
class Region( float x, float y, float w, float h ) {

  //! All region objects have this constant.
  constant is_region = 1;

  //! Move the region @[xp] units right and @[yp] units down.
  void move( float xp, float yp ) {
    x += xp;
    y += yp;
  }

  //! Make the region @[xs] units wider and @[ys] units higher.
  void resize( float xs, float ys ) {
    w += xs;
    y += ys;
  }

  static string _sprintf( int c ) {
    return c=='O' && sprintf("%O( %f,%f - %f,%f )", this_program, x, y, w, h );
  }

  //! Returns 1 if the region @[R] is fully outside this region.
  int(0..1) outside( Region R ) {
    return R->x+R->w < x || R->y+R->h < y || R->x > x+w || R->y > y+h;
  }

  //! Returns 1 if the region @[R] is fully inside this region.
  int(0..1) inside( Region R ) {
    return R->x >= x && R->y >= y && R->x+R->w <= x+w && R->y+R->h <= y+h;
  }

  //! Creates a new region with the intersection of this region and @[R].
  Region `&( mixed R ) {
    if(!objectp(R) || !R->is_region)
      error("Regions can only be ANDed with other regions.\n");

    float x0, y0;
    float nw, nh;

    x0 = max( R->x, x );
    y0 = max( R->y, y );

    nw = min( (R->x+R->w), x+w )-x0;
    nh = min( (R->y+R->h), y+h )-y0;
    if( nw > 0 && nh > 0 )
      return Region( x0, y0, nw, nh );
    return Region( x0, y0, 0.0, 0.0 );
  }
}


// --- Font

#define TX_HEIGHT 512
// The height of one font texture. The textures are generated on
// demand to save memory when using unicode fonts.
//
// Hopefully all characters will not be used, and even if they are,
// the system can swap in and out textures from the graphics card on
// demand.
//
// This would not have been the case if one big texture was used,
// even if it had been possible (it would require a 16k x 16k
// texture, aproximately, or aproximately 256Mb of texture memory.

//!  A font.
class Font
{
  // The width of a space character. Used for all nonprintables except
  // '\n'.
  static float spacew;

  // The hight of a character in the font, in pixels, +1. 1 is added
  // to give some spacing between characters in the texture.
  static int character_height;

  // The actual font.
  static Image.Fonts.Font font;

  // Scale factor for the character width.
  static float scale_width;

  // Scale factor for the horizontal character spacing.
  static float scale_spacing;

  //!
  void create( Image.Fonts.Font f, float|void _scale_width,
	       float|void _scale_spacing ) {
    font = f;
    scale_width = _scale_width || 0.69;
    scale_spacing = _scale_spacing || 0.40;

    character_height = f->write("A")->ysize()/2+1;
    if( f->font && f->font->advance )
      spacew = (f->font->advance( ' ' )/(2.0*(character_height-1)))
	*scale_width;
    else
      spacew = (f->write("f")->xsize()/(2.0*(character_height-1)))*scale_width;
  }


  //  On-demand texture allocation code.

  // Each element in this mapping contains
  // ({ character advance (pixels), texture region, texture })
  //
  // It's generated by get_character, which allocates a suitable slot
  // in a texture for the character.
  static mapping(int:array(int|Region|BaseTexture)) letters = ([]);

  //  The slot in the texture to use. Initially set so large that a
  //  new texture will be generated for the first character, to avoid
  //  code duplication of the texture allocation below.
  static int robin = 10000;

  // The current texture. 
  static BaseTexture current_txt;

  // FIXME: Include all nonprintables here. 
  static constant nonprint = (multiset)enumerate( 33 ) +
  (multiset)enumerate( 33, 1, 0x80 ) + (< 0x7f, 0xad >);

  //! Returns the advance (in pixels), the texture and the texture
  //! coordinates for the specified character, or 0 if it's
  //! nonprintable.
  //! @note
  //! If the font->write call fails, the backtrace will be written
  //! to stderr.
  array(int|BaseTexture|Region) get_character( int c ) {
    if( letters[c] || nonprint[c] )
      return letters[c];

    if( ((robin+1)*character_height) > TX_HEIGHT ) {
      // The current texture is full.
      robin=0;
      // Setting texture width to character_height and hoping that no
      // font is twice as wide as it is heigh (scale factor 0.5
      // below).
      current_txt = BaseTexture( character_height, TX_HEIGHT, 2,
				 0, 0, 0, "Font texture" );
    }

    Image.Image lw;
    mixed err;
    if( ((err=catch(lw = font->write( sprintf( "%c", c ) )->scale(0.5)))
	 && (err=catch( lw = font->write( sprintf( "", c ) )->scale(0.5))))
	|| (!lw || !lw->xsize()  || !lw->ysize()) )
      lw = Image.Image( 1,1 );
    if(err)
      werror(describe_backtrace(err)+"\n");

    int x = lw->xsize();
    int y = lw->ysize();

    // Fix distortion bug in glTextSubImage2D
    int nx = x + 4-(x%4);
    if(x!=nx) lw = lw->copy(0,0,nx-1,y-1);

    current_txt->paste(0, lw, 0, robin*character_height );

    letters[c] = ({
      ((font->font && font->font->advance) ?
       font->font->advance( c )/2 : x)*scale_width,
      Region( 0.0,
	      (robin*character_height)/(float)current_txt->i_height,
	      x/(float)current_txt->i_width,
	      (character_height-1)/(float)current_txt->i_height),
      current_txt
    });
    robin++;
    return letters[ c ];
  }

  //! A character to draw.
  class Character
  {
    inherit Region;

    Region pos; //! Character position in texture @[txt].
    BaseTexture txt; //! Texture holding the character.
    Region slice; //! Slice of character to be shown.

    //! Set character to be region @[_slice] of region @[_pos] of
    //! texture @[_txt].
    void set_data( Region _pos, BaseTexture _txt, void|Region _slice ) {
      pos = _pos;
      txt = _txt;
      slice = _slice;
    }

    //! Draw the character using the texture @[txt] with the
    //! texture-coordinates indicated in @[pos], possible cropped with
    //! @[slice].
    void draw() {
      txt->use();
      glBegin( GL_TRIANGLE_STRIP );

      if( slice ) {
	//          screen to pixels to texture units
	float txtx = (((slice->x-x)/(h/(character_height-1))) / txt->i_width)
	  + pos->x;
	float txty = (((slice->y-y)/(h/(character_height-1))) / txt->i_height)
	  + pos->y;

	float txtw = (slice->w/w)*pos->w;
	float txth = (slice->h/h)*pos->h;

        txt->coords( txtx, txty );
	glVertex( slice->x, slice->y, 0 );

        txt->coords( txtx+txtw,txty );
	glVertex( slice->x+slice->w, slice->y, 0 );

        txt->coords( txtx, txty+txth );
	glVertex( slice->x, slice->y+slice->h, 0 );

        txt->coords( txtx+txtw, txty+txth );
	glVertex( slice->x+slice->w, slice->y+slice->h, 0 );
      }
      else {
        txt->coords( pos->x, pos->y );  glVertex( x, y, 0 );
        txt->coords( pos->x+pos->w,pos->y );  glVertex( x+w, y, 0 );
        txt->coords( pos->x, pos->y+pos->h );  glVertex( x, y+h, 0 );
        txt->coords( pos->x+pos->w, pos->y+pos->h );  glVertex( x+w, y+h, 0 );
      }
      glEnd();
    }
  }

  //! Get the width and height of the area that the string @[text] in
  //! size @[h] would cover.
  array(float) text_extents( string text, float h ) {
    return get_characters(text, h, 100000000.0)[..1];
  }

  //! Write the @[text] in size [h], possibly restricted by region @[roi].
  //! Return the width and height of the resulting text area. If @[roi] is
  //! a float, @expr{Region(0.0, 0.0, roi, 10000.0)@} will be used.
  array(float) write_now( string text, float h, void|float|Region roi,
			  string|void align )
  {
    array ret = get_characters( text, h, roi, align );
    ret[2]->draw();
    return ret[..1];
  }

  static array get_characters( string text, float h, float|Region roi,
			       string|void align )
  {
    map( (array)text, get_character );
    float xp = 0.0, yp = 0.0, mxs=0.0;
    array(Character) chars = ({});

    if( floatp( roi ) )
      roi = Region( 0.0, 0.0, roi, 10000.0 );

    xp = 0.0; yp = 0.0; mxs=0.0;

    foreach( (array(int))text, int t )
    {
      if( !letters[t] ) {
	if( t == '\n' )	{
	  yp += h;
	  xp = 0.0;
	}
	else if( t == '\t' ) {
	  float ts = (spacew*h+scale_spacing*spacew*h)*8;
	  for( int i=0; i<10; i++ )
	    if( ts*i > xp ) {
	      xp = ts*i;
	      break;
	    }
	  if( xp > mxs ) mxs = xp;
	}
	else {
	  xp += spacew*h + scale_spacing*spacew*h;
	  if( xp > mxs ) mxs = xp;
	}
      }
      else 
      {
	if( xp == 100000000.0 ) // optimization
	  continue;
	[int ww, Region pos, BaseTexture txt] = letters[t];
	Character c = Character(xp,yp,(ww/(float)(character_height-1))*h,h);
	Region reg;
	if( roi ) {
	  if( roi->outside( c ) ) {
	    if( (xp+c->w) > (roi->x+roi->w) )
	      xp = 100000000.0;
	    continue;
	  }
	  if( !roi->inside( c ) )
	    reg = roi & c;
	}
	c->set_data( pos, txt, reg );
	chars += ({ c });
	xp += c->w + scale_spacing*spacew*h;
	if( xp > mxs )
	  mxs = xp;
      }
    }
    if( !align || align == "left" || !has_value( text, "\n" ) )
      return ({ mxs, yp+h, chars });

    float last_y = 0.0;
    array(Character) current_row = ({});
    array(array(Character)) rows = ({});
    foreach( chars, Character c )
    {
      if( last_y != c->y )
      {
	rows += ({ current_row });
	current_row = ({});
	last_y = c->y;
      }
      current_row += ({ c });
    }
    rows += ({ current_row });
    foreach( rows, current_row )
    {
      if( !sizeof( current_row ) )
	continue;
      Character last = current_row[-1];
      float x0, mx = last->x + last->w;
      switch( align )
      {
	case "right":	 x0 = mxs - mx; break;
	case "center":
	case "centered": x0 = (mxs - mx) / 2.0; break;
      }
      foreach( current_row, Character c )
	c->x += x0;
    }
    return ({ mxs, yp+h, chars });
  }

  class GAH( object q, string text, float h, float|object roi,
	     string|void align )
  {
    void menda() {
#ifdef __NT__
      if( _refs(object_program(this)) > 1000 )
	gc();
#endif
      q->write_now( text, h, roi, align );
    }
  }

  //! Create a display list that writes text.
  //! @param text
  //!   The text to write.
  //! @param h
  //!   The font height
  //! @param roi
  //!   The region, if supplied, to restrict writing to.
  //! @param align
  //!   The text justification; "left" (default), "center" or "right".
  array(List|float) write( string text, float h, void|float|Region roi,
			   string|void align )
  {
    // Note: This must be done before starting the
    // display-list. Specifically, the textures have to be loaded
    // before.  Therefore, it's not possible to optimize by checking
    // the width and height when generating the list below.
    [float x, float y] = text_extents(text, h);

    DynList l = DynList(GAH(this,text,h,roi,align)->menda);

    // OBS: Needed! The list might well try to instantiate itself
    // inside another list instantiation otherwise, which is not
    // possible.
    l->init();

    return ({ l, x, y });
  }
}


// --- Higher Order GL Primitives

//! @decl void draw_line( float x0, float y0, float x1, float y1,@
//!                  Image.Color.Color c, void|float a )
//! @decl void draw_line( float x0, float y0, float z0,@
//!                  float x1, float y1, float z1,@
//!                  Image.Color.Color c, void|float a )
//!
void draw_line( float|int a, float|int b, float|int c, float|int d,
		void|float|int|Image.Color.Color e, void|float|int f,
		Image.Color.Color|void g, float|void h ) {
  glDisable( GL_TEXTURE_2D );
  if(g) {
#ifdef GLUE_DEBUG
    if(!floatp(e)) error("Wrong type in argument 5. Expected float.\n");
    if(!floatp(f)) error("Wrong type in argument 6. Expected float.\n");
    if(!objectp(g)) error("Wrong type in argument 7. Expected object.\n");
    if(!floatp(h)) error("Wrong type in argument 8. Expected float.\n");
#endif
    glColor( @g->rgbf(), h||1.0 );
    glBegin( GL_LINES );
    glVertex( a, b, c );
    glVertex( d, e, f );
    glEnd();
  }
  else {
#ifdef GLUE_DEBUG
    if(!objectp(e)) error("Wrong type in argument 5. Expected object.\n");
#endif
    if (e) glColor( @e->rgbf(), f||1.0 );
    glBegin( GL_LINES );
    glVertex( a, b, 0.0 );
    glVertex( c, d, 0.0 );
    glEnd();
  }
  glEnable( GL_TEXTURE_2D );
}

static void low_draw_box( int mode,
			  float x0, float y0, float x1, float y1,
			  array(Image.Color.Color)|Image.Color.Color c,
			  array|float a )
{
  int one_col;
  if( !arrayp( c ) ) {
    one_col = 1;
    c = ({ c, c, c, c });
  }

  if( !a ) a = 1.0;
  if( !arrayp( a ) )
    a = ({ a, a, a, a });
  else
    one_col = 0;

  if( !one_col)
    glShadeModel( GL_SMOOTH );

  glDisable( GL_TEXTURE_2D );
  if( one_col )
    glColor( @c[0]->rgbf(), a[0] );    

  glBegin( mode );
  {
    if( !one_col )
      glColor( @c[0]->rgbf(), a[0] );
    glVertex( x0, y0, 0.0 );
    if( !one_col )
      glColor( @c[1]->rgbf(), a[1] );
    glVertex( x1, y0, 0.0 );
    if( !one_col )
      glColor( @c[2]->rgbf(), a[2] );
    glVertex( x1, y1, 0.0 );
    if( !one_col )
      glColor( @c[3]->rgbf(), a[3] );
    glVertex( x0, y1, 0.0 );
  }
  glEnd();

  glEnable( GL_TEXTURE_2D );
  if( !one_col )
    glShadeModel( GL_SMOOTH );
}

//! Draw a box outline around the specified coordinates. @[c] is
//! either a single color, in which case it will be used for all
//! corners, or an array of four colors, which will be used for each
//! corner.
//!
//! @[a] is similar to @[c], but is the alpha values for each coordinate.
void draw_obox( float x0, float y0, float x1, float y1,
		array(Image.Color.Color)|Image.Color.Color c,
		void|array(float)|float a )
{
  low_draw_box( GL_LINE_LOOP, x0, y0, x1, y1, c, a );
}

//! Draw a box at the specified coordinates. @[c] is either a single
//! color, in which case it will be used for all corners, or an array
//! of four colors, which will be used for each corner.
//!
//! @[a] is similar to @[c], but is the alpha values for each coordinate.
void draw_box( float x0, float y0, float x1, float y1,
	       array(Image.Color.Color)|Image.Color.Color c,
	       void|array(float)|float a )
{
  low_draw_box( GL_QUADS, x0, y0, x1, y1, c, a );
}

//!
void draw_polygon( array(float) coords, Image.Color.Color c, float a )
{
  glDisable( GL_TEXTURE_2D );
  glColor( @c->rgbf(), a );
  glBegin( GL_POLYGON );
  foreach(coords / 2, [float x, float y])
    glVertex( x, y, 0.0 );
  glEnd();
  glEnable( GL_TEXTURE_2D );
}

// --- Surface ADT

//! A mesh of squares.
class SquareMesh
{
  static int xsize;
  static int ysize;
  static int(0..1) light;
  static int(0..1) need_recalc;
  static function(float,float:Math.Matrix) corner_func;
  static array(array(Math.Matrix)) vertices;
  static array(array(Math.Matrix)) surface_normals;
  static array(array(array(float))) vertex_normals;
  static BaseTexture texture;

  //! Recalculate the mesh.
  void recalculate()
  {
    need_recalc = 0;
    for( int y = 0; y<ysize+1; y++ )
    {
      float yp = y/(float)ysize;
      float xf = 1.0/xsize;
      for( int x = 0; x<xsize+1; x++ )
      {
	vertices[x][y] = corner_func( x*xf, yp );
	if( light ) {
	  surface_normals[x][y] = 0;
	  vertex_normals[x][y] = 0;
	}
      }
    }
  }

  //! Return the normal for the surface at coordinates x,y.
  //! Used internally. 
  Math.Matrix surface_normal( int x, int y )
  {
    if( x < 0 ) x=0;
    else if( x >= xsize ) x=xsize-1;

    if( y < 0 ) y=0;
    else if( y >= ysize ) y=ysize-1;

    if( surface_normals[x][y] )
      return surface_normals[x][y];

    Math.Matrix v1 = vertices[x][y];
    Math.Matrix v2 = vertices[x+1][y];
    Math.Matrix v3 = vertices[x+1][y+1];
    return surface_normals[x][y] = (v2-v1)->cross( (v3-v2) )->normv();
  }

  // FIXME: Once the GL module accepts Math.Matrix objects we should
  // definately change this code to use them.
  static array vertex_normal( int x, int y )
  {
    return vertex_normals[x][y]  ||
      (vertex_normals[x][y] =
       ((surface_normal( x-1, y-1 ) + surface_normal( x, y-1 ) +
	 surface_normal( x-1, y ) + surface_normal( x, y )))->vect());
  }

  //! Set a texture to be mapped on the mesh.
  void set_texture( BaseTexture tex )
  {
    texture = tex;
  }

  //! Draw the mesh.
  void draw()
  {
    if( need_recalc ) recalculate();
    float ystep = 1.0/(float)ysize;
    for( int y = 0; y<ysize; y++ )
    {
      glBegin( GL_QUAD_STRIP );
      float yp = y/(float)ysize;
      float step = 1.0/(float)xsize;
      float xp = 0.0;
      for( int x = 0; x<xsize+1; x++,xp+=step )
      {
	if( light )   glNormal( vertex_normal( x, y ) );
	if( texture ) texture->coords( xp, yp );
	// FIXME2
	glVertex( vertices[x][y]->vect() );
	if( light )   glNormal( vertex_normal( x, y+1 ) );
	if( texture ) texture->coords( xp, yp+ystep );
	// FIXME3
	glVertex( vertices[x][y+1]->vect() );
      }
      glEnd();
    }
  }

  //! Indicate whether or not lighting is used. If it is, the normals
  //! of each vertex will be calculated as well as the coordinates.
  void set_lighting( int(0..1) do_lighting )
  {
    light = do_lighting;
  }
  
  //! Set the size of the mesh
  void set_size( int x, int y )
  {
    need_recalc = 1;
    xsize = x;
    ysize = y;
    vertices = map(allocate( x+1, y+1 ),allocate);
    surface_normals = map(allocate( x+1, y+1 ),allocate);
    vertex_normals = map(allocate( x+1, y+1 ),allocate);
  }
    
  //! The @[calculator] will be called for each corner and should
  //! return a 1x3 matrix describing the coordinates for the given
  //! spot om the surface.
  void create( function(float,float:Math.Matrix) calculator )
  {
    set_size( 10, 10 );
    need_recalc = 1;
    corner_func = calculator;
  }
}

//! Returns some internal states for debug purposes. The actual
//! content may change.
mapping(string:mixed) debug_stuff() {
  return ([
#if __NT__
    "all_lists" : sizeof(all_lists),
#endif
    "all_textures" : sizeof(all_textures),
    "fullscreen" : fullscreen,
    "resolution" : "" + resolution[0] + "x" + resolution[1],
    "aspect" : aspect,
    "gl_flags" : gl_flags,
    "depth" : depth,
    "rotation" : global_z_rotation,
    "mirror" : (mirrorx?"x":"") + (mirrory?"y":""),
    "texture_mem" : texture_mem,
    "fast_mipmap" : fast_mipmap,
  ]);
}

#else
constant this_program_does_not_exist=1;
#endif
