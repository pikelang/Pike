#pike __REAL_VERSION__
#require constant(GTK2.GObject)

constant Object = GTK2.GObject;

#if constant(GTK2.GInitiallyUnowned)
// GTK 2.10 and later.
constant InitiallyUnowned = GTK2.GInitiallyUnowned;
#endif
