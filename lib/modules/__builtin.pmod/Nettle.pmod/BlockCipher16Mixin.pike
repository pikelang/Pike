#pike __REAL_VERSION__
#pragma strict_types

//! Mixin class for block cipher algorithms that have a 16 byte block size.
//!
//! Implements some common convenience functions, and prototypes.
//!
//! Note that no actual cipher algorithm is implemented
//! in the base class. They are implemented in classes
//! that (indirectly) inherit this class.
