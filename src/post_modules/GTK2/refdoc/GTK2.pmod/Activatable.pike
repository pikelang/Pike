//! Activatable widgets can be connected to a GTK.Action and reflects
//! the state of its action. A GTK.Activatable can also provide
//! feedback through its action, as they are responsible for
//! activating their related actions.
//! Properties:
//! GTK2.Action related-action
//! int use-action-apperance
//!
//!

GTK2.Action get_related_action( );
//! Gets the related action
//!
//!

int get_use_action_appearance( );
//! Gets whether this activatable should reset its layout and
//! appearance when setting the related action or when the action
//! changes appearance.
//!
//!

GTK2.Activatable set_related_action( GTK2.Action a );
//! Sets the related action
//!
//!

GTK2.Activatable set_use_action_appearance( int use_apperance );
//! Sets whether this activatable should reset its layout and
//! appearance when setting the related action or when the action
//! changes appearance
//!
//!
