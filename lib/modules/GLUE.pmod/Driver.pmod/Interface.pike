//
// $Id: Interface.pike,v 1.1 2004/01/24 21:35:45 nilsson Exp $

//!
static void create( function event, function config );

//!
void set_resolution( int(0..) x, int(0..) y );

//!
void flush();

//!
void set_mode( int(0..1) fullscreen, int depth,
	       int width, int height, int gl_flags );

//!
array parse_argv( array argv );

//!
void swap_buffers();

//!
void exit();

//!
void init(void|string title, void|string icon);







