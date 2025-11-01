#pike __REAL_VERSION__
#pragma strict_types

//!
//! Low-level modules.
//!
//! This module contains various base classes that are intended to
//! be inherited and/or used from C-code.
//!
//! Some of the base classes are:
//! @ul
//!   @item
//!     @[__builtin.Nettle.BlockCipher]
//!
//!     Base class for block cipher algorithms.
//!   @item
//!     @[__builtin.Sql.Connection]
//!
//!     Base class for connections to SQL servers. It is a generic
//!     interface on top of which the DB server specific implement
//!     their specifics.
//!   @item
//!     @[__builtin.Stack] (aka @[ADT.LowLevelStack])
//!
//!     Simple stack implementation.
//! @endul

//!
inherit _static_modules.Builtin;
