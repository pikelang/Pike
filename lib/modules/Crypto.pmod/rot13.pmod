#pike __REAL_VERSION__

//! @namespace predef::

//! @module Crypto

//! @decl string(8bit) rot13(string(8bit) x)
//!
//! Perform case insensitive ROT13 "encryption"/"decryption".
//!
//! @seealso
//!   @[Substitution]

function _module_value =
    Crypto.Substitution()->set_rot_key()->crypt;

//! @endmodule

//! @endnamespace
