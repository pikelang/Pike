//! Properties:
//! 
//! int complete
//! GDK2.Pixbuf header-image
//! GTK2.AssistantPageType page-type
//! GDK2.Pixbuf sidebar-image
//! string title
//! 
//! Style properties:
//! 
//! int content-padding
//! int header-padding
//!
//!
//!  Signals:
//! @b{apply@}
//!
//! @b{cancel@}
//!
//! @b{close@}
//!
//! @b{prepare@}
//! A GTK2.Assistant is a widget used to represent a generally complex
//! operation splitted in several steps, guiding the user through its pages
//! and controlling the page flow to collect the necessary data.
//!
//!

inherit GTK2.Window;

GTK2.Assistant add_action_widget( GTK2.Widget child );
//! Adds a widget to the action area.
//!
//!

int append_page( GTK2.Widget page );
//! Appends a page to the assistant.
//!
//!

protected GTK2.Assistant create( mapping|void props );
//! Create a new assistant.
//!
//!

int get_current_page( );
//! Returns the page number of the current page.  Returns -1 if there are
//! no pages.
//!
//!

int get_n_pages( );
//! Returns the number of pages.
//!
//!

GTK2.Widget get_nth_page( int page_num );
//! Returnss the child widget contained in page number page_num.
//!
//!

int get_page_complete( GTK2.Widget page );
//! Gets whether page is complete.
//!
//!

GTK2.GdkPixbuf get_page_header_image( GTK2.Widget page );
//! Gets the header image for page.
//!
//!

GTK2.GdkPixbuf get_page_side_image( GTK2.Widget page );
//! Gets the side image for page.
//!
//!

string get_page_title( GTK2.Widget page );
//! Gets the title for page.
//!
//!

int get_page_type( GTK2.Widget page );
//! Gets the page type of page.
//!
//!

int insert_page( GTK2.Widget page, int pos );
//! Inserts a page at a given position.  If pos equals -1 it will append the
//! page.
//!
//!

int prepend_page( GTK2.Widget page );
//! Prepends a page to the assistant.
//!
//!

GTK2.Assistant remove_action_widget( GTK2.Widget child );
//! Removes a widget from the action area.
//!
//!

GTK2.Assistant set_current_page( int page_num );
//! Switches the page to page_num.
//!
//!

GTK2.Assistant set_forward_page_func( function f, mixed data );
//! Set the forward page function.
//!
//!

GTK2.Assistant set_page_complete( GTK2.Widget page, int complete );
//! Sets whether page contents are complete. This will make assistant update
//! the buttons state to be able to continue the task.
//!
//!

GTK2.Assistant set_page_header_image( GTK2.Widget page, GTK2.GdkPixbuf pixbuf );
//! Sets a header image for page.  This image is displayed in the header area
//! of the assistant when page is the current page.
//!
//!

GTK2.Assistant set_page_side_image( GTK2.Widget page, GTK2.GdkPixbuf pixbuf );
//! Sets a side image for page. This image is displayed in the side area of
//! the assistant when page is the current page.
//!
//!

GTK2.Assistant set_page_title( GTK2.Widget page, string title );
//! Sets a title for page. The title is displayed in the header area of the
//! assistant when page is the current page.
//!
//!

GTK2.Assistant set_page_type( GTK2.Widget page, int type );
//! Sets the page type for page.  The page type determines the page behavior.
//!
//!

GTK2.Assistant update_buttons_state( );
//! Forces the assistant to recompute the buttons state.
//! 
//! GTK+ automatically takes care of this in most situations, e.g. when the
//! user goes to a different page, or when the visibility or completeness
//! of a page changes.
//! 
//! One situation where it can be necessary to call this function is when
//! changing a value on the current page affects the future page flow of the
//! assistant.
//!
//!
