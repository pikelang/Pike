#pike __REAL_VERSION__
#pragma strict_types

#if constant(Nettle.CTR)
//! Implementation of Counter Mode (CTR). Works as
//! a wrapper for the cipher algorithm put in create.
//!
//! This cipher mode works like a stream cipher with
//! a block size >= 1. This means that the same key
//! and initialization vector (aka counter) should
//! never be reused, since a simple xor would reveal
//! information about the plain text. It also means
//! that it should never be used without a suiteable
//! Message Authentication Code (MAC).
//!
//! @seealso
//!   @[CBC], @[GCM]

inherit Nettle.CTR;

#else

// Fallback implementation.

//! @ignore

inherit __builtin.Nettle.Cipher;

//! @endignore

string name()
{
  return "CTR";
}

class State
{
  inherit ::this_program;

  protected object obj;
  protected Gmp.mpz iv;
  protected int _block_size;

  protected void create(program|object|function cipher, mixed ... more)
  {
    if (callablep(cipher)) cipher = cipher(@more);
    obj = cipher;
    _block_size = obj->block_size();
    iv = Gmp.mpz(0);
  }

  string(8bit) name()
  {
    return "CTR(" + obj->name() + ")";
  }

  int(1..) block_size()
  {
    return _block_size;
  }

  int(1..) iv_size()
  {
    return _block_size;
  }

  int(0..) key_size()
  {
    return obj->key_size();
  }

  this_program set_encrypt_key(string(8bit) key, int|void flags)
  {
    String.secure(key);
    obj->set_encrypt_key(key, flags);
    return this;
  }

  this_program set_decrypt_key(string(8bit) key, int|void flags)
  {
    String.secure(key);
    obj->set_encrypt_key(key, flags);
    return this;
  }

  this_program set_iv(string(8bit) iv)
  {
    String.secure(iv);
    this_program::iv = Gmp.mpz(iv, 256);
  }

  string(8bit) crypt(string(8bit) data)
  {
    int len = sizeof(data);
    String.Buffer buf = String.Buffer(len);
    while (len > 0) {
      string chunk = iv->digits(256);
      iv++;
      if (sizeof(chunk) < _block_size) {
	chunk = "\0"*(_block_size - sizeof(chunk)) + chunk;
      }
      chunk = obj->crypt(chunk);
      buf->add(chunk[..len-1]);
      len -= _block_size;
    }
    return data ^ (string)buf;
  }
}

State `()(program|object|function cipher, mixed ... more)
{
  return State(cipher, @more);
}

#endif
