//! The NoteBook Widget is a collection of 'pages' that overlap each
//! other, each page contains different information. This widget has
//! become more common lately in GUI programming, and it is a good way
//! to show blocks similar information that warrant separation in their
//! display.
//!
//!@expr{ GTK.Notebook( )->set_tab_pos( GTK.POS_LEFT )->append_page( GTK.Label("Page 1\nContents"), GTK.Label("Page 1"))->append_page( GTK.Label(""), GTK.Label("Page 2"))->append_page(GTK.Label("Page 3 contents\nare here!"), GTK.Label("Page 3"))@}
//!@xml{<image>../images/gtk_notebook.png</image>@}
//!
//!@expr{ GTK.Notebook( )->set_tab_pos( GTK.POS_TOP )->append_page( GTK.Label("Page 1\nContents"), GTK.Label("Page 1"))->append_page( GTK.Label(""), GTK.Label("Page 2"))->append_page(GTK.Label("Page 3 contents\nare here!"), GTK.Label("Page 3"))@}
//!@xml{<image>../images/gtk_notebook_2.png</image>@}
//!
//!@expr{ GTK.Notebook( )->set_tab_pos( GTK.POS_RIGHT )->append_page( GTK.Label("Page 1\nContents"), GTK.Label("Page 1"))->append_page( GTK.Label(""), GTK.Label("Page 2"))->append_page(GTK.Label("Page 3 contents\nare here!"), GTK.Label("Page 3"))->next_page()->next_page()@}
//!@xml{<image>../images/gtk_notebook_3.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{switch_page@}
//! Called when a different page is selected
//!
//!

inherit GTK.Container;

GTK.Notebook append_page( GTK.Widget contents, GTK.Widget label );
//! Add a new 'page' to the notebook. The first argument is the contents of
//! the page, the second argument is the label.
//!
//!

GTK.Notebook append_page_menu( GTK.Widget contents, GTK.Widget label, GTK.Menu menu );
//! Add a new 'page' to the notebook. The first argument is the
//! contents of the page, the second argument is the label, the third
//! argument is a menu widget.
//!
//!

static GTK.Notebook create( );
//!

int get_current_page( );
//!  Returns the index of the currently selected page
//!
//!

GTK.Widget get_menu_label( GTK.Widget page );
//!

GTK.Widget get_nth_page( int index );
//!  Returns the page for the specified index
//!
//!

GTK.Widget get_tab_label( GTK.Widget page );
//!

GTK.Notebook insert_page( GTK.Widget contents, GTK.Widget label, int pos );
//! Insert a page at the specified location, arguments as for
//! append_page, but an aditional integer specifies the location.
//!
//!

GTK.Notebook insert_page_menu( GTK.Widget contents, GTK.Widget label, GTK.Menu menu, int pos );
//! Insert a page at the specified location, arguments as for
//! append_page_menu, but an aditional integer specifies the location.
//!
//!

GTK.Notebook next_page( );
//! Go to the next page
//!
//!

int page_num( GTK.Widget widget );
//!  Returns the index for the specified page
//!
//!

GTK.Notebook popup_disable( );
//! Disable the popup menu (set with insert_page_menu)
//!
//!

GTK.Notebook popup_enable( );
//! Enable the popup menu (set with insert_page_menu)
//!
//!

GTK.Notebook prepend_page( GTK.Widget contents, GTK.Widget label );
//! Add a page at the end of the list of pages. The first argument is
//! the contents of the page, the second argument is the label.
//!
//!

GTK.Notebook prepend_page_menu( GTK.Widget contents, GTK.Widget label, GTK.Menu menu );
//! Add a new 'page' at the end of the list of pages. The first
//! argument is the contents of the page, the second argument is the
//! label, the third argument is a menu widget.
//!
//!

GTK.Notebook prev_page( );
//! Go to the previous page
//!
//!

mapping query_tab_label_packing( GTK.Widget page );
//! Returns ([ "expand":expandp, "fill":fillp, "pack_type":type ])
//!
//!

GTK.Notebook remove_page( int pos );
//! Remove a page.
//!
//!

GTK.Notebook reorder_child( GTK.Widget page, int new_index );
//! Move the specified page to the index new_index
//!
//!

GTK.Notebook set_homogeneous_tabs( int homogeneousp );
//! If true, all tabs will have the same size
//!
//!

GTK.Notebook set_menu_label( GTK.Widget page, GTK.Widget label );
//!

GTK.Notebook set_menu_label_text( GTK.Widget page, string label );
//!

GTK.Notebook set_page( int pos );
//! Go to the specified page
//!
//!

GTK.Notebook set_scrollable( int scrollablep );
//! If true, add scrollbars if nessesary.
//!
//!

GTK.Notebook set_show_border( int showborderp );
//! If true, show the borders around the contents and tabs.
//!
//!

GTK.Notebook set_show_tabs( int showtabsp );
//! If supplied with a true value, the tabs will be shown. Otherwise
//! they will not be shown. The user will not be able to select the
//! pages without them, but you can add 'next' and 'previous' buttons
//! to create a wizard-line interface.
//!
//!

GTK.Notebook set_tab_border( int border_width );
//! In pixels.
//!
//!

GTK.Notebook set_tab_hborder( int border_width );
//! In pixels.
//!
//!

GTK.Notebook set_tab_label( GTK.Widget page, GTK.Widget label );
//!

GTK.Notebook set_tab_label_packing( GTK.Widget child, int expand, int fill, int type );
//!

GTK.Notebook set_tab_label_text( GTK.Widget page, string title );
//!

GTK.Notebook set_tab_pos( int pos );
//! One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!

GTK.Notebook set_tab_vborder( int border_width );
//! In pixels.
//!
//!
