//! The NoteBook Widget is a collection of 'pages' that overlap each
//! other, each page contains different information. This widget has
//! become more common lately in GUI programming, and it is a good way
//! to show blocks similar information that warrant separation in their
//! display.
//!
//!@expr{ GTK1.Notebook( )->set_tab_pos( GTK1.POS_LEFT )->append_page( GTK1.Label("Page 1\nContents"), GTK1.Label("Page 1"))->append_page( GTK1.Label(""), GTK1.Label("Page 2"))->append_page(GTK1.Label("Page 3 contents\nare here!"), GTK1.Label("Page 3"))@}
//!@xml{<image>../images/gtk1_notebook.png</image>@}
//!
//!@expr{ GTK1.Notebook( )->set_tab_pos( GTK1.POS_TOP )->append_page( GTK1.Label("Page 1\nContents"), GTK1.Label("Page 1"))->append_page( GTK1.Label(""), GTK1.Label("Page 2"))->append_page(GTK1.Label("Page 3 contents\nare here!"), GTK1.Label("Page 3"))@}
//!@xml{<image>../images/gtk1_notebook_2.png</image>@}
//!
//!@expr{ GTK1.Notebook( )->set_tab_pos( GTK1.POS_RIGHT )->append_page( GTK1.Label("Page 1\nContents"), GTK1.Label("Page 1"))->append_page( GTK1.Label(""), GTK1.Label("Page 2"))->append_page(GTK1.Label("Page 3 contents\nare here!"), GTK1.Label("Page 3"))->next_page()->next_page()@}
//!@xml{<image>../images/gtk1_notebook_3.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{switch_page@}
//! Called when a different page is selected
//!
//!

inherit GTK1.Container;

GTK1.Notebook append_page( GTK1.Widget contents, GTK1.Widget label );
//! Add a new 'page' to the notebook. The first argument is the contents of
//! the page, the second argument is the label.
//!
//!

GTK1.Notebook append_page_menu( GTK1.Widget contents, GTK1.Widget label, GTK1.Menu menu );
//! Add a new 'page' to the notebook. The first argument is the
//! contents of the page, the second argument is the label, the third
//! argument is a menu widget.
//!
//!

protected GTK1.Notebook create( );
//!

int get_current_page( );
//!  Returns the index of the currently selected page
//!
//!

GTK1.Widget get_menu_label( GTK1.Widget page );
//!

GTK1.Widget get_nth_page( int index );
//!  Returns the page for the specified index
//!
//!

GTK1.Widget get_tab_label( GTK1.Widget page );
//!

GTK1.Notebook insert_page( GTK1.Widget contents, GTK1.Widget label, int pos );
//! Insert a page at the specified location, arguments as for
//! append_page, but an aditional integer specifies the location.
//!
//!

GTK1.Notebook insert_page_menu( GTK1.Widget contents, GTK1.Widget label, GTK1.Menu menu, int pos );
//! Insert a page at the specified location, arguments as for
//! append_page_menu, but an aditional integer specifies the location.
//!
//!

GTK1.Notebook next_page( );
//! Go to the next page
//!
//!

int page_num( GTK1.Widget widget );
//!  Returns the index for the specified page
//!
//!

GTK1.Notebook popup_disable( );
//! Disable the popup menu (set with insert_page_menu)
//!
//!

GTK1.Notebook popup_enable( );
//! Enable the popup menu (set with insert_page_menu)
//!
//!

GTK1.Notebook prepend_page( GTK1.Widget contents, GTK1.Widget label );
//! Add a page at the end of the list of pages. The first argument is
//! the contents of the page, the second argument is the label.
//!
//!

GTK1.Notebook prepend_page_menu( GTK1.Widget contents, GTK1.Widget label, GTK1.Menu menu );
//! Add a new 'page' at the end of the list of pages. The first
//! argument is the contents of the page, the second argument is the
//! label, the third argument is a menu widget.
//!
//!

GTK1.Notebook prev_page( );
//! Go to the previous page
//!
//!

mapping query_tab_label_packing( GTK1.Widget page );
//! Returns ([ "expand":expandp, "fill":fillp, "pack_type":type ])
//!
//!

GTK1.Notebook remove_page( int pos );
//! Remove a page.
//!
//!

GTK1.Notebook reorder_child( GTK1.Widget page, int new_index );
//! Move the specified page to the index new_index
//!
//!

GTK1.Notebook set_homogeneous_tabs( int homogeneousp );
//! If true, all tabs will have the same size
//!
//!

GTK1.Notebook set_menu_label( GTK1.Widget page, GTK1.Widget label );
//!

GTK1.Notebook set_menu_label_text( GTK1.Widget page, string label );
//!

GTK1.Notebook set_page( int pos );
//! Go to the specified page
//!
//!

GTK1.Notebook set_scrollable( int scrollablep );
//! If true, add scrollbars if nessesary.
//!
//!

GTK1.Notebook set_show_border( int showborderp );
//! If true, show the borders around the contents and tabs.
//!
//!

GTK1.Notebook set_show_tabs( int showtabsp );
//! If supplied with a true value, the tabs will be shown. Otherwise
//! they will not be shown. The user will not be able to select the
//! pages without them, but you can add 'next' and 'previous' buttons
//! to create a wizard-line interface.
//!
//!

GTK1.Notebook set_tab_border( int border_width );
//! In pixels.
//!
//!

GTK1.Notebook set_tab_hborder( int border_width );
//! In pixels.
//!
//!

GTK1.Notebook set_tab_label( GTK1.Widget page, GTK1.Widget label );
//!

GTK1.Notebook set_tab_label_packing( GTK1.Widget child, int expand, int fill, int type );
//!

GTK1.Notebook set_tab_label_text( GTK1.Widget page, string title );
//!

GTK1.Notebook set_tab_pos( int pos );
//! One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!

GTK1.Notebook set_tab_vborder( int border_width );
//! In pixels.
//!
//!
