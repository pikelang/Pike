#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.DES)

//! DES is the old Data Encryption Standard, specified by NIST. It
//! uses a block size of 64 bits (8 octets), and a key size of 56
//! bits. However, the key bits are distributed over 8 octets, where
//! the least significant bit of each octet is used for parity. A
//! common way to use DES is to generate 8 random octets in some way,
//! then set the least significant bit of each octet to get odd
//! parity, and initialize DES with the resulting key.
//!
//! The key size of DES is so small that keys can be found by brute
//! force, using specialized hardware or lots of ordinary work
//! stations in parallel. One shouldn't be using plain DES at all
//! today, if one uses DES at all one should be using @[DES3] or "triple
//! DES".
//!
//! DES also has some weak keys.

inherit Nettle.DES;

@Pike.Annotations.Implements(Crypto.BlockCipher);
