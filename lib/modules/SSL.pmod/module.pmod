#pike __REAL_VERSION__
#pragma strict_types

//! Secure Socket Layer (SSL) version 3.0 and
//! Transport Layer Security (TLS) versions 1.0 - 1.2.
//!
//! RFC 2246 (TLS 1.0): "The primary goal of the TLS Protocol is to provide
//! privacy and data integrity between two communicating applications."
//!
//! The classes that typical users need to use are
//! @dl
//!   @item @[File]
//!     This is an object that attempts to behave as a @[Stdio.File]
//!     as much as possible.
//!
//!   @item @[Port]
//!     This is an object that attempts to behave as a @[Stdio.Port]
//!     as much as possible, with @[Port()->accept()] returning
//!     @[File] objects.
//!
//!   @item @[Context]
//!     The configurated context for the @[File].
//!
//!   @item @[Constants.CertificatePair]
//!     A class for keeping track of certificate chains and their
//!     private keys.
//! @enddl
//!
//! The @[Constants] module also contains lots of constants that are
//! used by the various APIs, as well as functions for formatting
//! the constants for output.
//!
//! @seealso
//!   @[File], @[Port], @[Context], @[Constants.CertificatePair],
//!   @[Constants]

class BufferError
{
  inherit Error.Generic;
  constant buffer_error = 1;
}
