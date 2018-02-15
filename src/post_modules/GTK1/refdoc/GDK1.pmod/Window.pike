//! a GDK1.Window object.
//!
//! NOIMG
//!
//!

inherit GDK1.Drawable;

GDK1.Window change_property( GDK1.Atom property, GDK1.Atom type, int mode, string data );
//! mode is one of @[GDK_PROP_MODE_APPEND], @[GDK_PROP_MODE_PREPEND] and @[GDK_PROP_MODE_REPLACE]
//!
//!

array children( );
//! Returns an array of GDK1.Window objects.
//!
//!

protected GDK1.Window create( GDK1.Window parent, mapping|void attributes );
//! Not for non-experts. I promise.
//!
//!

GDK1.Window delete_property( GDK1.Atom a );
//!

mapping get_geometry( );
//! Returns ([ "x":xpos, "y":ypos, "width":width, "height":height, "depth":bits_per_pixel ])
//!
//!

mapping get_pointer( int deviceid );
//! Get the position of the specified device in this window.
//!
//!

mapping get_property( GDK1.Atom atom, int|void offset, int|void delete_when_done );
//! Returns the value (as a string) of the specified property.
//! The arguments are:
//! 
//! property: The property atom, as an example GDK1.Atom.__SWM_VROOT
//! offset (optional): The starting offset, in elements
//! delete_when_done (optional): If set, the property will be deleted when it has
//! been fetched.
//! 
//! Example usage: Find the 'virtual' root window (many window managers
//! put large windows over the screen)
//! 
//! @pre{
//!   GDK1.Window root = GTK1.root_window();
//!   array maybe=root->children()->
//!               get_property(GDK1.Atom.__SWM_VROOT)-({0});
//!   if(sizeof(maybe))
//!     root=GDK1.Window( maybe[0]->data[0] );
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

GDK1.Window lower( );
//! Lower this window if the window manager allows that.
//!
//!

GDK1.Window move_resize( int x, int y, int w, int h );
//! Move and resize the window in one call.
//!
//!

GDK1.Window raise( );
//! Raise this window if the window manager allows that.
//!
//!

GDK1.Window set_background( GDK1.Color to );
//! Set the background color or image.
//! The argument is either a GDK1.Pixmap or a GDK1.Color object.
//!
//!

GDK1.Window set_bitmap_cursor( GDK1.Bitmap image, GDK1.Bitmap mask, GDK1.Color fg, GDK1.Color bg, int xhot, int yhot );
//! xhot,yhot are the locations of the x and y hotspot relative to the
//! upper left corner of the cursor image.
//!
//!

GDK1.Window set_cursor( int new_cursor );
//! Change the window cursor.<table border="0" cellpadding="3" cellspacing="0">
//! CURS(GDK1.Arrow)
//! CURS(GDK1.BasedArrowDown)
//! CURS(GDK1.BasedArrowUp)
//! CURS(GDK1.Boat)
//! CURS(GDK1.Bogosity)
//! CURS(GDK1.BottomLeftCorner)
//! CURS(GDK1.BottomRightCorner)
//! CURS(GDK1.BottomSide)
//! CURS(GDK1.BottomTee)
//! CURS(GDK1.BoxSpiral)
//! CURS(GDK1.CenterPtr)
//! CURS(GDK1.Circle)
//! CURS(GDK1.Clock)
//! CURS(GDK1.CoffeeMug)
//! CURS(GDK1.Cross)
//! CURS(GDK1.CrossReverse)
//! CURS(GDK1.Crosshair)
//! CURS(GDK1.DiamondCross)
//! CURS(GDK1.Dot)
//! CURS(GDK1.Dotbox)
//! CURS(GDK1.DoubleArrow)
//! CURS(GDK1.DraftLarge)
//! CURS(GDK1.DraftSmall)
//! CURS(GDK1.DrapedBox)
//! CURS(GDK1.Exchange)
//! CURS(GDK1.Fleur)
//! CURS(GDK1.Gobbler)
//! CURS(GDK1.Gumby)
//! CURS(GDK1.Hand1)
//! CURS(GDK1.Hand2)
//! CURS(GDK1.Heart)
//! CURS(GDK1.Icon)
//! CURS(GDK1.IronCross)
//! CURS(GDK1.LeftPtr)
//! CURS(GDK1.LeftSide)
//! CURS(GDK1.LeftTee)
//! CURS(GDK1.Leftbutton)
//! CURS(GDK1.LlAngle)
//! CURS(GDK1.LrAngle)
//! CURS(GDK1.Man)
//! CURS(GDK1.Middlebutton)
//! CURS(GDK1.Mouse)
//! CURS(GDK1.Pencil)
//! CURS(GDK1.Pirate)
//! CURS(GDK1.Plus)
//! CURS(GDK1.QuestionArrow)
//! CURS(GDK1.RightPtr)
//! CURS(GDK1.RightSide)
//! CURS(GDK1.RightTee)
//! CURS(GDK1.Rightbutton)
//! CURS(GDK1.RtlLogo)
//! CURS(GDK1.Sailboat)
//! CURS(GDK1.SbDownArrow)
//! CURS(GDK1.SbHDoubleArrow)
//! CURS(GDK1.SbLeftArrow)
//! CURS(GDK1.SbRightArrow)
//! CURS(GDK1.SbUpArrow)
//! CURS(GDK1.SbVDoubleArrow)
//! CURS(GDK1.Shuttle)
//! CURS(GDK1.Sizing)
//! CURS(GDK1.Spider)
//! CURS(GDK1.Spraycan)
//! CURS(GDK1.Star)
//! CURS(GDK1.Target)
//! CURS(GDK1.Tcross)
//! CURS(GDK1.TopLeftArrow)
//! CURS(GDK1.TopLeftCorner)
//! CURS(GDK1.TopRightCorner)
//! CURS(GDK1.TopSide)
//! CURS(GDK1.TopTee)
//! CURS(GDK1.Trek)
//! CURS(GDK1.UlAngle)
//! CURS(GDK1.Umbrella)
//! CURS(GDK1.UrAngle)
//! CURS(GDK1.Watch)
//! CURS(GDK1.Xterm)
//! </table>
//!
//!

GDK1.Window set_events( int events );
//! events is a bitwise or of one or more of the following constants:
//! GDK1.ExposureMask,
//! GDK1.PointerMotionMask,
//! GDK1.PointerMotion_HINTMask,
//! GDK1.ButtonMotionMask,
//! GDK1.Button1MotionMask,
//! GDK1.Button2MotionMask,
//! GDK1.Button3MotionMask,
//! GDK1.ButtonPressMask,
//! GDK1.ButtonReleaseMask,
//! GDK1.KeyPressMask,
//! GDK1.KeyReleaseMask,
//! GDK1.EnterNotifyMask,
//! GDK1.LeaveNotifyMask,
//! GDK1.FocusChangeMask,
//! GDK1.StructureMask,
//! GDK1.PropertyChangeMask,
//! GDK1.VisibilityNotifyMask,
//! GDK1.ProximityInMask,
//! GDK1.ProximityOutMask and
//! GDK1.AllEventsMask
//!
//!

GDK1.Window set_icon( GDK1.Pixmap pixmap, GDK1.Bitmap mask, GDK1.Window window );
//! Set the icon to the specified image (with mask) or the specified
//! GDK1.Window.  It is up to the window manager to display the icon.
//! Most window manager handles window and pixmap icons, but only a few
//! can handle the mask argument. If you want a shaped icon, the only
//! safe bet is a shaped window.
//!
//!

GDK1.Window set_icon_name( string name );
//! Set the icon name to the specified string.
//!
//!

GDK1.Window shape_combine_mask( GDK1.Bitmap mask, int xoffset, int yoffset );
//! Set the shape of the widget, or, rather, it's window, to that of
//! the supplied bitmap.
//!
//!
