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

#endif /* POLY1305_AES */

#if constant(Nettle.UMAC32_AES)

//! @module UMAC32
//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC32] outputs a digest of 32 bits or 4 octets.
//!
//! @seealso
//!   @[UMAC64], @[UMAC96], @[UMAC128]

//! @ignore
protected class _UMAC32
{
//! @endignore

  inherit Nettle.UMAC32_AES;

  //! Get a UMAC32 @[State] object initialized with a password.
  State `()(string(8bit) password)
  {
    return State(password);
  }

//! @ignore
}

_UMAC32 UMAC32 = _UMAC32();

//! @endignore

//! @endmodule

#endif /* UMAC32_AES */

#if constant(Nettle.UMAC64_AES)

//! @module UMAC64
//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC64] outputs a digest of 64 bits or 8 octets.
//!
//! @seealso
//!   @[UMAC32], @[UMAC96], @[UMAC128]

//! @ignore
protected class _UMAC64
{
//! @endignore

  inherit Nettle.UMAC64_AES;

  //! Get a UMAC64 @[State] object initialized with a password.
  State `()(string(8bit) password)
  {
    return State(password);
  }

//! @ignore
}

_UMAC64 UMAC64 = _UMAC64();

//! @endignore

//! @endmodule

#endif /* UMAC64_AES */

#if constant(Nettle.UMAC96_AES)

//! @module UMAC96
//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC96] outputs a digest of 96 bits or 12 octets.
//!
//! @seealso
//!   @[UMAC32], @[UMAC64], @[UMAC128]

//! @ignore
protected class _UMAC96
{
//! @endignore

  inherit Nettle.UMAC96_AES;

  //! Get a UMAC96 @[State] object initialized with a password.
  State `()(string(8bit) password)
  {
    return State(password);
  }

//! @ignore
}

_UMAC96 UMAC96 = _UMAC96();

//! @endignore

//! @endmodule

#endif /* UMAC96_AES */

#if constant(Nettle.UMAC128_AES)

//! @module UMAC128
//! UMAC is a familty of message digest functions based on universal hashing
//! and @[AES] that is specified in RFC 4418. They differ mainly in the size
//! of the resulting digest.
//!
//! @[UMAC128] outputs a digest of 128 bits or 16 octets.
//!
//! @seealso
//!   @[UMAC32], @[UMAC64], @[UMAC96]

//! @ignore
protected class _UMAC128
{
//! @endignore

  inherit Nettle.UMAC128_AES;

  //! Get a UMAC128 @[State] object initialized with a password.
  State `()(string(8bit) password)
  {
    return State(password);
  }

//! @ignore
}

_UMAC128 UMAC128 = _UMAC128();

//! @endignore

//! @endmodule

#endif /* UMAC128_AES */
