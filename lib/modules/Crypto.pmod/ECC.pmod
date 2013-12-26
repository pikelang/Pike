#pike __REAL_VERSION__

//! Elliptic Curve Cipher Constants.
//!
//! This module contains constants used with elliptic curve algorithms.

#if constant(Nettle.ECC_Curve)

//! The definition of an elliptic curve.
class Curve {
  inherit Nettle.ECC_Curve;
}

//! The supported Elliptic Curves.
//!
//! These are typically used as arguments for @[ECDSA].
constant SECP_192R1 = Nettle.SECP_192R1;
constant SECP_224R1 = Nettle.SECP_224R1;
constant SECP_256R1 = Nettle.SECP_256R1;
constant SECP_384R1 = Nettle.SECP_384R1;
constant SECP_521R1 = Nettle.SECP_521R1;

#endif /* Nettle.ECC_Curve */
