//! Properties:
//! string id
//! GTK2.SourceTagStyle tag-style
//!
//!

inherit GTK2.TextTag;

mapping get_style( );
//! Gets the style associated with this tag.
//!
//!

GTK2.SourceTag set_style( mapping style );
//! Associates a style with this tag.
//! See GTK2.SourceBuffer->set_bracket_match_style() for format.
//!
//!
