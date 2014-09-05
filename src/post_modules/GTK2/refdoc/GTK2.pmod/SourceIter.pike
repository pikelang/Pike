//!
//!
//!

inherit GTK2.TextIter;

array backward_search( string str, int flags, GTK2.TextIter limit );
//! Same as GTK2.TextIter->backward_search(), but supports case insensitive
//! searching.
//!
//!

array forward_search( string str, int flags, GTK2.TextIter limit );
//! Same as GTK2.TextIter->backward_search(), but supports case insensitive
//! searching.
//!
//!
