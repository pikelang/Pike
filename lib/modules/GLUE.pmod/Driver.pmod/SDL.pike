//
// $Id: SDL.pike,v 1.4 2004/04/08 21:52:50 nilsson Exp $

#pike __REAL_VERSION__

#if constant(SDL.Surface)

inherit .Interface;
import GLUE.Events;

static int bpp;
static SDL.Surface screen;
#if constant(SDL.Joystick)
static array(SDL.Joystick) sticks = ({});
#endif
static function key_evt, configure_event;
static int is_full;
static SDL.Event evt = SDL.Event();
static mapping active = ([]);
static int nomove;

static mapping pressed = ([]);
static mapping(int:int) keymap = ([]);

static void event_handler()
{
#ifndef THREAD_EVENTS
  call_out( event_handler, 0.02 );  
  low_handle_events();
#else
  thread_create( low_handle_events );
#endif
}

class MouseAbs
{
    inherit Event;
    float xp, yp;

    this_program dup()
    {
	object x = ::dup();
	x->xp = xp;
	x->yp = yp;
	return x;
    }

    void create( int q, int pr, float _xp, float _yp  )
    {
	if( floatp(_xp) && floatp(_yp) )
	{
	    xp = _xp;
	    yp = _yp;
	}
	::create( MOUSE_ABS, pr );
    }
}

float mmx, mmy;
int odx, ody;
void low_handle_events()
{
  while( 1 )
  {
    int mode = 1;
    if( !evt->get() )
    {
#ifdef THREAD_EVENTS
      sleep( 0.02 );
      continue;
#else
      return;
#endif
    }
    if( mixed err = catch {
      Event res;
      switch( evt->type )
      {
	case SDL.KEYUP:   mode = 0;
	case SDL.KEYDOWN:
	  SDL.Keysym sym = evt->keysym;
	  if( evt->type == SDL.KEYUP || !pressed[ sym->scancode ] )
	  {
	    res = key_event( X_sym(sym), mode );
	    if( sym->unicode>31 )
	      res->data = sprintf("%c", sym->unicode);
	    else if( sym->unicode )
	      res->data = sprintf("%c", X_sym(sym));
	    
	    pressed[ sym->scancode ] = (evt->type == SDL.KEYDOWN);
	    key_evt( res );
	  }
	  break;

	case SDL.MOUSEBUTTONUP:  mode = 0;
	case SDL.MOUSEBUTTONDOWN:
	  key_evt( Event( -evt->button, mode ) );
	  break;

	case SDL.JOYBUTTONUP:    mode = 0;
	case SDL.JOYBUTTONDOWN:
	  key_evt( Event( -(evt->button+100), mode ) );
	  break;
	case SDL.JOYAXISMOTION:
	  int code = -((evt->axis+1) * 1000);
	  if( abs(evt->value) < 4000 )
	  {
	    active[code]=0;
	    key_evt( Event( code-1, 0 ) );
	    key_evt( Event( code-2, 0 ) );
	  }
	  else
	  {
	    int dir = evt->value<0 ? 2 : 1;
	    res = Event( code-dir, 1+active[code] );
	    res->pressure = abs((float)evt->value) / 65535;
	    key_evt( res );
	    active[code]=1;
	  }
	  break;
	case SDL.JOYHATMOTION:
	case SDL.JOYBALLMOTION:
	  break;
	case SDL.MOUSEMOTION:
	  if( is_full )
	  {
	    int dx = evt->x - screen->w/2;
	    int dy = evt->y - screen->h/2;

	    if( nomove ) {
		odx=0;
		ody=0;
		break;
	    }
	    mmx += ((float)(dx-odx) / screen->w)*2.0;
	    mmy += ((float)(dy-ody) / screen->h)*2.0;
	    odx = dx; ody = dy;

	    if( mmx > 1.0 ) mmx = 1.0; else if( mmx < 0.0 ) mmx = 0.0;
	    if( mmy > 1.0 ) mmy = 1.0; else if( mmy < 0.0 ) mmy = 0.0;

#define SC 30
	    if( (abs(dx)>SC) || (abs(dy)>SC) )
	    {
	      if( dx > SC )
	      {
		key_evt( Event( MOUSE_RIGHT, 1 ) );
		key_evt( Event( MOUSE_RIGHT, 0 ) );
	      }
	      else if( dy > SC )
	      {
		key_evt( Event( MOUSE_DOWN, 1 ) );
		key_evt( Event( MOUSE_DOWN, 0 ) );
	      }
	      else if( dx < -SC )
	      {
		key_evt( Event( MOUSE_LEFT, 1 ) );
		key_evt( Event( MOUSE_LEFT, 0 ) );
	      }
	      else if( dy < -SC )
	      {
		key_evt( Event( MOUSE_UP, 1 ) );
		key_evt( Event( MOUSE_UP, 0 ) );
	      }
	      nomove = 1;
	      SDL.warp_mouse( screen->w/2, screen->h/2 );
	      call_out(lambda(){nomove = 0;},0.01);
	    }
	  }
	  else
	  {
	      mmx = evt->x/(float)screen->w;
	      mmy = evt->y/(float)screen->h;
	  }
	  key_evt( MouseAbs( 0,1,mmx,mmy ) );
	  key_evt( MouseAbs( 0,0,mmx,mmy ) );
	  break;
	case SDL.VIDEORESIZE:
	  if( !is_full )
	    set_mode( is_full, bpp, evt->w, evt->h, 0 );
	  break;
	case SDL.ACTIVEEVENT:
 	  configure_event( 0,0,!evt->gain );
	  break;
	case SDL.VIDEOEXPOSE: 
	  // This will be handled shortly anyway.
	  break;
        case SDL.QUIT:
	  key_evt( Event( EXIT, 1 ) );
	  break;
	default:
//#ifdef VP_DEBUG
	  werror("SDL: Got unhandled event of type %d: %O\n", evt->type, evt );
//#endif
      }
    } )
      werror("Error in event handler: %s\n", describe_backtrace( err ) );
  }
}

static void create( function event, function config )
{
  key_evt = event;
  configure_event = config;
}

int set_resolution( int x, int y )
{
  return set_mode( is_full, bpp, x, y, 0 );
}

int set_mode( int fullscreen, int depth,
	      int width, int height, int _gl_flags )
{
  if( depth == 16 )
  {
    SDL.gl_set_attribute( SDL.GL_RED_SIZE,   5 );
    SDL.gl_set_attribute( SDL.GL_GREEN_SIZE, 6 );
    SDL.gl_set_attribute( SDL.GL_BLUE_SIZE,  5 );
  }
  is_full = fullscreen;
//   if(fullscreen)
    hide_cursor();
//   else
//     show_cursor();
  screen = SDL.set_video_mode( width, height, (bpp=depth),
			       SDL.OPENGL|SDL.ANYFORMAT|
			       (fullscreen*SDL.FULLSCREEN)|
			       ((!fullscreen)*SDL.RESIZABLE));
  configure_event( width, height );
  return 1;
}

void flush() { }

void swap_buffers() { SDL.gl_swap_buffers(); };
void exit() { SDL.quit(); }

void init(void|string title, void|string icon)
{
  SDL.init( SDL.INIT_VIDEO 
#if constant(SDL.Joystick)
	    | SDL.INIT_JOYSTICK 
#endif
	    /*| SDL.INIT_EVENTTHREAD*/ );
  SDL.enable_unicode( 1 );
  event_handler();
  SDL.gl_set_attribute( SDL.GL_RED_SIZE, 8 );
  SDL.gl_set_attribute( SDL.GL_GREEN_SIZE, 8 );
  SDL.gl_set_attribute( SDL.GL_BLUE_SIZE, 8 );
  SDL.gl_set_attribute( SDL.GL_DEPTH_SIZE, 8 );
  SDL.gl_set_attribute( SDL.GL_DOUBLEBUFFER, 1 );

#if constant(SDL.Joystick)
  for( int i=0; i<SDL.num_joysticks(); i++ )
    catch {
      SDL.Joystick j = SDL.Joystick(i);
      sticks += ({ j });
    };
#endif

  if(title || icon)
    SDL.set_caption( title||"", icon||"" );
}

static int X_sym( SDL.Keysym sym )
{
  int v = sym->scancode;
#if 0
  //  if(sym->mod & 3) v = -v;
  if( sym->unicode )
    keymap[ v ] = sym->unicode;
#endif
  return ([
    SDL.K_F1:F1,  SDL.K_F2:F2,   SDL.K_F3:F3,     SDL.K_F4:F4,
    SDL.K_F5:F5,  SDL.K_F6:F6,   SDL.K_F7:F7,     SDL.K_F8:F8,  
    SDL.K_F9:F9,  SDL.K_F10:F10, SDL.K_F11:F11,   SDL.K_F12:F12,    
    SDL.K_BACKSPACE:BACKSPACE,  SDL.K_DELETE:DELETE,SDL.K_TAB:TAB,
    SDL.K_ESCAPE:ESCAPE,SDL.K_UP:UP,SDL.K_DOWN:DOWN,SDL.K_RIGHT:RIGHT,
    SDL.K_LEFT:LEFT,SDL.K_PAGEUP:PGUP, SDL.K_PAGEDOWN:PGDWN, 
    SDL.K_RETURN:ENTER,SDL.K_HOME:HOME,SDL.K_END:END,SDL.K_BREAK:PAUSE,
    SDL.K_PAUSE:PAUSE,SDL.K_INSERT:INSERT, SDL.K_SYSREQ:SYS_REQ,
    SDL.K_CAPSLOCK:CAPSLOCK,SDL.K_MENU:MENU,SDL.K_NUMLOCK:NUMLOCK,
    SDL.K_PRINT:PRINT_SCRN,

    SDL.K_LSHIFT:LSHIFT, SDL.K_RSHIFT:RSHIFT, 
    SDL.K_LCTRL:LCTRL, SDL.K_RCTRL:RCTRL, 
    SDL.K_LALT:LALT, SDL.K_RALT:RALT, 
    SDL.K_LMETA:LALT, SDL.K_RMETA:RALT, 

#ifdef __NT__
    SDL.K_BACKQUOTE:167,
#endif
#if 0
    SDL.K_a:'a', SDL.K_b:'b', SDL.K_c:'c', SDL.K_d:'d',
    SDL.K_e:'e', SDL.K_f:'f', SDL.K_g:'g', SDL.K_h:'h',
    SDL.K_i:'i', SDL.K_j:'j', SDL.K_k:'k', SDL.K_l:'l',
    SDL.K_m:'m', SDL.K_n:'n', SDL.K_o:'o', SDL.K_p:'p',
    SDL.K_q:'q', SDL.K_r:'r', SDL.K_s:'s', SDL.K_t:'t',
    SDL.K_u:'u', SDL.K_v:'v', SDL.K_w:'w', SDL.K_x:'x',
    SDL.K_y:'y', SDL.K_z:'z',

    SDL.K_1:'1', SDL.K_2:'2', SDL.K_3:'3', SDL.K_4:'4',
    SDL.K_5:'5', SDL.K_6:'6', SDL.K_7:'7', SDL.K_8:'8',
    SDL.K_9:'9', SDL.K_0:'0',

    SDL.K_AMPERSAND:'&', SDL.K_ASTERISK:'*', SDL.K_AT:'@',
    SDL.K_BACKQUOTE:'`', SDL.K_BACKSLASH:'\\', SDL.K_COLON:':',
    SDL.K_COMMA:',', SDL.K_DOLLAR:'$', SDL.K_EQUALS:'=',
    SDL.K_EXCLAIM:'!', SDL.K_GREATER:'>', SDL.K_HASH:'#',
    SDL.K_LEFTBRACKET:'[', SDL.K_LEFTPAREN:'(',
    SDL.K_LESS:'<', SDL.K_MINUS:'-', SDL.K_PERIOD:'.',
    SDL.K_PLUS:'+', SDL.K_QUESTION:'?', SDL.K_QUOTE:'\'',
    SDL.K_QUOTEDBL:'"', SDL.K_RIGHTBRACKET:']',
    SDL.K_RIGHTPAREN:')', SDL.K_SEMICOLON:';',
    SDL.K_SLASH:'/', SDL.K_SPACE:' ', SDL.K_UNDERSCORE:'_',

    // CARET CLEAR COMPOSE EURO FIRST HELP KP* LMETA LSUPER
    // MODE POWER RMETA RSUPER
#endif
  ])[ sym->sym ] || keymap[ v ] || sym->sym;
}

void show_cursor() {
  SDL.show_cursor( SDL.ENABLE );
}

void hide_cursor() {
  SDL.show_cursor( SDL.DISABLE );
}

#endif /* constant(SDL.Surface) */
