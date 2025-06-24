// Automatically generated from "gdkcolor.pre".
// Do NOT edit.

//! The GDK2.Color object is used to represent a color.
//! When you call GDK2.Color(r,g,b) the color will be allocated
//! from the X-server. The constructor can return an  exception if there are
//! no more colors to allocate.
//! NOIMG
//!
//!

protected GDK2.Color _destruct( );
//! Destroys the color object. Please note that this function does
//! not free the color from the X-colormap (in case of pseudocolor)
//! right now.
//!
//!

int blue( );
//! Returns the blue color component.
//!
//!

protected void create( void color_or_r, void g, void b );
//! r g and b are in the range 0 to 255, inclusive.
//! If color is specified, it should be an Image.Color object, and the
//! only argument.
//!
//!

int green( );
//! Returns the green color component.
//!
//!

Image.Color.Color image_color_object( );
//! Return a Image.Color.Color instance.
//! This gives better precision than the rgb function.
//!
//!

int pixel( );
//! Returns the pixel value of the color. See @xml{<url  href="gdk.image.xml#set_pixel">GDK2.Image->set_pixel</url>@}.
//!
//!

int red( );
//! Returns the red color component.
//!
//!

array rgb( );
//! Returns the red green and blue color components as an array.
//!
//!
