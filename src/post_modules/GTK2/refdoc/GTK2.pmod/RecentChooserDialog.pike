//!

inherit GTK2.Dialog;

inherit GTK2.RecentChooser;

protected GTK2.RecentChooserDialog create( mapping|string title, GTK2.Window parent, array buttons, GTK2.RecentManager manager );
//! Creates a new RecentChooserDialog.  Analogous to
//! GTK2.Dialog->create().
//!
//!
