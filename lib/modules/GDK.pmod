#pike __REAL_VERSION__

//! GDK wrapper module.
//!
//! This is a convenience module that is identical to the latest
//! supported GDK version, if available.
//!
//! @seealso
//!   @[GDK2], @[GI.repository]

#if constant(GI.repository.Gdk)

constant _module_value = GI.repository.Gdk;

#else
#require constant(GTK.Widget)

//! @decl inherit GDK2;

constant _module_value = GDK2;
#endif
