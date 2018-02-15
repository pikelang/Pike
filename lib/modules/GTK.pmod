#pike __REAL_VERSION__

//! GTK wrapper module.
//!
//! This is a convenience module that is identical to either
//! either the @[GTK2] or the @[GTK1] module depending on
//! which (if any) of them is available.
//!
//! @seealso
//!   @[GTK1], @[GTK2]

#if constant(GTK2.Widget)
//!
inherit GTK2;
#elif constant(GTK1.Widget)
//!
inherit GTK1;
#else
constant module_value = UNDEFINED;
#endif

 