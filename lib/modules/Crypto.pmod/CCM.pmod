#pike __REAL_VERSION__

//! Implementation of the Counter with Cipher Block Chaining
//! Message Authentication Code mode (CCM). Works as a wrapper
//! for the cipher algorithm put in create.
//!
//! This is a so-called authenticated encryption with associated data
//! (AEAD) algorithm, and in addition to encryption also provides
//! message digests.
//!
//! The operation of CCM is specified in
//! NIST Special Publication 800-38C.
//!
//! @note
//!   Wrapped ciphers MUST have a block size of @expr{16@}.
//!
//! @note
//!   This mode of operation is not suited for streaming operation,
//!   as the sizes of the associated data and payload data need to
//!   be known for the CBC-MAC operation to start. Currently this
//!   means that the associated data and payload data are buffered
//!   until @[State()->digest()] is called.
//!
//! @seealso
//!   @[CBC], @[GCM], @[CTR]

inherit __builtin.Nettle.AEAD;

inherit Crypto.CTR;

//! Returns the string @expr{"CCM"@}.
string(7bit) name()
{
  return "CCM";
}

//! Default digest size.
//!
//! @returns
//!   Returns @expr{16@}, but overloading via inherit is supported,
//!   and may return any even number in the range @expr{[4..16]@}.
int(4..16) digest_size()
{
  return 16;
}

//! 
class State
{
  inherit ::this_program;

  protected int decrypt_mode;

  protected string(8bit) mac_mask;

  protected string(8bit) nonce;

  protected String.Buffer abuf = String.Buffer();
  protected int asize;

  protected String.Buffer pbuf = String.Buffer();
  protected int psize;

  protected void create(program|State|function cipher, mixed ... more)
  {
    ::create(cipher, @more);
    if (block_size() != 16) {
      error("Invalid block cipher for CCM: %d.\n", block_size());
    }
    abuf->clear();
    pbuf->clear();
    asize = psize = 0;
    nonce = UNDEFINED;
  }

  string(8bit) name()
  {
    return "CCM(" + obj->name() + ")";
  }

  this_program set_encrypt_key(string(8bit) key, int|void flags)
  {
    abuf->clear();
    pbuf->clear();
    decrypt_mode = asize = psize = 0;
    return ::set_encrypt_key(key, flags);
  }

  this_program set_decrypt_key(string(8bit) key, int|void flags)
  {
    abuf->clear();
    pbuf->clear();
    asize = psize = 0;
    decrypt_mode = 1;
    return ::set_decrypt_key(key, flags);
  }

  this_program set_iv(string(8bit) iv)
  {
    abuf->clear();
    pbuf->clear();
    asize = psize = 0;
    if (sizeof(iv) > 13) {
      iv = iv[..12];
    } else if (sizeof(iv) < 7) {
      error("Too short nonce for CCM. Must be at least 7 bytes.\n");
    }
    nonce = iv;
    iv = sprintf("%1c%s%*c", 14 - sizeof(iv), iv, 15 - sizeof(iv), 0);
    // werror("CCM: IV: %s\n", String.string2hex(iv));
    return ::set_iv(iv);
  }

  void update(string(8bit) public_data)
  {
    int len = sizeof(public_data);
    if (!len) return;
    asize += len;
    abuf->add(public_data);
  }

  string(8bit) crypt(string(8bit) data)
  {
    int len = sizeof(data);
    if (!len) return "";
    if (!sizeof(pbuf)) {
      // Save the first block from the CTR to encrypt the mac.
      mac_mask = ::crypt("\0" * 16);
      // werror("CCM: S0: %s\n", String.string2hex(mac_mask));
    }
    psize += len;
    if (decrypt_mode) {
      data = ::crypt(data);
      pbuf->add(data);
      return data;
    } else {
      pbuf->add(data);
      return ::crypt(data);
    }
  }

  //! Default digest size.
  //!
  //! This function is used by @[digest()] to determine the digest
  //! size if no argument was given.
  //!
  //! @returns
  //!   The default implementation returns the result from calling
  //!   @[global::digest_size()], but overloading via inherit is supported,
  //!   and may return any even number in the range @expr{[4..16]@}.
  //!
  //! @seealso
  //!   @[digest()], @[global::digest_size()]
  int digest_size()
  {
    return global::digest_size();
  }

  //! Returns the CBC-MAC digest of the specified size.
  //!
  //! @param bytes
  //!   Size in bytes for the desired digest. Any even number in
  //!   the range @expr{[4..16]@}. If not specified the value from
  //!   calling @[digest_size()] will be used.
  //!
  //! @seealso
  //!   @[digest_size()], @[global::digest_size()]
  string(8bit) digest(int(4..16)|void bytes)
  {
    if (bytes & 1) {
      bytes++;
    }
    if (!bytes) {
      bytes = digest_size();
    }
    if (bytes < 4) {
      bytes = 4;
    } else if (bytes > 16) {
      bytes = 16;
    }

    if (!sizeof(pbuf)) {
      // Unlikely, but make sure that it is initialized.
      mac_mask = ::crypt("\0" * 16);
      // werror("CCM: S0: %s\n", String.string2hex(mac_mask));
    }

    int flags = ((bytes-2)<<2) | (14 - sizeof(nonce));
    if (sizeof(abuf)) {
      // Set the a-bit.
      flags |= 0x40;
    }

    string buf = obj->crypt(sprintf("%1c%s%*c",
				    flags, nonce, 15 - sizeof(nonce), psize));

    if (sizeof(abuf)) {
      string a;
      int asize = sizeof(abuf);
      if (asize < 0xff00) {
	if ((asize + 2) & 0x0f) {
	  abuf->add("\0" * (16 - ((asize + 2) & 0x0f)));
	}
	a = sprintf("%2c%s", asize, abuf);
      } else if (asize <= 0xffffffff) {
	if ((asize + 6) & 0x0f) {
	  abuf->add("\0" * (16 - ((asize + 6) & 0x0f)));
	}
	a = sprintf("%2c%4c%s", 0xfffe, asize, abuf);
      } else {
	if ((sizeof(abuf) + 10) & 0x0f) {
	  abuf->add("\0" * (16 - ((asize + 10) & 0x0f)));
	}
	a = sprintf("%2c%8c%s", 0xffff, asize, abuf);
      }
      abuf->clear();
      foreach(a / 16, string b) {
	buf = obj->crypt(b ^ buf);
      }
      // werror("CCM: A: %s\n", String.string2hex(a));
    }

    if (sizeof(pbuf) & 0x0f) {
      pbuf->add("\0" * (16 - (sizeof(pbuf) & 0x0f)));
    }
    foreach(pbuf->get()/16, string b) {
      buf = obj->crypt(b ^ buf);
    }

    // werror("CCM: T: %s\n", String.string2hex(buf));

    return (buf ^ mac_mask)[..bytes-1];
  }
}
