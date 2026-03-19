#pike __REAL_VERSION__
//! Pango wrapper module.
//!
//! This is a convenience module that is identical to the latest
//! supported Pango version, if available.
//!
//! @seealso
//!   @[GTK2], @[GI.repository]

#if constant(GI.repository.Pango)
constant _module_value = GI.repository.Pango;
#else
#require constant(GTK2.PangoLayout)

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
#endif
