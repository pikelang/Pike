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
//!   @item @[sslfile]
//!     This is an object that attempts to behave as a @[Stdio.File]
//!     as much as possible.
//!
//!   @item @[sslport]
//!     This is an object that attempts to behave as a @[Stdio.Port]
//!     as much as possible, with @[sslport()->accept()] returning
//!     @[sslfile] objects.
//!
//!   @item @[context]
//!     The configurated context for the @[sslfile].
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
//!   @[sslfile], @[sslport], @[context], @[Constants.CertificatePair],
//!   @[Constants]
