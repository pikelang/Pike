#pike __REAL_VERSION__

//! GTK wrapper module.
//!
//! This is a convenience module that is identical to the latest
//! supported GTK version, if available.
//!
//! @seealso
//!   @[GTK2], @[GI.repository]

#if constant(GI.repository.Gtk)
constant _module_value = GI.repository.Gtk;
#else
#require constant(GTK2.Widget)

//! @decl inherit GTK2

constant _module_value = GTK2;
#endif


