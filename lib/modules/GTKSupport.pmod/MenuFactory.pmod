#pike __REAL_VERSION__

#define FLAG_SHIFT   1
#define FLAG_CTRL    4
#define FLAG_ALT     8
#define FLAG_ALTGR  16
#define FLAG_META   64
#define FLAG_HYPER 128
#define FLAG_SUPER 256

array parse_shortcut(string what)
{
  int flags;
  int key;
  
  if(!what)
    return ({0,0 });

  string a, b;
  if(sscanf(what, "%s-%s", a, b) != 2)
  {
    a="";
    b=what;
  }

  foreach(a/"", string s)
    switch(lower_case(s))
    {
     case "s": flags |= FLAG_SHIFT; break;
     case "c": flags |= FLAG_CTRL; break;
     case "1":
     case "a": flags |= FLAG_ALT; break;
     case "2":
     case "g": flags |= FLAG_ALTGR; break; /* not visible.. */
     case "3":
     case "m": flags |= FLAG_META; break;  /* not visible.. */
     case "4":
     case "h": flags |= FLAG_HYPER; break; /* not visible.. */
     case "5":
     case "u": flags |= FLAG_SUPER; break; /* not visible.. */
    }
  return ({ flags, (b+"-")[0] });
}

string shortcut_to_string( int key, int mod )
{
  if(!key) return 0;
  string m="";
  if(mod & FLAG_SHIFT) m += "S";
  if(mod & FLAG_CTRL) m += "C";
  if(mod & FLAG_ALT) m += "A";
  if(mod & FLAG_ALTGR) m += "G";
  if(mod & FLAG_META) m += "M";
  if(mod & FLAG_HYPER) m += "H";
  if(mod & FLAG_SUPER) m += "U";
  if(sizeof(m)) m+= "-";
  return sprintf("%s%c",m, key);
}

class MenuDef
{
  string menu_path;
  int shortcut;
  int flags, modifiers;
  mixed arg;
  function callback;

  void assign_shortcut(string sc)
  {
    [modifiers,shortcut] = parse_shortcut(sc);
  }

  void selected(mixed q, object widget, mixed ... args)
  {
    if(callback) 
    {
      if(arrayp(arg))
	callback( @arg, widget, @args );
      else
	callback( arg, widget, @args );
    }
  }

  void create(string path, function|void cb, 
	      mixed|void cbarg,
	      string|void binding,
	      int|void right)
  {
    menu_path = path;
    flags = right;
    callback = cb;
    arg = cbarg;
    assign_shortcut( binding );
  }

  function mbc;
  array(object(MenuDef)) siblings;
  mapping modifier_table;
  void slice_n_dice()
  {
    if(!mbc) return;
    if(!modifier_table)
    {
      modifier_table = ([]);
      foreach(siblings, MenuDef o)
      {
	o->modifier_table = modifier_table;
	if(o->shortcut)
	  modifier_table[o->menu_path] = shortcut_to_string( o->shortcut,
							     o->modifiers );
      }
    } else	      
      modifier_table[menu_path]= shortcut_to_string( shortcut, modifiers );
    mbc( modifier_table );
  }

  void install_accelerator(int i, object w, int flags, 
                           mixed g, int k, int mods)
  {
    shortcut = k;
    modifiers = mods;
    remove_call_out(slice_n_dice);
    call_out(slice_n_dice, 0.1);
  }

  void remove_accelerator(int i, object w)
  {
    modifiers=shortcut=0;
    remove_call_out(slice_n_dice);
    call_out(slice_n_dice, 0.1);
  }
}


function mbar_mc;
void set_menubar_modify_callback( function to )
{
  mbar_mc = to;
}

mapping menubar_objects = ([]);
mapping get_menubar_mapping(  )
{
  return copy_value(menubar_objects);
}

mapping(string:GTK.Menu|GTK.MenuBar) submenues;
mapping get_submenu_mapping(  )
{
  return copy_value(submenues);
}

array(object) PopupMenuFactory( MenuDef ... definition )
{
  GTK.Menu bar = GTK.Menu();
  GTK.AccelGroup table= GTK.AccelGroup();
  menubar_objects = ([]);
  submenues = (["":bar]);
  mapping(string:GTK.RadioMenuItem) radiogroups = ([]);
  foreach(definition, object d)
  {
    string path="";
    object parent = bar;
    array p = d->menu_path/"/";
    foreach(p[..sizeof(p)-2], string segment)
    {
      path += segment+"/";
      if(!submenues[path])
      {
     GTK.MenuItem i = GTK.MenuItem( segment );
     submenues[path] = GTK.Menu();
     submenues[path]->set_accel_group( table );
//         d->menu_obj = submenues[path];
     parent->append( i );
     i->set_submenu( submenues[path] );
     i->show();
     menubar_objects[ path ] = i;
      }
      parent = submenues[path];
    }
    GTK.Item i;
    string q,g;
    sscanf(p[-1], "<%s>%s", q, p[-1]);
    if(q) sscanf(q, "%s:%s", q, g);
    switch( q )
    {
     default:
       i = GTK.MenuItem( p[-1] );
       break;
     case "check":
       i = GTK.CheckMenuItem( p[-1] );
       break;
     case "separator":
       i = GTK.MenuItem();
       i->set_state( GTK.StateInsensitive );
       break;
     case "tearoff":
       i = GTK.TearoffMenuItem();
       break;
    case "radio":
      if (!radiogroups[path+":"+g]) {
     i = GTK.RadioMenuItem( p[-1] );
     radiogroups[path+":"+g] = i;
      } else {
     i = GTK.RadioMenuItem( p[-1], radiogroups[path+":"+g] );
      }
      break;
    }
    menubar_objects[ d->menu_path ] = i;
    i->show();
    if(d->shortcut)
      i->add_accelerator( "activate", table, d->shortcut, d->modifiers,
                          GTK.AccelVisible);
    i->signal_connect( "activate", d->selected, 0 );
    i->signal_connect("add_accelerator", d->install_accelerator,  0);
    i->signal_connect("remove_accelerator",  d->remove_accelerator,   0);
    parent->add( i );
    d->mbc = mbar_mc;
    d->siblings = definition;
  }
  return ({bar,table});
}

array(object) MenuFactory( MenuDef ... definition )
{
  GTK.MenuBar bar = GTK.MenuBar();
  GTK.AccelGroup table= GTK.AccelGroup();
  menubar_objects = ([]);
  submenues = (["":bar]);
  mapping(string:GTK.RadioMenuItem) radiogroups = ([]);
  foreach(definition, object d)
  {
    string path="";
    object parent = bar;
    array p = d->menu_path/"/";
    foreach(p[..sizeof(p)-2], string segment)
    {
      path += segment+"/";
      if(!submenues[path])
      {
	GTK.MenuItem i = GTK.MenuItem( segment );
	submenues[path] = GTK.Menu();
	submenues[path]->set_accel_group( table );
//         d->menu_obj = submenues[path];
	parent->append( i );
	i->set_submenu( submenues[path] );
	i->show();
	menubar_objects[ path ] = i;
      }
      parent = submenues[path];
    }
    GTK.Item i;
    string q,g;
    sscanf(p[-1], "<%s>%s", q, p[-1]);
    if(q) sscanf(q, "%s:%s", q, g);
    switch( q )
    {
     default:
       i = GTK.MenuItem( p[-1] );
       break;
     case "check":
       i = GTK.CheckMenuItem( p[-1] );
       break;
     case "separator":
       i = GTK.MenuItem();
       i->set_state( GTK.StateInsensitive );
       break;
     case "tearoff":
       i = GTK.TearoffMenuItem();
       break;
    case "radio":
      if (!radiogroups[path+":"+g]) {
	i = GTK.RadioMenuItem( p[-1] );
	radiogroups[path+":"+g] = i;
      } else {
	i = GTK.RadioMenuItem( p[-1], radiogroups[path+":"+g] );
      }
      break;
    }
    menubar_objects[ d->menu_path ] = i;
    i->show();
    parent->add( i );
    if(d->shortcut)
      i->add_accelerator( "activate", table, d->shortcut, d->modifiers,
                          GTK.AccelVisible);
    i->signal_connect( "activate", d->selected, 0 );
    i->signal_connect("add_accelerator", d->install_accelerator,  0);
    i->signal_connect("remove_accelerator",  d->remove_accelerator,   0);
    d->mbc = mbar_mc;
    d->siblings = definition;
  }
  return ({bar,table});
}
