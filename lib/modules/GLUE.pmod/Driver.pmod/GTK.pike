inherit .Interface;
import GLUE.Events;

static GTK.GLArea area;
static GTK.Window window;
static int gl_flags;

static class MEvent
{
  inherit Event;
  constant use_modifiers = 1;
}

static function evt, configure_event;

static void create( function event, function config )
{
  evt = event;
  configure_event = config;
}

void set_resolution( int x, int y )
{
  area->set_usize( x,y );
  window->set_usize( x,y );
  window->set_upos( 0, 0 );
#if constant( ChangeRes.change_res )
  ChangeRes.change_res( x,y );
  ChangeRes.pan_to( 0,0 );
#endif
}

void flush()
{
  GTK.flush();
}

void set_mode( int fullscreen, int depth,
	       int width, int height, int _gl_flags )
{
  gl_flags = _gl_flags;
  if( window )
    window->destroy();

  window =
    GTK.Window( fullscreen ? GTK.WindowPopup : GTK.WindowToplevel);

  window->signal_connect( "configure_event", lambda() {
					       configure_event(window->xsize(),
							       window->ysize());
					     });  


  window->signal_connect( "key_press_event",
			  lambda(int x, GTK.Widget w, GDK.Event e){
			    evt( MEvent( e->keyval, 1, e->data, e->state ) );
			  } );

  window->signal_connect( "key_release_event",
			  lambda(int x, GTK.Widget w, GDK.Event e){
			    evt( MEvent( e->keyval, 0, e->data, e->state ) );
			  } );

  window->signal_connect( "button_press_event",
			  lambda(int x, GTK.Widget w, GDK.Event e){
			    evt( MEvent( -e->button, 1,0,e->state ) );
			  } );
  window->signal_connect( "button_release_event",
			  lambda(int x, GTK.Widget w, GDK.Event e){
			    evt( MEvent( -e->button, 0,0,e->state ) );
			  } );
  window->add_events( GDK.KeyPressMask | GDK.KeyReleaseMask |
		      GDK.StructureMask | GDK.ButtonPressMask |
		      GDK.ButtonReleaseMask );
  window->set_policy( 1, 1, 1 );

  window->set_usize( width, height );

  window->realize();
  if( fullscreen )
  {
#if constant( ChangeRes.change_res )
    restore_res =
      lambda() {
	ChangeRes.change_res( @rows( GTK.root_window()->get_geometry(),
				     ({ "width", "height" })) );
      };
    ChangeRes.change_res( width, height );
    ChangeRes.pan_to( 0,0 );
#endif
	    
    window->show_now();
    GDK.Window w = window->get_gdkwindow();
    GTK.move_cursor_abs( w, 10, 10 );
    w->grab_input_focus();

    hide_cursor();
    window->add_events( GDK.PointerMotionMask );
    int nomove;
    window->signal_connect( "motion_notify_event",
			    lambda( int x, GTK.Widget wi, GDK.Event e )
			    {
			      if( nomove )
				return;
			      int dx = ((int)e->x) - window->xsize()/2;
			      int dy = ((int)e->y) - window->ysize()/2;
#define SC 30
			      if( (abs(dx)>SC) || (abs(dy)>SC) )
			      {
				if( dx > SC )
				{
				  evt( MEvent( MOUSE_RIGHT, 1 ) );
				  evt( MEvent( MOUSE_RIGHT, 0 ) );
				}
				else if( dy > SC )
				{
				  evt( MEvent( MOUSE_DOWN, 1 ) );
				  evt( MEvent( MOUSE_DOWN, 0 ) );
				}
				else if( dx < -SC )
				{
				  evt( MEvent( MOUSE_LEFT, 1 ) );
				  evt( MEvent( MOUSE_LEFT, 0 ) );
				}
				else if( dy < -SC )
				{
				  evt( MEvent( MOUSE_UP, 1 ) );
				  evt( MEvent( MOUSE_UP, 0 ) );
				}
				nomove = 1;
				GTK.flush();
				GTK.move_cursor_abs( w, window->xsize()/2,
						     window->ysize()/2 );
				GTK.flush();
				call_out(lambda(){nomove = 0;},0.01);
			      }
			    }
			  );
  }
#if constant( ChangeRes.change_res )
  else
    ChangeRes.change_res( @rows( GTK.root_window()->get_geometry(),
				 ({ "width", "height" })) );
#endif

  area = GTK.GLArea(({gl_flags | GTK.GDK_GL_RGBA,
		      GTK.GDK_GL_DOUBLEBUFFER,
		      /* GTK.GDK_GL_DEPTH_SIZE,1,*/0 }) )->show();
  window->add( area )->show();
  area->make_current();
  configure_event( width, height );
}

void swap_buffers()
{
  area->swap_buffers();
}

static void repeat( int r )
{
#ifdef __NT__
  // What?
#else
  Process.create_process( ({ "xset", "r", r?"on":"off" }) );
#endif
}

void init(void|string title, void|string icon)
{
  GTK.setup_gtk();
  if(title)
    GTK.root_window()->set_title(title);
  if(icon)
    GTK.root_window()->set_icon_name(icon);
  repeat(0);
}

static function(void:void) restore_res;
void exit() {
  repeat(1);
  if(restore_res) restore_res();
}

void hide_cursor() {
  GDK.Bitmap bm = GDK.Bitmap( Image.Image( 1,1 ) );
  GDK.Color c   = GDK.Color( 0,0,0 );
  GDK.Window w = window->get_gdkwindow();
  w->set_bitmap_cursor( bm,bm,c,c,0,0 );
}

void show_cursor() {
  // FIXME: What do we do.
}
