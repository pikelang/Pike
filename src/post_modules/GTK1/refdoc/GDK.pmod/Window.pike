//! a GDK.Window object.
//!
//! NOIMG
//!
//!

inherit GDK.Drawable;

GDK.Window change_property( GDK.Atom property, GDK.Atom type, int mode, string data );
//! mode is one of @[GDK_PROP_MODE_APPEND], @[GDK_PROP_MODE_PREPEND] and @[GDK_PROP_MODE_REPLACE]
//!
//!

array children( );
//! Returns an array of GDK.Window objects.
//!
//!

static GDK.Window create( GDK.Window parent, mapping|void attributes );
//! Not for non-experts. I promise.
//!
//!

GDK.Window delete_property( GDK.Atom a );
//!

mapping get_geometry( );
//! Returns ([ "x":xpos, "y":ypos, "width":width, "height":height, "depth":bits_per_pixel ])
//!
//!

mapping get_property( GDK.Atom atom, int|void offset, int|void delete_when_done );
//! Returns the value (as a string) of the specified property.
//! The arguments are:
//! 
//! property: The property atom, as an example GDK.Atom.__SWM_VROOT
//! offset (optional): The starting offset, in elements
//! delete_when_done (optional): If set, the property will be deleted when it has
//! been fetched.
//! 
//! Example usage: Find the 'virtual' root window (many window managers
//! put large windows over the screen)
//! 
//! @pre{
//!   GDK.Window root = GTK.root_window();
//!   array maybe=root->children()->
//!               get_property(GDK.Atom.__SWM_VROOT)-({0});
//!   if(sizeof(maybe))
//!     root=GDK.Window( maybe[0]->data[0] );
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

GDK.Window lower( );
//! Lower this window if the window manager allows that.
//!
//!

GDK.Window move_resize( int x, int y, int w, int h );
//! Move and resize the window in one call.
//!
//!

GDK.Window raise( );
//! Raise this window if the window manager allows that.
//!
//!

GDK.Window set_background( GDK.Color to );
//! Set the background color or image.
//! The argument is either a GDK.Pixmap or a GDK.Color object.
//!
//!

GDK.Window set_bitmap_cursor( GDK.Bitmap image, GDK.Bitmap mask, GDK.Color fg, GDK.Color bg, int xhot, int yhot );
//! xhot,yhot are the locations of the x and y hotspot relative to the
//! upper left corner of the cursor image.
//!
//!

GDK.Window set_cursor( int new_cursor );
//! Change the window cursor.<table border="0" cellpadding="3" cellspacing="0">
//! CURS(GDK.Arrow)
//! CURS(GDK.BasedArrowDown)
//! CURS(GDK.BasedArrowUp)
//! CURS(GDK.Boat)
//! CURS(GDK.Bogosity)
//! CURS(GDK.BottomLeftCorner)
//! CURS(GDK.BottomRightCorner)
//! CURS(GDK.BottomSide)
//! CURS(GDK.BottomTee)
//! CURS(GDK.BoxSpiral)
//! CURS(GDK.CenterPtr)
//! CURS(GDK.Circle)
//! CURS(GDK.Clock)
//! CURS(GDK.CoffeeMug)
//! CURS(GDK.Cross)
//! CURS(GDK.CrossReverse)
//! CURS(GDK.Crosshair)
//! CURS(GDK.DiamondCross)
//! CURS(GDK.Dot)
//! CURS(GDK.Dotbox)
//! CURS(GDK.DoubleArrow)
//! CURS(GDK.DraftLarge)
//! CURS(GDK.DraftSmall)
//! CURS(GDK.DrapedBox)
//! CURS(GDK.Exchange)
//! CURS(GDK.Fleur)
//! CURS(GDK.Gobbler)
//! CURS(GDK.Gumby)
//! CURS(GDK.Hand1)
//! CURS(GDK.Hand2)
//! CURS(GDK.Heart)
//! CURS(GDK.Icon)
//! CURS(GDK.IronCross)
//! CURS(GDK.LeftPtr)
//! CURS(GDK.LeftSide)
//! CURS(GDK.LeftTee)
//! CURS(GDK.Leftbutton)
//! CURS(GDK.LlAngle)
//! CURS(GDK.LrAngle)
//! CURS(GDK.Man)
//! CURS(GDK.Middlebutton)
//! CURS(GDK.Mouse)
//! CURS(GDK.Pencil)
//! CURS(GDK.Pirate)
//! CURS(GDK.Plus)
//! CURS(GDK.QuestionArrow)
//! CURS(GDK.RightPtr)
//! CURS(GDK.RightSide)
//! CURS(GDK.RightTee)
//! CURS(GDK.Rightbutton)
//! CURS(GDK.RtlLogo)
//! CURS(GDK.Sailboat)
//! CURS(GDK.SbDownArrow)
//! CURS(GDK.SbHDoubleArrow)
//! CURS(GDK.SbLeftArrow)
//! CURS(GDK.SbRightArrow)
//! CURS(GDK.SbUpArrow)
//! CURS(GDK.SbVDoubleArrow)
//! CURS(GDK.Shuttle)
//! CURS(GDK.Sizing)
//! CURS(GDK.Spider)
//! CURS(GDK.Spraycan)
//! CURS(GDK.Star)
//! CURS(GDK.Target)
//! CURS(GDK.Tcross)
//! CURS(GDK.TopLeftArrow)
//! CURS(GDK.TopLeftCorner)
//! CURS(GDK.TopRightCorner)
//! CURS(GDK.TopSide)
//! CURS(GDK.TopTee)
//! CURS(GDK.Trek)
//! CURS(GDK.UlAngle)
//! CURS(GDK.Umbrella)
//! CURS(GDK.UrAngle)
//! CURS(GDK.Watch)
//! CURS(GDK.Xterm)
//! </table>
//!
//!

GDK.Window set_events( int events );
//! events is a bitwise or of one or more of the following constants:
//! GDK.ExposureMask,
//! GDK.PointerMotionMask,
//! GDK.PointerMotion_HINTMask,
//! GDK.ButtonMotionMask,
//! GDK.Button1MotionMask,
//! GDK.Button2MotionMask,
//! GDK.Button3MotionMask,
//! GDK.ButtonPressMask,
//! GDK.ButtonReleaseMask,
//! GDK.KeyPressMask,
//! GDK.KeyReleaseMask,
//! GDK.EnterNotifyMask,
//! GDK.LeaveNotifyMask,
//! GDK.FocusChangeMask,
//! GDK.StructureMask,
//! GDK.PropertyChangeMask,
//! GDK.VisibilityNotifyMask,
//! GDK.ProximityInMask,
//! GDK.ProximityOutMask and
//! GDK.AllEventsMask
//!
//!

GDK.Window set_icon( GDK.Pixmap pixmap, GDK.Bitmap mask, GDK.Window window );
//! Set the icon to the specified image (with mask) or the specified
//! GDK.Window.  It is up to the window manager to display the icon.
//! Most window manager handles window and pixmap icons, but only a few
//! can handle the mask argument. If you want a shaped icon, the only
//! safe bet is a shaped window.
//!
//!

GDK.Window set_icon_name( string name );
//! Set the icon name to the specified string.
//!
//!

GDK.Window shape_combine_mask( GDK.Bitmap mask, int xoffset, int yoffset );
//! Set the shape of the widget, or, rather, it's window, to that of
//! the supplied bitmap.
//!
//!
