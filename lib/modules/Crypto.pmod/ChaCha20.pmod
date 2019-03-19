#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.CHACHA)

//! ChaCha20 is a stream cipher by D. J. Bernstein.
//!
//! @note
//!   This module is not available in all versions of Nettle.

inherit Nettle.CHACHA;

#if constant(Nettle.CHACHA_POLY1305)

//! @module POLY1305
//!
//! This is an @[AEAD] cipher consisting of the @[CHACHA] cipher
//! and a @[MAC] based on the POLY1305 algorithm.
//!
//! @note
//!   Note that this is an @[AEAD] cipher, while @[AES.POLY1305]
//!   (aka POLY1305-AES) is a @[MAC].
//!
//! @note
//!   Note also that the POLY1305 algorithm used here is NOT identical
//!   to the one in the @[AES.POLY1305] @[MAC]. The iv/nonce handling
//!   differs.
//!
//! @note
//!   This module is not available in all versions of Nettle.

//! @ignore
class _POLY1305
{
  //! @endignore

  inherit Nettle.CHACHA_POLY1305;

  //! @ignore
}

_POLY1305 POLY1305 = _POLY1305();

//! @endignore

//! @endmodule

#endif
