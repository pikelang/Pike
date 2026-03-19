//

#pike __REAL_VERSION__

#if constant(GTK2.GLArea)

inherit .Interface;
import GLUE.Events;

protected GTK2.GLArea area;
protected GTK2.Window window;
protected int gl_flags;

protected class MEvent
{
  inherit Event;
  constant use_modifiers = 1;
}

protected function evt, configure_event;

protected void create( function event, function config )
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
  GTK2.flush();
}

void set_mode( int fullscreen, int depth,
	       int width, int height, int _gl_flags )
{
  gl_flags = _gl_flags;
  if( window )
    window->destroy();

  window =
    GTK2.Window( fullscreen ? GTK2.WindowPopup : GTK2.WindowToplevel);

  window->signal_connect( "configure_event", lambda() {
					       configure_event(window->xsize(),
							       window->ysize());
					     });


  window->signal_connect( "key_press_event",
                          lambda(int x, GTK2.Widget w, GDK2.Event e){
			    evt( MEvent( e->keyval, 1, e->data, e->state ) );
			  } );

  window->signal_connect( "key_release_event",
                          lambda(int x, GTK2.Widget w, GDK2.Event e){
			    evt( MEvent( e->keyval, 0, e->data, e->state ) );
			  } );

  window->signal_connect( "button_press_event",
                          lambda(int x, GTK2.Widget w, GDK2.Event e){
			    evt( MEvent( -e->button, 1,0,e->state ) );
			  } );
  window->signal_connect( "button_release_event",
                          lambda(int x, GTK2.Widget w, GDK2.Event e){
			    evt( MEvent( -e->button, 0,0,e->state ) );
			  } );
  window->add_events( GDK2.KeyPressMask | GDK2.KeyReleaseMask |
                      GDK2.StructureMask | GDK2.ButtonPressMask |
                      GDK2.ButtonReleaseMask );
  window->set_policy( 1, 1, 1 );

  window->set_usize( width, height );

  window->realize();
  if( fullscreen )
  {
#if constant(GTK2.move_cursor_abs)
#if constant( ChangeRes.change_res )
    restore_res =
      lambda() {
        ChangeRes.change_res( @rows( GTK2.root_window()->get_geometry(),
				     ({ "width", "height" })) );
      };
    ChangeRes.change_res( width, height );
    ChangeRes.pan_to( 0,0 );
#endif

    window->show_now();
    GDK2.Window w = window->get_gdkwindow();
    GTK2.move_cursor_abs( w, 10, 10 );
    w->grab_input_focus();

    hide_cursor();
    window->add_events( GDK2.PointerMotionMask );
    int nomove;
    window->signal_connect( "motion_notify_event",
                            lambda( int x, GTK2.Widget wi, GDK2.Event e )
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
                                GTK2.flush();
                                GTK2.move_cursor_abs( w, window->xsize()/2,
                                                      window->ysize()/2 );
                                GTK2.flush();
				call_out(lambda(){nomove = 0;},0.01);
			      }
			    }
			  );
#else /* !constant(GTK2.move_cursor_abs) */
    error("Full screen mode not supported with this version of GTK.\n");
#endif /* constant(GTK2.move_cursor_abs) */
  }
#if constant( ChangeRes.change_res )
  else
    ChangeRes.change_res( @rows( GTK2.root_window()->get_geometry(),
				 ({ "width", "height" })) );
#endif

  area = GTK2.GLArea(({gl_flags | GTK2.GDK_GL_RGBA,
                       GTK2.GDK_GL_DOUBLEBUFFER,
                       /* GTK2.GDK_GL_DEPTH_SIZE,1,*/0 }) )->show();
  window->add( area )->show();
  area->make_current();
  configure_event( width, height );
}

void swap_buffers()
{
  area->swap_buffers();
}

protected void repeat( int r )
{
#ifdef __NT__
  // What?
#else
  Process.create_process( ({ "xset", "r", r?"on":"off" }) );
#endif
}

void init(void|string title, void|string icon)
{
  GTK2.setup_gtk();
  if(title)
    GTK2.root_window()->set_title(title);
  if(icon)
    GTK2.root_window()->set_icon_name(icon);
  repeat(0);
}

protected function(void:void) restore_res;
void exit() {
  repeat(1);
  if(restore_res) restore_res();
}

void hide_cursor() {
  GDK2.Bitmap bm = GDK2.Bitmap( Image.Image( 1,1 ) );
  GDK2.Color c   = GDK2.Color( 0,0,0 );
  GDK2.Window w = window->get_gdkwindow();
  w->set_bitmap_cursor( bm,bm,c,c,0,0 );
}

void show_cursor() {
  // FIXME: What do we do.
}

#endif /* constant(GTK2.GLArea) */
