//! Applets are basically GNOME applications whose window sits inside
//! the panel. Also the panel "takes care" of the applets by providing
//! them with session saving and restarting, window management (inside
//! of the panel), and a context menu.
//! 
//! The simplest applet one can write would be along the lines of:
//! 
//! @pre{
//! int main( int argc, array argv )
//! {
//!   Gnome.init( "hello", "1.0", argv, 0 );
//!   Gnome.AppletWidget("hello")-&gt;add(GTK.Label("Hello World!"))-&gt;show_all();
//!   GTK.applet_widget_gtk_main();
//! }
//! @}
//! This creates an applet which just sits on the panel, not really
//! doing anything, in real life the label would be substituted by
//! something which actually does something useful. As you can see the
//! applet doesn't really take care of restarting itself.
//! 
//! For the applet to be added to the menus, you need to install two
//! files. Your x.gnorba file goes into $sysconfdir/CORBA/servers/ and
//! the x.desktop file goes into $prefix/share/applets/&lt;category&gt;/.
//! 
//! Example hello.desktop:
//! @pre{
//! [Desktop Entry]
//!  Name=Hello Applet
//!  Comment=An example Hello World type Applet
//!  Type=PanelApplet
//!  Exec=hello.pike
//!  Icon=gnome-hello.png
//!  Terminal=0
//! @}
//! Example hello.gnorba:
//! @pre{
//! [hello]
//!  type=exe
//!  repo_id=IDL:GNOME/Applet:1.0
//!  description=Hello Applet
//!  location_info=hello.pike
//! @}
//! One thing to keep in mind is that the Exec line for the .desktop
//! doesn't actually get executed when the Type is PanelApplet. The
//! Exec line should be the GOAD ID specified in the .gnorba file (the
//! "hello" enclosed by brackets). For a simple applet all you need to
//! do is replace the hello.pike with the name of your applet
//! executable.
//! 
//!  When the user right clicks on the applet, a menu appears, this is
//!  all handeled by the panel, so in order to add items to it you use
//!  a special interface to "add callbacks" to the menu. A very simple
//!  example would be (making our hello applet even more feature full):
//! 
//! @pre{
//! void hello_there()
//! {
//!   write( "Hello there, indeed!\n" );
//! }
//!
//! int main( int argc, array argv )
//! {
//!   Gnome.AppletWidget w;
//!   Gnome.init( "hello", "1.0", argv, 0 );
//!   w = Gnome.AppletWidget("hello");
//!   w-&gt;add(GTK.Label("Hello World!"))-&gt;show_all();
//!   w-&gt;register_callback( "hello", "Hello there", hello_there, 0 );
//!   GTK.applet_widget_gtk_main();
//! }
//! @}
//! Now the user will see a "Hello There" menu item on the applet menu,
//! and when selected, the applet will print "Hello There". Useful huh?
//! 
//! Note that the first argument to the register_callback is just a
//! string identifier of this callback, and can really be whatever you
//! want. But it should NOT be translated as the label (the 2nd
//! argument) should be.
//!
//!
//!  Signals:
//! @b{back_change@}
//!
//! @b{change_orient@}
//!
//! @b{change_pixel_size@}
//!
//! @b{change_position@}
//!
//! @b{tooltip_state@}
//!

inherit GTK.Plug;

Gnome.AppletWidget abort_load( );
//!  Abort the applet loading, once applet has been created, this is a
//!  way to tell the panel to forget about us if we decide we want to
//!  quit before we add the actual applet to the applet-widget. This is
//!  only useful before before add() is called.
//!
//!

Gnome.AppletWidget add( GTK.Widget what );
//! Add a child (widget) to the applet. This finishes the handshaking
//! with the panel started in applet_widget_new. You should never call
//! this function twice for the same applet. If you have already
//! created an applet widget, but need to cancel the loading of the
//! applet, use abort_load.
//!
//!

Gnome.AppletWidget callback_set_sensitive( string name, int sensitive );
//! Sets the sensitivity of a menu item in the applet's context menu.
//!
//!

static Gnome.AppletWidget create( string applet_name );
//! Make a new applet and register us with the panel, if you decide to
//! cancel the load before calling add, you should call
//! abort_load.
//!
//!

int get_free_space( );
//! Gets the free space left that you can use for your applet. This is
//! the number of pixels around your applet to both sides. If you
//! strech by this amount you will not disturb any other applets. If
//! you are on a packed panel 0 will be returned.
//!
//!

string get_globcfgpath( );
//!

int get_panel_orient( );
//! Gets the orientation of the panel this widget is on. it can be one
//! of @[GNOME_Panel_ORIENT_DOWN], @[GNOME_Panel_ORIENT_LEFT], @[GNOME_Panel_ORIENT_RIGHT] and @[GNOME_Panel_ORIENT_UP]. This is not the position of the
//! panel, but rather the direction that the applet should be "reaching
//! out". So any arrows should for example point in this direction. It
//! will be OrientUp or OrientDown for horizontal panels and OrientLeft
//! or OrientRight for vertical panels
//!
//!

int get_panel_pixel_size( );
//! Gets the width of the panel in pixels. This is not the actual size,
//! but the recomended one. The panel may be streched if the applets
//! use larger sizes then this.
//!
//!

string get_privcfgpath( );
//!

Gnome.AppletWidget register_callback( string name, string menutext, function callback_cb, mixed callback_arg );
//! Adds a menu item to the applet's context menu. The name should be a
//! path that is separated by '/' and ends in the name of this
//! item. You need to add any submenus with register_callback_dir.
//!
//!

Gnome.AppletWidget register_callback_dir( string name, string menutext );
//! Adds a submenu to the applet's context menu. The name should be the
//! full path of the new submenu with the name of the new submenu as
//! the last part of the path. The name can, but doesn't have to be
//! terminated with a '/'.
//!
//!

Gnome.AppletWidget register_stock_callback( string name, string stock_type, string menutext, function callback_cb, mixed callback_arg );
//! Adds a menu item to the applet's context menu with a stock GNOME
//! pixmap. This works almost exactly the same as register_callback.
//!
//!

Gnome.AppletWidget register_stock_callback_dir( string name, string stock_type, string menutext );
//! Adds a submenu to the applet's context menu with a stock GNOME
//! pixmap. This is similiar to register_callback_dir.
//!
//!

Gnome.AppletWidget remove( );
//! Remove the plug from the panel, this will destroy the applet. You
//! can only call this once for each applet.
//!
//!

Gnome.AppletWidget send_position( int enable );
//! If you need to get a signal everytime this applet changes position
//! relative to the screen, you need to run this function with TRUE for
//! enable and bind the change_position signal on the applet. This
//! signal can be quite CPU/bandwidth consuming so only applets which
//! need it should use it. By default change_position is not sent.
//!
//!

Gnome.AppletWidget set_tooltip( string to );
//! Set a tooltip on the entire applet that will follow the tooltip
//! setting from the panel configuration.
//!
//!

Gnome.AppletWidget set_widget_tooltip( GTK.Widget widget, string text );
//! Set a tooltip on the widget that will follow the tooltip setting
//! from the panel configuration.
//!
//!

Gnome.AppletWidget sync_config( );
//! Tell the panel to save our session here (just saves, no
//! shutdown). This should be done when you change some of your config
//! and want the panel to save it's config, you should NOT call this in
//! the session_save handler as it will result in a locked panel, as it
//! will actually trigger another session_save signal for you. However
//! it also asks for a complete panel save, so you should not do this
//! too often, and only when the user has changed some preferences and
//! you want to sync them to disk. Theoretically you don't even need to
//! do that if you don't mind loosing settings on a panel crash or when
//! the user kills the session without logging out properly, since the
//! panel will always save your session when it exists.
//!
//!

Gnome.AppletWidget unregister_callback( string name );
//! Remove a menu item from the applet's context menu. The name should
//! be the full path to the menu item. This will not remove any
//! submenus.
//!
//!

Gnome.AppletWidget unregister_callback_dir( string name );
//! Removes a submenu from the applet's context menu. Use this instead
//! of unregister_callback to remove submenus. The name can be, but
//! doesn't have to be terminated with a '/'. If you have not removed
//! the subitems of this menu, it will still be shown but without it's
//! title or icon. So make sure to first remove any items and submenus
//! before calling this function.
//!
//!
