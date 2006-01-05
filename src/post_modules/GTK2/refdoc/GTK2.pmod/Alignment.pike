//! The W(Alignment) widget controls the alignment and size of its
//! child widget. It has four settings: xscale, yscale, xalign, and
//! yalign.
//! 
//! The scale settings are used to specify how much the child widget
//! should expand to fill the space allocated to the W(Alignment). The
//! values can range from 0 (meaning the child doesn't expand at all)
//! to 1 (meaning the child expands to fill all of the available
//! space).
//! 
//! The align settings are used to place the child widget within the
//! available area. The values range from 0 (top or left) to 1 (bottom
//! or right). Of course, if the scale settings are both set to 1, the
//! alignment settings have no effect.
//! 
//! NOIMG
//! Properties:
//! int bottom-padding
//! int left-padding
//! int right-padding
//! int top-padding
//! float xalign
//! float xscale
//! float yalign
//! float yscale
//!
//!

inherit GTK2.Bin;

static GTK2.Alignment create( float|mapping xalign_or_props, float|void yalign, float|void xscale, float|void yscale );
//! @xml{<matrix>@}
//! @xml{<r>@}@xml{<c>@}xalign :@xml{</c>@}
//! @xml{<c>@}the horizontal alignment of the child widget, from 0 (left) to 1 (right).@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}                  yalign :@xml{</c>@}
//! @xml{<c>@}the vertical alignment of the child widget, from 0 (top) to 1 (bottom).@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}                  xscale :@xml{</c>@}
//! @xml{<c>@}the amount that the child widget expands horizontally to fill up unused space, from 0 to 1. A value of 0 indicates that the child widget should never expand. A value of 1 indicates that the child widget will expand to fill all of the space allocated for the GTK2.Alignment.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}                  yscale :@xml{</c>@}
//! @xml{<c>@}the amount that the child widget expands vertically to fill up unused space, from 0 to 1. The values are similar to xscale.@xml{</c>@}@xml{</r>@}
//! @xml{</matrix>@}
//!
//!

GTK2.Alignment set( float xalign, float yalign, float xscale, float yscale );
//! @xml{<matrix>@}
//! @xml{<r>@}@xml{<c>@}xalign :@xml{</c>@}
//! @xml{<c>@}the horizontal alignment of the child widget, from 0 (left) to 1 (right).@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}                  yalign :@xml{</c>@}
//! @xml{<c>@}the vertical alignment of the child widget, from 0 (top) to 1 (bottom).@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}                  xscale :@xml{</c>@}
//! @xml{<c>@}the amount that the child widget expands horizontally to fill up unused space, from 0 to 1. A value of 0 indicates that the child widget should never expand. A value of 1 indicates that the child widget will expand to fill all of the space allocated for the GTK2.Alignment.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}                  yscale :@xml{</c>@}
//! @xml{<c>@}the amount that the child widget expands vertically to fill up unused space, from 0 to 1. The values are similar to xscale.@xml{</c>@}@xml{</r>@}
//! @xml{</matrix>@}
//!
//!

GTK2.Alignment set_padding( int padding_top, int padding_bottom, int padding_left, int padding_right );
//! Sets the padding on the different sides.
//!
//!
