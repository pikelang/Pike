#if constant(GTK.parse_rc)

inherit GTK;
#define INDEX(x) predef::`->(this_object(),(x))

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
      return ra = Gdk_Atom( n, 0 );
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
  if(what == "_module_value") return ([])[0];

  if(what == "Atom") return Atom;

  if(!zero_type(INDEX("Gdk"+what)))
    return INDEX("Gdk"+what);
  if(!zero_type(INDEX("GDK_"+what)))
    return INDEX("GDK_"+what);
  if(!zero_type(INDEX("GDK_"+upper_case(GTK.unsillycaps(what)))))
    return INDEX("GDK_"+upper_case(GTK.unsillycaps(what)));
  return  GDKSupport[what];
}

#endif
