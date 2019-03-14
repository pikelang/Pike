#pike __REAL_VERSION__
#pragma strict_types

//! Base class for Message Authentication Codes (MAC)s.
//!
//! These are hashes that have been extended with a secret key.

inherit .__Hash;

//! Returns the recomended size of the key.
int(0..) key_size();

//! Returns the size of the iv/nonce (if any).
//!
//! Some MACs like eg @[Crypto.SHA1.HMAC] have fixed ivs,
//! in which case this function will return @expr{0@}.
int(0..) iv_size();

//! The state for the MAC.
class State
{
  inherit ::this_program;

  //! @param key
  //!   The secret key for the hash.
  protected void create(string key);
}
