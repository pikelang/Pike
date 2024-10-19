#pike __REAL_VERSION__
#require constant(GTK2.PangoLayout)

/* //! @decl import GTK2 */

protected mixed `[](string what)
{
  if (what == "_module_value") return UNDEFINED;
  if (has_prefix(what, "PANGO_")) return GTK2[what];
  return GTK2["Pango"+what];
}

protected array _indices()
{
  return glob( "PANGO_*", indices(GTK2) ) +
    map(glob( "Pango*", indices(GTK2) ), predef::`[],
	sizeof("Pango"), 0x7fffffff);
}

protected array _values()
{
  return map(_indices(), `[]);
}
