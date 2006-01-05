//!

inherit GTK2.Object;

static Gnome2.Client create( );
//! Gets the master session management client.
//!
//!

Gnome2.Client disconnect( );
//! Disconnect the client from the session manager.
//!
//!

Gnome2.Client flush( );
//! This will force the underlying connection to the session manager to be
//! flushed.  This is useful if you have some pending changes that you want to
//! make sure get committed.
//!
//!

string get_config_prefix( );
//! Get the config prefix.  This config prefix provides a suitable place to
//! store any details about the state of the client which can not be described
//! using the app's command line arguments (as set in the restart command).
//!
//!

string get_desktop_id( );
//! Get the client ID of the desktop's current instance, i.e. if you consider
//! the desktop as a whole as a session managed app, this returns its session
//! ID using a gnome extension to session management.  May return empty for
//! apps not running under a recent version of gnome-session; apps should
//! handle that case.
//!
//!

int get_flags( );
//! Determine the client's status with the session manager.
//!
//!

string get_global_config_prefix( );
//! Get the config prefix that will be returned by get_config_prefix() for
//! clients which have NOT been restarted or cloned (i.e. for clients started
//! by the user without '--sm-' options).  This config prefix may be used to
//! write the user's preferred config for these "new"clients".
//! 
//! You could also use this prefix as a place to store and retrieve config
//! details that you wish to apply to ALL instances of the app.  However, this
//! practice limits the users freedom to configure each instance in a different
//! way so it should be used with caution.
//!
//!

string get_id( );
//! Returns the session management ID.
//!
//!

string get_previous_id( );
//! Get the session management ID from the previous session.
//!
//!

Gnome2.Client request_phase_2( );
//! Request the session manager to emit the "save-yourself" signal for a second
//! time after all the clients in the session have ceased interacting with the
//! user and entered an idle state.  This might be useful if your app managers
//! other apps and requires that they are in an idle state before saving its
//! final data.
//!
//!

Gnome2.Client request_save( int save_style, int shutdown, int interact_style, int fast, int global );
//! Request the session manager to save the session in some way.  The arguments
//! correspond with the arguments passed to the "save-yourself" signal handler.
//! 
//! The save_style (@[GNOME_SAVE_BOTH], @[GNOME_SAVE_GLOBAL] and @[GNOME_SAVE_LOCAL]) indicates whether the save should
//! affect data accessible to other users (GTK2.GNOME_SAVE_GLOBAL) or only the
//! state visible to the current user (GTK2.GNOME_SAVE_LOCAL), or both.  Setting
//! shutdown to true will initiate a logout.  The interact_style
//! (@[GNOME_INTERACT_ANY], @[GNOME_INTERACT_ERRORS] and @[GNOME_INTERACT_NONE]) specifies which kinds of interaction will be
//! available.  Setting fast to true will limit the save to setting the session
//! manager properties plus any essential data.  Setting the value of global to
//! true will request that all the other apps in the session do a save as well.
//! A global save is mandatory when doing a shutdown.
//!
//!

Gnome2.Client save_any_dialog( GTK2.Dialog dialog );
//! May be called during a "save-yourself" handler to request that a (modal)
//! dialog is presented to the user.  The session manager decides when the
//! dialog is shown, but it will not be shown it unless the session manager is
//! sending an interaction style of GTK2.GNOME_INTERACT_ANY.  "Cancel" and
//! "Log out" buttons will be added during a shutdown.
//!
//!

Gnome2.Client save_error_dialog( GTK2.Dialog dialog );
//! May be called during a "save-yourself" handler when an error has occurred
//! during the save.
//!
//!

Gnome2.Client set_clone_command( array argv );
//! Set a command the session manager can use to create a new instance of the
//! application.
//! Not implemented yet.
//!
//!

Gnome2.Client set_current_directory( string dir );
//! Set the directory to be in when running shutdown, discard, restart, etc.
//! commands.
//!
//!

Gnome2.Client set_discard_command( array argv );
//! Provides a command to run when a client is removed from the session.  It
//! might delete session-specific config files, for example.  Executing the
//! discard command on the local host should delete the information saved as
//! part of the session save that was in progress when the discard command was
//! set.  For example:
//! string prefix=client->get_config_prefix();
//! array argv=({ "rm","-r" });
//! argv+=({ Gnome2.Config->get_real_path(prefix) });
//! client->set_discard_command(argv);
//! Not implemented yet.
//!
//!

Gnome2.Client set_environment( string name, string value );
//! Set an environment variable to be placed in the client's environment prior
//! to running restart, shutdown, discard, etc. commands.
//!
//!

Gnome2.Client set_global_config_prefix( string prefix );
//! Set the value used for the global config prefix.
//! The global config prefix defaults to a name based on the name of the
//! executable.  This function allows you to set it to a different value.  It
//! should be called BEFORE retrieving the config prefix for the first time.
//! Later calls will be ignored.
//!
//!

Gnome2.Client set_priority( int priority );
//! The gnome-session manager restarts clients in order of their priorities in
//! a similar way to the start up ordering in SysV.  This function allows the
//! app to suggest a position in this ordering.  The value should be between
//! 0 and 99.  A default value of 50 is assigned to apps that do not provide a
//! value.  The user may assign a different priority.
//!
//!

Gnome2.Client set_resign_command( array argv );
//! Some clients can be "undone", removing their effects and deleting any saved
//! state.  For example, xmodmap could register a resign command to undo the
//! keymap changes it saved.
//! 
//! Used by clients that use the GTK2.GNOME_RESTART_ANYWAY restart style to undo
//! their effects (these clients usually perform initialization functions and
//! leave effects behind after they die).  The resign command combines the
//! effects of a shutdown command and a discard command.  It is executed when
//! the user decides that the client should cease to be restarted.
//! Not implemented yet.
//!
//!

Gnome2.Client set_restart_command( array argv );
//! When clients crash or the user logs out and back in, they are restarted.
//! This command should perform the restart.  Executing the restart command on
//! the local host should reproduce the state of the client at the time of the
//! session save as closely as possible.  Saving config info under the
//! get_config_prefix() is generally useful.
//! Not implemented yet.
//!
//!

Gnome2.Client set_restart_style( int style );
//! Tells the session manager how the client should be restarted in future
//! sessions.  One of @[GNOME_RESTART_ANYWAY], @[GNOME_RESTART_IF_RUNNING], @[GNOME_RESTART_IMMEDIATELY] and @[GNOME_RESTART_NEVER]
//!
//!

Gnome2.Client set_shutdown_command( array argv );
//! GTK2.GNOME_RESTART_ANYWAY clients can set this command to run when the user
//! logs out but the client is no longer running.
//! 
//! Used by clients that use the GTK2.GNOME_RESTART_ANYWAY restart style to undo
//! their effects (these clients usually perform initialization functions and
//! leave effects behind after they die).  The shutdown command simply undoes
//! the effects of the client.  It is executed during a normal logout.
//! Not implemented yet.
//!
//!
