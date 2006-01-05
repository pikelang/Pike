//! Actions are organized into groups.  An action group is essentially a map
//! from names to GTK2.Action objects.
//! 
//! All actions that would make sense to use in a particular context should be
//! in a single group.  Multiple action groups may be used for a particular
//! user interface.  In fact, it is expected that most non-trivial applications
//! will make use of multiple groups.  For example, in an application that can
//! edit multiple documents, one group holding global actions (e.g. quit,
//! about, new), and one group per document holding actions that act on that
//! document (eg. save, cut/copy/paste, etc).  Each window's menus would be
//! constructed from a combination of two action groups.
//! 
//! Accelerators are handled by the GTK2+ accelerator map.  All actions are
//! assigned an accelerator path (which normally has the form
//! &lt;Actions&gt;/group-name/action-name) and a shortcut is associated with
//! this accelerator path.  All menuitems and toolitems take on this
//! accelerator path.  The GTK2+ accelerator map code makes sure that the
//! correct shortcut is displayed next to the menu item.
//! Properties:
//! string name
//! int sensitive
//! int visible
//!
//!
//!  Signals:
//! @b{connect_proxy@}
//!
//! @b{disconnect_proxy@}
//!
//! @b{post_activate@}
//!
//! @b{pre_activate@}
//!

inherit G.Object;

GTK2.ActionGroup add_action( GTK2.Action action, string|void accelerator );
//! Adds an action object to the action group and sets up the accelerator.
//! 
//! If accelerator is omitted, attempts to use the accelerator associated with
//! the stock_id of the action.
//! 
//! Accel paths are set to &lt;Actions&gt;/group-name/action-name.
//!
//!

GTK2.ActionGroup add_actions( array entries );
//! This is a convenience function to create a number of actions and add them
//! to the action group.
//! 
//! The "activate" signals of the actions are connect to the callbacks and
//! their accel paths are set to <Actions>/group-name/action-name.
//! 
//! Mapping is:
//! ([ "name": string,
//!    "stock_id": string,
//!    "label": string,
//!    "accelerator": string,
//!    "tooltip": string,
//!    "callback": function(callback)
//!    "data": mixed
//! ]);
//!
//!

GTK2.ActionGroup add_radio_actions( array entries, function cb, mixed user_data );
//! This is a convenience routine to create a group of radio actions and add
//! them to the action group.
//! 
//! Mapping is:
//! ([ "name": string,
//!    "stock_id": string,
//!    "label": string,
//!    "accelerator": string,
//!    "tooltip": string,
//!    "value": int
//! ]);
//!
//!

GTK2.ActionGroup add_toggle_actions( array entries );
//! This is a convenience function to create a number of toggle actions and
//! add them to the action group.
//! 
//! Mapping is:
//! ([ "name": string,
//!    "stock_id": string,
//!    "label": string,
//!    "accelerator": string,
//!    "tooltip": string,
//!    "callback": function(callback),
//!    "data": mixed,
//!    "is_active": int
//! ]);
//!
//!

static GTK2.ActionGroup create( string|mapping name_or_props );
//! Creates a new GTK2.ActionGroup object.  The name of the action group is
//! used when associating keybindings with the actions.
//!
//!

GTK2.Action get_action( string name );
//! Looks up an action in the action group by name.
//!
//!

string get_name( );
//! Gets the name of the action group.
//!
//!

int get_sensitive( );
//! Returns true if the group is sensitive.  The constituent actions can only
//! be logically sensitive if they are sensitive and their group is sensitive.
//!
//!

int get_visible( );
//! Returns true if the group is visible.  The constituent actions can only be
//! logically visible if they are visible and their group is visible.
//!
//!

array list_actions( );
//! Lists the actions in the action group.
//!
//!

GTK2.ActionGroup remove_action( GTK2.Action action );
//! Removes an action object.
//!
//!

GTK2.ActionGroup set_sensitive( int setting );
//! Changes the sensitivity.
//!
//!

GTK2.ActionGroup set_visible( int setting );
//! Changes the visibility.
//!
//!
