#pike __REAL_VERSION__

#if constant(GTK)

mixed `[](string what)
{
  if(what == "_module_value") return UNDEFINED;
  return (GTK["Gnome"+what] || GTK["gnome_"+what]);
}

array _indices()
{
  return glob( "GNOME_*", indices(GTK) ) + glob( "Gnome_*", indices(GTK) );
}

#else /* !constant(GTK) */

static void create()
{
  /* Destroy ourselves. */
  destruct();
}

#endif /* constant(GTK) */
