#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.AES)

//! AES (American Encryption Standard) is a quite new block cipher,
//! specified by NIST as a replacement for the older DES standard. The
//! standard is the result of a competition between cipher designers.
//! The winning design, also known as RIJNDAEL, was constructed by
//! Joan Daemen and Vincent Rijnmen.
//!
//! Like all the AES candidates, the winning design uses a block size
//! of 128 bits, or 16 octets, and variable key-size, 128, 192 and 256
//! bits (16, 24 and 32 octets) being the allowed key sizes. It does
//! not have any weak keys.

inherit Nettle.AES;

#if constant(Nettle.POLY1305_AES)

//! @module POLY1305

//! @ignore
protected class Poly1305
{
//! @endignore

  inherit Nettle.POLY1305_AES;

  //! Get a POLY1305 @[State] object initialized with a password.
  State `()(string(8bit) password)
  {
    return State(password);
  }

//! @ignore
}

Poly1305 POLY1305 = Poly1305();

//! @endignore

//! @endmodule

#endif
