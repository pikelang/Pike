//! a GDK2.Window object.
//!
//! NOIMG
//!
//!

inherit GDK2.Drawable;

GDK2.Window change_property( GDK2.Atom property, GDK2.Atom type, int mode, string data );
//! mode is one of @[GDK_PROP_MODE_APPEND], @[GDK_PROP_MODE_PREPEND] and @[GDK_PROP_MODE_REPLACE]
//!
//!

array children( );
//! Returns an array of GDK2.Window objects.
//!
//!

static GDK2.Window create( GTK2.GdkWindow parent, mapping|void attributes );
//! Not for non-experts. I promise.
//!
//!

GDK2.Window delete_property( GDK2.Atom a );
//! Delete a property.
//!
//!

mapping get_geometry( );
//! Returns ([ "x":xpos, "y":ypos, "width":width, "height":height, "depth":bits_per_pixel ])
//!
//!

mapping get_property( GDK2.Atom atom, int|void offset, int|void delete_when_done );
//! Returns the value (as a string) of the specified property.
//! The arguments are:
//! 
//! property: The property atom, as an example GDK2.Atom.__SWM_VROOT
//! offset (optional): The starting offset, in elements
//! delete_when_done (optional): If set, the property will be deleted when it has
//! been fetched.
//! 
//! Example usage: Find the 'virtual' root window (many window managers
//! put large windows over the screen)
//! 
//! @pre{
//!   GDK2.Window root = GTK.root_window();
//!   array maybe=root->children()->
//!               get_property(GDK2.Atom.__SWM_VROOT)-({0});
//!   if(sizeof(maybe))
//!     root=GDK2.Window( maybe[0]->data[0] );
//! @}
//!
//!

int is_viewable( );
//! Return 1 if the window is mapped.
//!
//!

int is_visible( );
//! Return 1 if the window, or a part of the window, is visible right now.
//!
//!

GDK2.Window lower( );
//! Lower this window if the window manager allows that.
//!
//!

GDK2.Window move_resize( int x, int y, int w, int h );
//! Move and resize the window in one call.
//!
//!

GDK2.Window raise( );
//! Raise this window if the window manager allows that.
//!
//!

GDK2.Window set_background( GTK2.GdkColor to );
//! Set the background color or image.
//! The argument is either a GDK2.Pixmap or a GDK2.Color object.
//!
//!

GDK2.Window set_bitmap_cursor( GTK2.GdkBitmap image, GTK2.GdkBitmap mask, GTK2.GdkColor fg, GTK2.GdkColor bg, int xhot, int yhot );
//! xhot,yhot are the locations of the x and y hotspot relative to the
//! upper left corner of the cursor image.
//!
//!

GDK2.Window set_cursor( int new_cursor );
//! Change the window cursor.<table border="0" cellpadding="3" cellspacing="0">
//! CURS(GDK2.Arrow)
//! CURS(GDK2.BasedArrowDown)
//! CURS(GDK2.BasedArrowUp)
//! CURS(GDK2.Boat)
//! CURS(GDK2.Bogosity)
//! CURS(GDK2.BottomLeftCorner)
//! CURS(GDK2.BottomRightCorner)
//! CURS(GDK2.BottomSide)
//! CURS(GDK2.BottomTee)
//! CURS(GDK2.BoxSpiral)
//! CURS(GDK2.CenterPtr)
//! CURS(GDK2.Circle)
//! CURS(GDK2.Clock)
//! CURS(GDK2.CoffeeMug)
//! CURS(GDK2.Cross)
//! CURS(GDK2.CrossReverse)
//! CURS(GDK2.Crosshair)
//! CURS(GDK2.DiamondCross)
//! CURS(GDK2.Dot)
//! CURS(GDK2.Dotbox)
//! CURS(GDK2.DoubleArrow)
//! CURS(GDK2.DraftLarge)
//! CURS(GDK2.DraftSmall)
//! CURS(GDK2.DrapedBox)
//! CURS(GDK2.Exchange)
//! CURS(GDK2.Fleur)
//! CURS(GDK2.Gobbler)
//! CURS(GDK2.Gumby)
//! CURS(GDK2.Hand1)
//! CURS(GDK2.Hand2)
//! CURS(GDK2.Heart)
//! CURS(GDK2.Icon)
//! CURS(GDK2.IronCross)
//! CURS(GDK2.LeftPtr)
//! CURS(GDK2.LeftSide)
//! CURS(GDK2.LeftTee)
//! CURS(GDK2.Leftbutton)
//! CURS(GDK2.LlAngle)
//! CURS(GDK2.LrAngle)
//! CURS(GDK2.Man)
//! CURS(GDK2.Middlebutton)
//! CURS(GDK2.Mouse)
//! CURS(GDK2.Pencil)
//! CURS(GDK2.Pirate)
//! CURS(GDK2.Plus)
//! CURS(GDK2.QuestionArrow)
//! CURS(GDK2.RightPtr)
//! CURS(GDK2.RightSide)
//! CURS(GDK2.RightTee)
//! CURS(GDK2.Rightbutton)
//! CURS(GDK2.RtlLogo)
//! CURS(GDK2.Sailboat)
//! CURS(GDK2.SbDownArrow)
//! CURS(GDK2.SbHDoubleArrow)
//! CURS(GDK2.SbLeftArrow)
//! CURS(GDK2.SbRightArrow)
//! CURS(GDK2.SbUpArrow)
//! CURS(GDK2.SbVDoubleArrow)
//! CURS(GDK2.Shuttle)
//! CURS(GDK2.Sizing)
//! CURS(GDK2.Spider)
//! CURS(GDK2.Spraycan)
//! CURS(GDK2.Star)
//! CURS(GDK2.Target)
//! CURS(GDK2.Tcross)
//! CURS(GDK2.TopLeftArrow)
//! CURS(GDK2.TopLeftCorner)
//! CURS(GDK2.TopRightCorner)
//! CURS(GDK2.TopSide)
//! CURS(GDK2.TopTee)
//! CURS(GDK2.Trek)
//! CURS(GDK2.UlAngle)
//! CURS(GDK2.Umbrella)
//! CURS(GDK2.UrAngle)
//! CURS(GDK2.Watch)
//! CURS(GDK2.Xterm)
//! </table>
//!
//!

GDK2.Window set_events( int events );
//! events is a bitwise or of one or more of the following constants:
//! GDK2.ExposureMask,
//! GDK2.PointerMotionMask,
//! GDK2.PointerMotion_HINTMask,
//! GDK2.ButtonMotionMask,
//! GDK2.Button1MotionMask,
//! GDK2.Button2MotionMask,
//! GDK2.Button3MotionMask,
//! GDK2.ButtonPressMask,
//! GDK2.ButtonReleaseMask,
//! GDK2.KeyPressMask,
//! GDK2.KeyReleaseMask,
//! GDK2.EnterNotifyMask,
//! GDK2.LeaveNotifyMask,
//! GDK2.FocusChangeMask,
//! GDK2.StructureMask,
//! GDK2.PropertyChangeMask,
//! GDK2.VisibilityNotifyMask,
//! GDK2.ProximityInMask,
//! GDK2.ProximityOutMask and
//! GDK2.AllEventsMask
//!
//!

GDK2.Window set_icon( GTK2.GdkPixmap pixmap, GTK2.GdkBitmap mask, GTK2.GdkWindow window );
//! Set the icon to the specified image (with mask) or the specified
//! GDK2.Window.  It is up to the window manager to display the icon.
//! Most window manager handles window and pixmap icons, but only a few
//! can handle the mask argument. If you want a shaped icon, the only
//! safe bet is a shaped window.
//!
//!

GDK2.Window set_icon_name( string name );
//! Set the icon name to the specified string.
//!
//!

GDK2.Window shape_combine_mask( GTK2.GdkBitmap mask, int xoffset, int yoffset );
//! Set the shape of the widget, or, rather, it's window, to that of
//! the supplied bitmap.
//!
//!
