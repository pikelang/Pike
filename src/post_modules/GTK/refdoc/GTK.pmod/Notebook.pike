//! The NoteBook Widget is a collection of 'pages' that overlap each
//! other, each page contains different information. This widget has
//! become more common lately in GUI programming, and it is a good way
//! to show blocks similar information that warrant separation in their
//! display.
//!
//!@code{ GTK.Notebook( )->set_tab_pos( GTK.POS_LEFT )->append_page( GTK.Label("Page 1\nContents"), GTK.Label("Page 1"))->append_page( GTK.Label(""), GTK.Label("Page 2"))->append_page(GTK.Label("Page 3 contents\nare here!"), GTK.Label("Page 3"))@}
//!@xml{<image src='../images/gtk_notebook.png'/>@}
//!
//!@code{ GTK.Notebook( )->set_tab_pos( GTK.POS_TOP )->append_page( GTK.Label("Page 1\nContents"), GTK.Label("Page 1"))->append_page( GTK.Label(""), GTK.Label("Page 2"))->append_page(GTK.Label("Page 3 contents\nare here!"), GTK.Label("Page 3"))@}
//!@xml{<image src='../images/gtk_notebook_2.png'/>@}
//!
//!@code{ GTK.Notebook( )->set_tab_pos( GTK.POS_RIGHT )->append_page( GTK.Label("Page 1\nContents"), GTK.Label("Page 1"))->append_page( GTK.Label(""), GTK.Label("Page 2"))->append_page(GTK.Label("Page 3 contents\nare here!"), GTK.Label("Page 3"))->next_page()->next_page()@}
//!@xml{<image src='../images/gtk_notebook_3.png'/>@}
//!
//!
//!
//!  Signals:
//! @b{switch_page@}
//! Called when a different page is selected
//!
//!
inherit Container;

Notebook append_page( GTK.Widget contents, GTK.Widget label )
//! Add a new 'page' to the notebook. The first argument is the contents of
//! the page, the second argument is the label.
//!
//!

Notebook append_page_menu( GTK.Widget contents, GTK.Widget label, GTK.Menu menu )
//! Add a new 'page' to the notebook. The first argument is the
//! contents of the page, the second argument is the label, the third
//! argument is a menu widget.
//!
//!

static Notebook create( )
//!

int get_current_page( )
//!  Returns the index of the currently selected page
//!
//!

GTK.Widget get_menu_label( GTK.Widget page )
//!

GTK.Widget get_nth_page( int index )
//!  Returns the page for the specified index
//!
//!

GTK.Widget get_tab_label( GTK.Widget page )
//!

Notebook insert_page( GTK.Widget contents, GTK.Widget label, int pos )
//! Insert a page at the specified location, arguments as for
//! append_page, but an aditional integer specifies the location.
//!
//!

Notebook insert_page_menu( GTK.Widget contents, GTK.Widget label, GTK.Menu menu, int pos )
//! Insert a page at the specified location, arguments as for
//! append_page_menu, but an aditional integer specifies the location.
//!
//!

Notebook next_page( )
//! Go to the next page
//!
//!

int page_num( GTK.Widget widget )
//!  Returns the index for the specified page
//!
//!

Notebook popup_disable( )
//! Disable the popup menu (set with insert_page_menu)
//!
//!

Notebook popup_enable( )
//! Enable the popup menu (set with insert_page_menu)
//!
//!

Notebook prepend_page( GTK.Widget contents, GTK.Widget label )
//! Add a page at the end of the list of pages. The first argument is
//! the contents of the page, the second argument is the label.
//!
//!

Notebook prepend_page_menu( GTK.Widget contents, GTK.Widget label, GTK.Menu menu )
//! Add a new 'page' at the end of the list of pages. The first
//! argument is the contents of the page, the second argument is the
//! label, the third argument is a menu widget.
//!
//!

Notebook prev_page( )
//! Go to the previous page
//!
//!

mapping query_tab_label_packing( GTK.Widget page )
//! Returns ([ "expand":expandp, "fill":fillp, "pack_type":type ])
//!
//!

Notebook remove_page( int pos )
//! Remove a page.
//!
//!

Notebook reorder_child( GTK.Widget page, int new_index )
//! Move the specified page to the index new_index
//!
//!

Notebook set_homogeneous_tabs( int homogeneousp )
//! If true, all tabs will have the same size
//!
//!

Notebook set_menu_label( GTK.Widget page, GTK.Widget label )
//!

Notebook set_menu_label_text( GTK.Widget page, string label )
//!

Notebook set_page( int pos )
//! Go to the specified page
//!
//!

Notebook set_scrollable( int scrollablep )
//! If true, add scrollbars if nessesary.
//!
//!

Notebook set_show_border( int showborderp )
//! If true, show the borders around the contents and tabs.
//!
//!

Notebook set_show_tabs( int showtabsp )
//! If supplied with a true value, the tabs will be shown. Otherwise
//! they will not be shown. The user will not be able to select the
//! pages without them, but you can add 'next' and 'previous' buttons
//! to create a wizard-line interface.
//!
//!

Notebook set_tab_border( int border_width )
//! In pixels.
//!
//!

Notebook set_tab_hborder( int border_width )
//! In pixels.
//!
//!

Notebook set_tab_label( GTK.Widget page, GTK.Widget label )
//!

Notebook set_tab_label_packing( GTK.Widget child, int expand, int fill, int type )
//!

Notebook set_tab_label_text( GTK.Widget page, string title )
//!

Notebook set_tab_pos( int pos )
//! One of @[POS_BOTTOM], @[POS_TOP], @[POS_RIGHT] and @[POS_LEFT]
//!
//!

Notebook set_tab_vborder( int border_width )
//! In pixels.
//!
//!
