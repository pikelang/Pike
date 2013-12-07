#if constant(Nettle.GCM)
//! Implementation of the Galois Counter Mode (GCM). Works as
//! a wrapper for the cipher algorithm put in create.
//!
//! This is a so-called authenticated encryption with associated data
//! (AEAD) algorithm, and in addition to encryption also provides
//! message digests.
//!
//! The operation of GCM is specified in
//! NIST Special Publication 800-38D.
//!
//! @note
//!   Wrapped ciphers MUST have a block size of @expr{16@}.
//!
//! @note
//!   Note that this class is not available in all versions of Nettle.
//!
//! @seealso
//!   @[CBC]

inherit Nettle.GCM;

#endif
