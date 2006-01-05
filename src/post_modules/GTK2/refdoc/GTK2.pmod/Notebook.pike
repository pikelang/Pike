//! The NoteBook Widget is a collection of 'pages' that overlap each
//! other, each page contains different information. This widget has
//! become more common lately in GUI programming, and it is a good way
//! to show blocks similar information that warrant separation in their
//! display.
//!
//!@expr{ GTK2.Notebook()->set_tab_pos(GTK2.POS_LEFT)->append_page(GTK2.Label("Page 1\nContents"),GTK2.Label("Page 1"))->append_page(GTK2.Label(""),GTK2.Label("Page 2"))->append_page(GTK2.Label("Page 3 contents\nare here!"),GTK2.Label("Page 3"))@}
//!@xml{<image>../images/gtk2_notebook.png</image>@}
//!
//!@expr{ GTK2.Notebook()->set_tab_pos(GTK2.POS_TOP)->append_page(GTK2.Label("Page 1\nContents"),GTK2.Label("Page 1"))->append_page(GTK2.Label(""),GTK2.Label("Page 2"))->append_page(GTK2.Label("Page 3 contents\nare here!"),GTK2.Label("Page 3"))@}
//!@xml{<image>../images/gtk2_notebook_2.png</image>@}
//!
//!@expr{ GTK2.Notebook()->set_tab_pos(GTK2.POS_RIGHT)->append_page(GTK2.Label("Page 1\nContents"),GTK2.Label("Page 1"))->append_page(GTK2.Label(""),GTK2.Label("Page 2"))->append_page(GTK2.Label("Page 3 contents\nare here!"),GTK2.Label("Page 3"))->next_page()->next_page()@}
//!@xml{<image>../images/gtk2_notebook_3.png</image>@}
//!
//! Properties:
//! int enable-popup
//! int homogeneous
//! int page
//! int scrollable
//! int show-border
//! int show-tabs
//! int tab-border
//! int tab-hborder
//! int tab-pos
//! int tab-vborder
//! 
//! Child properties:
//! string menu-label
//! int position
//! int tab-expand
//! int tab-fill
//! string tab-label
//! int tab-pack
//! 
//!
//!
//!  Signals:
//! @b{change_current_page@}
//!
//! @b{focus_tab@}
//!
//! @b{move_focus_out@}
//!
//! @b{select_page@}
//!
//! @b{switch_page@}
//! Called when a different page is selected
//!
//!

inherit GTK2.Container;

GTK2.Notebook append_page( GTK2.Widget contents, GTK2.Widget label );
//! Add a new 'page' to the notebook. The first argument is the contents of
//! the page, the second argument is the label.
//!
//!

GTK2.Notebook append_page_menu( GTK2.Widget contents, GTK2.Widget label, GTK2.Widget menu );
//! Add a new 'page' to the notebook. The first argument is the
//! contents of the page, the second argument is the label, the third
//! argument is a menu label widget.
//!
//!

static GTK2.Notebook create( mapping|void props );
//! Create a W(Notebook) widget with no pages.
//!
//!

int get_current_page( );
//! Returns the index of the currently selected page
//!
//!

GTK2.Widget get_menu_label( GTK2.Widget page );
//! Return the menu label widget.
//!
//!

string get_menu_label_text( GTK2.Widget child );
//! Retrieves the text of the menu label for the page containing child.
//!
//!

GTK2.Widget get_nth_page( int index );
//!  Returns the page for the specified index
//!
//!

int get_scrollable( );
//! Returns whether the tab label area has arrows for scrolling.
//!
//!

int get_show_border( );
//! Returns whether a bevel will be drawn around the pages.
//!
//!

int get_show_tabs( );
//! Returns whether the tabs of the notebook are shown.
//!
//!

GTK2.Widget get_tab_label( GTK2.Widget page );
//! Returns the tab label widget.
//!
//!

string get_tab_label_text( GTK2.Widget child );
//! Retrieves the text of the tab label for the page containing child.
//!
//!

int get_tab_pos( );
//! Gets the edge at which the tabs are located.
//!
//!

GTK2.Notebook insert_page( GTK2.Widget contents, GTK2.Widget label, int pos );
//! Insert a page at the specified location, arguments as for
//! append_page, but an aditional integer specifies the location.
//!
//!

GTK2.Notebook insert_page_menu( GTK2.Widget contents, GTK2.Widget label, GTK2.Widget menu, int pos );
//! Insert a page at the specified location, arguments as for
//! append_page_menu, but an additional integer specifies the location.
//!
//!

GTK2.Notebook next_page( );
//! Go to the next page
//!
//!

int page_num( GTK2.Widget widget );
//! Returns the index for the specified page.
//!
//!

GTK2.Notebook popup_disable( );
//! Disable the popup menu (set with insert_page_menu).
//!
//!

GTK2.Notebook popup_enable( );
//! Enable the popup menu (set with insert_page_menu).
//!
//!

GTK2.Notebook prepend_page( GTK2.Widget contents, GTK2.Widget label );
//! Add a page at the end of the list of pages. The first argument is
//! the contents of the page, the second argument is the label.
//!
//!

GTK2.Notebook prepend_page_menu( GTK2.Widget contents, GTK2.Widget label, GTK2.Widget menu );
//! Add a new 'page' at the end of the list of pages. The first
//! argument is the contents of the page, the second argument is the
//! label, the third argument is a menu label widget.
//!
//!

GTK2.Notebook prev_page( );
//! Go to the previous page
//!
//!

mapping query_tab_label_packing( GTK2.Widget page );
//! Returns ([ "expand":expandp, "fill":fillp, "pack_type":type ])
//!
//!

GTK2.Notebook remove_page( int pos );
//! Remove a page.
//!
//!

GTK2.Notebook reorder_child( GTK2.Widget child, int position );
//! Reorders the page containing child, so that it appears at
//! position.
//!
//!

GTK2.Notebook set_current_page( int pos );
//! Go to the specified page
//!
//!

GTK2.Notebook set_menu_label( GTK2.Widget child, GTK2.Widget label );
//! Changes the menu label for the page containing child.
//!
//!

GTK2.Notebook set_menu_label_text( GTK2.Widget child, string label_text );
//! Creates a new label with label_text sets it as the menu label.
//!
//!

GTK2.Notebook set_scrollable( int scrollablep );
//! If true, add scrollbars if necessary.
//!
//!

GTK2.Notebook set_show_border( int showborderp );
//! If true, show the borders around the contents and tabs.
//!
//!

GTK2.Notebook set_show_tabs( int showtabsp );
//! If supplied with a true value, the tabs will be shown. Otherwise
//! they will not be shown. The user will not be able to select the
//! pages without them, but you can add 'next' and 'previous' buttons
//! to create a wizard-line interface.
//!
//!

GTK2.Notebook set_tab_label( GTK2.Widget child, GTK2.Widget label );
//! Changes the tab label for child.
//!
//!

GTK2.Notebook set_tab_label_packing( GTK2.Widget child, int expand, int fill, int type );
//! Sets the packing parameters for the tab label of the page child.
//!
//!

GTK2.Notebook set_tab_label_text( GTK2.Widget child, string title );
//! Creates a new label and sets it as the tab label for the page
//! containing child.
//!
//!

GTK2.Notebook set_tab_pos( int pos );
//! Sets the edge at which the tabs for switching pages in the
//! notebook are drawn.
//! One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!
