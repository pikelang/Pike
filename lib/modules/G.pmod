#pike __REAL_VERSION__
#require constant(GTK2.GObject)

//! The base object classes used by @[GTK2] et al.
//!
//! @seealso
//!   @[G.Object], @[GTK2], @url{https://docs.gtk.org/gobject/@}

constant Object = GTK2.GObject;

#if constant(GTK2.GInitiallyUnowned)
// GTK 2.10 and later.
constant InitiallyUnowned = GTK2.GInitiallyUnowned;
#endif
