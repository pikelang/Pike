#pike __REAL_VERSION__

#if constant(GTK) && constant(GTK.Widget)

//! @decl import GTK

mixed `[](string what)
{
  if(what == "_module_value") return UNDEFINED;
  return (GTK["Gnome"+what] || GTK["gnome_"+what]);
}

array _indices()
{
  return glob( "GNOME_*", indices(GTK) ) + glob( "Gnome_*", indices(GTK) );
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(GTK.Widget) */
