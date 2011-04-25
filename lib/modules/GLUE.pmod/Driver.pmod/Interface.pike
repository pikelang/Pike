//
// $Id$

#pike __REAL_VERSION__

// Internal interface class for Drivers.

static void create( function event, function config );

void set_resolution( int(0..) x, int(0..) y );

void flush();

void set_mode( int(0..1) fullscreen, int depth,
	       int width, int height, int gl_flags );

array parse_argv( array argv );

void swap_buffers();

void exit();

void init(void|string title, void|string icon);







