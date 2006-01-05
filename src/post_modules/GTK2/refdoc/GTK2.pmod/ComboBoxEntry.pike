//! Properties:
//! int text-column
//!
//!
//!  Signals:
//! @b{editing_done@}
//!
//! @b{remove_widget@}
//!

inherit GTK2.ComboBox;

inherit GTK2.CellLayout;

static GTK2.ComboBoxEntry create( string|TreeModel|mapping model, int|void text_column );
//! Create a new ComboBoxEntry, either empty or with a model.
//!
//!

GTK2.ComboBoxEntry editing_done( );
//! Emits the "editing-done" signal.  This signal is a sign for the cell
//! renderer to update its value from the cell.
//!
//!

GTK2.Widget entry( );
//! Returns the GTK2.Entry widget
//!
//!

int get_text_column( );
//! Returns the column which this widget is using to get the strings
//! from.
//!
//!

GTK2.ComboBoxEntry remove_widget( );
//! Emits the "remove-widget" signal.  This signal is meant to indicate that
//! the cell is finished editing, and the widget may now be destroyed.
//!
//!

GTK2.ComboBoxEntry set_text_column( int text_column );
//! Sets the model column which this widget should use to get strings
//! from.
//!
//!

GTK2.ComboBoxEntry start_editing( GTK2.GdkEvent event );
//! Begins editing.  event is the GDK2.Event that began the editing process.
//! It may be empty, in the instance that editing was initiated through
//! programmatic means.
//!
//!
