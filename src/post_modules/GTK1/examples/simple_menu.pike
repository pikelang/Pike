import GTK.MenuFactory;

// Display a simple menu, and handle saving and loading of shortcuts.
// 

int main()
{
  GTK.setup_gtk( "simple menu" );
  GTK.Window win = GTK.Window( GTK.WindowToplevel );

  mapping sc = GTK.Util.parse_shortcut_file( "simple_menu_shortcuts" );

  array defs = ({
    MenuDef( "File/Load...", 0, 0 ),
    MenuDef( "File/Save...", 0, 0 ),
    MenuDef( "File/<separator>", 0, 0 ),
    MenuDef( "File/Quit...", _exit, 0 ),

    MenuDef( "Misc/number 1", 0, 0 ),
    MenuDef( "Misc/number 2", 0, 0 ),
    MenuDef( "Misc/<separator>", 0, 0 ),
    MenuDef( "Misc/Number 3/Number 1", 0, 0 ),
    MenuDef( "Misc/Number 3/Number 2", 0, 0 ),
    MenuDef( "Misc/Number 3/Number 3", 0, 0 ),
  });

  foreach(defs, object o) 
    if(sc[o->menu_path])
      o->assign_shortcut( sc[o->menu_path] );

  set_menubar_modify_callback( lambda(mapping m) {
          GTK.Util.save_shortcut_file( "simple_menu_shortcuts", m );
     });  
  [object bar,object map] = MenuFactory(@defs);
  set_menubar_modify_callback( 0 );  
  
  win->add_accel_group( map );
  win->add( bar->show() )->show();
  return -1;
}
