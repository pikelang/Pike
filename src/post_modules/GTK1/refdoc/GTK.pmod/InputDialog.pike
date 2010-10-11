//! This dialog is used to enable XInput devices.  By default, no
//! extension devices are enabled. We need a mechanism to allow users
//! to enable and configure their extension devices. GTK provides the
//! InputDialog widget to automate this process. The following
//! procedure manages an InputDialog widget. It creates the dialog if
//! it isn't present, and shows it otherwise.
//! 
//! @pre{
//! GTK.InputDialog inputd;
//! void create_input_dialog ()
//! {
//!   if (!inputd)
//!   {
//!     inputd = GTK.InputDialog();
//!     inputd-&gt;close_button()-&gt;signal_connect("clicked",inputd-&gt;hide, 0);
//!     inputd-&gt;save_button()-&gt;hide();
//!     inputd-&gt;show();
//!   }
//!   else
//!     inputd-&gt;show();
//! }
//! @}
//!@expr{ GTK.InputDialog()@}
//!@xml{<image>../images/gtk_inputdialog.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{disable_device@}
//! his signal is emitted when the user changes the mode of a device from a GDK_MODE_SCREEN or GDK_MODE_WINDOW to GDK_MODE_ENABLED
//!
//!
//! @b{enable_device@}
//! This signal is emitted when the user changes the mode of a device from GDK_MODE_DISABLED to a GDK_MODE_SCREEN or GDK_MODE_WINDOW.
//!
//!

inherit GTK.Dialog;

static GTK.InputDialog create( );
//! Create a new input dialog window.
//!
//!

GTK.Button get_close_button( );
//! The 'close' button of the dialog.
//!
//!

GTK.Button get_save_button( );
//! The 'save' button of the dialog.
//!
//!
