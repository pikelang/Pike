#pike __REAL_VERSION__

//!
//! The plaintext algorithm.
//!
//! This modules implements several of the crypto APIs,
//! but without any crypto. It is intended to be used
//! for testing of higher level algorithms.
//!

//! Implements the empty AE algorithm.
inherit .AE;

//! Implements the empty MAC algorithm.
inherit .MAC;

string(7bit) name()
{
  return "Plaintext";
}

int(0..0) key_size()
{
  return 0;
}

int(0..0) iv_size()
{
  return 0;
}

int(0..0) block_size()
{
  return 0;
}

int(0..0) digest_size()
{
  return 0;
}

//! Implements the "none" JWS algorithm.
protected constant mac_jwa_id = "none";

class State
{
  inherit AE::State;
  inherit MAC::State;

  protected void create(string|void key) {}

  this_program set_encrypt_key(string(8bit) key, void|int force)
  {
    return this;
  }

  this_program set_decrypt_key(string(8bit) key, void|int force)
  {
    return this;
  }

  string(0..0) make_key()
  {
    return "";
  }

  string(8bit) crypt(string(8bit) data)
  {
    return data;
  }

  this_program init(string|void data)
  {
    return this;
  }

  this_program update(string data)
  {
    return this;
  }

  string(0..0) digest(int|void length)
  {
    return "";
  }
}

protected State `()(string|void key)
{
  return State();
}
