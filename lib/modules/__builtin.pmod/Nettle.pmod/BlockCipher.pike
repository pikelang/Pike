#pike __REAL_VERSION__
#pragma strict_types

//! Base class for block cipher algorithms.
//!
//! Implements some common convenience functions, and prototypes.
//!
//! It also implements operating modes other than ECB.
//!
//! Note that no actual cipher algorithm is implemented
//! in the base class. They are implemented in classes
//! that inherit this class.

inherit .Cipher;

//! @module CTR
//! Implementation of Counter Mode (CTR). Works as
//! a wrapper for the cipher algorithm in the parent module.
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
//!   @[CBC], @[CCM], @[GCM], @[MAC]

//! @ignore
protected class _CTR
{
  //! @endignore

  inherit .Cipher;

  //! Returns the name of the base cipher extended with @expr{".CTR"@}.
  string(7bit) name()
  {
    return global::name() + ".CTR";
  }

  //! The state for the embedded algorithm 
  class State
  {
    inherit ::this_program;

    protected global::State obj;
    protected Gmp.mpz iv;
    protected int(1..) _block_size;

    protected void create()
    {
      obj = global::State();
      _block_size = obj->block_size();
      iv = Gmp.mpz(0);
    }

    string(8bit) name()
    {
      return obj->name() + ".CTR";
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
      this::iv = Gmp.mpz(iv, 256);
    }

    string(8bit) crypt(string(8bit) data)
    {
      int len = sizeof(data);
      String.Buffer buf = String.Buffer(len);
      while (len > 0) {
	string(8bit) chunk = iv->digits(256);
	iv++;
	if (sizeof(chunk) < _block_size) {
	  chunk = "\0"*(_block_size - sizeof(chunk)) + chunk;
	}
	chunk = obj->crypt(chunk);
	buf->add(chunk[..len-1]);
	len -= _block_size;
      }
      return [string(8bit)](data ^ (string(8bit))buf);
    }
  }

  State `()()
  {
    return State();
  }

  //! @ignore
}

_CTR CTR = _CTR();
//! @endignore
