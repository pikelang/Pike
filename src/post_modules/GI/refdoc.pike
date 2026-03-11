//
// Some stub refdoc for common modules loaded via GI.
//
// NOTE: No actual code here. This file is NOT installed.
//

//! @module Gdk
//! @belongs predef::GI.repository
//!
//!   The latest installed version of the GIMP Drawing Kit (aka GDK)
//!   (typically 3.0 or 4.0).
//!
//! @seealso
//!   @url{https://docs.gtk.org/gtd3@}, @url{https://docs.gtk.org/gtd4@}

//! @endmodule

//! @module Gtk
//! @belongs predef::GI.repository
//!
//!   The lastest installed version of the GIMP ToolKit (aka GTK)
//!   (typically 3.0 or 4.0).
//!
//! @seealso
//!   @url{https://docs.gtk.org/gtk3@}, @url{https://docs.gtk.org/gtk4@}

//! @class Application
//!
//!   This is the Pike interface to
//!   @url{https://docs.gtk.org/gtk3/class.Application.html@}/
//!   @url{https://docs.gtk.org/gtk4/class.Application.html@}

//! @decl protected void create(mapping(string:string|int) props)
//!
//!   Create a new GTK @[Application] instance.
//!
//! @param props
//!   Properties used to create the new @[Application].
//!   @mapping
//!     @member string "application_id"
//!       The application ID.
//!     @member int "flags"
//!       The application flags.
//!   @endmapping
//!
//! @seealso
//!   @url{https://docs.gtk.org/gtk3/ctor.Application.new.html@},
//!   @url{https://docs.gtk.org/gtk4/ctor.Application.new.html@}

//! @endclass

//! @class Window
//!
//!   This is the Pike interface to
//!   @url{https://docs.gtk.org/gtk3/class.Window.html@}/
//!   @url{https://docs.gtk.org/gtk4/class.Window.html@}

//! @endclass

//! @class Widget
//!
//!   The base class for all @[Gtk] widgets.
//!
//!   This is the Pike interface to
//!   @url{https://docs.gtk.org/gtk3/class.Widget.html@}/
//!   @url{https://docs.gtk.org/gtk4/class.Widget.html@}

//! @endclass

//! @endmodule

//! @module Pango
//! @belongs predef::GI.repository
//!
//!   The latest installed version of Pango - Internationalized text
//!   layout and rendering (typically 1.0).
//!
//! @seealso
//!   @url{https://docs.gtk.org/Pango@},

//! @endmodule
