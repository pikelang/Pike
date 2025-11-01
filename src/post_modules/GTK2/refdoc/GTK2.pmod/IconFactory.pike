// Automatically generated from "gtkiconfactory.pre".
// Do NOT edit.

//! Icon factory, for holding icon sets.
//!
//!

inherit G.Object;
//!

GTK2.IconFactory add( string stock_id, GTK2.IconSet icon_set );
//! Adds the given icon_set to the icon factory, under the name of stock_id.
//! stock_id should be namespaced for your application, e.g. 
//! "myapp-whatever-icon".  Normally applications create a GTK2.IconFactory,
//! then add it to the list of default factories with add_default().  Then they
//! pass the stock_id to widgets such as GTK2.Image to display the icon.  Themes
//! can provide an icon with the same name (such as "myapp-whatever-icon") to
//! override your application's default icons.  If an icon already existed in
//! this factory for stock_id, it is unreferenced and replaced with the new
//! icon_set.
//!
//!

GTK2.IconFactory add_default( );
//! Adds an icon factory to the list of icon factories search by 
//! GTK2.Style->lookup_icon_set().  This means that, for example, 
//! GTK2.Image->create("stock-id") will be able to find icons in factory.  There
//! will normally be an icon factory added for each library or application that
//! comes with icons.  The default icon factories can be overridden by themes.
//!
//!

protected void create( );
//! Creates a new GTK2.IconFactory.  An icon factory manages a collection of
//! GTK2.IconSets; a GTK2.IconSet manages a set of variants of a particular icon
//! (i.e. a GTK2.IconSet contains variants for different sizes and widget
//! states).  Icons in an icon factory are named by a stock ID, which is a
//! simple string identifying the icon.  Each GTK2.Style has a list of
//! GTK2.IconFactorys derived from the current theme; those icon factories are
//! consulted first when searching for an icon.  If the theme doesn't set a
//! particular icon, GTK2+ looks for the icon in a list of default icon
//! factories, maintained by add_default() and remove_default().  Applications
//! with icons should add a default icon factory with their icons, which will
//! allow themes to override the icons for the application.
//!
//!

GTK2.IconSet lookup( string stock_id );
//! Looks up stock_id in the icon factory, returning an icon set if found,
//! otherwise 0.  For display to the user, you should use 
//! GTK2.Style->lookup_icon_set() on the GTK2.Style for the widget that will 
//! display the icon, instead of using this function directly, so that themes
//! are taken into account.
//!
//!

GTK2.IconSet lookup_default( string stock_id );
//! Looks for an icon in the list of default icon factories.  For display to
//! the user, you should use GTK2.Style->lookup_icon_set() on the GTK2.Style for
//! the widget that will display the icon, instead of using this function
//! directly, so that themes are taken into account.
//!
//!

GTK2.IconFactory remove_default( );
//! Removes an icon factory from the list of default icon factories.
//!
//!
