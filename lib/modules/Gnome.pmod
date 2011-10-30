#pike __REAL_VERSION__

#if constant(GTK1) && constant(GTK1.Widget)

//! @decl import GTK1

mixed `[](string what)
{
  if(what == "_module_value") return UNDEFINED;
  if (has_prefix(what, "GNOME_")) return GTK1[what];
  return (GTK1["Gnome"+what] || GTK1["gnome_"+what]);
}

array _indices()
{
  return glob( "GNOME_*", indices(GTK1) ) +
    map(glob( "Gnome*", indices(GTK1) ), predef::`[],
	sizeof("Gnome"), 0x7fffffff);
}

array _values()
{
  return map(_indices(), `[]);
}

#else
constant this_program_does_not_exist=1;
#endif /* constant(GTK1.Widget) */
