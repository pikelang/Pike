
//! HMAC, defined by @rfc{2104@}.
//!
//! Backward-compatibility implementation. New code should
//! use @[Crypto.Hash.HMAC].

#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.Hash)

protected object(.Hash)|zero H;  // hash object

// B is the size of one compression block, in octets.
protected int(1..) B = 1;

//! @param h
//!   The hash object on which the HMAC object should base its
//!   operations. Typical input is @[Crypto.MD5].
//! @param b
//!   The block size of one compression block, in octets. Defaults to
//!   block_size() of @[h].
protected void create(.Hash h, int(1..) b = h->block_size())
{
  H = h;
  B = b;

  if(H->digest_size()>B)
    error("Block size is less than hash digest size.\n");
}

//! Calls the hash function given to @[create] and returns the hash
//! value of @[s].
string(8bit) raw_hash(string(8bit) s)
{
  return H->hash(s);
}

//! Makes a PKCS-1 digestinfo block with the message @[s].
//! @seealso
//!   @[Standards.PKCS.Signature.build_digestinfo]
string(8bit) pkcs_digest(string(8bit) s)
{
  return Standards.PKCS.Signature.build_digestinfo(s, H);
}

//! Calling the HMAC object with a password returns a new object that
//! can perform the actual HMAC hashing. E.g. doing a HMAC hash with
//! MD5 and the password @expr{"bar"@} of the string @expr{"foo"@}
//! would require the code @expr{Crypto.HMAC(Crypto.MD5)("bar")("foo")@}.
protected Crypto.MAC.State `()(string(8bit) passwd)
{
  return H->HMAC(passwd, B);
}
