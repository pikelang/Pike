//! A dialog is a window with a few default widgets added.
//! The 'vbox' is the main content area of the widget. The
//! 'action_area' is allocated for buttons (ok, cancel etc)
//! Properties:
//! int has-separator
//! 
//! Style properties:
//! int action-area-border
//! int button-spacing
//! int content-area-border
//!
//!
//!  Signals:
//! @b{close@}
//!
//! @b{response@}
//!

inherit GTK2.Window;

GTK2.Dialog add_action_widget( GTK2.Widget child, int response_id );
//! Adds an activatable widget to the action area, connecting
//! a signal handler that will emit the "response" signal on
//! the dialog when the widget is activated.
//!
//!

GTK2.Widget add_button( string button_text, int response_id );
//! Adds a button with the given text (or a stock button)
//! and sets things up so that clicking the button will emit
//! the "response" signal with the given response_id.
//!
//!

static GTK2.Dialog create( mapping|void props );
//! Create a new dialog widget.
//!
//!

GTK2.HbuttonBox get_action_area( );
//! The action area, this is where the buttons (ok, cancel etc) go
//!
//!

int get_has_separator( );
//! Accessor for whether the dialog has a separator.
//!
//!

int get_response_for_widget( GTK2.Widget widget );
//! Gets the response id of a widget in the action area.
//!
//!

GTK2.Vbox get_vbox( );
//! The vertical box that should contain the contents of the dialog
//!
//!

GTK2.Dialog response( int response_id );
//! Emits the "response" signal with the given response ID.
//!
//!

int run( );
//! Run the selected dialog.
//!
//!

GTK2.Dialog set_default_response( int response_id );
//! Sets the last widget in the action area with the given response_id as the
//! default widget.  Pressing "Enter" normally activates the default widget.
//!
//!

GTK2.Dialog set_has_separator( int setting );
//! Sets whether this dialog has a separator above the buttons.  True by
//! default.
//!
//!

GTK2.Dialog set_response_sensitive( int response_id, int setting );
//! Calls GTK2.Widget->set_sensitive() for each widget in the dialog's action
//! area with the given response_id.
//!
//!
