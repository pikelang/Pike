#pike __REAL_VERSION__
#require constant(GTK2.Widget)

//! @decl import GTK2

mixed `[](string what)
{
  if(what == "_module_value") return UNDEFINED;
  if (has_prefix(what, "GNOME_")) return GTK2[what];
  return GTK2["Gnome2"+what];
}

array _indices()
{
  return glob( "GNOME_*", indices(GTK2) ) +
    map(glob( "Gnome2*", indices(GTK2) ), predef::`[],
	sizeof("Gnome2"), 0x7fffffff);
}

array _values()
{
  return map(_indices(), `[]);
}
