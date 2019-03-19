#pike __REAL_VERSION__

//! GTK wrapper module.
//!
//! This is a convenience module that is identical to the latest
//! supported GTK versionm, if available. Currently only @[GTK2] is
//! possible.
//!
//! @seealso
//!   @[GTK2]

#if constant(GTK2.Widget)
//!
inherit GTK2;
#else
constant module_value = UNDEFINED;
#endif


