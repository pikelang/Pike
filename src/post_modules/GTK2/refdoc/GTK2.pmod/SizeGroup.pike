//! Provides a mechanism for grouping a number of widgets together so they all
//! request the same amount of space.  This is typically usefull when you want
//! a column of widgets to have the same size, but you can't use a W(Table)
//! widget.
//! 
//! In detail, the size requiest for each widget in a GTK2.SizeGroup is the
//! maximum of the sizes that would have been requested for each widget in the
//! size group if they were not in the size group.  The mode of the size group
//! (see set_mode() determines whether this applies to the horizontal size, the
//! vertical size, or both sizes.
//! 
//! Note that size groups only affect the amount of space requested, not the
//! size that the widgets finally receive.  If you want the widgets in a
//! GTK2.SizeGroup to actually be the same size, you need to pack them in such
//! a way that they get the size they request and not more.  For example, if
//! you are packing your widgets into a table, you would not include the
//! GTK2.FILL flag.
//! 
//! GTK2.SizeGroup objects are referenced by each widget in the size group, so
//! one you have added all widgets to a GTK2.SizeGroup, you can drop the initial
//! reference to the size group.  If the widgets in the size group are
//! subsequently destroyed, then they will be removed from the size group; when
//! allow idgets have been removed, the size group will be freed.
//! 
//! Widgets can be part of multiple size groups; GTK+ will compute the
//! horizontal size of a widget from the horizontal requisition of all widgets
//! that can be reached from the widget by a chain of size groups of type
//! GTK2.SIZE_GROUP_HORIZONTAL or GTK2.SIZE_GROUP_BOTH, and the vertical size
//! from the vertical requisition of all widgets that can be reached from the
//! widget by a chain of size groups of type GTK2.SIZE_GROUP_VERTICAL or
//! GTK2.SIZE_GROUP_BOTH.
//! Properties:
//! int mode
//!    The directions in which the size group effects the requested sizes of
//!    its componenent widgets.
//!
//!

inherit G.Object;

GTK2.SizeGroup add_widget( GTK2.Widget widget );
//! Adds a widget to the group.  In the future, the requisition of the widget
//! will be determined as the maximum of its requisition and the requisition
//! of the other widgets in the size group.  Whether this applies horizontally,
//! vertically, or in both directions depends on the mode.
//!
//!

static GTK2.SizeGroup create( int|mapping mode_or_props );
//! Create a new group.
//!
//!

int get_mode( );
//! Gets the current mode.
//!
//!

GTK2.SizeGroup remove_widget( GTK2.Widget widget );
//! Removes a widget.
//!
//!

GTK2.SizeGroup set_mode( int mode );
//! Sets the mode of the size group.  One of @[SIZE_GROUP_BOTH], @[SIZE_GROUP_HORIZONTAL], @[SIZE_GROUP_NONE] and @[SIZE_GROUP_VERTICAL].  The mode
//! of the size group determines whether the widgets in the size group should
//! all have the same horizontal requisition, all have the same vertical
//! requisition, or should all have the same requisition in both directions.
//!
//!
