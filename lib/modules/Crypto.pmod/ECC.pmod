#pike __REAL_VERSION__

//! Elliptic Curve Cipher Constants.
//!
//! This module contains constants used with elliptic curve algorithms.

#if constant(Nettle.ECC_Curve)

//! The definition of an elliptic curve.
//!
//! Objects of this class are typically not created by the user.
//!
//! @seealso
//!   @[SECP_192R1], @[SECP_224R1], @[SECP_256R1], @[SECP_384R1], @[SECP_521R1]
class Curve {
  inherit Nettle.ECC_Curve;
}

//! @module SECP_192R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_224R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_256R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_384R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_521R1

//! @decl inherit Curve

//! @endmodule

//! @ignore
Curve SECP_192R1 = Curve(1, 192, 1);
Curve SECP_224R1 = Curve(1, 224, 1);
Curve SECP_256R1 = Curve(1, 256, 1);
Curve SECP_384R1 = Curve(1, 384, 1);
Curve SECP_521R1 = Curve(1, 521, 1);
//! @endignore

#endif /* Nettle.ECC_Curve */
