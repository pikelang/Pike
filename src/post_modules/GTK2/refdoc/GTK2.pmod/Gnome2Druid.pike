//! The GNOME druid is a system for assisting the user with installing
//! a service. It is roughly equivalent in functionality to the
//! "Wizards" available in Windows.
//! 
//! There are two major parts of the druid, the Gnome2.Druid widget, and
//! the set of W(Gnome2.DruidPage) widgets. The Gnome2.Druid widget is the
//! main widget that interacts with the user. It has a Next, a Prev,
//! and a Cancel button, and acts as a container for the pages. It is
//! not a top-level window, so it needs to be put in a W(GTK2.Window) in
//! almost all cases. The W(Gnome2.DruidPage) is a virtual widget, from
//! which all of the actual content of the page inherits from. There
//! are currently three of these available within gnome-libs.
//! 
//! GNOME druids are fairly simple to program with. You start by
//! creating a GnomeDruid into which you put all of your pages. This
//! widget will handle the presentation of the W(GnomeDruidPage) widgets.
//! 
//! You then create all appropriate W(GnomeDruidPage) widgets. There
//! are three implementations of these, although there is no reason why
//! more couldn't be written. They are the W(GnomeDruidPageStart), the
//! W(GnomeDruidPageStandard), and the W(GnomeDruidPageFinish). The
//! W(GnomeDruidPageStandard) acts as a W(Container), and is probably
//! the most commonly used druid page. The other ones, as their names
//! might suggest, are used at the endpoints of the druid. More
//! information on the specific properties of these widgets can be
//! found on their respective pages.
//! 
//! You will need to add the pages to the druid in order for them to
//! appear. The druid itself keeps an internal list of all pages, and
//! using the prepend_page(), append_page(), and insert_page()
//! functions will place them into it.
//! 
//! Properties:
//! int show-finish
//! int show-help
//!
//!
//!  Signals:
//! @b{cancel@}
//! This signal is emitted when the "cancel" button has been
//! pressed. Note that the current druid page has the option to trap
//! the signal and use it, if need be, preventing this signal from
//! being emitted.
//!
//!
//! @b{help@}
//!

inherit GTK2.Container;

Gnome2.Druid append_page( GTK2.Gnome2DruidPage page );
//! This will append a GnomeDruidPage into the internal list of pages
//! that the druid has.
//!
//!

static Gnome2.Druid create( );
//! Create a new druid
//!
//!

Gnome2.Druid insert_page( GTK2.Gnome2DruidPage back_page, GTK2.Gnome2DruidPage page );
//! This will insert page after back_page into the list of internal
//! pages that the druid has. If back_page is not present in the list
//! or is 0, page will be prepended to the list.
//!
//!

Gnome2.Druid prepend_page( GTK2.Gnome2DruidPage page );
//! This will prepend a GnomeDruidPage into the internal list of pages
//! that the druid has.
//!
//!

Gnome2.Druid set_buttons_sensitive( int back_sensitive, int next_sensitive, int cancel_sensitive, int help_sensitive );
//! Sets the sensitivity of the druid's control-buttons. If the
//! variables are TRUE, then they will be clickable. This function is
//! used primarily by the actual W(GnomeDruidPage) widgets.
//!
//!

Gnome2.Druid set_page( GTK2.Gnome2DruidPage page );
//! This will make page the currently showing page in the druid. page
//! must already be in the druid.
//!
//!

Gnome2.Druid set_show_finish( int show_finish );
//! Sets the text on the last button on the druid. If show_finish is
//! TRUE, then the text becomes "Finish". If show_finish is FALSE,
//! then the text becomes "Cancel".
//!
//!

Gnome2.Druid set_show_help( int show_help );
//! Sets the "Help" button on the druid to be visible in the lower left corner
//! of the widget, if show_help is true.
//!
//!
