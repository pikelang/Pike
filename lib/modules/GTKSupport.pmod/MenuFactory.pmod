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

//! Definition of a menu item.
class MenuDef
{
  string menu_path;
  int shortcut;
  int flags, modifiers;
  mixed arg;
  function callback;

  //! Sets a new shortcut as the current one.
  //!
  //! The shortcut syntax is: m[m[..]]-key, where m is one or more
  //! modifier character, and key is the desired key (@b{NOTE@}: Key
  //! must be in the range 0-255 currently, this will hopefully be
  //! fixed by the GTK people in the future)
  //!
  //! The modifiers are:
  //! @string
  //!   @value "s"
  //!      Shift
  //!   @value "c"
  //!      Control
  //!   @value "a"
  //!   @value "1"
  //!      Modifier 1 (called alt by the GTK people, that's not true, though)
  //!   @value "g"
  //!   @value "2"
  //!      Modifier 2 (called altgr by the GTK people, that's not true, though)
  //!   @value "m"
  //!   @value "3"
  //!      Modifier 3 (not mapped by the GTK people, meta on _my_ keyboard)
  //!   @value "h"
  //!   @value "4"
  //!      Modifier 4 (not mapped by the GTK people, hyper on _my_ keyboard)
  //!   @value "u"
  //!   @value "5"
  //!      Modifier 5 (not mapped by the GTK people, super on _my_ keyboard)
  //! @endstring
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

  //! @param path
  //! Path is the menupath. A submenu will be created for each
  //! "Directory" in the path, and menuitems will be created for the
  //! "files". There are two special cases: The "file"
  //! @expr{"<separator>"@} will create a thin line. The "file"-prefix
  //! @expr{"<check>"@} will make the menuitem a checkmenuitem instead
  //! of a normal menuitem.
  //!
  //! @param cb
  //! @param cbarg
  //! The second and third arguments are the callback function and the
  //! first callback function argument. If the callback function
  //! argument is an array, the indices of the array will be pushed as
  //! arguments. To call the function with an array as the only
  //! argument, make an array with the array in. The callback function
  //! will be called like @expr{callback( arg, widget )@}, or if arg
  //! is an array, @expr{callback( arg[0], arg[1], ..., widget )@}.
  //!
  //! @param shortcut
  //! The fourth argument, shortcut, is the shortcut to bind to this
  //! menu item. The shortcut can be changed later on by calling
  //! @[assign_shortcut], or by the user by pressing the desired
  //! keycombination over the menu item.
  //!
  //! The shortcut syntax is: m[m[..]]-key, where m is one or more
  //! modifier character, and key is the desired key (@b{NOTE@}: Key
  //! must be in the range 0-255 currently, this will hopefully be
  //! fixed by the GTK people in the future)
  //!
  //! The modifiers are:
  //! @string
  //!   @value "s"
  //!      Shift
  //!   @value "c"
  //!      Control
  //!   @value "a"
  //!   @value "1"
  //!      Modifier 1 (called alt by the GTK people, that's not true, though)
  //!   @value "g"
  //!   @value "2"
  //!      Modifier 2 (called altgr by the GTK people, that's not true, though)
  //!   @value "m"
  //!   @value "3"
  //!      Modifier 3 (not mapped by the GTK people, meta on _my_ keyboard)
  //!   @value "h"
  //!   @value "4"
  //!      Modifier 4 (not mapped by the GTK people, hyper on _my_ keyboard)
  //!   @value "u"
  //!   @value "5"
  //!      Modifier 5 (not mapped by the GTK people, super on _my_ keyboard)
  //! @endstring
  //!
  //! @param right
  //!   Currently ignored.
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

//! The function passed as the argument to this function will be
//! called each time the accelerator key mapping is changed by the
//! user with the new mapping as the argument.
//!
//! @note
//! This function is only used when the menubar is created, once you
//! are done with the menubar creation, the callbacks for that menubar
//! will be fixed.
void set_menubar_modify_callback( function to )
{
  mbar_mc = to;
}

mapping menubar_objects = ([]);

//! Returns a (flat) mapping @expr{([ path:GTK.MenuItem ])@}.
//!
//! @note
//! This function can only be called @i{after@} the menubar is
//! created.
mapping get_menubar_mapping(  )
{
  return copy_value(menubar_objects);
}

mapping(string:GTK.Menu|GTK.MenuBar) submenues;
mapping get_submenu_mapping(  )
{
  return copy_value(submenues);
}

//! Identical to @[MenuFactory], but creates popup menus instead.
//!
//! @returns
//! @array
//!   @elem GTK.Menu 0
//!     GTK.Menu
//!   @elem GTK.AccelGroup 1
//!     GTK.AccelGroup
//! @endarray
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

//! This is the function that actually builds the menubar.
//!
//! @example
//! import GTK.MenuFactory;
//! [GTK.MenuBar bar, GTK.AcceleratorTable map] = 
//!  MenuFactory( 
//!    MenuDef( "File/New", new_file, 0, "A-N" ), 
//!    MenuDef( "File/Open", new_file, 1, "A-O" ), 
//!    MenuDef( "File/Save", save_file, 0, "A-S" ), 
//!    MenuDef( "File/<separator>", 0, 0 ),
//!    MenuDef( "File/Quit", _exit, 0, "A-Q" ), 
//!  );
//!
//! @returns
//! @array
//!   @elem GTK.MenuBar 0
//!     GTK.MenuBar
//!   @elem GTK.AcceleratorTable 1
//!     GTK.AcceleratorTable
//! @endarray
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
