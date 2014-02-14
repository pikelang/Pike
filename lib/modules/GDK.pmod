#pike __REAL_VERSION__
#require constant(GTK.Widget)

#define INDEX(x) GTK[x]

//! GDK wrapper module.
//!
//! This is a convenience module that is identical to either
//! either the @[GDK2] or the @[GDK1] module depending on
//! which (if any) of them is available.
//!
//! @seealso
//!   @[GDK1], @[GDK2]

//! @decl import GTK

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
      return ra = GTK->Gdk_Atom( n, 0 );
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
  if(!zero_type(INDEX("Gdk"+what)))
    return INDEX("Gdk"+what);
  if(!zero_type(INDEX("GDK_"+what)))
    return INDEX("GDK_"+what);
  if(!zero_type(INDEX("GDK_"+upper_case(GTK->unsillycaps(what)))))
    return INDEX("GDK_"+upper_case(GTK->unsillycaps(what)));
  return UNDEFINED;
//   return  GDKSupport[what];
}
