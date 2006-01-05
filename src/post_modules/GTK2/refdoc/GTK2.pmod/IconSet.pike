//! Iconset.  A single icon.
//!
//!

GTK2.IconSet add_source( GTK2.IconSource source );
//! Icon sets have a list of GTK2.IconSource, which they use as base icons for
//! rendering icons in different states and sizes.  Icons are scaled, made to
//! look insensitive, etc. in render_icon(), but GTK2.IconSet needs base images
//! to work with.  The base images and when to use them are described by a
//! GTK2.IconSource.
//! 
//! This function copies source, so you can reuse the same source immediately
//! without affecting the icon set.
//! 
//! An example of when you'd use this function:  a web browser's "Back to
//! Previous Page" icon might point in a different direciton in Hebrew and in
//! English; it might look different when insensitive; and it might change size
//! depending on toolbar mode (small/large icons).  So a single icon set would
//! contain all those variants of the icon, and you might add a separate source
//! for each one.
//! 
//! You should nearly always add a "default" icon source with all fields
//! wildcarded, which will be used as a fallback if no more specific source
//! matches.  GTK2.IconSet always prefers more specific icon sources to more
//! generic icon sources.  The order in which you add the sources to the icon
//! set does not matter.
//!
//!

GTK2.IconSet copy( );
//! Create a copy.
//!
//!

static GTK2.IconSet create( GTK2.GdkPixbuf pixbuf );
//! Create a new GTK2.IconSet.  A GTK2.IconSet represents a single icon in
//! various sizes and widget states.  It can provide a GDK2.Pixbuf for a given
//! size and state on request, and automatically caches some of the rendered
//! GDK2.Pixbuf objects.
//! 
//! Normally you would use GTK2.Widget->render_icon() instead of using
//! GTK2.IconSet directly.  The one case where you'd use GTK2.IconSet is to
//! create application-specific icon sets to place in a GTK2.IconFactory.
//!
//!

GTK2.IconSet destroy( );
//! Destructor.
//!
//!

array get_sizes( );
//! Obtains a list of icon sizes this icon set can render.
//!
//!
