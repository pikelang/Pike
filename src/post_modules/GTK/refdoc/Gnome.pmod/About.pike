//! A standard way of providing a small about box for your
//! application. You provide the name of your application, version,
//! copyright, a list of authors and some comments about your
//! application. It also allows the programmer to provide a logo to be
//! displayed.
//! 
//!@code{ Gnome.About( "Example", "1.0", "(c) Roxen IS 2000", ({"Per Hedbor"}), "Some nice documentation\nabout this example" );@}
//!@xml{<image>../images/gnome_about.png</image>@}
//!
//! 
//!
//!

inherit Gnome.Dialog;

static Gnome.About create( string title, string version, string copyright, array authors, string comment, string|void logo );
//! Creates a new GNOME About dialog. title, version, copyright, and
//! authors are displayed first, in that order. comments is typically
//! the location for multiple lines of text, if necessary. (Separate
//! with "\n".) logo is the filename of a optional pixmap to be
//! displayed in the dialog, typically a product or company logo of
//! some sort; omit this argument if no logo file is available.
//!
//!
