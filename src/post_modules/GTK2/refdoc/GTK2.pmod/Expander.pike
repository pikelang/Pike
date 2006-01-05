//! Properties:
//! int expanded
//! string label
//! GTK2.Widget label-widget
//! int spacing
//! int use-markup
//! int use-underline
//! 
//! Style properties:
//! int expander-size
//! int expander-spacing
//!
//!
//!  Signals:
//! @b{activate@}
//!

inherit GTK2.Bin;

static GTK2.Expander create( string|mapping label );
//! Create a new GTK2.Expander.  Set mnemonic to 1
//! for a mnemonic expander, otherwise omitting it
//! creates a regular expander.
//!
//!

int get_expanded( );
//! Returns true if the child widget is revealed.
//!
//!

string get_label( );
//! Fetches the text from the label of the expander.
//!
//!

GTK2.Widget get_label_widget( );
//! Retrieves the label widget for the frame.
//!
//!

int get_spacing( );
//! Returns the spacing.
//!
//!

int get_use_markup( );
//! Returns whether the label's text is interpreted as marked up
//! with the Pango text markup language.
//!
//!

int get_use_underline( );
//! Returns whether an embedded underline in the label
//! indicates a mnemonic.
//!
//!

GTK2.Expander set_expanded( int expanded );
//! Sets the state of the expander.  Set to true if you want the
//! child widget to be revealed, and false if you want the child
//! widget to be hidden.
//!
//!

GTK2.Expander set_label( string label );
//! Sets the text of the label of the expander.
//!
//!

GTK2.Expander set_label_widget( GTK2.Widget label );
//! Set the label widget for the expander.  This is the widget
//! that will appear embedded alongside the expander arrow.
//!
//!

GTK2.Expander set_spacing( int spacing );
//! Sets the spacing field of the expander, which is the number
//! of pixels to place between the expander and the child.
//!
//!

GTK2.Expander set_use_markup( int use_markup );
//! Sets whether the text of the label contains markup in
//! Pango's text markup language.
//!
//!

GTK2.Expander set_use_underline( int use_underline );
//! If true, an underline in the text of the label indicates
//! the next character should be used for the mnemonic
//! accelarator key.
//!
//!
