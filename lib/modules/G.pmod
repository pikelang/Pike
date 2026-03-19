#pike __REAL_VERSION__

//! The base object classes used by @[GTK2] et al.
//!
//! @seealso
//!   @[GI], @[G.Object], @[GTK2], @url{https://docs.gtk.org/gobject/@}

#if constant(GI.repository.GObject)

constant _module_value = GI.repository.GObject;

#else
#require constant(GTK2.GObject)

constant Object = GTK2.GObject;

#if constant(GTK2.GInitiallyUnowned)
// GTK 2.10 and later.
constant InitiallyUnowned = GTK2.GInitiallyUnowned;
#endif
#endif
