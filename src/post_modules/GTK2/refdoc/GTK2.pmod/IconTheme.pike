//!

inherit G.Object;

GTK2.IconTheme append_search_path( string path );
//! Appends a directory to the search path.
//!
//!

static GTK2.IconTheme create( );
//! Creates a new icon theme object.  Icon theme objects are used to lookup
//! an icon by name in a particular icon theme.  Usually you'll want to use
//! get_default() rather than creating a new icon theme object from scratch.
//!
//!

string get_example_icon_name( );
//! Gets the name of an icon that is representative of the current theme (for
//! instance, to use when presenting a list of themes to the user.)
//!
//!

array get_icon_sizes( string name );
//! Returns an array of integers describing the sizes at which the icon is
//! available without scaling.  A size of -1 means that the icon is available
//! in a scalable format.
//!
//!

array get_search_path( );
//! Gets the current search path.
//!
//!

int has_icon( string icon_name );
//! Checks whether this icon theme includes an icon for a particular name.
//!
//!

array list_icons( string|void context );
//! Lists the icons in the current icon theme.  Only a subset of the icons can
//! be listed by providing a context string.  The set of values for the
//! context string is system dependent, but will typically include such values
//! as 'apps' and 'mimetypes'.
//!
//!

GTK2.GdkPixbuf load_icon( string name, int size, int flags );
//! Looks up an icon in an icon theme, scales it to the given size and renders
//! it into a pixbuf.
//!
//!

GTK2.IconInfo lookup_icon( string name, int size, int flags );
//! Looks up a named icon and returns an object containing information such as
//! the filename of the icon.  The icon can then be rendered into a pixbuf
//! using GTK2.IconInfo->load_icon().
//!
//!

GTK2.IconTheme prepend_search_path( string path );
//! Prepends a directory to the search path.
//!
//!

int rescan_if_needed( );
//! Checks to see if the icon theme has changed; if it has, any currently
//! cached information is discarded and will be reloaded next time this theme
//! is accessed.
//!
//!

GTK2.IconTheme set_custom_theme( string theme_name );
//! Sets the name of the icon theme that the GTK2.IconTheme object uses
//! overriding system configuration.  This function cannot be called on the
//! icon theme objects return from get_default().
//!
//!

GTK2.IconTheme set_search_path( array path );
//! Sets the search path for the icon theme object.  When looking for an icon
//! theme, GTK2+ will search for a subdirectory of one or more of the
//! directories in path with the same name as the icon theme.  (Themes from
//! multiple of the path elemets are combined to allow themes to be extended
//! by adding icons in the user's home directory.)
//! 
//! In addition if an icon found isn't found either in the current icon theme
//! or the default icon theme, and an image file with the right name is found
//! directly in one of the elements of path, then that image will be used for
//! the icon name.  (This is a legacy feature, and new icons should be put into
//! the default icon theme, which is called DEFAULT_THEME_NAME, rather than
//! directly on the icon path.)
//!
//!
