//! A container that can only contain one child, and accepts events.
//! draws a bevelbox around itself.
//!@expr{ GTK2.Button("A button")@}
//!@xml{<image>../images/gtk2_button.png</image>@}
//!
//!@expr{ GTK2.Button("A button\nwith multiple lines\nof text")@}
//!@xml{<image>../images/gtk2_button_2.png</image>@}
//!
//!@expr{ GTK2.Button()->add(GTK2.Image(GTK2.GdkImage(0)->set(Image.Image(100,40)->test())))@}
//!@xml{<image>../images/gtk2_button_3.png</image>@}
//!
//! Properties:
//! int focus-on-click
//! GTK2.Widget image
//! string label
//! int relief
//! int use-stock
//! int use-underline
//! float xalign
//! float yalign
//! 
//! Style properties:
//! int child-displacement-x
//! int child-displacement-y
//! GTK2.Border default-border
//! GTK2.Border default-outside-border
//! int displace-focus
//!
//!
//!  Signals:
//! @b{activate@}
//!
//! @b{clicked@}
//! Called when the button is pressed, and then released
//!
//!
//! @b{enter@}
//! Called when the mouse enters the button
//!
//!
//! @b{leave@}
//! Called when the mouse leaves the button
//!
//!
//! @b{pressed@}
//! Called when the button is pressed
//!
//!
//! @b{released@}
//! Called when the button is released
//!
//!

inherit GTK2.Bin;

GTK2.Button clicked( );
//! Emulate a 'clicked' event (press followed by release).
//!
//!

static GTK2.Button create( string|mapping label_or_props );
//! If a string is supplied, a W(Label) is created and added to the button.
//!
//!

GTK2.Button enter( );
//! Emulate a 'enter' event.
//!
//!

int get_focus_on_click( );
//! Returns whether the button grabs focus when it is clicked.
//!
//!

GTK2.Widget get_image( );
//! Gets the widget that is currently set as the image of the button.
//!
//!

string get_label( );
//! Get the text from the label of the button.
//!
//!

int get_relief( );
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL], set with set_relief()
//!
//!

int get_use_stock( );
//! Returns whether the button label is a stock item.
//!
//!

int get_use_underline( );
//! Returns whether an embedded underline in the button indicates
//! a mnemonic.
//!
//!

GTK2.Button leave( );
//! Emulate a 'leave' event.
//!
//!

GTK2.Button pressed( );
//! Emulate a 'press' event.
//!
//!

GTK2.Button released( );
//! Emulate a 'release' event.
//!
//!

GTK2.Button set_alignment( float xalign, float yalign );
//! Sets the alignment of the child.
//!
//!

GTK2.Button set_focus_on_click( int focus_on_click );
//! Sets whether the button will grab focus when it is clicked.
//!
//!

GTK2.Button set_image( GTK2.Widget widget );
//! Set the image of the button to the given widget.  Note that it depends on
//! the gtk-button-images setting whether the image will be displayed or not.
//!
//!

GTK2.Button set_label( string label );
//! Set the text of the label.
//!
//!

GTK2.Button set_relief( int newstyle );
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL]
//!
//!

GTK2.Button set_use_stock( int use_stock );
//! If true, the label set on the button is used as a stock id
//! to select the stock item for the button.
//!
//!

GTK2.Button set_use_underline( int use_underline );
//! If true, an underline in the text of the button label indicates the
//! next character should be used for the mnemonic accelerator key.
//!
//!
