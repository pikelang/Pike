#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.SHA3_256.shake)

//! SHAKE-256 is an extendable output function (XOF) based on @[SHA3_256].
//! It can provide an output digest of any length.
//!
//! @seealso
//!   @[SHA3_256.shake()]

inherit Crypto.SHA3_256;

string(8bit) hash(string(8bit) data, int(0..) bytes = digest_size())
{
  return shake(data, bytes);
}

class State
{
  inherit ::this_program;

  string(8bit) digest(int(0..) bytes = digest_size())
  {
    return shake(bytes);
  }
}

// NB: Fall back to using the generic implementation of HMAC.
_HMAC HMAC = SHA3_256::SHA3_256::Hash::Hash::_HMAC();
