#pike __REAL_VERSION__
#require constant(GTK2.Widget)

#define INDEX(x) GTK2[x]

//! @decl import GTK2

//! @decl constant Atom

object Atom = class
{
  mapping atoms = ([]);

  class fake_atom
  {
    object ra;
    string n;
    object get_atom()
    {
      if(ra) return ra;
      return ra = GTK2->Gdk_Atom( n, 0 );
    }
    string get_name()
    {
      return get_atom()->get_name();
    }
    void create(string q)
    {
      n = q;
    }
  }

  object `[](string what)
  {
    if(atoms[what])
      return atoms[what];
    return atoms[what] = fake_atom( what );
  }
}();

mixed `[](string what)
{
  if(what == "_module_value") return UNDEFINED;
  if(what == "Atom") return Atom;
  if(has_index(GTK2, "Gdk"+what))
    return INDEX("Gdk"+what);
  if(has_index(GTK2, "GDK_"+what))
    return INDEX("GDK_"+what);
  if(has_index(GTK2, "GDK_"+upper_case(GTK2->unsillycaps(what))))
    return INDEX("GDK_"+upper_case(GTK2->unsillycaps(what)));
  return UNDEFINED;
//   return  GDKSupport[what];
}
