//! Properties:
//! int activatable
//!   The toggle button can be activated.
//! int active
//!   The toggle state of the button.
//! int inconsistent
//!   The inconsistent state of the button.
//! int radio
//!   Draw the toggle butotn as a radio button.
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit GTK2.CellRenderer;

static GTK2.CellRendererToggle create( mapping|void props );
//! Creates a new W(CellRendererToggle).  Adjust rendering parameters using
//! object properties.  Object properties can be set globally with
//! set().
//!
//!

int get_active( );
//! Returns whether the cell renderer is active.
//!
//!

int get_radio( );
//! Returns whether we're rendering radio toggles rather than checkboxes.
//!
//!

GTK2.CellRendererToggle set_active( int setting );
//! Activates or deactivates a cell renderer.
//!
//!

GTK2.CellRendererToggle set_radio( int radio );
//! If radio is true, the cell renderer renders a radio toggle (i.e a toggle
//! in a group of mutually-exclusive toggles).  If false, it renders a check
//! toggle (a standalone boolean option).  This can be set globally for the
//! cell renderer, or changed just before rendering each cell in the model
//! (for W(TreeView), you set up a per-row setting using W(TreeViewColumn)
//! to associate model columns with cell renderer properties).
//!
//!
