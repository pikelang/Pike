#if constant(GTK.ScrolledWindow)
inherit GTK.ScrolledWindow;

object cl;
void create( mixed ... args )
{
  ::create( GTK.Adjustment(), GTK.Adjustment() );
  cl = GTK.Clist( @args );
  add( cl->show() );
}


mixed `[](string what ) 
{
  switch(what)
  {
    case "create":     return create;
    case "show":       return show;
    case "set_usize":  return set_usize;
    case "set_policy": return set_policy;
  }
  return cl[what]; 
}

mixed `->(string what ) { return `[]( what ); }
#endif
