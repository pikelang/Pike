//! A Vbutton_box is very similar to a Vbox.
//! The major diffference is that the button box
//! is made to pack buttons in, and has a few convenience function for
//! normal button layouts.
//!
//!@code{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_usize(100,300)@}
//!@xml{<image src='../images/gtk_vbuttonbox.png'/>@}
//!
//!@code{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_SPREAD)->set_usize(100,300)@}
//!@xml{<image src='../images/gtk_vbuttonbox_2.png'/>@}
//!
//!@code{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_EDGE)->set_usize(100,300)@}
//!@xml{<image src='../images/gtk_vbuttonbox_3.png'/>@}
//!
//!@code{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_START)->set_usize(100,300)@}
//!@xml{<image src='../images/gtk_vbuttonbox_4.png'/>@}
//!
//!@code{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_END)->set_usize(100,300)@}
//!@xml{<image src='../images/gtk_vbuttonbox_5.png'/>@}
//!
//!
//!
inherit ButtonBox;

static VbuttonBox create( )
//! Create a new vertical button box
//!
//!
