/* _Xlib.pmod
 *
 * $Id$
 *
 * Kluge, should be in Xlib.pmod
 */

/*
 *    Protocols.X, a Pike interface to the X Window System
 *
 *    See COPYRIGHT for copyright information.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */

#pike __REAL_VERSION__

object display_re = Regexp("^([^:]*):([0-9]+)(.[0-9]+|)$");

string pad(string s)
{
  return s + ({ "", "\0\0\0", "\0\0", "\0" })[sizeof(s) % 4];
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

array(string) predefined_atoms =
({ "Foo", // ignored
   "PRIMARY",
   "SECONDARY",
   "ARC",
   "ATOM",
   "BITMAP",
   "CARDINAL",
   "COLORMAP",
   "CURSOR",
   "CUT_BUFFER0",
   "CUT_BUFFER1",
   "CUT_BUFFER2",
   "CUT_BUFFER3",
   "CUT_BUFFER4",
   "CUT_BUFFER5",
   "CUT_BUFFER6",
   "CUT_BUFFER7",
   "DRAWABLE",
   "FONT",
   "INTEGER",
   "PIXMAP",
   "POINT",
   "RECTANGLE",
   "RESOURCE_MANAGER",
   "RGB_COLOR_MAP",
   "RGB_BEST_MAP",
   "RGB_BLUE_MAP",
   "RGB_DEFAULT_MAP",
   "RGB_GRAY_MAP",
   "RGB_GREEN_MAP",
   "RGB_RED_MAP",
   "STRING",
   "VISUALID",
   "WINDOW",
   "WM_COMMAND",
   "WM_HINTS",
   "WM_CLIENT_MACHINE",
   "WM_ICON_NAME",
   "WM_ICON_SIZE",
   "WM_NAME",
   "WM_NORMAL_HINTS",
   "WM_SIZE_HINTS",
   "WM_ZOOM_HINTS",
   "MIN_SPACE",
   "NORM_SPACE",
   "MAX_SPACE",
   "END_SPACE",
   "SUPERSCRIPT_X",
   "SUPERSCRIPT_Y",
   "SUBSCRIPT_X",
   "SUBSCRIPT_Y",
   "UNDERLINE_POSITION",
   "UNDERLINE_THICKNESS",
   "STRIKEOUT_ASCENT",
   "STRIKEOUT_DESCENT",
   "ITALIC_ANGLE",
   "X_HEIGHT",
   "QUAD_WIDTH",
   "WEIGHT",
   "POINT_SIZE",
   "RESOLUTION",
   "COPYRIGHT",
   "NOTICE",
   "FONT_NAME",
   "FAMILY_NAME",
   "FULL_NAME",
   "CAP_HEIGHT",
   "WM_CLASS",
   "WM_TRANSIENT_FOR"
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

array(string) amiwm_pens =
({
  "detail",             /* compatible Intuition rendering pens */
  "block",              /* compatible Intuition rendering pens */
  "text",               /* text on background                  */
  "shine",              /* bright edge on 3D objects           */
  "shadow",             /* dark edge on 3D objects             */
  "fill",               /* active-window/selected-gadget fill  */
  "filltext",           /* text over FILLPEN                   */
  "background",         /* always color 0                      */
  "highlighttext",      /* special color text, on background   */
  /* New for V39, only present if DRI_VERSION >= 2: */
  "bardetail",          /* text/detail in screen-bar/menus */
  "barblock",           /* screen-bar/menus fill */
  "bartrim",            /* trim under screen-bar */
}); 
  
