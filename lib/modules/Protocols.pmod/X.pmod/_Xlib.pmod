/* _Xlib.pmod
 *
 * kluge
 */

object display_re = Regexp("([^:]*):([0-9]+).([0-9]+)");

string pad(string s)
{
  return s + ({ "", "\0\0\0", "\0\0", "\0" })[strlen(s) % 4];
}

array(string) window_attributes =
({ "BackPixmap",
   "BackPixel",
   "BorderPixmap",
   "BorderPixel",
   "BitGravity",
   "WinGravity",
   "BackingStore",
   "BackingPlanes",
   "BackingPixel",
   "OverrideRedirect",
   "SaveUnder",
   "EventMask",
   "DontPropagate",
   "Colormap",
   "Cursor" });

array(string) window_configure_attributes =
({
  "X",
  "Y",
  "Width",
  "Height",
  "BorderWidth",
  "Sibling",
  "StackMode"
});

array(string) gc_attributes =
({
  "Function",
  "PlaneMask",
  "Foreground",
  "Background",
  "LineWidth",
  "LineStyle",
  "CapStyle",
  "JoinStyle",
  "FillStyle",
  "FillRule",
  "Tile",
  "Stipple",
  "TileStipXOrigin",
  "TileStipYOrigin",
  "Font",
  "SubwindowMode",
  "GraphicsExposures",
  "ClipXOrigin",
  "ClipYOrigin",
  "ClipMask",
  "DashOffset",
  "DashList",
  "ArcMode"
});

mapping(string:int) event_masks =
([
  "KeyPress" : 1<<0,
  "KeyRelease" : 1<<1,
  "ButtonPress" : 1<<2,
  "ButtonRelease" : 1<<3,
  "EnterWindow" : 1<<4,
  "LeaveWindow" : 1<<5,
  "PointerMotion" : 1<<6,
  "PointerMotionHint" : 1<<7,
  "Button1Motion" : 1<<8,
  "Button2Motion" : 1<<9,
  "Button3Motion" : 1<<10,
  "Button4Motion" : 1<<11,
  "Button5Motion" : 1<<12,
  "ButtonMotion" : 1<<13,
  "KeymapState" : 1<<14,
  "Exposure" : 1<<15,
  "VisibilityChange" : 1<<16,
  "StructureNotify" : 1<<17,
  "ResizeRedirect" : 1<<18,
  "SubstructureNotify" : 1<<19,
  "SubstructureRedirect" : 1<<20,
  "FocusChange" : 1<<21,
  "PropertyChange" : 1<<22,
  "ColormapChange" : 1<<23,
  "OwnerGrabButton" : 1<<24
 ]);

array(string) event_types =
({
  "Error",
  "Reply",
  "KeyPress",
  "KeyRelease",
  "ButtonPress",
  "ButtonRelease",
  "MotionNotify",
  "EnterNotify",
  "LeaveNotify",
  "FocusIn",
  "FocusOut",
  "KeymapNotify",
  "Expose",
  "GraphicsExpose",
  "NoExpose",
  "VisibilityNotify",
  "CreateNotify",
  "DestroyNotify",
  "UnmapNotify",
  "MapNotify",
  "MapRequest",
  "ReparentNotify",
  "ConfigureNotify",
  "ConfigureRequest",
  "GravityNotify",
  "ResizeRequest",
  "CirculateNotify",
  "CirculateRequest",
  "PropertyNotify",
  "SelectionClear",
  "SelectionRequest",
  "SelectionNotify",
  "ColormapNotify",
  "ClientMessage",
  "MappingNotify"
});

array(string) error_codes =
({
  "Success",		/* everything's okay */
  "Request",		/* bad request code */
  "Value",		/* int parameter out of range */
  "Window",		/* parameter not a Window */
  "Pixmap",		/* parameter not a Pixmap */
  "Atom",		/* parameter not an Atom */
  "Cursor",		/* parameter not a Cursor */
  "Font",		/* parameter not a Font */
  "Match",		/* parameter mismatch */
  "Drawable",		/* parameter not a Pixmap or Window */
  "Access",		/* depending on context:
			   - key/button already grabbed
			   - attempt to free an illegal 
			     cmap entry 
			   - attempt to store into a read-only 
			     color map entry.
			   - attempt to modify the access control
			     list from other than the local host.
			     */
  "Alloc",		/* insufficient resources */
  "Color",		/* no such colormap */
  "GC",			/* parameter not a GC */
  "IDChoice",		/* choice not in range or already used */
  "Name",		/* font or color name doesn't exist */
  "Length",		/* Request length incorrect */
  "Implementation"	/* server is defective */
});

array(string) visual_classes =
({
  "StaticGray",
  "GrayScale",
  "StaticColor",
  "PseudoColor",
  "TrueColor",
  "DirectColor",
});
