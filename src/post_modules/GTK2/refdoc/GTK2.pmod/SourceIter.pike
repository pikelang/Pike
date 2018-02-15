//!

inherit GTK2.TextIter;

array backward_search( string str, int flags, GTK2.TextIter limit );
//! Same as GTK2.TextIter->backward_search(), but supports case insensitive
//! searching.
//!
//!

int find_matching_bracket( );
//! Tries to match the bracket character currently at this iter with its
//! opening/closing counterpart, and if found moves iter to the position where
//! it was found.
//!
//!

array forward_search( string str, int flags, GTK2.TextIter limit );
//! Same as GTK2.TextIter->backward_search(), but supports case insensitive
//! searching.
//!
//!
