//! Properties:
//! int text-column
//!
//!

inherit GTK2.ComboBox;

inherit GTK2.CellLayout;

inherit GTK2.CellEditable;

static GTK2.ComboBoxEntry create( string|TreeModel|mapping model, int|void text_column );
//! Create a new ComboBoxEntry, either empty or with a model.
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

GTK2.ComboBoxEntry set_text_column( int text_column );
//! Sets the model column which this widget should use to get strings
//! from.
//!
//!
