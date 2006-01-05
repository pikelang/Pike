//! This widget is a virtual widget to define the interface to a druid
//! page. It's descendants are placed in a W(Gnome2.Druid) widget.
//!
//!
//!  Signals:
//! @b{back@}
//!
//! @b{cancel@}
//!
//! @b{finish@}
//!
//! @b{next@}
//!
//! @b{prepare@}
//!

inherit GTK2.Bin;

int back( );
//! This will emit the "back" signal for that particular page.
//!
//!

int cancel( );
//! This will emit the "cancel" signal for that particular page.
//!
//!

static Gnome2.DruidPage create( );
//! Creates a new Gnome2.DruidPage.
//!
//!

Gnome2.DruidPage finish( );
//! This emits the "finish" signal for the page.
//!
//!

int next( );
//! This will emit the "next" signal for that particular page.  It is called by
//! gnome-druid exclusviely.  It is expected that non-linear Druid's will
//! override this signal and return true if it handles changing pages.
//!
//!

Gnome2.DruidPage prepare( );
//! This emits the "prepare" signal for the page.  It is called by gnome-druid
//! exclusively.  This function is called immediately prior to a druid page
//! being show so that it can "prepare" for display.
//!
//!
