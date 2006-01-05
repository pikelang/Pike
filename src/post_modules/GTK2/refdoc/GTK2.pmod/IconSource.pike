//! Iconsource.
//!
//!

static GTK2.IconSource create( );
//! Creates a new GTK2.IconSource.  A GTK2.IconSource contains a GDK2.Pixbuf (or
//! image filename) that serves as the base image for one or more of the icons
//! in a GTK2.IconSet, along with a specification for which icons in the icon
//! set will be based on that pixbuf or image file.  An icon set contains a set
//! of icons that represent "the same" logical concept in different states,
//! different global text directions, and different sizes.
//! 
//! So for example a web browser's "Back to Previous Page" icon might point in
//! a different direction in Hebrew and in English; it might look different
//! when insensitive; and it might change size depending on toolbar mode
//! small/large icons).  So a single icon set would contain all those variants
//! of the icon.  GTK2.IconSet contains a list of GTK2.IconSource from which it
//! can derive specific icon variants in the set.
//! 
//! In the simplest case, GTK2.IconSet contains one source pixbuf from which it
//! derives all variants.  The convenience function GTK2.IconSet->create(pixbuf)
//! handles this case; if you only have one source pixbuf, just use that
//! function.
//! 
//! If you want to use a different base pixbuf for different icon variants, you
//! create multiple icon sources, mark which variants they'll be used to
//! create, and add them to the icon set with GTK2.IconSet->add_source().
//! 
//! By default, the icon source has all parameters wildcarded.  That is, the
//! icon source will be used as the base icon for any desired text direction,
//! widget state, or icon size.
//!
//!

int get_direction( );
//! Obtains the text direction this icon source applies to.  The return value
//! is only useful/meaningful if the text direction is not wildcarded.
//!
//!

int get_direction_wildcarded( );
//! Gets the value set by set_direction_wildcarded().
//!
//!

string get_filename( );
//! Retrieves the source filename, or empty.
//!
//!

string get_icon_name( );
//! Retrieves the source icon name, or empty.
//!
//!

GTK2.GdkPixbuf get_pixbuf( );
//! Retrieves the source pixbuf, or 0.  In addition, if a filename source is in
//! use, this function in some cases will return the pixbuf loaded from the
//! filename.  This is, for example, true for the GTK2.IconSource passed to the
//! GTK2.Style->render_icon() virtual function.
//!
//!

int get_size( );
//! Obtains the icon size this source applies to.  The return value is only
//! useful/meaningful if the icon size is not wildcarded.
//!
//!

int get_size_wildcarded( );
//! Gets the value set by set_size_wildcarded().
//!
//!

int get_state( );
//! Obtains the widget state this icon source applies to.  The return value is
//! only useful/meaningful if the widget state is not wildcarded.
//!
//!

int get_state_wildcarded( );
//! Gets the value set by set_state_wildcarded().
//!
//!

GTK2.IconSource set_direction( int dir );
//! Sets the text direction this icon source is intended to be used with.
//! 
//! Setting the text direction on an icon source makes no difference if the
//! text direction is wildcarded.  Therefore, you should usually call
//! set_direction_wildcarded() to un-wildcard it in addition to calling this
//! function.
//!
//!

GTK2.IconSource set_direction_wildcarded( int setting );
//! If the text direction is wildcarded, this source can be used as the base
//! image for an icon in any text direction.  If the text direction is not
//! wildcarded, then the text direction the icon source applies to should be
//! set with set_direction(), and the icon source will only be used with that
//! text direction.
//! 
//! GTK2.IconSet prefers non-wildcarded sources (exact matches) over wildcarded
//! sources, and will use an exact match when possible.
//!
//!

GTK2.IconSource set_filename( string filename );
//! Sets the name of an image file to use as a base image when creating icon
//! variants for GTK2.IconSet.  The filename must be absolute.
//!
//!

GTK2.IconSource set_icon_name( string name );
//! Sets the name of an icon to look up in the current icon theme to use as a
//! base image.
//!
//!

GTK2.IconSource set_pixbuf( GTK2.GdkPixbuf pixbuf );
//! Sets a pixbuf to use as a base image.
//!
//!

GTK2.IconSource set_size( int size );
//! Sets the icon size this icon source is intended to be with.
//!
//!

GTK2.IconSource set_size_wildcarded( int setting );
//! If the icon size is wildcarded, this source can be used as the base image
//! for an icon of any size.  If the size is not wildcarded, then the size the
//! source applies to should be set with set_size() and the icon source will
//! only be used with that specific size.
//!
//!

GTK2.IconSource set_state( int state );
//! Sets the widget state this icon source is intended to be used with.
//!
//!

GTK2.IconSource set_state_wildcarded( int setting );
//! If the widget state is wildcarded, this source can be used as the base
//! image for an icon in any state.  If the widget state is not wildcarded,
//! then the state the source applies to should be set with set_state() and
//! the icon source will only be used with that specific state.
//!
//!
